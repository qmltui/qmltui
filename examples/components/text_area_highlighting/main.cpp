#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("TextAreaHighlightingDemo", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
