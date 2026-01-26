#pragma once

#include <QRhiWidget>
#include <rhi/qrhi.h>

class RenderWidget : public QRhiWidget {
  Q_OBJECT

public:
  explicit RenderWidget(QWidget *parent = nullptr);
  ~RenderWidget();

  void initialize(QRhiCommandBuffer *cb) override;
  void render(QRhiCommandBuffer *cb) override;

private:
  QRhi *m_rhi = nullptr;
  QMatrix4x4 m_viewProjection;
  float m_rotation = 0.0f;
};
