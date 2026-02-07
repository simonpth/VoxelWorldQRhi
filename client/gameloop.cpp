#include "gameloop.h"
#include "region.h"

#include <QCoreApplication>
#include <QDebug>
#include <QObject>
#include <QThread>
#include <QThreadPool>
#include <QTransform>
#include <QtCore/qlogging.h>
#include <chrono>
#include <cstdint>

GameLoop::GameLoop(QObject *parent, World *world)
    : QObject(parent), m_world(world) {
  // also creates inital relative regions rendered positions

  m_playerPos.localPlayerPosition = QVector3D(0, 0, 0);
  m_playerPos.playerWorldChunkPos = PlayerWorldChunkPos(1, 18, 1);

  setRegionRenderDistance(2);
}

GameLoop::~GameLoop() {}

const int REGION_HEIGHT = 8; // 1024 blocks high

void GameLoop::setRegionRenderDistance(uint32_t regionRenderDistance) {
  m_regionRenderDistance = regionRenderDistance;

  m_relativeRegionsRenderedPositions.clear();

  int x = regionRenderDistance;
  int z = 0;
  int decision = 1 - regionRenderDistance;

  while (x >= z) {
    for (int ix = -x; ix <= x; ix++) {
      for (int iy = 0; iy < REGION_HEIGHT; iy++) {
        m_relativeRegionsRenderedPositions.push_back({ix, iy, z});
        if (z != 0) {
          m_relativeRegionsRenderedPositions.push_back({ix, iy, -z});
        }
      }
    }
    if (x != z && z != 0) {
      for (int ix = -z; ix <= z; ix++) {
        for (int iy = 0; iy < REGION_HEIGHT; iy++) {
          m_relativeRegionsRenderedPositions.push_back({ix, iy, x});
          if (x != 0) {
            m_relativeRegionsRenderedPositions.push_back({ix, iy, -x});
          }
        }
      }
    }

    z++;
    if (decision <= 0) {
      decision += z * 2 + 1;
    } else {
      x--;
      decision += (z - x) * 2 + 1;
    }
  }

  updateRegionsRendered();
}

void GameLoop::start() {
  if (m_running)
    return;
  m_running = true;
  QMetaObject::invokeMethod(this, "mainLoop", Qt::QueuedConnection);
}

void GameLoop::stop() { m_running = false; }

void GameLoop::mainLoop() {
  using namespace std::chrono;

  // Target 50ms per tick (20 TPS)
  const milliseconds TICK_INTERVAL(50);
  // Maximum catchup time to prevent spiral of death (e.g., 250ms = 5 ticks)
  const milliseconds MAX_CATCHUP(250);

  auto previousTime = steady_clock::now();
  nanoseconds lag(0);

  while (m_running) {
    auto currentTime = steady_clock::now();
    auto elapsed = currentTime - previousTime;
    previousTime = currentTime;

    lag += elapsed;

    // Spiral of death protection
    if (lag > MAX_CATCHUP) {
      lag = MAX_CATCHUP;
    }

    // Process game ticks
    while (lag >= TICK_INTERVAL && m_running) {
      tick();
      lag -= TICK_INTERVAL;
    }

    // Process events (e.g., signals for input)
    QCoreApplication::processEvents();

    if (!m_running)
      break;

    // Sleep if we have time
    auto timeToSleep = TICK_INTERVAL - lag;

    if (timeToSleep > milliseconds(1)) {
      auto ms = duration_cast<milliseconds>(timeToSleep).count();
      if (ms > 0) {
        QThread::msleep(static_cast<unsigned long>(ms));
      }
    }
  }
}

void GameLoop::tick() {
  // 20 ticks per second = 50ms per tick

  updatePlayerVelocity();

  m_velocity *= 0.6f;

  if (m_velocity.length() < 0.01f) {
    m_velocity = QVector3D(0, 0, 0);
  }

  m_playerPos.localPlayerPosition += m_velocity;
  m_interpolatedPlayerPos.localPlayerPosition = m_playerPos.localPlayerPosition;

  updateLocalPlayerPosition();
}

void GameLoop::updatePlayerVelocity() {
  QVector3D wishDir(0, 0, 0);

  if (m_playerInput.wPressed) {
    wishDir.setZ(wishDir.z() - 1);
  }
  if (m_playerInput.sPressed) {
    wishDir.setZ(wishDir.z() + 1);
  }
  if (m_playerInput.aPressed) {
    wishDir.setX(wishDir.x() - 1);
  }
  if (m_playerInput.dPressed) {
    wishDir.setX(wishDir.x() + 1);
  }

  if (m_playerInput.spacePressed) {
    wishDir.setY(wishDir.y() + 1);
  }
  if (m_playerInput.shiftPressed) {
    wishDir.setY(wishDir.y() - 1);
  }

  if (!wishDir.isNull()) {
    wishDir.normalize();

    // Rotate around y axis (Yaw)
    QTransform t;
    t.rotate(m_playerPos.cameraRotation.y());
    QPointF inputFlat(wishDir.x(), wishDir.z());
    QPointF rotatedFlat = t.map(inputFlat);

    QVector3D finalDir(rotatedFlat.x(), wishDir.y(), rotatedFlat.y());

    float acceleration = 0.20f; // Base acceleration
    if (m_playerInput.ctrlPressed) {
      acceleration *= 2.0f;
    }

    m_velocity += finalDir * acceleration;
  }
}

void GameLoop::updateLocalPlayerPosition() {
  bool horizontalRegionChanged = false;
  if (m_playerPos.localPlayerPosition.x() > Chunk::CHUNK_SIZE) {
    m_playerPos.localPlayerPosition -= QVector3D(Chunk::CHUNK_SIZE, 0.0f, 0.0f);
    m_playerPos.playerWorldChunkPos += PlayerWorldChunkPos(1, 0, 0);
    if (m_playerPos.playerWorldChunkPos.x % Region::REGION_SIZE == 0) {
      horizontalRegionChanged = true;
    }
  }

  if (m_playerPos.localPlayerPosition.x() < 0) {
    m_playerPos.localPlayerPosition += QVector3D(Chunk::CHUNK_SIZE, 0.0f, 0.0f);
    m_playerPos.playerWorldChunkPos -= PlayerWorldChunkPos(1, 0, 0);
    if (m_playerPos.playerWorldChunkPos.x % Region::REGION_SIZE == Region::REGION_SIZE - 1) {
      horizontalRegionChanged = true;
    }
  }

  if (m_playerPos.localPlayerPosition.y() > Chunk::CHUNK_SIZE) {
    m_playerPos.localPlayerPosition -= QVector3D(0.0f, Chunk::CHUNK_SIZE, 0.0f);
    m_playerPos.playerWorldChunkPos += PlayerWorldChunkPos(0, 1, 0);
  }

  if (m_playerPos.localPlayerPosition.y() < 0) {
    m_playerPos.localPlayerPosition += QVector3D(0.0f, Chunk::CHUNK_SIZE, 0.0f);
    m_playerPos.playerWorldChunkPos -= PlayerWorldChunkPos(0, 1, 0);
  }

  if (m_playerPos.localPlayerPosition.z() > Chunk::CHUNK_SIZE) {
    m_playerPos.localPlayerPosition -= QVector3D(0.0f, 0.0f, Chunk::CHUNK_SIZE);
    m_playerPos.playerWorldChunkPos += PlayerWorldChunkPos(0, 0, 1);
    if (m_playerPos.playerWorldChunkPos.z % Region::REGION_SIZE == 0) {
      horizontalRegionChanged = true;
    }
  }

  if (m_playerPos.localPlayerPosition.z() < 0) {
    m_playerPos.localPlayerPosition += QVector3D(0.0f, 0.0f, Chunk::CHUNK_SIZE);
    m_playerPos.playerWorldChunkPos -= PlayerWorldChunkPos(0, 0, 1);
    if (m_playerPos.playerWorldChunkPos.z % Region::REGION_SIZE == Region::REGION_SIZE - 1) {
      horizontalRegionChanged = true;
    }
  }

  if (horizontalRegionChanged) {
    updateRegionsRendered();
  }
}

void GameLoop::updateRegionsRendered() {
  std::lock_guard<std::mutex> lock(m_regionsRenderedMutex);

  m_regionsRendered.clear();

  RegionPos playerRegionPos =
      RegionPos(m_playerPos.playerWorldChunkPos.x / Region::REGION_SIZE,
                m_playerPos.playerWorldChunkPos.y / Region::REGION_SIZE,
                m_playerPos.playerWorldChunkPos.z / Region::REGION_SIZE);

  for (const auto &relativeRegionPos : m_relativeRegionsRenderedPositions) {
    RegionPos targetRegion = relativeRegionPos.addHorizontal(playerRegionPos);

    m_regionsRendered.push_back(targetRegion);

    if (!m_world->region(targetRegion)) {
      QThreadPool::globalInstance()->start([this, targetRegion]() {
        m_world->generateRegion(targetRegion);

        emit regionGenerated(targetRegion);
      });
    }
  }

  m_regionsRenderedDirty = true;
}

void GameLoop::handleKeyPressed(Qt::Key key) {
  switch (key) {
  case Qt::Key_W:
    m_playerInput.wPressed = true;
    break;
  case Qt::Key_S:
    m_playerInput.sPressed = true;
    break;
  case Qt::Key_A:
    m_playerInput.aPressed = true;
    break;
  case Qt::Key_D:
    m_playerInput.dPressed = true;
    break;
  case Qt::Key_Space:
    m_playerInput.spacePressed = true;
    break;
  case Qt::Key_Shift:
    m_playerInput.shiftPressed = true;
    break;
  case Qt::Key_Control:
    m_playerInput.ctrlPressed = true;
    break;
  default:
    break;
  }
}

void GameLoop::handleKeyReleased(Qt::Key key) {
  switch (key) {
  case Qt::Key_W:
    m_playerInput.wPressed = false;
    break;
  case Qt::Key_S:
    m_playerInput.sPressed = false;
    break;
  case Qt::Key_A:
    m_playerInput.aPressed = false;
    break;
  case Qt::Key_D:
    m_playerInput.dPressed = false;
    break;
  case Qt::Key_Space:
    m_playerInput.spacePressed = false;
    break;
  case Qt::Key_Shift:
    m_playerInput.shiftPressed = false;
    break;
  case Qt::Key_Control:
    m_playerInput.ctrlPressed = false;
    break;
  default:
    break;
  }
}

void GameLoop::handleMouseDelta(const QVector2D &delta) {
  float sensitivity = 0.15f;
  m_playerPos.cameraRotation +=
      QVector3D(delta.y() * sensitivity, delta.x() * sensitivity, 0.0f);

  // Clamp pitch to avoid flipping (around -89 to 89 degrees)
  if (m_playerPos.cameraRotation.x() > 89.0f)
    m_playerPos.cameraRotation.setX(89.0f);
  if (m_playerPos.cameraRotation.x() < -89.0f)
    m_playerPos.cameraRotation.setX(-89.0f);

  // sync interpolated player position
  m_interpolatedPlayerPos.cameraRotation = m_playerPos.cameraRotation;
}