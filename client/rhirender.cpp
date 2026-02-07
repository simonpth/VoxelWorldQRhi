#include "rhirender.h"
#include "chunkmeshgeneration.h"
#include "world.h"
#include <QFile>
#include <QtCore/qdebug.h>
#include <QtCore/qtypes.h>
#include <QtGui/qmatrix4x4.h>
#include <QtQuick/qquickwindow.h>
#include <cstdint>
#include <memory>
#include <rhi/qrhi.h>

#include <QQuaternion>

#include <QThreadPool>
#include <utility>

// verticies for a quad
// 3 uint32_t per x, y, z
static uint32_t quadVertices[] = {
    1, 0, 0, // vertex 0
    1, 1, 0, // vertex 1
    0, 0, 0, // vertex 2
    0, 1, 0  // vertex 3
};

RHIRender::RHIRender(QObject *parent) : QObject(parent) {}

RHIRender::~RHIRender() {}

static QShader getShader(const QString &name) {
  QFile f(name);
  if (!f.open(QIODevice::ReadOnly)) {
    qWarning() << "Failed to open shader file:" << name;
    return QShader();
  }
  return QShader::fromSerialized(f.readAll());
}

void RHIRender::frameStart() {
  if (!m_engine) {
    return;
  }

  QRhi *rhi = m_window->rhi();
  if (!rhi) {
    qWarning("QQuickWindow is not using QRhi for rendering");
    return;
  }
  QRhiSwapChain *swapChain = m_window->swapChain();
  if (!swapChain) {
    qWarning("No QRhiSwapChain?");
    return;
  }
  QRhiResourceUpdateBatch *resourceUpdates = rhi->nextResourceUpdateBatch();

  if (!m_pipeline) {
    qDebug() << "initPipeline";
    initPipeline(rhi, swapChain);
    resourceUpdates->uploadStaticBuffer(m_quadVBuf.get(), quadVertices);
  }

  // Chunk mesh updates
  doOneTaskFromChunkUpdateQueue(resourceUpdates); // this has to happen first
  checkRegionUpdates();

  // MVP buffer
  updateMVPBuffer(rhi, swapChain, resourceUpdates);

  updateRelativeChunkPosUbuf(resourceUpdates);

  swapChain->currentFrameCommandBuffer()->resourceUpdate(resourceUpdates);
}

void RHIRender::mainPassRecordingStart() {
  if (!m_engine) {
    return;
  }

  QRhi *rhi = m_window->rhi();
  QRhiSwapChain *swapChain = m_window->swapChain();
  if (!rhi || !swapChain)
    return;

  const QSize outputPixelSize =
      swapChain->currentFrameRenderTarget()->pixelSize();
  QRhiCommandBuffer *cb = m_window->swapChain()->currentFrameCommandBuffer();

  cb->setViewport({0.0f, 0.0f, float(outputPixelSize.width()),
                   float(outputPixelSize.height())});
  cb->setGraphicsPipeline(m_pipeline.get());

  // draw chunk meshes using instancing
  if (false) {
    qDebug() << "Render chunk meshes:" << m_renderChunkMeshes.size();
  }

  const QRhiCommandBuffer::VertexInput quadVertexInput = {m_quadVBuf.get(), 0};
  cb->setVertexInput(0, 1, &quadVertexInput);

  auto it = m_renderChunkMeshes.begin();
  for (int i = 0; i < m_renderChunkMeshes.size(); i++) {

    cb->setShaderResources(it->second->m_srb.get());

    for (int j = 0; j < 6; j++) {
      if (it->second->chunkMesh->faces[j].vertices.empty()) {
        continue;
      }
      const QRhiCommandBuffer::VertexInput chunkMeshVertexInput = {
          it->second->vBuffers[j].get(), 0};
      cb->setVertexInput(1, 1, &chunkMeshVertexInput);
      cb->draw(4, it->second->chunkMesh->faces[j].vertices.size(), 0, 0);
    }
    ++it;
  }

  // FPS calculation
  auto now = std::chrono::steady_clock::now();
  if (false) {
    float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(
                          now - m_lastFrameTime)
                          .count();
    m_fps = 1.0f / deltaTime;
    m_lastFrameTime = now;
  } else {
    m_frameTimes.push_back(now);
    // Remove frames older than 5 seconds
    float timeWindow = 5.0f;
    float timeSinceOldestFrame =
        std::chrono::duration_cast<std::chrono::duration<float>>(
            now - m_frameTimes.front())
            .count();
    while (!m_frameTimes.empty() && timeSinceOldestFrame > timeWindow) {
      m_frameTimes.pop_front();
    }
    m_fps = static_cast<float>(m_frameTimes.size()) / timeSinceOldestFrame;
  }

  m_window->update();
}

// HELPER FUNCTIONS -------------------------------------
void RHIRender::initPipeline(QRhi *rhi, QRhiSwapChain *swapChain) {
  m_quadVBuf.reset(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::VertexBuffer,
                                  sizeof(quadVertices)));
  m_quadVBuf->create();

  m_ubuf.reset(
      rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
  m_ubuf->create();

  m_layoutSrb.reset(rhi->newShaderResourceBindings());
  m_layoutSrb->setBindings({
      QRhiShaderResourceBinding::uniformBuffer(
          0, QRhiShaderResourceBinding::VertexStage, m_ubuf.get()),
      QRhiShaderResourceBinding::uniformBuffer(
          1, QRhiShaderResourceBinding::VertexStage, nullptr, 0,
          sizeof(uint32_t)) // instance data buffer, set later
  });
  m_layoutSrb->create();

  m_pipeline.reset(rhi->newGraphicsPipeline());
  // backface culling
  m_pipeline->setCullMode(QRhiGraphicsPipeline::None);
  m_pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
  m_pipeline->setDepthTest(true);
  m_pipeline->setDepthWrite(true);

  /*
  QRhiGraphicsPipeline::TargetBlend blend;
  blend.enable = true;
  blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
  blend.srcAlpha = QRhiGraphicsPipeline::SrcAlpha;
  blend.dstColor = QRhiGraphicsPipeline::One;
  blend.dstAlpha = QRhiGraphicsPipeline::One;
  m_pipeline->setTargetBlends({blend});
  */

  m_pipeline->setShaderStages(
      {{QRhiShaderStage::Vertex, getShader(QLatin1String(":/shader.vert.qsb"))},
       {QRhiShaderStage::Fragment,
        getShader(QLatin1String(":/shader.frag.qsb"))}});

  QRhiVertexInputLayout inputLayout;
  // 0: quad mesh vertex buffer, 1: instance data buffer
  inputLayout.setBindings(
      {{3 * sizeof(uint32_t), QRhiVertexInputBinding::PerVertex},
       {sizeof(uint64_t), QRhiVertexInputBinding::PerInstance}});

  inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::UInt3, 0},
                             {1, 1, QRhiVertexInputAttribute::UInt2, 0}});

  m_pipeline->setVertexInputLayout(inputLayout);

  m_pipeline->setShaderResourceBindings(m_layoutSrb.get());

  m_pipeline->setRenderPassDescriptor(
      swapChain->currentFrameRenderTarget()->renderPassDescriptor());
  m_pipeline->create();
}

void RHIRender::updateMVPBuffer(QRhi *rhi, QRhiSwapChain *swapChain,
                                QRhiResourceUpdateBatch *resourceUpdates) {
  const QSize outputPixelSize =
      swapChain->currentFrameRenderTarget()->pixelSize();
  QMatrix4x4 mvp;
  mvp.perspective(
      60.0f, float(outputPixelSize.width()) / float(outputPixelSize.height()),
      0.1f, 1000000.0f);
  mvp.rotate(m_engine->gameLoop()->cameraRotation().x(), 1.0f, 0.0f, 0.0f);
  mvp.rotate(m_engine->gameLoop()->cameraRotation().y(), 0.0f, 1.0f, 0.0f);
  mvp.translate(-m_engine->gameLoop()->localPlayerPosition());

  mvp = rhi->clipSpaceCorrMatrix() * mvp;

  resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.data());
}

void RHIRender::doOneTaskFromChunkUpdateQueue(
    QRhiResourceUpdateBatch *resourceUpdates) {
  std::lock_guard<std::mutex> lock(m_chunkUpdateMutex);
  if (m_chunkUpdateQueue.empty()) {
    return;
  }
  auto [chunkPos, remove, chunkMesh] = std::move(m_chunkUpdateQueue.front());
  m_chunkUpdateQueue.pop();

  if (remove) {
    m_renderChunkMeshes.erase(chunkPos);
    return;
  }

  if (!chunkMesh) {
    qWarning() << "Chunk mesh is null";
    return;
  }

  if (!m_renderChunkMeshes.count(chunkPos)) {
    // add this chunk mesh to the render chunk meshes
    m_renderChunkMeshes[chunkPos] = std::make_unique<RenderChunkMesh>(
        std::move(chunkMesh), m_window->rhi(), m_ubuf.get());
  } else {
    // update this chunk mesh
    m_renderChunkMeshes[chunkPos]->updateChunkMesh(std::move(chunkMesh),
                                                   m_window->rhi());
  }

  for (int i = 0; i < 6; i++) {
    resourceUpdates->uploadStaticBuffer(
        m_renderChunkMeshes[chunkPos]->vBuffers[i].get(),
        m_renderChunkMeshes[chunkPos]->chunkMesh->faces[i].vertices.data());
  }
  qDebug() << "Updated chunk mesh at" << chunkPos.x << chunkPos.y << chunkPos.z;
}

void RHIRender::checkRegionUpdates() {
  if (m_engine->gameLoop()->regionsRenderedDirty()) {
    auto regionsRendered = m_engine->gameLoop()->regionsRendered();
    for (auto &regionPos : regionsRendered) {
      if (!m_renderChunkMeshes.count(WorldChunkPos(regionPos, {0, 0, 0}))) {
        const Region *region = m_engine->world()->region(regionPos);
        if (!region) {
          // the region is not generated yet, but will be soon
          continue;
        }
        generateChunkMeshesForRegionAsync(regionPos);
      }
    }
    m_engine->gameLoop()->setRegionsRenderedDirty(false);
  }
}

void RHIRender::updateRelativeChunkPosUbuf(
    QRhiResourceUpdateBatch *resourceUpdates) {
  PlayerWorldChunkPos playerPos = m_engine->gameLoop()->playerWorldChunkPos();

  for (const auto &mesh : m_renderChunkMeshes) {
    WorldChunkPos updatedChunkPos = mesh.first;

    uint32_t relativeX = updatedChunkPos.x - playerPos.x;
    uint32_t relativeY = updatedChunkPos.y - playerPos.y;
    uint32_t relativeZ = updatedChunkPos.z - playerPos.z;

    // encoding in uint32_t: 10 bits for x, 10 bits for y, 10 bits for z, 2 bits
    // unused
    relativeX = relativeX & 0x3FF;         // 10 bits
    relativeY = (relativeY & 0x3FF) << 10; // 10 bits
    relativeZ = (relativeZ & 0x3FF) << 20; // 10 bits

    uint32_t relativePos = relativeX | relativeY | relativeZ;

    resourceUpdates->updateDynamicBuffer(mesh.second->uRelPosBuf.get(), 0,
                                         sizeof(uint32_t), &relativePos);
  }
}

void RHIRender::generateChunkMeshesForRegionAsync(const RegionPos &regionPos) {
  QThreadPool::globalInstance()->start([this, regionPos]() {
    const Region *region = m_engine->world()->region(regionPos);

    if (!region) {
      qWarning() << "Region not found in generateChunkMeshesForRegion";
      return;
    }

    for (uint8_t x = 0; x < Region::REGION_SIZE; x++) {
      for (uint8_t y = 0; y < Region::REGION_SIZE; y++) {
        for (uint8_t z = 0; z < Region::REGION_SIZE; z++) {
          WorldChunkPos chunkPos(regionPos, {x, y, z});
          if (!m_renderChunkMeshes.count(chunkPos)) {
            addChunkUpdateTask(
                chunkPos, false,
                ChunkMeshGenerator::generateChunkMesh(region->chunk(x, y, z)));
          }
        }
      }
    }
  });
}

void RHIRender::onRegionGenerated(const RegionPos &regionPos) {
  generateChunkMeshesForRegionAsync(regionPos);
}
