#include "renderwidget.h"
#include <QApplication>
#include <QLoggingCategory>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  QLoggingCategory::setFilterRules(QLatin1String("qt.rhi.*=true"));

  RenderWidget w;
  w.show();

  return a.exec();
}
