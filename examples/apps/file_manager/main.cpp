#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
    // Automated scenario runs (QAMUI_RUNNER_SCRIPT/QAMUI_RUNNER_EVAL/QAMUI_RUNNER_CAPTURE_AFTER_MS) default to
    // deterministic mock filesystem; set QAMUI_FM_LIVE=1 explicitly to test live system.
    const bool scenarioRequested = qEnvironmentVariableIsSet("QAMUI_RUNNER_SCRIPT")
        || qEnvironmentVariableIsSet("QAMUI_RUNNER_EVAL")
        || qEnvironmentVariableIsSet("QAMUI_RUNNER_CAPTURE_AFTER_MS");

    if (scenarioRequested && !qEnvironmentVariableIsSet("QAMUI_FM_LIVE")) {
        qputenv("QAMUI_FM_MOCK", "1");
    }

    QCoreApplication app(argc, argv);
    app.setApplicationName("qamui_file_manager");

    QQmlApplicationEngine engine;
    engine.loadFromModule("Qamui.Examples.FileManager", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
