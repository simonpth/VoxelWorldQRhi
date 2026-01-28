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
  Q_PROPERTY(PlayerWorldChunkPos playerWorldChunkPos READ playerWorldChunkPos
                 WRITE setPlayerWorldChunkPos)
  Q_PROPERTY(QVector3D localPlayerPosition READ localPlayerPosition WRITE
                 setLocalPlayerPosition)
  Q_PROPERTY(
      QVector3D cameraRotation READ cameraRotation WRITE setCameraRotation)

public:
  RHIView();

  float fps() const { return m_rhiRender ? m_rhiRender->fps() : 0.0f; }
  PlayerWorldChunkPos playerWorldChunkPos() const {
    return m_rhiRender ? m_rhiRender->playerWorldChunkPos()
                       : PlayerWorldChunkPos();
  }
  QVector3D localPlayerPosition() const {
    return m_rhiRender ? m_rhiRender->localPlayerPosition() : QVector3D();
  }
  QVector3D cameraRotation() const {
    return m_rhiRender ? m_rhiRender->cameraRotation() : QVector3D();
  }

  void setPlayerWorldChunkPos(const PlayerWorldChunkPos &position) {
    if (m_rhiRender)
      m_rhiRender->setPlayerWorldChunkPos(position);
  }
  void setLocalPlayerPosition(const QVector3D &position) {
    if (m_rhiRender)
      m_rhiRender->setLocalPlayerPosition(position);
  }
  void setCameraRotation(const QVector3D &rotation) {
    if (m_rhiRender)
      m_rhiRender->setCameraRotation(rotation);
  }

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