#include "renderwidget.h"
#include <QFile>

RenderWidget::RenderWidget(QWidget *parent) : QRhiWidget(parent) {}

RenderWidget::~RenderWidget() {}

static QShader getShader(const QString &name) {
  QFile f(name);
  return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll())
                                     : QShader();
}
