#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
    // Automated scenario runs (QTUI_SCRIPT/QTUI_EVAL/QTUI_INSPECT) default to
    // deterministic mock filesystem; set QTUI_FM_LIVE=1 explicitly to test live system.
    const bool scenarioRequested = qEnvironmentVariableIsSet("QTUI_SCRIPT")
        || qEnvironmentVariableIsSet("QTUI_EVAL")
        || qEnvironmentVariableIsSet("QTUI_INSPECT");

    if (scenarioRequested && !qEnvironmentVariableIsSet("QTUI_FM_LIVE")) {
        qputenv("QTUI_FM_MOCK", "1");
    }

    QCoreApplication app(argc, argv);
    app.setApplicationName("qtui_file_manager");

    QQmlApplicationEngine engine;
    engine.loadFromModule("Qtui.Examples.FileManager", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
