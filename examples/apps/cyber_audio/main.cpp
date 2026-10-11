#include "AudioEngine.h"
#include "Startup.h"

#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <memory>

#include <cstdio>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("qamui_cyber_audio"));
    app.setApplicationVersion(QStringLiteral("1.0"));

    StartupOptions options = parseStartupOptions(app);
    if (!AudioEngine::multimediaAvailable() && (!options.inputs.isEmpty() || options.mic)) {
        std::fprintf(stderr, "qamui_cyber_audio: built without Qt Multimedia; no audio source available "
                             "(use --demo for the showcase).\n");
        options.inputs.clear();
        options.mic = false;
    }

    // Keep libav / PipeWire chatter off the terminal while the UI owns the screen.
    // Only needed when a real backend can run; the demo source prints nothing.
    std::unique_ptr<StderrRedirect> quietStderr;
    if (AudioEngine::multimediaAvailable() && !options.demo)
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
