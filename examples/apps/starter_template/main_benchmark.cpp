#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    qputenv("QTUI_STARTER_BACKEND", "observe");
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.loadFromModule("StarterTemplateApp", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
