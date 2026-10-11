#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
    // Scenario runs (QAMUI_RUNNER_SCRIPT/QAMUI_RUNNER_EVAL/QAMUI_RUNNER_CAPTURE_AFTER_MS) default to the
    // deterministic mock providers; set QAMUI_COCKPIT_MOCK=0 explicitly to run
    // a scenario against the live system.
    const bool scenarioRequested = qEnvironmentVariableIsSet("QAMUI_RUNNER_SCRIPT")
        || qEnvironmentVariableIsSet("QAMUI_RUNNER_EVAL")
        || qEnvironmentVariableIsSet("QAMUI_RUNNER_CAPTURE_AFTER_MS");
    if (scenarioRequested && !qEnvironmentVariableIsSet("QAMUI_COCKPIT_MOCK"))
        qputenv("QAMUI_COCKPIT_MOCK", "1");

    QCoreApplication app(argc, argv);
    app.setApplicationName("qamui_device_cockpit");

    QQmlApplicationEngine engine;
    engine.loadFromModule("Qamui.Examples.DeviceCockpit", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
