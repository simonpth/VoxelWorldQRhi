#ifndef RHIRENDER_H
#define RHIRENDER_H

#include <QtCore/qobject.h>
#include <QtQuick/qquickitem.h>
#include <memory>
#include <rhi/qrhi.h>

#include "chunkmeshgeneration.h"
#include "engine.h"
#include "world.h"
#include <deque>
#include <unordered_map>

struct RenderChunkMesh {
  std::unique_ptr<ChunkMesh> chunkMesh;
  std::unique_ptr<QRhiBuffer> uRelPosBuf;
  std::unique_ptr<QRhiShaderResourceBindings> m_srb;
  std::array<std::unique_ptr<QRhiBuffer>, 6> vBuffers;

  RenderChunkMesh(std::unique_ptr<ChunkMesh> chunkMesh, QRhi *rhi, QRhiBuffer *globalUbuf) {
    this->chunkMesh = std::move(chunkMesh);
    for (int i = 0; i < 6; i++) {
      vBuffers[i].reset(rhi->newBuffer(
          QRhiBuffer::Static, QRhiBuffer::VertexBuffer,
          this->chunkMesh->faces[i].vertices.size() * sizeof(uint64_t)));
      vBuffers[i]->create();
    }
    uRelPosBuf.reset(rhi->newBuffer(
        QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(uint32_t)));
    uRelPosBuf->create();

    m_srb.reset(rhi->newShaderResourceBindings());
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage, globalUbuf),
        QRhiShaderResourceBinding::uniformBuffer(
            1, QRhiShaderResourceBinding::VertexStage, uRelPosBuf.get())
    });
    m_srb->create();
  }

  void updateChunkMesh(std::unique_ptr<ChunkMesh> chunkMesh, QRhi *rhi) {
    this->chunkMesh = std::move(chunkMesh);
    for (int i = 0; i < 6; i++) {
      vBuffers[i]->setSize(this->chunkMesh->faces[i].vertices.size() *
                           sizeof(uint64_t));
      vBuffers[i]->create();
    }
  }
};

class RHIRender : public QObject {
  Q_OBJECT

public:
  explicit RHIRender(QObject *parent = nullptr);
  ~RHIRender();

  void setWindow(QQuickWindow *window) { m_window = window; }

public slots:
  void frameStart();
  void mainPassRecordingStart();
  void onRegionGenerated(const RegionPos &regionPos);

private:
  void initPipeline(QRhi *rhi, QRhiSwapChain *swapChain);
  void updateMVPBuffer(QRhi *rhi, QRhiSwapChain *swapChain,
                       QRhiResourceUpdateBatch *resourceUpdates);

  void doOneTaskFromChunkUpdateQueue(QRhiResourceUpdateBatch *resourceUpdates);
  void checkRegionUpdates();
  void updateRelativeChunkPosUbuf(QRhiResourceUpdateBatch *resourceUpdates);

  void generateChunkMeshesForRegionAsync(const RegionPos &regionPos);

public:
  void setEngine(const Engine *engine) {
    m_engine = engine;
    if (m_engine) {
      connect(m_engine->gameLoop(), &GameLoop::regionGenerated, this,
              &RHIRender::onRegionGenerated);
    }
  }

  float fps() const { return m_fps; }

private:
  QQuickWindow *m_window = nullptr;

  const Engine *m_engine = nullptr;

  std::unique_ptr<QRhiBuffer> m_quadVBuf;
  std::unique_ptr<QRhiBuffer> m_ubuf;
  std::unique_ptr<QRhiShaderResourceBindings> m_layoutSrb;
  std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

  std::mutex m_chunkUpdateMutex;
  std::queue<std::tuple<WorldChunkPos, bool, std::unique_ptr<ChunkMesh>>>
      m_chunkUpdateQueue;
  void addChunkUpdateTask(const WorldChunkPos &pos, bool remove,
                          std::unique_ptr<ChunkMesh> chunkMesh) {
    std::lock_guard<std::mutex> lock(m_chunkUpdateMutex);
    m_chunkUpdateQueue.emplace(pos, remove, std::move(chunkMesh));
  }

  std::unordered_map<WorldChunkPos, std::unique_ptr<RenderChunkMesh>,
                     WorldChunkPosHash>
      m_renderChunkMeshes;

  // fps logic: average over last 5 seconds
  std::chrono::steady_clock::time_point m_lastFrameTime;
  std::deque<std::chrono::steady_clock::time_point> m_frameTimes;
  float m_fps = 0.0f;
};

#endif // RHIRENDER_H
