#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QDir>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.addImportPath(QDir(QCoreApplication::applicationDirPath()).filePath("../../.."));
    engine.loadFromModule("BrailleDemo", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
