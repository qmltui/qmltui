#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QDir>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // Add build directory import paths for QML modules (Terminal, Terminal.Widgets, Terminal.Controls)
    QString appDir = QCoreApplication::applicationDirPath();
    engine.addImportPath(appDir);
    engine.addImportPath(QDir(appDir).filePath(".."));
    engine.addImportPath(QDir(appDir).filePath("../Terminal"));

    engine.loadFromModule("qtasktree_runner", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
