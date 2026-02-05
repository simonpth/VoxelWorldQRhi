#ifndef ENGINE_H
#define ENGINE_H

#include "gameloop.h"
#include "world.h"

#include <QCursor>
#include <QObject>
#include <QQuickWindow>
#include <QThread>
#include <memory>
#include <qqmlintegration.h>

class Engine : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(GameLoop *gameLoop READ gameLoop)

public:
  explicit Engine(QObject *parent = nullptr);
  ~Engine();

  void startGameLoop();
  void stopGameLoop();

  GameLoop *gameLoop() const { return m_gameLoop; }

  World *world() const { return m_world.get(); }

public slots:
  void moveMouseToCenter(QQuickWindow *window);

private:
  std::unique_ptr<World> m_world;

  QThread m_gameLoopThread;
  GameLoop *m_gameLoop;
};

#endif // ENGINE_H