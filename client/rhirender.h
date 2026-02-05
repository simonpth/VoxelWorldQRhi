#ifndef RHIRENDER_H
#define RHIRENDER_H

#include <QtCore/qobject.h>
#include <QtQuick/qquickitem.h>
#include <rhi/qrhi.h>

#include "chunkmeshgeneration.h"
#include "engine.h"
#include "world.h"
#include <unordered_map>

struct RenderChunkMesh {
  std::unique_ptr<ChunkMesh> chunkMesh;
  std::array<std::unique_ptr<QRhiBuffer>, 6> vBuffers;

  RenderChunkMesh(std::unique_ptr<ChunkMesh> chunkMesh, QRhi *rhi) {
    this->chunkMesh = std::move(chunkMesh);
    for (int i = 0; i < 6; i++) {
      vBuffers[i].reset(rhi->newBuffer(
          QRhiBuffer::Static, QRhiBuffer::VertexBuffer,
          this->chunkMesh->faces[i].vertices.size() * sizeof(uint64_t)));
      vBuffers[i]->create();
    }
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

private:
  void initPipeline(QRhi *rhi, QRhiSwapChain *swapChain);
  void updateMVPBuffer(QRhi *rhi, QRhiSwapChain *swapChain,
                       QRhiResourceUpdateBatch *resourceUpdates);

  void doOneTaskFromChunkUpdateQueue();
  void checkRegionUpdates();
  void resizeRelativeChunkPosUbuf();

public:
  void setEngine(const Engine *engine) { m_engine = engine; }

  float fps() const { return m_fps; }

private:
  QQuickWindow *m_window = nullptr;

  const Engine *m_engine = nullptr;

  std::unique_ptr<QRhiBuffer> m_ubuf;
  std::unique_ptr<QRhiBuffer> m_relativeChunkPosUbuf;
  std::unique_ptr<QRhiShaderResourceBindings> m_srb;
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

  // fps logic: time since last frame
  std::chrono::steady_clock::time_point m_lastFrameTime;
  float m_fps = 0.0f;
};

#endif // RHIRENDER_H
