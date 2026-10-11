#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    qputenv("QTUI_STARTER_BACKEND", "mock");
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("StarterTemplateApp", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
