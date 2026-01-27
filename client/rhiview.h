#ifndef RHIVIEW_H
#define RHIVIEW_H

#include "rhirender.h"
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <qqmlintegration.h>

class RHIView : public QQuickItem {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(float fps READ fps)

public:
  RHIView();

  float fps() const { return m_rhiRender ? m_rhiRender->fps() : 0.0f; }

  RHIRender *rhiRender() const { return m_rhiRender; }

public slots:
  void sync();
  void cleanup();

private slots:
  void handleWindowChanged(QQuickWindow *win);

signals:
  void rhiRenderReady();

private:
  void releaseResources() override;

  RHIRender *m_rhiRender;
};

#endif // RHIVIEW_H