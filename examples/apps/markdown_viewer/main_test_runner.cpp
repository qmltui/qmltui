#include <QCoreApplication>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QSize>
#include <QTimer>

#include <Terminal.h>
#include <Testing/TestRunner.h>

namespace {
int environmentDimension(const char *name, int fallback) {
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    return ok && value > 0 ? value : fallback;
}
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.loadFromModule("Qtui.Examples.MarkdownViewer", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    auto *terminal = qobject_cast<qtui::Terminal *>(engine.rootObjects().constFirst());
    if (!terminal)
        return -1;

    QString artifacts = QString::fromUtf8(qgetenv("QTUI_TEST_ARTIFACT_DIR")).trimmed();
    if (artifacts.isEmpty())
        artifacts = QDir::temp().filePath(QStringLiteral("qtui-markdown-viewer-artifacts"));

    const QSize viewport(environmentDimension("QTUI_TEST_COLUMNS", 100),
                         environmentDimension("QTUI_TEST_ROWS", 32));
    auto *runner = new qtui::TestRunner(
        &engine, terminal,
        QStringLiteral(":/qt/qml/Qtui/Examples/MarkdownViewer/test_markdown_viewer.js"),
        artifacts, viewport, 2000, 15000, QString());
    QTimer::singleShot(0, runner, &qtui::TestRunner::start);
    return app.exec();
}
