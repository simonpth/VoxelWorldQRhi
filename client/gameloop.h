#ifndef GAMELOOP_H
#define GAMELOOP_H

#include "world.h"
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVector2D>
#include <QVector3D>
#include <QtGui/qvectornd.h>
#include <atomic>
#include <qqmlintegration.h>

class GameLoop : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(PlayerWorldChunkPos playerWorldChunkPos READ playerWorldChunkPos)
  Q_PROPERTY(QVector3D localPlayerPosition READ localPlayerPosition)
  Q_PROPERTY(QVector3D cameraRotation READ cameraRotation)
  Q_PROPERTY(QVector3D velocity READ velocity)
  Q_PROPERTY(uint32_t regionRenderDistance READ regionRenderDistance WRITE
                 setRegionRenderDistance)

public:
  Q_INVOKABLE QString getPlayerWorldChunkPosString() const {
    return QString("%1, %2, %3")
        .arg(m_playerPos.playerWorldChunkPos.x)
        .arg(m_playerPos.playerWorldChunkPos.y)
        .arg(m_playerPos.playerWorldChunkPos.z);
  }

public:
  explicit GameLoop(QObject *parent = nullptr, World *world = nullptr);
  ~GameLoop();

  // getters for debug
  PlayerWorldChunkPos playerWorldChunkPos() const {
    return m_playerPos.playerWorldChunkPos;
  }
  QVector3D localPlayerPosition() const {
    return m_playerPos.localPlayerPosition;
  }
  QVector3D cameraRotation() const { return m_playerPos.cameraRotation; }
  QVector3D velocity() const { return m_velocity; }

  uint32_t regionRenderDistance() const { return m_regionRenderDistance; }
  void setRegionRenderDistance(uint32_t regionRenderDistance);

  std::vector<RegionPos> regionsRendered() {
    std::lock_guard<std::mutex> lock(m_regionsRenderedMutex);
    return m_regionsRendered;
  }

  int regionsRenderedSize() {
    std::lock_guard<std::mutex> lock(m_regionsRenderedMutex);
    return m_regionsRendered.size();
  }

  bool regionsRenderedDirty() const { return m_regionsRenderedDirty; }
  void setRegionsRenderedDirty(bool regionsRenderedDirty) {
    m_regionsRenderedDirty = regionsRenderedDirty;
  }

public slots:
  void start();
  void stop();

  void handleKeyPressed(Qt::Key key);
  void handleKeyReleased(Qt::Key key);

  void handleMouseDelta(const QVector2D &delta);

private slots:
  void mainLoop();

private:
  World *m_world;

  void tick();
  void updatePlayerVelocity();
  void updateLocalPlayerPosition();

  PlayerPos m_playerPos;
  PlayerPos m_interpolatedPlayerPos;

  QVector3D m_velocity;

  struct PlayerInput {
    bool wPressed = false;
    bool sPressed = false;
    bool aPressed = false;
    bool dPressed = false;
    bool spacePressed = false;
    bool shiftPressed = false;
    bool ctrlPressed = false;
  };

  PlayerInput m_playerInput;

  std::atomic<uint32_t> m_regionRenderDistance;
  std::vector<RegionPos> m_relativeRegionsRenderedPositions;

  std::mutex m_regionsRenderedMutex;
  std::vector<RegionPos> m_regionsRendered;
  std::atomic<bool> m_regionsRenderedDirty = true;
  void clearRegionsRenderedDirty() { m_regionsRenderedDirty = false; }
  void updateRegionsRendered();

  std::atomic<bool> m_running{false};
};

#endif // GAMELOOP_H