#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QSize>
#include <QTimer>
#include <QUrl>

#include <Qamui/Core/items/Terminal.h>
#include <Qamui/Runner/Runner.h>

#include <cstring>
#include <optional>

// The Runner fixture. Without arguments it is driven through the environment
// (QAMUI_RUNNER_SCRIPT, _EVAL, _CAPTURE_AFTER_MS) like any Runner-linked
// application. With --embedded it constructs the Runner itself on the
// resource scenario and controls it through the C++ API alone: it removes the
// QAMUI_RUNNER_EMBEDDED_SCENARIO and QAMUI_RUNNER_LIVE hints from its
// environment first, so a quiet or live run proves useEmbeddedScenario() and
// setLiveOutput(), and --update-snapshots / --no-update-snapshots prove that
// setUpdateSnapshots() wins over QAMUI_RUNNER_UPDATE_SNAPSHOTS. --scenario=<path>
// selects another filesystem or resource scenario for that path.
namespace {
int environmentInteger(const char* name, int fallback)
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    return (ok && value > 0) ? value : fallback;
}
}

int main(int argc, char* argv[])
{
    bool embedded = false;
    bool live = false;
    bool liveAfterConstruction = false;
    std::optional<bool> updateSnapshots;
    QString script = QStringLiteral(":/qamui-tests/resource_snapshot.js");
    QString fixtureQml = QString::fromUtf8(QAMUI_RUNNER_FIXTURE_QML);
    for (int i = 1; i < argc; ++i) {
        const QByteArray argument(argv[i]);
        if (argument.startsWith("--scenario=")) {
            script = QString::fromUtf8(argument.mid(int(strlen("--scenario="))));
        } else if (argument == "--fixture=inline") {
            // Inline-mode fixture with a Scrollback, for the scrollback
            // assertions. Other fixtures reuse this file without one.
#ifdef QAMUI_RUNNER_INLINE_FIXTURE_QML
            fixtureQml = QString::fromUtf8(QAMUI_RUNNER_INLINE_FIXTURE_QML);
#else
            qCritical() << "This fixture has no inline variant";
            return 2;
#endif
        } else if (argument == "--embedded") {
            embedded = true;
        } else if (argument == "--live") {
            live = true;
        } else if (argument == "--live-after-construction") {
            liveAfterConstruction = true;
        } else if (argument == "--update-snapshots") {
            updateSnapshots = true;
        } else if (argument == "--no-update-snapshots") {
            updateSnapshots = false;
        } else {
            qCritical() << "Unknown Runner fixture argument" << argument;
            return 2;
        }
    }
    if (embedded) {
        qunsetenv("QAMUI_RUNNER_EMBEDDED_SCENARIO");
        qunsetenv("QAMUI_RUNNER_LIVE");
        qamui::Runner::useEmbeddedScenario(live);
    } else if (qEnvironmentVariableIsSet("QAMUI_RUNNER_EMBEDDED_SCENARIO")) {
        // The environment hint alone selects the embedded path too; the engine
        // then owes its quiet startup to the variable, not to the API.
        embedded = true;
    }

    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(fixtureQml));
    if (engine.rootObjects().isEmpty())
        return 2;

    auto* terminal = qobject_cast<qamui::Terminal*>(engine.rootObjects().constFirst());
    if (!terminal) {
        qCritical() << "Runner fixture root is not a Terminal";
        return 2;
    }

    if (!embedded) {
        if (!qamui::Runner::startFromEnvironment(&engine, terminal)) {
            qCritical() << "QAMUI_RUNNER_SCRIPT is required by the Runner fixture";
            return 2;
        }
        return app.exec();
    }

    QString artifacts = QString::fromUtf8(qgetenv("QAMUI_RUNNER_ARTIFACT_DIR")).trimmed();
    if (artifacts.isEmpty())
        artifacts = QDir::temp().filePath(QStringLiteral("qamui-runner-fixture-artifacts"));
    const QSize viewport(environmentInteger("QAMUI_RUNNER_COLUMNS", 80),
                         environmentInteger("QAMUI_RUNNER_ROWS", 24));
    auto* runner = new qamui::Runner(
        &engine, terminal,
        script,
        artifacts, viewport,
        environmentInteger("QAMUI_RUNNER_OPERATION_TIMEOUT_MS", 2000),
        environmentInteger("QAMUI_RUNNER_TIMEOUT_MS", 10000),
        QString());
    if (liveAfterConstruction)
        runner->setLiveOutput(true);
    if (updateSnapshots.has_value())
        runner->setUpdateSnapshots(*updateSnapshots);
    QTimer::singleShot(0, runner, &qamui::Runner::start);
    return app.exec();
}
