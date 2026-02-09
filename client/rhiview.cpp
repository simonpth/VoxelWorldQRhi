#include "rhiview.h"
#include "rhirender.h"
#include <QtCore/QRunnable>
#include <QtCore/qnamespace.h>
#include <QtGui/qcolor.h>
#include <QtQuick/QQuickGraphicsConfiguration>

RHIView::RHIView() {
  connect(this, &QQuickItem::windowChanged, this,
          &RHIView::handleWindowChanged);
}

void RHIView::handleWindowChanged(QQuickWindow *win) {
  if (win) {
    // Disable VSync by setting swap interval to 0
    //QSurfaceFormat format = win->format();
    //format.setSwapInterval(0);
    //win->setFormat(format);

    connect(win, &QQuickWindow::beforeSynchronizing, this, &RHIView::sync,
            Qt::DirectConnection);
    connect(win, &QQuickWindow::sceneGraphInvalidated, this, &RHIView::cleanup,
            Qt::DirectConnection);
    // sky blue
    win->setColor(QColor::fromRgb(160, 217, 239));
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

    m_rhiRender->setEngine(m_engine);
  }
  m_rhiRender->setWindow(window());
}