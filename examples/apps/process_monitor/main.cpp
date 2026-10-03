#include "ProcessTableModel.h"
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  QQmlApplicationEngine engine;

  engine.loadFromModule("TopMonitorDemo", "Main");
  if (engine.rootObjects().isEmpty())
    return -1;

  return app.exec();
}
