// Scenario executable for the C++-only example.
//
// The Runner's auto-hook starts from a Terminal loaded by QML. This Terminal
// is constructed in C++, so the executable constructs the Runner itself: an
// "embedded" scenario. The application code is unchanged; only telemetry is
// left to the script, which calls tick() instead of waiting on a timer.

#include "ServiceDashboard.h"

#include <Qtui/Runner/Runner.h>

#include <QCoreApplication>
#include <QDir>
#include <QQmlEngine>
#include <QSize>
#include <QTimer>

namespace {

int environmentInteger(const char *name, int fallback)
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    return ok && value > 0 ? value : fallback;
}

} // namespace

int main(int argc, char *argv[])
{
    // Before any Terminal exists: the run stays off the real terminal.
    qtui::Runner::useEmbeddedScenario();

    QCoreApplication app(argc, argv);

    ServiceDashboard dashboard;

    // The scenario script runs in a JavaScript engine; the dashboard itself
    // never needs one. The engine must not take ownership of the stack object
    // it exposes as terminal.rootObject().
    QQmlEngine scriptEngine;
    QJSEngine::setObjectOwnership(&dashboard, QJSEngine::CppOwnership);

    QString script = QString::fromUtf8(qgetenv("QTUI_RUNNER_SCRIPT")).trimmed();
    if (script.isEmpty())
        script = QStringLiteral(QTUI_CPP_ONLY_SCENARIO);

    QString artifacts = QString::fromUtf8(qgetenv("QTUI_RUNNER_ARTIFACT_DIR")).trimmed();
    if (artifacts.isEmpty())
        artifacts = QDir::temp().filePath(QStringLiteral("qtui-cpp-only-artifacts"));

    const QSize viewport(environmentInteger("QTUI_RUNNER_COLUMNS", 80),
                         environmentInteger("QTUI_RUNNER_ROWS", 24));

    auto *runner = new qtui::Runner(&scriptEngine, &dashboard, script, artifacts, viewport,
                                    environmentInteger("QTUI_RUNNER_OPERATION_TIMEOUT_MS", 2000),
                                    environmentInteger("QTUI_RUNNER_TIMEOUT_MS", 10000),
                                    QString());
    QTimer::singleShot(0, runner, &qtui::Runner::start);
    return app.exec();
}
