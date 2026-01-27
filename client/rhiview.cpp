#include "rhiview.h"
#include "rhirender.h"
#include <QtCore/QRunnable>
#include <QtCore/qnamespace.h>

RHIView::RHIView() {
  connect(this, &QQuickItem::windowChanged, this,
          &RHIView::handleWindowChanged);
}

void RHIView::handleWindowChanged(QQuickWindow *win) {
  if (win) {
    connect(win, &QQuickWindow::beforeSynchronizing, this, &RHIView::sync,
            Qt::DirectConnection);
    connect(win, &QQuickWindow::sceneGraphInvalidated, this, &RHIView::cleanup,
            Qt::DirectConnection);
    win->setColor(Qt::blue);
  }
}

// cleanup
void RHIView::cleanup() {
  delete m_rhiRender;
  m_rhiRender = nullptr;
}

class CleanupJob : public QRunnable {
public:
  CleanupJob(RHIRender *renderer) : m_rhiRender(renderer) {}
  void run() override { delete m_rhiRender; }

private:
  RHIRender *m_rhiRender;
};

void RHIView::releaseResources() {
  window()->scheduleRenderJob(new CleanupJob(m_rhiRender),
                              QQuickWindow::BeforeSynchronizingStage);
  m_rhiRender = nullptr;
}

// setup rhi render

void RHIView::sync() {
  // This function is invoked on the render thread, if there is one.

  if (!m_rhiRender) {
    m_rhiRender = new RHIRender;
    // Initializing resources is done before starting to record the
    // renderpass, regardless of wanting an underlay or overlay.
    connect(window(), &QQuickWindow::beforeRendering, m_rhiRender,
            &RHIRender::frameStart, Qt::DirectConnection);
    // Here we want an underlay and therefore connect to
    // beforeRenderPassRecording. Changing to afterRenderPassRecording
    // would render the squircle on top (overlay).
    connect(window(), &QQuickWindow::beforeRenderPassRecording, m_rhiRender,
            &RHIRender::mainPassRecordingStart, Qt::DirectConnection);
    emit rhiRenderReady();
  }
  m_rhiRender->setWindow(window());
}