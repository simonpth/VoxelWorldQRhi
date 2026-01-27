#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  // QLoggingCategory::setFilterRules(QLatin1String("qt.rhi.*=true"));

  QQmlApplicationEngine engine;
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.loadFromModule("Client", "Main");

  return app.exec();
}
