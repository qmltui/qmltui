// Runner-linked entry point. Linking Qamui::Runner installs the Terminal auto-hook
// that honours QAMUI_RUNNER_SCRIPT / QAMUI_RUNNER_EVAL / QAMUI_RUNNER_CAPTURE_AFTER_MS. Scenario runs pass no
// arguments, so without inputs the deterministic demo source is selected; manual
// QAMUI_RUNNER_EVAL runs can still pass files or --mic to exercise the real backends.
#include "AudioEngine.h"
#include "Startup.h"

#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <memory>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("qamui_cyber_audio_test"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    StartupOptions options = parseStartupOptions(app);
    if (options.inputs.isEmpty() && !options.mic && !options.idle)
        options.demo = true;

    // Keep libav / PipeWire chatter off the terminal while the UI owns the screen.
    // Quiet scenario runs (QAMUI_RUNNER_SCRIPT / QAMUI_RUNNER_EVAL / QAMUI_RUNNER_CAPTURE_AFTER_MS without
    // QAMUI_RUNNER_LIVE) render to committed frames, not the tty, and the runner's own
    // summary must stay on stderr, so they are left alone.
    const bool scriptedQuiet = (qEnvironmentVariableIsSet("QAMUI_RUNNER_SCRIPT")
                                || qEnvironmentVariableIsSet("QAMUI_RUNNER_EVAL")
                                || qEnvironmentVariableIsSet("QAMUI_RUNNER_CAPTURE_AFTER_MS"))
                               && !qEnvironmentVariableIsSet("QAMUI_RUNNER_LIVE");
    std::unique_ptr<StderrRedirect> quietStderr;
    if (AudioEngine::multimediaAvailable() && !options.demo && !scriptedQuiet)
        quietStderr = std::make_unique<StderrRedirect>(StderrRedirect::defaultLogPath(app.applicationName()));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("cliOptions"), toVariantMap(options));
    engine.loadFromModule("CyberAudio", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    const int result = app.exec();
    // Restore stderr before the QML engine (and Qamui's buffered Qt messages) go away.
    quietStderr.reset();
    return result;
}
