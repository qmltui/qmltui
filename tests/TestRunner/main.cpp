#include <QCoreApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QUrl>

#include <Qtui/Core/items/Terminal.h>
#include <Qtui/TestRunner/TestRunner.h>

int main(int argc, char* argv[])
{
    if (qEnvironmentVariableIsSet("QTUI_TEST_EMBEDDED_SCENARIO"))
        qputenv("QTUI_SCRIPT", QByteArrayLiteral(":/qtui-tests/resource_snapshot.js"));

    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(QString::fromUtf8(QTUI_TEST_RUNNER_FIXTURE_QML)));
    if (engine.rootObjects().isEmpty())
        return 2;

    auto* terminal = qobject_cast<qtui::Terminal*>(engine.rootObjects().constFirst());
    if (!terminal) {
        qCritical() << "TestRunner fixture root is not a Terminal";
        return 2;
    }

    if (!qtui::TestRunner::startFromEnvironment(&engine, terminal)) {
        qCritical() << "QTUI_SCRIPT is required by the TestRunner fixture";
        return 2;
    }

    return app.exec();
}
