#include "ChildSession.h"
#include "Formatters.h"
#include "SessionCli.h"
#include "SessionDaemon.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSize>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUuid>
#include <algorithm>

namespace {
QJsonObject readObject(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

QString readTextFile(const QString& path) {
    // Runner writes text artifacts with QIODevice::Text (CRLF on Windows).
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(file.readAll());
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

bool parseViewportSizes(const QString& spec, QList<QSize>& out, QString& err) {
    static const QRegularExpression sizeRe(QStringLiteral("^(\\d+)x(\\d+)$"));
    const QStringList parts = spec.split(',', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        err = QStringLiteral("--sizes requires at least one <columns>x<rows> entry.");
        return false;
    }
    for (const QString& raw : parts) {
        const QRegularExpressionMatch m = sizeRe.match(raw.trimmed());
        if (!m.hasMatch()) {
            err = QStringLiteral("Invalid viewport size '%1': expected <columns>x<rows>.")
                      .arg(raw.trimmed());
            return false;
        }
        bool okCols = false;
        bool okRows = false;
        const int cols = m.captured(1).toInt(&okCols);
        const int rows = m.captured(2).toInt(&okRows);
        if (!okCols || !okRows || cols < 1 || cols > 1000 || rows < 1 || rows > 1000) {
            err = QStringLiteral("Viewport dimensions out of range (1..1000): %1x%2.")
                      .arg(cols)
                      .arg(rows);
            return false;
        }
        out.append(QSize(cols, rows));
    }
    return true;
}

QString buildSynthesizedEval(const QList<QSize>& sizes, int settleMs, const QString& userEval,
                             int idleMs, bool captureStyled, bool captureScrollback) {
    QStringList lines;
    lines << QStringLiteral("function main(app) {");
    lines << QStringLiteral("    const __qmltuiOuterMain = main;");
    lines << QStringLiteral("    const __qmltuiAfterUser = function() {");
    if (!sizes.isEmpty()) {
        const QSize first = sizes.constFirst();
        const QString firstTag = QStringLiteral("%1x%2").arg(first.width()).arg(first.height());
        lines << QStringLiteral("        app.captureText(\"frame_%1.txt\");").arg(firstTag);
        if (captureStyled)
            lines << QStringLiteral("        app.captureStyled(\"frame_%1_styled.txt\");")
                         .arg(firstTag);
    } else {
        lines << QStringLiteral("        app.captureText(\"capture.txt\");");
        if (captureStyled)
            lines << QStringLiteral("        app.captureStyled(\"capture_styled.txt\");");
    }

    if (captureScrollback) {
        lines << QStringLiteral("        app.captureScrollback(\"scrollback.txt\");");
        if (captureStyled)
            lines << QStringLiteral(
                "        app.captureScrollbackStyled(\"scrollback_styled.txt\");");
    }

    for (int i = 1; i < sizes.size(); ++i) {
        const QSize sz = sizes.at(i);
        const QString tag = QStringLiteral("%1x%2").arg(sz.width()).arg(sz.height());
        lines << QStringLiteral("        app.resize(%1, %2);").arg(sz.width()).arg(sz.height());
        lines << QStringLiteral("        app.captureText(\"frame_%1.txt\");").arg(tag);
        if (captureStyled)
            lines << QStringLiteral("        app.captureStyled(\"frame_%1_styled.txt\");").arg(tag);
    }

    if (idleMs > 0) {
        lines << QStringLiteral("        const __idleBefore = app.frameCount();");
        lines << QStringLiteral("        app.sleep(%1);").arg(idleMs);
        lines << QStringLiteral("        const __idleDelta = app.frameCount() - __idleBefore;");
        lines << QStringLiteral(
                     "        if (__idleDelta !== 0) app.fail(\"Idle repaint budget exceeded: "
                     "frameCount advanced by \" + __idleDelta + \" over %1 ms\");")
                     .arg(idleMs);
    }
    lines << QStringLiteral("    };");

    if (settleMs > 0) lines << QStringLiteral("    app.sleep(%1);").arg(settleMs);
    if (!userEval.isEmpty()) {
        lines << QStringLiteral("    const __qmltuiUserResult = (function(app) {");
        lines << QStringLiteral("        {");
        lines << userEval;
        lines << QStringLiteral(
            "            if (typeof main === \"function\" && main !== __qmltuiOuterMain) {");
        lines << QStringLiteral("                return main(app);");
        lines << QStringLiteral("            }");
        lines << QStringLiteral("        }");
        lines << QStringLiteral("    })(app);");
        lines << QStringLiteral(
            "    if (__qmltuiUserResult && typeof __qmltuiUserResult.then === \"function\") {");
        lines << QStringLiteral("        return __qmltuiUserResult.then(__qmltuiAfterUser);");
        lines << QStringLiteral("    }");
    }
    lines << QStringLiteral("    __qmltuiAfterUser();");
    lines << QStringLiteral("}");
    return lines.join('\n');
}

using qmltui::runner_cli::extractStylesSection;
using qmltui::runner_cli::formatFrameStatsLine;
using qmltui::runner_cli::formatRulerFrame;

void printProbeOutput(QTextStream& out, const QString& artifacts, const QJsonObject& result,
                      const QList<QSize>& sizes, int defaultCols, int defaultRows, bool showRuler,
                      bool showStyled, bool showScrollback) {
    const QString statsLine =
        formatFrameStatsLine(result.value(QStringLiteral("frameStats")).toObject());
    if (!statsLine.isEmpty()) out << statsLine << '\n';
    const QString message = result.value(QStringLiteral("message")).toString();
    if (!message.isEmpty()) out << "Message: " << message << '\n';

    const QDir dir(artifacts);
    if (!sizes.isEmpty()) {
        for (const QSize& sz : sizes) {
            const QString tag = QStringLiteral("%1x%2").arg(sz.width()).arg(sz.height());
            const QString framePath = dir.filePath(QStringLiteral("frame_%1.txt").arg(tag));
            if (!QFileInfo::exists(framePath)) continue;
            out << "\n=== Viewport " << tag << " ===\n";
            out << formatRulerFrame(readTextFile(framePath), sz.width(), sz.height(), showRuler)
                << '\n';
            if (showStyled) {
                const QString styledPath =
                    dir.filePath(QStringLiteral("frame_%1_styled.txt").arg(tag));
                if (QFileInfo::exists(styledPath)) {
                    const QString styles = extractStylesSection(readTextFile(styledPath));
                    out << "--- Styles (" << tag << ") ---\n"
                        << (styles.isEmpty() ? QStringLiteral("(all default styles)") : styles)
                        << '\n';
                }
            }
        }
    } else {
        const QJsonObject finalVp = result.value(QStringLiteral("finalViewport")).toObject();
        const int cols = finalVp.value(QStringLiteral("columns")).toInt(defaultCols);
        const int rows = finalVp.value(QStringLiteral("rows")).toInt(defaultRows);
        const QString finalRel = result.value(QStringLiteral("artifacts"))
                                     .toObject()
                                     .value(QStringLiteral("finalFrame"))
                                     .toString();
        const QString candidate = !finalRel.isEmpty() ? dir.filePath(finalRel)
                                                      : dir.filePath(QStringLiteral("capture.txt"));
        if (QFileInfo::exists(candidate)) {
            out << "\n=== Viewport " << cols << 'x' << rows << " ("
                << QFileInfo(candidate).fileName() << ") ===\n";
            out << formatRulerFrame(readTextFile(candidate), cols, rows, showRuler) << '\n';
        }
        if (showStyled) {
            const bool isCaptureMode =
                result.value(QStringLiteral("mode")).toString() == QLatin1String("capture");
            const QString finalStyledPath =
                dir.filePath(result.value(QStringLiteral("exitCode")).toInt() == 0
                                 ? QStringLiteral("final_styled.txt")
                                 : QStringLiteral("failure_styled.txt"));
            const QString captureStyledPath = dir.filePath(QStringLiteral("capture_styled.txt"));
            QString styledPath;
            if (isCaptureMode) {
                styledPath =
                    QFileInfo::exists(captureStyledPath) ? captureStyledPath : finalStyledPath;
            } else {
                styledPath =
                    QFileInfo::exists(finalStyledPath) ? finalStyledPath : captureStyledPath;
            }
            if (QFileInfo::exists(styledPath)) {
                const QString styles = extractStylesSection(readTextFile(styledPath));
                out << "--- Styles ---\n"
                    << (styles.isEmpty() ? QStringLiteral("(all default styles)") : styles) << '\n';
            }
        }
    }

    if (showScrollback) {
        const QString sbPath = dir.filePath(QStringLiteral("scrollback.txt"));
        if (QFileInfo::exists(sbPath)) {
            const QString sb = readTextFile(sbPath);
            out << "\n=== Inline Scrollback ===\n"
                << (sb.trimmed().isEmpty() ? QStringLiteral("(empty scrollback)\n") : sb);
            if (!sb.endsWith('\n') && !sb.trimmed().isEmpty()) out << '\n';
        }
        if (showStyled) {
            const QString sbStyled = dir.filePath(QStringLiteral("scrollback_styled.txt"));
            if (QFileInfo::exists(sbStyled)) {
                const QString styles = extractStylesSection(readTextFile(sbStyled));
                out << "--- Scrollback Styles ---\n"
                    << (styles.isEmpty() ? QStringLiteral("(all default styles)") : styles) << '\n';
            }
        }
    }

    if (result.value(QStringLiteral("exitCode")).toInt() != 0 && !sizes.isEmpty()) {
        const QString failPath = dir.filePath(QStringLiteral("failure.txt"));
        if (QFileInfo::exists(failPath)) {
            const QSize fallback = sizes.constLast();
            const QJsonObject finalVp = result.value(QStringLiteral("finalViewport")).toObject();
            const int failCols = finalVp.value(QStringLiteral("columns")).toInt(fallback.width());
            const int failRows = finalVp.value(QStringLiteral("rows")).toInt(fallback.height());
            out << "\n=== Failure Frame (failure.txt) ===\n"
                << formatRulerFrame(readTextFile(failPath), failCols, failRows, showRuler) << '\n';
        }
    }
    out.flush();
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("qmltui-runner"));
    app.setApplicationVersion(QStringLiteral(QMLTUI_VERSION));

    const QStringList rawArgs = app.arguments();
    if (rawArgs.size() >= 2 &&
        (rawArgs.at(1) == QLatin1String("session") || rawArgs.at(1) == QLatin1String("s"))) {
        return qmltui::runner_cli::runSessionCli(rawArgs.mid(2));
    }
#if QMLTUI_RUNNER_HAS_SESSION_DAEMON
    if (rawArgs.size() >= 3 && rawArgs.at(1) == QLatin1String("__session-daemon"))
        return qmltui::runner_cli::runSessionDaemon(rawArgs.at(2));
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Launch or interactively drive a Runner-linked QmlTui application.\n"
                       "Use `qmltui-runner session --help` for stateful interactive sessions."));
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
              "interactive",
              "Run an interactive NDJSON command loop over stdin/stdout without a session daemon."),
          QCommandLineOption(
              "sizes", "Comma-separated <cols>x<rows> viewport sweep (e.g. 60x18,80x24,120x36).",
              "list"),
          QCommandLineOption("sweep", "Shorthand for --sizes 60x18,80x24,120x36."),
          QCommandLineOption("idle-ms", "Verify zero unscripted repaints across <ms> (1..30000).",
                             "ms"),
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
          QCommandLineOption("clean-artifacts", "Clear an existing Runner artifact directory "
                                                "(containing result.json) before running."),
          QCommandLineOption("clock", "Clock: real or virtual.", "clock", "real"),
          QCommandLineOption("record", "Record asciicast beneath the artifact directory.",
                             "relative-path"),
          QCommandLineOption("operation-timeout-ms", "Operation timeout (1..30000).", "ms", "2000"),
          QCommandLineOption("timeout-ms", "Scenario timeout (1..300000).", "ms", "300000"),
          QCommandLineOption("startup-timeout-ms", "Activation timeout (1..300000).", "ms", "5000"),
          QCommandLineOption("print", "Print frameStats and captured frame(s) to stdout."),
          QCommandLineOption("ruler",
                             "Print frameStats and 0-based (x, y) ruler frame(s) to stdout."),
          QCommandLineOption("styled", "Include --- styles section when printing/probing frames."),
          QCommandLineOption("scrollback",
                             "Capture and print Terminal.Inline committed scrollback rows."),
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

    const bool hasProbeModifiers = parser.isSet("sizes") || parser.isSet("sweep") ||
                                   parser.isSet("idle-ms") || parser.isSet("scrollback");
    const bool interactive = parser.isSet("interactive");
    const int baseModes = int(parser.isSet("script")) + int(parser.isSet("eval")) +
                          int(parser.isSet("capture-after-ms")) + int(interactive);
    if (baseModes > 1 || (baseModes == 0 && !hasProbeModifiers))
        return error("Specify one of --script, --eval, --capture-after-ms, --interactive, or "
                     "probe options (--sizes/--sweep/--idle-ms/--scrollback).");
    if ((parser.isSet("script") || interactive) && hasProbeModifiers)
        return error("--sizes, --sweep, --idle-ms, and --scrollback cannot be combined with "
                     "--script or --interactive.");
    if (interactive && (parser.isSet("live") || parser.isSet("print") || parser.isSet("ruler")))
        return error("--live, --print, and --ruler cannot be combined with --interactive.");
    if (parser.isSet("live") && (parser.isSet("print") || parser.isSet("ruler")))
        return error("--print and --ruler cannot be combined with --live.");

    QList<QSize> sweepSizes;
    if (parser.isSet("sizes") || parser.isSet("sweep")) {
        const QString sizeSpec =
            parser.isSet("sizes") ? parser.value("sizes") : QStringLiteral("60x18,80x24,120x36");
        QString sizeErr;
        if (!parseViewportSizes(sizeSpec, sweepSizes, sizeErr)) return error(sizeErr);
    }

    QMap<QString, int> numbers;
    for (const auto& name : {"columns", "rows", "operation-timeout-ms", "timeout-ms",
                             "startup-timeout-ms", "capture-after-ms", "idle-ms"}) {
        const QString key = QString::fromLatin1(name);
        if ((key == "capture-after-ms" || key == "idle-ms") && !parser.isSet(name)) continue;
        bool ok = false;
        const int value = parser.value(name).toInt(&ok);
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

    const int settleMs = numbers.value(QStringLiteral("capture-after-ms"), 0);
    const int idleMs = numbers.value(QStringLiteral("idle-ms"), 0);
    if ((parser.isSet("capture-after-ms") || parser.isSet("idle-ms")) &&
        settleMs + idleMs + numbers["operation-timeout-ms"] > numbers["timeout-ms"])
        return error("Capture/idle delay plus operation timeout must fit within --timeout-ms.");
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
        QDir directory(artifacts);
        if (QFileInfo::exists(artifacts)) {
            if (!QFileInfo(artifacts).isDir())
                return error("Artifact directory must be new or empty: " + artifacts);
            const bool nonEmpty = !directory
                                       .entryList(QDir::AllEntries | QDir::Hidden | QDir::System |
                                                  QDir::NoDotAndDotDot)
                                       .isEmpty();
            if (nonEmpty) {
                if (!parser.isSet("clean-artifacts"))
                    return error("Artifact directory must be new or empty: " + artifacts);
                if (!QFileInfo::exists(directory.filePath(QStringLiteral("result.json"))))
                    return error("Refusing to clean non-Runner directory (missing result.json): " +
                                 artifacts);
                if (!directory.removeRecursively())
                    return error("Cannot clean artifact directory: " + artifacts);
            }
        }
        if (!QDir().mkpath(artifacts))
            return error("Cannot create artifact directory: " + artifacts);
    } else {
        QTemporaryDir directory(QDir::tempPath() + "/qmltui-runner-XXXXXX");
        if (!directory.isValid()) return error("Cannot create artifact directory.");
        directory.setAutoRemove(false);
        artifacts = directory.path();
    }
    if (interactive) {
        qmltui::runner_cli::ChildSession::LaunchConfig cfg;
        cfg.program = positional.first();
        cfg.arguments = positional.mid(1);
        cfg.workingDirectory = QDir::currentPath();
        cfg.clock = parser.value("clock");
        cfg.columns = numbers["columns"];
        cfg.rows = numbers["rows"];
        cfg.operationTimeoutMs = numbers["operation-timeout-ms"];
        cfg.startupTimeoutMs = numbers["startup-timeout-ms"];
        cfg.artifactDir = artifacts;
        cfg.recordPath = parser.isSet("record") ? recording : QString();
        cfg.environment = QProcessEnvironment::systemEnvironment();
        return qmltui::runner_cli::runInteractiveStdioMode(cfg);
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

    const bool synthesizeProbeEval = hasProbeModifiers;
    QString expectedMode;
    if (parser.isSet("script")) {
        set("SCRIPT", script);
        expectedMode = QStringLiteral("script");
    } else if (synthesizeProbeEval) {
        const QString evalBody =
            buildSynthesizedEval(sweepSizes, settleMs, parser.value("eval"), idleMs,
                                 parser.isSet("styled"), parser.isSet("scrollback"));
        set("EVAL", evalBody);
        expectedMode = QStringLiteral("eval");
    } else if (parser.isSet("eval")) {
        set("EVAL", parser.value("eval"));
        expectedMode = QStringLiteral("eval");
    } else {
        set("CAPTURE_AFTER_MS", parser.value("capture-after-ms"));
        expectedMode = QStringLiteral("capture");
    }

    const int initialCols =
        !sweepSizes.isEmpty() ? sweepSizes.constFirst().width() : numbers["columns"];
    const int initialRows =
        !sweepSizes.isEmpty() ? sweepSizes.constFirst().height() : numbers["rows"];
    if (!parser.isSet("live") || parser.isSet("columns") || !sweepSizes.isEmpty())
        set("COLUMNS", QString::number(initialCols));
    if (!parser.isSet("live") || parser.isSet("rows") || !sweepSizes.isEmpty())
        set("ROWS", QString::number(initialRows));
    set("OPERATION_TIMEOUT_MS", parser.value("operation-timeout-ms"));
    set("TIMEOUT_MS", parser.value("timeout-ms"));
    set("CLOCK", parser.value("clock"));
    set("LIVE", parser.isSet("live") ? "1" : "0");
    set("UPDATE_SNAPSHOTS", parser.isSet("update-snapshots") ? "1" : "0");
    set("ARTIFACT_DIR", artifacts);
    set("HANDSHAKE_PATH", handshake);
    set("HANDSHAKE_TOKEN", token);
    if (parser.isSet("styled")) set("FINAL_STYLED", "1");
    if (parser.isSet("record")) set("RECORD", QDir(artifacts).filePath(recording));
    environment.insert("QT_FORCE_STDERR_LOGGING", "1");
    if ((hasProbeModifiers || parser.isSet("print") || parser.isSet("ruler")) &&
        !environment.contains("QMLTUI_DEBUG_PAINT_DIRTY")) {
        environment.insert("QMLTUI_DEBUG_PAINT_DIRTY", "1");
    }

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
        failure = "Runner did not activate. Link a handshake-capable QmlTui::Runner, call its "
                  "environment "
                  "hook for C++ terminals, or increase --startup-timeout-ms for slow startup.";
    if (!failure.isEmpty()) {
        stop(child);
        return error(failure);
    }
    if (child.exitStatus() != QProcess::NormalExit) return error("Application crashed.");
    const auto result = readObject(QDir(artifacts).filePath("result.json"));
    const int code = result.value("exitCode").toInt(-1);
    if (result.value("schemaVersion").toInt(-1) != 1 || (code != 0 && code != 1) ||
        code != child.exitCode() || result.value("mode").toString() != expectedMode ||
        result.value("status").toString() != (code == 0 ? "passed" : "failed"))
        return error("Missing, invalid, or inconsistent Runner result.json.");
    if (code == 0 && parser.isSet("record")) {
        const QString castFile = QDir(artifacts).filePath(recording);
        if (!QFileInfo::exists(castFile) || QFileInfo(castFile).size() == 0)
            return error("Recorded cast file was not created or is empty: " + castFile);
    }

    const bool wantPrint = parser.isSet("print") || parser.isSet("ruler");
    if (wantPrint) {
        QTextStream out(stdout);
        printProbeOutput(out, artifacts, result, sweepSizes, initialCols, initialRows,
                         parser.isSet("ruler"), parser.isSet("styled"), parser.isSet("scrollback"));
    }

    const int dirtyTotal =
        result.value("frameStats").toObject().value("dirtyDuringPaintTotal").toInt(0);
    int finalCode = code;
    if (finalCode == 0 && hasProbeModifiers && dirtyTotal > 0) {
        QTextStream(stderr) << "qmltui-runner: dirtyDuringPaintTotal=" << dirtyTotal << '\n';
        finalCode = 1;
    }

    QTextStream(stderr) << "qmltui-runner: " << (finalCode == 0 ? "passed" : "failed")
                        << "; artifacts: " << artifacts << '\n';
    return finalCode;
}
