#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QDir>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../../qml");
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../qml");
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");
    engine.loadFromModule("ProgressBarDemo", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
