#ifndef ENGINE_H
#define ENGINE_H

#include "world.h"
#include <QObject>
#include <QThreadPool>
#include <memory>
#include <qqmlintegration.h>

// #include "rhirender.h"
#include "rhiview.h"

class Engine : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(RHIView *rhiView READ rhiView WRITE setRHIView)

public:
  explicit Engine(QObject *parent = nullptr);
  ~Engine();

  void tick();

  RHIView *rhiView() const { return m_rhiView; }
  void setRHIView(RHIView *rhiView);

public slots:
  void handleRHIRenderReady();

private:
  std::unique_ptr<World> m_world;
  RHIView *m_rhiView = nullptr;
};

#endif // ENGINE_H