#include "rhirender.h"
#include "chunkmeshgeneration.h"
#include "world.h"
#include <QFile>
#include <QtCore/qdebug.h>
#include <QtCore/qtypes.h>
#include <QtGui/qmatrix4x4.h>
#include <QtQuick/qquickwindow.h>
#include <cstdint>
#include <rhi/qrhi.h>

#include <QQuaternion>

#include <QThreadPool>

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
  }

  // Update buffers here

  // Chunk mesh updates
  doOneTaskFromChunkUpdateQueue();
  checkRegionUpdates();

  // MVP buffer
  updateMVPBuffer(rhi, swapChain, resourceUpdates);

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

  cb->setShaderResources();

  // TODO: draw chunk meshes using instancing

  auto now = std::chrono::steady_clock::now();
  float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(
                        now - m_lastFrameTime)
                        .count();
  m_fps = 1.0f / deltaTime;
  m_lastFrameTime = now;

  m_window->update();
}

// HELPER FUNCTIONS -------------------------------------
void RHIRender::initPipeline(QRhi *rhi, QRhiSwapChain *swapChain) {
  m_ubuf.reset(
      rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
  m_ubuf->create();

  // uint32_t is 4 bytes; for every chunk we store 1 uint32
  m_relativeChunkPosUbuf.reset(
      rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 4));
  m_relativeChunkPosUbuf->create();

  m_srb.reset(rhi->newShaderResourceBindings());
  m_srb->setBindings({
      QRhiShaderResourceBinding::uniformBuffer(
          0, QRhiShaderResourceBinding::VertexStage, m_ubuf.get()),
      QRhiShaderResourceBinding::uniformBuffer(
          1, QRhiShaderResourceBinding::VertexStage,
          m_relativeChunkPosUbuf.get()),
  });
  m_srb->create();

  m_pipeline.reset(rhi->newGraphicsPipeline());
  // backface culling
  m_pipeline->setCullMode(QRhiGraphicsPipeline::Back);
  m_pipeline->setTopology(QRhiGraphicsPipeline::Lines);
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
  inputLayout.setBindings({{sizeof(uint64_t)}});
  inputLayout.setAttributes({{0, 0, QRhiVertexInputAttribute::UInt2, 0}});
  m_pipeline->setVertexInputLayout(inputLayout);

  m_pipeline->setShaderResourceBindings(m_srb.get());

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

void RHIRender::doOneTaskFromChunkUpdateQueue() {
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
        std::move(chunkMesh), m_window->rhi());
  } else {
    // update this chunk mesh
    m_renderChunkMeshes[chunkPos]->updateChunkMesh(std::move(chunkMesh),
                                                   m_window->rhi());
  }
}

void RHIRender::checkRegionUpdates() {
  if (m_engine->gameLoop()->regionsRenderedDirty()) {
    resizeRelativeChunkPosUbuf();

    auto regionsRendered = m_engine->gameLoop()->regionsRendered();
    for (auto &regionPos : regionsRendered) {
      if (!m_renderChunkMeshes.count(WorldChunkPos(regionPos, {0, 0, 0}))) {
        const Region *region = m_engine->world()->region(regionPos);
        if (!region) {
          qWarning() << "Region not found";
          continue;
        }
        for (uint8_t x = 0; x < Region::REGION_SIZE; x++) {
          for (uint8_t y = 0; y < Region::REGION_SIZE; y++) {
            for (uint8_t z = 0; z < Region::REGION_SIZE; z++) {
              WorldChunkPos chunkPos(regionPos, {x, y, z});
              QThreadPool::globalInstance()->start(
                  [this, chunkPos, region, x, y, z]() {
                    addChunkUpdateTask(chunkPos, false,
                                       ChunkMeshGenerator::generateChunkMesh(
                                           region->chunk(x, y, z)));
                  });
            }
          }
        }
      }

      m_engine->gameLoop()->setRegionsRenderedDirty(false);
    }
  }
}

void RHIRender::resizeRelativeChunkPosUbuf() {
  int regionsRenderedSize = m_engine->gameLoop()->regionsRenderedSize();
  if (m_relativeChunkPosUbuf->size() !=
      regionsRenderedSize * Region::REGION_VOLUME * sizeof(uint32_t)) {
    m_relativeChunkPosUbuf->setSize(regionsRenderedSize *
                                    Region::REGION_VOLUME * sizeof(uint32_t));
    m_relativeChunkPosUbuf->create();
  }
}
