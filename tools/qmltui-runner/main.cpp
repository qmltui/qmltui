#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUuid>

namespace {
QJsonObject readObject(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

int error(const QString& message) {
    QTextStream(stderr) << "qmltui-runner: " << message << '\n';
    return 2;
}

void stop(QProcess& child) {
    if (child.state() == QProcess::NotRunning) return;
    child.terminate();
    if (!child.waitForFinished(1000)) {
        child.kill();
        child.waitForFinished(1000);
    }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("qmltui-runner"));
    app.setApplicationVersion(QStringLiteral(QMLTUI_VERSION));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Launch a Runner-linked QmlTui application."));
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("application", "Executable followed by its arguments.",
                                 "[--] application [arguments...]");
    for (const auto& option :
         {QCommandLineOption("script", "JavaScript scenario (.js, .mjs, or resource path).",
                             "path"),
          QCommandLineOption("eval", "Inline Runner JavaScript.", "code"),
          QCommandLineOption("capture-after-ms", "Capture after this delay (0..30000).", "ms"),
          QCommandLineOption(
              "columns",
              "Viewport columns (1..1000; default: terminal width with --live, else 80).", "count",
              "80"),
          QCommandLineOption(
              "rows", "Viewport rows (1..1000; default: terminal height with --live, else 24).",
              "count", "24"),
          QCommandLineOption(
              "artifacts",
              "New or empty artifact directory; default: retained temporary directory.",
              "directory"),
          QCommandLineOption("clock", "Clock: real or virtual.", "clock", "real"),
          QCommandLineOption("record", "Record asciicast beneath the artifact directory.",
                             "relative-path"),
          QCommandLineOption("operation-timeout-ms", "Operation timeout (1..30000).", "ms", "2000"),
          QCommandLineOption("timeout-ms", "Scenario timeout (1..300000).", "ms", "300000"),
          QCommandLineOption("startup-timeout-ms", "Activation timeout (1..300000).", "ms", "5000"),
          QCommandLineOption("live", "Forward live terminal rendering."),
          QCommandLineOption("update-snapshots", "Rewrite snapshot baselines.")})
        parser.addOption(option);
    if (!parser.parse(app.arguments())) return error(parser.errorText());
    if (parser.isSet("help")) {
        QTextStream(stdout) << parser.helpText();
        return 0;
    }
    if (parser.isSet("version")) {
        QTextStream(stdout) << app.applicationName() << ' ' << app.applicationVersion() << '\n';
        return 0;
    }
    const QStringList positional = parser.positionalArguments();
    if (positional.isEmpty()) return error("An application executable is required.");
    const int modes = int(parser.isSet("script")) + int(parser.isSet("eval")) +
                      int(parser.isSet("capture-after-ms"));
    if (modes != 1) return error("Specify exactly one of --script, --eval, or --capture-after-ms.");
    QMap<QString, int> numbers;
    for (const auto& name : {"columns", "rows", "operation-timeout-ms", "timeout-ms",
                             "startup-timeout-ms", "capture-after-ms"}) {
        if (QString::fromLatin1(name) == "capture-after-ms" && !parser.isSet(name)) continue;
        bool ok = false;
        const int value = parser.value(name).toInt(&ok);
        const QString key = QString::fromLatin1(name);
        const int maximum = key == "columns" || key == "rows"                    ? 1000
                            : key == "timeout-ms" || key == "startup-timeout-ms" ? 300000
                                                                                 : 30000;
        const int minimum = key == "capture-after-ms" ? 0 : 1;
        if (!ok || value < minimum || value > maximum)
            return error(QStringLiteral("Invalid --%1: expected %2..%3.")
                             .arg(key)
                             .arg(minimum)
                             .arg(maximum));
        numbers.insert(key, value);
    }
    if (parser.isSet("capture-after-ms") &&
        numbers["capture-after-ms"] + numbers["operation-timeout-ms"] > numbers["timeout-ms"])
        return error("Capture delay plus operation timeout must fit within --timeout-ms.");
    if (parser.value("clock") != "real" && parser.value("clock") != "virtual")
        return error("--clock must be real or virtual.");
    if (parser.isSet("record") && parser.isSet("live"))
        return error("--record cannot be combined with --live.");
    QString script = parser.value("script");
    if (parser.isSet("script")) {
        if (script.trimmed().isEmpty()) return error("--script must not be empty.");
        if (!script.startsWith(":/")) {
            script = QFileInfo(script).absoluteFilePath();
            if (!QFileInfo(script).isFile() || !QFileInfo(script).isReadable())
                return error("Cannot read script: " + script);
        }
    }
    if (parser.isSet("eval") && parser.value("eval").trimmed().isEmpty())
        return error("--eval must not be empty.");
    const QString recording = QDir::fromNativeSeparators(parser.value("record"));
    if (parser.isSet("record") &&
        (recording.isEmpty() || QDir::isAbsolutePath(recording) || recording.startsWith(':') ||
         recording.split('/').contains("..")))
        return error("--record must be a relative path without parent traversal.");

    QString artifacts;
    if (parser.isSet("artifacts")) {
        if (parser.value("artifacts").trimmed().isEmpty())
            return error("--artifacts must not be empty.");
        artifacts = QFileInfo(parser.value("artifacts")).absoluteFilePath();
        const QDir directory(artifacts);
        if (QFileInfo::exists(artifacts) &&
            (!QFileInfo(artifacts).isDir() ||
             !directory
                  .entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)
                  .isEmpty()))
            return error("Artifact directory must be new or empty: " + artifacts);
        if (!QDir().mkpath(artifacts))
            return error("Cannot create artifact directory: " + artifacts);
    } else {
        QTemporaryDir directory(QDir::tempPath() + "/qmltui-runner-XXXXXX");
        if (!directory.isValid()) return error("Cannot create artifact directory.");
        directory.setAutoRemove(false);
        artifacts = directory.path();
    }
    QTextStream(stderr) << "qmltui-runner: artifacts: " << artifacts << '\n';
    QTemporaryDir control;
    if (!control.isValid()) return error("Cannot create handshake directory.");
    const QString handshake = control.filePath("activation.json");
    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto environment = QProcessEnvironment::systemEnvironment();
    for (const auto& key : environment.keys()) {
        if (key.startsWith("QMLTUI_RUNNER_")) environment.remove(key);
    }
    const auto set = [&](const QString& key, const QString& value) {
        environment.insert("QMLTUI_RUNNER_" + key, value);
    };
    if (parser.isSet("script")) set("SCRIPT", script);
    if (parser.isSet("eval")) set("EVAL", parser.value("eval"));
    if (parser.isSet("capture-after-ms")) set("CAPTURE_AFTER_MS", parser.value("capture-after-ms"));
    if (!parser.isSet("live") || parser.isSet("columns")) set("COLUMNS", parser.value("columns"));
    if (!parser.isSet("live") || parser.isSet("rows")) set("ROWS", parser.value("rows"));
    set("OPERATION_TIMEOUT_MS", parser.value("operation-timeout-ms"));
    set("TIMEOUT_MS", parser.value("timeout-ms"));
    set("CLOCK", parser.value("clock"));
    set("LIVE", parser.isSet("live") ? "1" : "0");
    set("UPDATE_SNAPSHOTS", parser.isSet("update-snapshots") ? "1" : "0");
    set("ARTIFACT_DIR", artifacts);
    set("HANDSHAKE_PATH", handshake);
    set("HANDSHAKE_TOKEN", token);
    if (parser.isSet("record")) set("RECORD", QDir(artifacts).filePath(recording));
    environment.insert("QT_FORCE_STDERR_LOGGING", "1");

    QProcess child;
    child.setProcessEnvironment(environment);
    child.setProcessChannelMode(QProcess::ForwardedChannels);
    child.setInputChannelMode(QProcess::ForwardedInputChannel);
    QElapsedTimer startup;
    startup.start();
    child.start(positional.first(), positional.mid(1));
    if (!child.waitForStarted(numbers["startup-timeout-ms"])) {
        stop(child);
        return error("Cannot start application: " + child.errorString());
    }
    const qint64 pid = child.processId();
    QElapsedTimer active;
    bool activated = false;
    QString failure;
    while (true) {
        if (!activated && QFileInfo::exists(handshake)) {
            const auto ack = readObject(handshake);
            if (ack.value("protocolVersion").toInt(-1) != 1 ||
                ack.value("token").toString() != token || ack.value("pid").toInteger(-1) != pid) {
                failure = "Invalid Runner activation handshake.";
                break;
            }
            activated = true;
            active.start();
        }
        if (child.state() == QProcess::NotRunning) break;
        if (!activated && startup.elapsed() >= numbers["startup-timeout-ms"]) break;
        if (activated && active.elapsed() >= numbers["timeout-ms"] + 5000) {
            failure = "Application exceeded scenario timeout and shutdown grace period.";
            break;
        }
        child.waitForFinished(10);
    }
    if (!activated && failure.isEmpty())
        failure = "Runner did not activate. Link a handshake-capable qmltui::runner, call its "
                  "environment "
                  "hook for C++ terminals, or increase --startup-timeout-ms for slow startup.";
    if (!failure.isEmpty()) {
        stop(child);
        return error(failure);
    }
    if (child.exitStatus() != QProcess::NormalExit) return error("Application crashed.");
    const auto result = readObject(QDir(artifacts).filePath("result.json"));
    const QString mode = parser.isSet("script") ? "script"
                         : parser.isSet("eval") ? "eval"
                                                : "capture";
    const int code = result.value("exitCode").toInt(-1);
    if (result.value("schemaVersion").toInt(-1) != 1 || (code != 0 && code != 1) ||
        code != child.exitCode() || result.value("mode").toString() != mode ||
        result.value("status").toString() != (code == 0 ? "passed" : "failed"))
        return error("Missing, invalid, or inconsistent Runner result.json.");
    if (code == 0 && parser.isSet("record")) {
        const QString castFile = QDir(artifacts).filePath(recording);
        if (!QFileInfo::exists(castFile) || QFileInfo(castFile).size() == 0)
            return error("Recorded cast file was not created or is empty: " + castFile);
    }
    QTextStream(stderr) << "qmltui-runner: " << (code == 0 ? "passed" : "failed")
                        << "; artifacts: " << artifacts << '\n';
    return code;
}
