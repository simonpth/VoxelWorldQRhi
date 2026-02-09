#include "rhirender.h"
#include "chunkmeshgeneration.h"
#include "world.h"
#include <QFile>
#include <QtCore/qdebug.h>
#include <QtCore/qlogging.h>
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
    0, 1, 0, // vertex 0
    0, 0, 0, // vertex 1
    1, 1, 0, // vertex 2
    1, 0, 0  // vertex 3
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
  for (int i = 0; i < 32 && !m_chunkUpdateQueue.empty(); i++) {
    doOneTaskFromChunkUpdateQueue(resourceUpdates); // this has to happen first
  }
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

  const QRhiCommandBuffer::VertexInput quadVertexInput = {m_quadVBuf.get(), 0};
  cb->setVertexInput(0, 1, &quadVertexInput);

  // Get player position once for frustum culling
  const PlayerWorldChunkPos playerPos = m_engine->gameLoop()->playerWorldChunkPos();

  int renderedChunks = 0;

  auto it = m_renderChunkMeshes.begin();
  for (int i = 0; i < m_renderChunkMeshes.size(); i++) {
    // Frustum culling: skip chunks outside the view frustum
    if (!isChunkInFrustum(it->first, playerPos)) {
      ++it;
      continue;
    }

    cb->setShaderResources(it->second->m_srb.get());

    for (int j = 0; j < 6; j++) {
      if (it->second->chunkMesh->faces[j].vertices.empty()) {
        continue;
      }

      renderedChunks++;

      // Face mapping: 0=x+, 1=y+, 2=z+, 3=x-, 4=y-, 5=z-
      switch (j) {
        case 0:
          if (it->first.x > playerPos.x)
            continue; // skip left face if it's facing away
          break;
        case 1:
          if (it->first.y > playerPos.y)
            continue; // skip bottom face if it's facing away
          break;
        case 2:
          if (it->first.z > playerPos.z)
            continue; // skip back face if it's facing away
          break;
        case 3:
          if (it->first.x < playerPos.x)
            continue; // skip right face if it's facing away
          break;
        case 4:
          if (it->first.y < playerPos.y)
            continue; // skip top face if it's facing away
          break;
        case 5:
          if (it->first.z < playerPos.z)
            continue; // skip front face if it's facing away
          break;
        default:
          break;
      }
      const QRhiCommandBuffer::VertexInput chunkMeshVertexInput = {
          it->second->vBuffers[j].get(), 0};
      cb->setVertexInput(1, 1, &chunkMeshVertexInput);
      cb->draw(4, it->second->chunkMesh->faces[j].vertices.size(), 0, 0);
    }
    ++it;
  }
  //qDebug() << "Rendered chunks:" << renderedChunks;

  // FPS calculation
  auto now = std::chrono::steady_clock::now();
  if (true) {
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
  m_pipeline->setCullMode(QRhiGraphicsPipeline::Back);
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

  // Extract frustum planes for culling
  extractFrustumPlanes(mvp);

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
}

void RHIRender::checkRegionUpdates() {
  if (m_engine->gameLoop()->regionsRenderedDirty()) {
    m_engine->gameLoop()->setRegionsRenderedDirty(false);

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

    // Remove chunk meshes that are no longer in rendered regions
    std::vector<WorldChunkPos> chunkMeshesToRemove;
    for (const auto &entry : m_renderChunkMeshes) {
      WorldChunkPos chunkPos = entry.first;
      RegionPos chunkRegionPos = World::worldChunkPosToRegionPos(chunkPos);
      if (std::find(regionsRendered.begin(), regionsRendered.end(), chunkRegionPos) ==
          regionsRendered.end()) {
        chunkMeshesToRemove.push_back(chunkPos);
      }
    }
    for (const auto &chunkPos : chunkMeshesToRemove) {
      addChunkUpdateTask(chunkPos, true, nullptr); // remove this chunk mesh
    }
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
    Region *region = m_engine->world()->region(regionPos);
    auto readLock = region->claimReadLock();

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
                ChunkMeshGenerator::generateChunkMesh(region->chunk(x, y, z), region,
                                                     ChunkPos(x, y, z)));
          }
        }
      }
    }
  });
}

void RHIRender::onRegionGenerated(const RegionPos &regionPos) {
  auto regionsRendered = m_engine->gameLoop()->regionsRendered();
  if(std::find(regionsRendered.begin(), regionsRendered.end(), regionPos) ==
     regionsRendered.end()) {
    // this region is not in the rendered regions, so we don't need to generate chunk meshes for it
    return;
  }
  generateChunkMeshesForRegionAsync(regionPos);
}

// FRUSTUM CULLING IMPLEMENTATION -------------------------------------

void RHIRender::extractFrustumPlanes(const QMatrix4x4 &mvp) {
  // Extract the 6 frustum planes from the MVP matrix
  // Plane coefficients are extracted from the rows of the MVP matrix
  // The planes are: left, right, bottom, top, near, far

  const float *m = mvp.data();

  // Left plane: row 3 + row 0
  m_frustumPlanes[0].a = m[3] + m[0];
  m_frustumPlanes[0].b = m[7] + m[4];
  m_frustumPlanes[0].c = m[11] + m[8];
  m_frustumPlanes[0].d = m[15] + m[12];

  // Right plane: row 3 - row 0
  m_frustumPlanes[1].a = m[3] - m[0];
  m_frustumPlanes[1].b = m[7] - m[4];
  m_frustumPlanes[1].c = m[11] - m[8];
  m_frustumPlanes[1].d = m[15] - m[12];

  // Bottom plane: row 3 + row 1
  m_frustumPlanes[2].a = m[3] + m[1];
  m_frustumPlanes[2].b = m[7] + m[5];
  m_frustumPlanes[2].c = m[11] + m[9];
  m_frustumPlanes[2].d = m[15] + m[13];

  // Top plane: row 3 - row 1
  m_frustumPlanes[3].a = m[3] - m[1];
  m_frustumPlanes[3].b = m[7] - m[5];
  m_frustumPlanes[3].c = m[11] - m[9];
  m_frustumPlanes[3].d = m[15] - m[13];

  // Near plane: row 3 + row 2
  m_frustumPlanes[4].a = m[3] + m[2];
  m_frustumPlanes[4].b = m[7] + m[6];
  m_frustumPlanes[4].c = m[11] + m[10];
  m_frustumPlanes[4].d = m[15] + m[14];

  // Far plane: row 3 - row 2
  m_frustumPlanes[5].a = m[3] - m[2];
  m_frustumPlanes[5].b = m[7] - m[6];
  m_frustumPlanes[5].c = m[11] - m[10];
  m_frustumPlanes[5].d = m[15] - m[14];

  // Normalize planes (optional but improves precision)
  for (int i = 0; i < 6; i++) {
    float length = std::sqrt(m_frustumPlanes[i].a * m_frustumPlanes[i].a +
                             m_frustumPlanes[i].b * m_frustumPlanes[i].b +
                             m_frustumPlanes[i].c * m_frustumPlanes[i].c);
    if (length > 0.0f) {
      m_frustumPlanes[i].a /= length;
      m_frustumPlanes[i].b /= length;
      m_frustumPlanes[i].c /= length;
      m_frustumPlanes[i].d /= length;
    }
  }
}

AABB RHIRender::getChunkAABB(const WorldChunkPos &chunkPos, const PlayerWorldChunkPos &playerPos) const {
  // Calculate the chunk's position relative to the player's chunk
  // The frustum is in "local player space" where the player's chunk is at origin
  int relX = static_cast<int>(chunkPos.x - playerPos.x) * Chunk::CHUNK_SIZE;
  int relY = static_cast<int>(chunkPos.y - playerPos.y) * Chunk::CHUNK_SIZE;
  int relZ = static_cast<int>(chunkPos.z - playerPos.z) * Chunk::CHUNK_SIZE;

  return {
    relX,           // minX
    relY,           // minY
    relZ,           // minZ
    relX + Chunk::CHUNK_SIZE,  // maxX
    relY + Chunk::CHUNK_SIZE,  // maxY
    relZ + Chunk::CHUNK_SIZE   // maxZ
  };
}

bool RHIRender::isChunkInFrustum(const WorldChunkPos &chunkPos, const PlayerWorldChunkPos &playerPos) const {
  AABB aabb = getChunkAABB(chunkPos, playerPos);

  // Test the AABB against all 6 frustum planes
  // Using the "positive vertex" approach: for each plane, we find the vertex
  // of the AABB that is most aligned with the plane normal (the positive vertex)
  // If the positive vertex is outside, the entire box is outside

  for (int i = 0; i < 6; i++) {
    const FrustumPlane &plane = m_frustumPlanes[i];

    // Select the positive vertex based on the plane normal direction
    // For a plane with normal (a, b, c), the positive vertex is:
    // (minX if a < 0 else maxX, minY if b < 0 else maxY, minZ if c < 0 else maxZ)
    int positiveX = (plane.a < 0.0f) ? aabb.minX : aabb.maxX;
    int positiveY = (plane.b < 0.0f) ? aabb.minY : aabb.maxY;
    int positiveZ = (plane.c < 0.0f) ? aabb.minZ : aabb.maxZ;

    // Compute the signed distance from the plane to the positive vertex
    float distance = plane.a * positiveX + plane.b * positiveY + plane.c * positiveZ + plane.d;

    // If the positive vertex is outside (distance < 0), the entire box is outside
    if (distance < 0.0f) {
      return false;
    }
  }

  // The chunk is inside or intersecting the frustum
  return true;
}
