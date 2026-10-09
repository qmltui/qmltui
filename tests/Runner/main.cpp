#include <QCoreApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QUrl>

#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Runner/Runner.h>

int main(int argc, char* argv[])
{
    if (qEnvironmentVariableIsSet("QTUI_RUNNER_EMBEDDED_SCENARIO")
        || qEnvironmentVariableIsSet("QTUI_TEST_EMBEDDED_SCENARIO"))
        qputenv("QTUI_SCRIPT", QByteArrayLiteral(":/qtui-tests/resource_snapshot.js"));

    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(QString::fromUtf8(QTUI_RUNNER_FIXTURE_QML)));
    if (engine.rootObjects().isEmpty())
        return 2;

    auto* terminal = qobject_cast<qtui::Terminal*>(engine.rootObjects().constFirst());
    if (!terminal) {
        qCritical() << "Runner fixture root is not a Terminal";
        return 2;
    }

    if (!qtui::Runner::startFromEnvironment(&engine, terminal)) {
        qCritical() << "QTUI_SCRIPT is required by the Runner fixture";
        return 2;
    }

    return app.exec();
}
