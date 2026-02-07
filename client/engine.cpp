#include "engine.h"

Engine::Engine(QObject *parent) : QObject(parent) {
  // setup world
  m_world.reset(new World());

  // Setup game loop
  m_gameLoop = std::make_unique<GameLoop>(nullptr, m_world.get());
  m_gameLoop->moveToThread(&m_gameLoopThread);
  connect(&m_gameLoopThread, &QThread::started, m_gameLoop.get(), &GameLoop::start);
  m_gameLoopThread.setObjectName("GameLoopThread");
  m_gameLoopThread.start();
  
}

Engine::~Engine() {
  m_gameLoop->stop();
  m_gameLoopThread.quit();
  m_gameLoopThread.wait();
}

void Engine::startGameLoop() { m_gameLoop->start(); }
void Engine::stopGameLoop() { m_gameLoop->stop(); }

void Engine::moveMouseToCenter(QQuickWindow *window) {
  if (!window)
    return;

  QPoint center =
      window->mapToGlobal(QPoint(window->width() / 2, window->height() / 2));
  QCursor::setPos(center);
}
