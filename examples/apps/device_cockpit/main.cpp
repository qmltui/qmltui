#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
    // Scenario runs (QTUI_SCRIPT/QTUI_EVAL/QTUI_INSPECT) default to the
    // deterministic mock providers; set QTUI_COCKPIT_MOCK=0 explicitly to run
    // a scenario against the live system.
    const bool scenarioRequested = qEnvironmentVariableIsSet("QTUI_SCRIPT")
        || qEnvironmentVariableIsSet("QTUI_EVAL")
        || qEnvironmentVariableIsSet("QTUI_INSPECT");
    if (scenarioRequested && !qEnvironmentVariableIsSet("QTUI_COCKPIT_MOCK"))
        qputenv("QTUI_COCKPIT_MOCK", "1");

    QCoreApplication app(argc, argv);
    app.setApplicationName("qtui_device_cockpit");

    QQmlApplicationEngine engine;
    engine.loadFromModule("Qtui.Examples.DeviceCockpit", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
