#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QTimer>
#include <QDebug>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    QQmlEngine engine;
    
    // Load the QML file
#ifdef QAMUI_AGENT_STREAM_SOURCE
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(QAMUI_AGENT_STREAM_SOURCE)));
#else
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qt/qml/AgentStreamDemo/Main.qml")));
#endif
    
    if (component.isError()) {
        qWarning() << component.errors();
        return -1;
    }
    
    QObject *topLevel = component.create();
    if (!topLevel) {
        qWarning() << "Failed to create QML component.";
        return -1;
    }

    // Connect to quit signal
    QObject::connect(&engine, &QQmlEngine::quit, &app, &QCoreApplication::quit);

    return app.exec();
}
