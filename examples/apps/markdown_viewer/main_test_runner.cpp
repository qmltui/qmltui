#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QSize>
#include <QTimer>

#include <Terminal.h>
#include <TestRunner/TestRunner.h>

namespace {
int environmentDimension(const char *name, int fallback) {
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    return ok && value > 0 ? value : fallback;
}
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qtui Markdown Viewer Test Runner"));
    parser.addHelpOption();
    QCommandLineOption scenarioOption(
        {QStringLiteral("s"), QStringLiteral("scenario")},
        QStringLiteral("Scenario JavaScript file to execute."),
        QStringLiteral("file"),
        QStringLiteral(":/qt/qml/Qtui/Examples/MarkdownViewer/test_markdown_viewer.js"));
    parser.addOption(scenarioOption);
    parser.process(app);

    QString script = parser.value(scenarioOption);
    if (script.isEmpty())
        script = QStringLiteral(":/qt/qml/Qtui/Examples/MarkdownViewer/test_markdown_viewer.js");

    if (qEnvironmentVariableIsSet("QTUI_SCRIPT"))
        script = QString::fromUtf8(qgetenv("QTUI_SCRIPT"));

    QQmlApplicationEngine engine;
    engine.loadFromModule("Qtui.Examples.MarkdownViewer", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    auto *terminal = qobject_cast<qtui::Terminal *>(engine.rootObjects().constFirst());
    if (!terminal)
        return -1;

    if (qEnvironmentVariableIsSet("QTUI_CAPTURE_AFTER_MS") || qEnvironmentVariableIsSet("QTUI_EVAL")) {
        if (qtui::TestRunner::startFromEnvironment(&engine, terminal))
            return app.exec();
    }

    QString artifacts = QString::fromUtf8(qgetenv("QTUI_TEST_ARTIFACT_DIR")).trimmed();
    if (artifacts.isEmpty())
        artifacts = QDir::temp().filePath(QStringLiteral("qtui-markdown-viewer-artifacts"));

    const QSize viewport(environmentDimension("QTUI_TEST_COLUMNS", 100),
                         environmentDimension("QTUI_TEST_ROWS", 32));
    auto *runner = new qtui::TestRunner(
        &engine, terminal,
        script,
        artifacts, viewport, 2000, 15000, QString());
    QTimer::singleShot(0, runner, &qtui::TestRunner::start);
    return app.exec();
}
