#include "rhirender.h"
#include <QFile>
#include <QtCore/qdebug.h>
#include <QtGui/qmatrix4x4.h>
#include <QtQuick/qquickwindow.h>
#include <rhi/qrhi.h>

#include <QQuaternion>

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

void RHIRender::setWindow(QQuickWindow *window) { m_window = window; }

void RHIRender::frameStart() {
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
  updateDirtyRenderChunkMeshes(resourceUpdates);

  // MVP buffer
  const QSize outputPixelSize =
      swapChain->currentFrameRenderTarget()->pixelSize();
  QMatrix4x4 mvp;
  mvp.perspective(
      60.0f, float(outputPixelSize.width()) / float(outputPixelSize.height()),
      0.1f, 100.0f);
  mvp.rotate(QQuaternion::fromEulerAngles(m_cameraRotation));
  mvp.translate(-m_localPlayerPosition);

  mvp = rhi->clipSpaceCorrMatrix() * mvp;

  resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.data());

  swapChain->currentFrameCommandBuffer()->resourceUpdate(resourceUpdates);
}

void RHIRender::mainPassRecordingStart() {
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

  for (auto &renderChunkMesh : m_renderChunkMeshes) {
    for (int i = 0; i < 3; i++) {
      if (renderChunkMesh.second->chunkMesh->faces[i].indices.empty())
        continue;

      const QRhiCommandBuffer::VertexInput vbufBinding(
          renderChunkMesh.second->vBuffers[i].get(), 0);
      cb->setVertexInput(0, 1, &vbufBinding,
                         renderChunkMesh.second->iBuffers[i].get(), 0,
                         QRhiCommandBuffer::IndexUInt32);
      cb->drawIndexed(
          renderChunkMesh.second->chunkMesh->faces[i].indices.size());

      // qDebug() << "drawIndexed" << i << ":"
      //<< renderChunkMesh.second->chunkMesh->faces[i].vertices.size();
    }
  }

  auto now = std::chrono::steady_clock::now();
  m_fps = 1.0f / std::chrono::duration_cast<std::chrono::duration<float>>(
                     now - m_lastFrameTime)
                     .count();
  m_lastFrameTime = now;

  m_window->update();
}

// HELPER FUNCTIONS -------------------------------------
void RHIRender::initPipeline(QRhi *rhi, QRhiSwapChain *swapChain) {
  m_ubuf.reset(
      rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
  m_ubuf->create();

  m_srb.reset(rhi->newShaderResourceBindings());
  m_srb->setBindings({
      QRhiShaderResourceBinding::uniformBuffer(
          0, QRhiShaderResourceBinding::VertexStage, m_ubuf.get()),
  });
  m_srb->create();

  m_pipeline.reset(rhi->newGraphicsPipeline());
  m_pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
  m_pipeline->setDepthTest(true);
  m_pipeline->setDepthWrite(true);
  QRhiGraphicsPipeline::TargetBlend blend;
  blend.enable = true;
  blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
  blend.srcAlpha = QRhiGraphicsPipeline::SrcAlpha;
  blend.dstColor = QRhiGraphicsPipeline::One;
  blend.dstAlpha = QRhiGraphicsPipeline::One;
  m_pipeline->setTargetBlends({blend});

  // backface culling
  m_pipeline->setCullMode(QRhiGraphicsPipeline::Back);

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

void RHIRender::addChunkMesh(const WorldChunkPos &chunkPos,
                             std::unique_ptr<ChunkMesh> chunkMesh) {
  std::unique_ptr<RenderChunkMesh> renderChunkMesh =
      std::make_unique<RenderChunkMesh>(std::move(chunkMesh), m_window->rhi());

  m_dirtyRenderChunkMeshes.push_back(renderChunkMesh.get());

  m_renderChunkMeshes[chunkPos] = std::move(renderChunkMesh);
}

void RHIRender::removeChunkMesh(const WorldChunkPos &chunkPos) {
  m_renderChunkMeshes.erase(chunkPos);
}

void RHIRender::updateChunkMesh(const WorldChunkPos &chunkPos,
                                std::unique_ptr<ChunkMesh> chunkMesh) {
  auto it = m_renderChunkMeshes.find(chunkPos);
  if (it != m_renderChunkMeshes.end()) {
    RenderChunkMesh *renderChunkMesh = it->second.get();
    renderChunkMesh->chunkMesh = std::move(chunkMesh);

    for (int i = 0; i < 6; i++) {
      renderChunkMesh->vBuffers[i]->setSize(
          renderChunkMesh->chunkMesh->faces[i].vertices.size() *
          sizeof(uint64_t));
      renderChunkMesh->vBuffers[i]->create();
      renderChunkMesh->iBuffers[i]->setSize(
          renderChunkMesh->chunkMesh->faces[i].indices.size() *
          sizeof(uint32_t));
      renderChunkMesh->iBuffers[i]->create();
    }

    m_dirtyRenderChunkMeshes.push_back(renderChunkMesh);
  }
}

void RHIRender::updateDirtyRenderChunkMeshes(
    QRhiResourceUpdateBatch *resourceUpdates) {

  if (m_dirtyRenderChunkMeshes.empty()) {
    return;
  }
  qDebug() << "updateDirtyRenderChunkMeshes" << m_dirtyRenderChunkMeshes.size();
  for (RenderChunkMesh *renderChunkMesh : m_dirtyRenderChunkMeshes) {
    for (int i = 0; i < 6; i++) {
      resourceUpdates->uploadStaticBuffer(
          renderChunkMesh->vBuffers[i].get(), 0,
          renderChunkMesh->chunkMesh->faces[i].vertices.size() *
              sizeof(uint64_t),
          renderChunkMesh->chunkMesh->faces[i].vertices.data());
      resourceUpdates->uploadStaticBuffer(
          renderChunkMesh->iBuffers[i].get(), 0,
          renderChunkMesh->chunkMesh->faces[i].indices.size() *
              sizeof(uint32_t),
          renderChunkMesh->chunkMesh->faces[i].indices.data());
    }
  }
  m_dirtyRenderChunkMeshes.clear();
}

// Setters and Getters
void RHIRender::setPlayerWorldChunkPos(const PlayerWorldChunkPos &position) {
  m_playerWorldChunkPos = position;
}

PlayerWorldChunkPos RHIRender::playerWorldChunkPos() const {
  return m_playerWorldChunkPos;
}

void RHIRender::setLocalPlayerPosition(const QVector3D &position) {
  m_localPlayerPosition = position;
}

QVector3D RHIRender::localPlayerPosition() const {
  return m_localPlayerPosition;
}

void RHIRender::setCameraRotation(const QVector3D &rotation) {
  m_cameraRotation = rotation;
}

QVector3D RHIRender::cameraRotation() const { return m_cameraRotation; }