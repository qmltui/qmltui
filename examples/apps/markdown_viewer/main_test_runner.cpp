#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QSize>
#include <QTimer>

#include <Qamui/Core/items/Terminal.h>
#include <Qamui/Runner/Runner.h>

namespace {
int environmentDimension(const char *name, int fallback) {
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    return (ok && value > 0) ? value : fallback;
}
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qamui Markdown Viewer Runner"));
    parser.addHelpOption();
    QCommandLineOption scenarioOption(
        {QStringLiteral("s"), QStringLiteral("scenario")},
        QStringLiteral("Scenario JavaScript file to execute."),
        QStringLiteral("file"),
        QStringLiteral(":/qt/qml/Qamui/Examples/MarkdownViewer/test_markdown_viewer.js"));
    parser.addOption(scenarioOption);
    parser.process(app);

    QString script = parser.value(scenarioOption);
    if (script.isEmpty())
        script = QStringLiteral(":/qt/qml/Qamui/Examples/MarkdownViewer/test_markdown_viewer.js");

    if (qEnvironmentVariableIsSet("QAMUI_RUNNER_SCRIPT"))
        script = QString::fromUtf8(qgetenv("QAMUI_RUNNER_SCRIPT"));

    // Before QML loads: the Terminal created below stays quiet even when no
    // QAMUI_RUNNER_* variable announced a scenario (QAMUI_RUNNER_LIVE=1 still
    // renders to the terminal).
    qamui::Runner::useEmbeddedScenario();

    QQmlApplicationEngine engine;
    engine.loadFromModule("Qamui.Examples.MarkdownViewer", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    auto *terminal = qobject_cast<qamui::Terminal *>(engine.rootObjects().constFirst());
    if (!terminal)
        return -1;

    if (qEnvironmentVariableIsSet("QAMUI_RUNNER_CAPTURE_AFTER_MS") || qEnvironmentVariableIsSet("QAMUI_RUNNER_EVAL")) {
        if (qamui::Runner::startFromEnvironment(&engine, terminal))
            return app.exec();
    }

    QString artifacts = QString::fromUtf8(qgetenv("QAMUI_RUNNER_ARTIFACT_DIR")).trimmed();
    if (artifacts.isEmpty())
        artifacts = QDir::temp().filePath(QStringLiteral("qamui-markdown-viewer-artifacts"));

    const QSize viewport(environmentDimension("QAMUI_RUNNER_COLUMNS", 100),
                         environmentDimension("QAMUI_RUNNER_ROWS", 32));
    auto *runner = new qamui::Runner(
        &engine, terminal,
        script,
        artifacts, viewport, 2000, 15000, QString());
    QTimer::singleShot(0, runner, &qamui::Runner::start);
    return app.exec();
}
