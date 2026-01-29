#ifndef RHIRENDER_H
#define RHIRENDER_H

#include <QtCore/qobject.h>
#include <QtQuick/qquickitem.h>
#include <rhi/qrhi.h>

#include "chunkmeshgeneration.h"
#include "world.h"
#include <unordered_map>

struct RenderChunkMesh {
  std::unique_ptr<ChunkMesh> chunkMesh;
  std::array<std::unique_ptr<QRhiBuffer>, 6> vBuffers;
  std::array<std::unique_ptr<QRhiBuffer>, 6> iBuffers;

  RenderChunkMesh(std::unique_ptr<ChunkMesh> chunkMesh, QRhi *rhi) {
    this->chunkMesh = std::move(chunkMesh);
    for (int i = 0; i < 6; i++) {
      vBuffers[i].reset(rhi->newBuffer(
          QRhiBuffer::Static, QRhiBuffer::VertexBuffer,
          this->chunkMesh->faces[i].vertices.size() * sizeof(uint64_t)));
      vBuffers[i]->create();
      iBuffers[i].reset(rhi->newBuffer(
          QRhiBuffer::Static, QRhiBuffer::IndexBuffer,
          this->chunkMesh->faces[i].indices.size() * sizeof(uint32_t)));
      iBuffers[i]->create();
    }
  }
};

class RHIRender : public QObject {
  Q_OBJECT

public:
  explicit RHIRender(QObject *parent = nullptr);
  ~RHIRender();

  void setWindow(QQuickWindow *window);

public slots:
  void frameStart();
  void mainPassRecordingStart();

private:
  void initPipeline(QRhi *rhi, QRhiSwapChain *swapChain);
  void updateDirtyRenderChunkMeshes(QRhiResourceUpdateBatch *resourceUpdates);

public:
  void setPlayerWorldChunkPos(const PlayerWorldChunkPos &position);
  PlayerWorldChunkPos playerWorldChunkPos() const;
  void setLocalPlayerPosition(const QVector3D &position);
  QVector3D localPlayerPosition() const;
  void setCameraRotation(const QVector3D &rotation);
  QVector3D cameraRotation() const;

  void addChunkMesh(const WorldChunkPos &chunkPos,
                    std::unique_ptr<ChunkMesh> chunkMesh);
  void removeChunkMesh(const WorldChunkPos &chunkPos);
  void updateChunkMesh(const WorldChunkPos &chunkPos,
                       std::unique_ptr<ChunkMesh> chunkMesh);

  float fps() const { return m_fps; }

private:
  QQuickWindow *m_window = nullptr;
  // rhi available at m_window->rhi();

  std::unique_ptr<QRhiBuffer> m_ubuf;
  std::unique_ptr<QRhiShaderResourceBindings> m_srb;
  std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

  PlayerWorldChunkPos m_playerWorldChunkPos = {0, 0, 0};
  QVector3D m_localPlayerPosition =
      QVector3D(16.0f, 30.0f, 60.0f); // local player position in chunk [0...32]
  QVector3D m_cameraRotation = QVector3D(0, 0, 0);

  std::unordered_map<WorldChunkPos, std::unique_ptr<RenderChunkMesh>,
                     WorldChunkPosHash>
      m_renderChunkMeshes;

  std::vector<RenderChunkMesh *> m_dirtyRenderChunkMeshes;

  // fps logic: time since last frame
  std::chrono::steady_clock::time_point m_lastFrameTime;
  float m_fps = 0.0f;
};

#endif // RHIRENDER_H
