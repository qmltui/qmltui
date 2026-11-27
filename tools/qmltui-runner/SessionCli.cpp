#include "SessionCli.h"
#include "DetachedProcess.h"
#include "Formatters.h"
#include "SessionCommand.h"
#include "SessionConnect.h"
#include "SessionDaemon.h"
#include "TerminalSize.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <QUuid>

namespace qmltui::runner_cli {

namespace {

int cliError(const QString& message, bool json = false) {
    if (json) {
        QTextStream(stdout) << QString::fromUtf8(
                                   QJsonDocument(QJsonObject{{QStringLiteral("ok"), false},
                                                             {QStringLiteral("error"), message}})
                                       .toJson(QJsonDocument::Compact))
                            << '\n';
    } else {
        QTextStream(stderr) << "qmltui-runner session: " << message << '\n';
    }
    return 2;
}

void printSessionHelp() {
    QTextStream(stdout) << R"(Usage: qmltui-runner session <verb> [options] [arguments...]
       qmltui-runner s <verb> [options] [arguments...]

Session lifecycle:
  start [options] -- <app> [args...]   Start a live session (add --live to render in foreground)
  connect [id] [--no-frame] [--diff]   Open interactive command console (alias: shell)
  batch [id] [--diff] [--no-frame]     Execute newline-delimited commands from stdin
  watch [--session <id>]               Attach a live ANSI viewer to an active session
  list [--json]                        List active sessions
  stop [--session <id>|--all] [--export <file>] [--json]
                                       Stop one or all sessions
  export [-o <file>] [--session <id>] [--json]
                                       Export successful steps to a CTest .js scenario

Input actions (auto-return ruled frame, FrameStats, and new logs):
  press <key> [--modifiers Shift,Ctrl]
  type <text>
  paste <text>
  inject-key <key> [--modifiers ...]
  inject-text <text>
  inject-paste <text>
  click <x> <y> [--button left|middle|right] [--modifiers ...]
  double-click <x> <y> [--button ...] [--modifiers ...]
  mouse-down <x> <y> [--button ...] [--modifiers ...]
  mouse-up <x> <y> [--button ...] [--modifiers ...]
  hover <x> <y> [--modifiers ...]
  drag <x1> <y1> <x2> <y2> [--button ...] [--modifiers ...]
  scroll <x> <y> <deltaX> <deltaY> [--modifiers ...]
  click-text <text> [--occurrence N] [--button ...] [--modifiers ...]
  hover-text <text> [--occurrence N] [--modifiers ...]
  click-item <objectName> [--button ...] [--modifiers ...]

Viewport, clock, waits, assertions & recording:
  resize <columns> <rows>
  advance <ms>                         Advance virtual clock time
  sleep <ms>
  wait-frame [--timeout <ms>]
  expect-repaint [--timeout <ms>]
  wait-text <text> [--timeout <ms>]
  wait-regex <pattern> [--timeout <ms>]
  wait-item <objectName> [--timeout <ms>]
  wait-scrollback <text> [--timeout <ms>]
  expect-text <text> [--timeout <ms>]
  expect-no-text <text> [--timeout <ms>]
  expect-regex <pattern> [--timeout <ms>]
  expect-scrollback <text> [--timeout <ms>]
  expect-no-scrollback <text> [--timeout <ms>]
  marker <label>                       Write an asciicast chapter marker
  record start <rel.cast> | stop       Start or stop .cast recording mid-session

Inspection & JavaScript escape hatch:
  frame [--ruler|--no-ruler|--plain] [--diff] [--styled] [--scrollback] [--from-row N] [--tree]
  scrollback [--from-row N] [--styled]
  tree [--include-hidden]              Inspect live QML Item tree & activeFocus
  cell <x> <y>
  find-text <text>
  item-rect <objectName>
  stats
  logs [--all]
  eval <js-code> [--timeout <ms>] [--no-record]
                                       Run Runner JS with `app` in scope

Common step options:
  --session <id>       Target a specific session (defaults to latest active)
  --ruler / --no-ruler Show or hide 0-based (x, y) coordinate rulers (default: on; alias: --plain)
  --diff               Print only changed rows instead of the full viewport grid
  --no-frame           Omit the rendered frame from the output
  --styled             Include --- Styles --- SGR/color runs
  --scrollback         Include Terminal.Inline committed scrollback
  --tree               Include QML Item tree & focus state
  --include-hidden     Include hidden subtrees in --tree / tree
  --no-record          Do not record this step in `session export` history
  --json               Emit compact JSON instead of formatted text
)";
}

#if QMLTUI_RUNNER_HAS_SESSION_DAEMON

int runSessionStart(const QStringList& args) {
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Start an interactive QmlTui Runner session."));
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("application"),
                                 QStringLiteral("Executable followed by its arguments."),
                                 QStringLiteral("[--] application [arguments...]"));
    for (const auto& option : {
             QCommandLineOption(QStringLiteral("name"),
                                QStringLiteral("Optional session name (1..32 of A-Za-z0-9_-)."),
                                QStringLiteral("id")),
             QCommandLineOption(QStringLiteral("clock"), QStringLiteral("Clock: real or virtual."),
                                QStringLiteral("clock"), QStringLiteral("real")),
             QCommandLineOption(QStringLiteral("columns"),
                                QStringLiteral("Initial viewport columns (1..1000; default: "
                                               "terminal width with --live, else 80)."),
                                QStringLiteral("count"), QStringLiteral("80")),
             QCommandLineOption(QStringLiteral("rows"),
                                QStringLiteral("Initial viewport rows (1..1000; default: terminal "
                                               "height with --live, else 24)."),
                                QStringLiteral("count"), QStringLiteral("24")),
             QCommandLineOption(
                 QStringLiteral("artifacts"),
                 QStringLiteral("New or empty artifact directory; default: retained temp dir."),
                 QStringLiteral("directory")),
             QCommandLineOption(
                 QStringLiteral("clean-artifacts"),
                 QStringLiteral("Clear an existing Runner artifact directory before starting.")),
             QCommandLineOption(QStringLiteral("record"),
                                QStringLiteral("Record asciicast beneath the artifact directory."),
                                QStringLiteral("relative-path")),
             QCommandLineOption(QStringLiteral("operation-timeout-ms"),
                                QStringLiteral("Operation timeout (1..30000)."),
                                QStringLiteral("ms"), QStringLiteral("2000")),
             QCommandLineOption(QStringLiteral("startup-timeout-ms"),
                                QStringLiteral("Activation timeout (1..300000)."),
                                QStringLiteral("ms"), QStringLiteral("5000")),
             QCommandLineOption(
                 QStringLiteral("idle-timeout-ms"),
                 QStringLiteral("Auto-stop session after inactivity (0 to disable, or "
                                "100..3600000; default 300000, disabled with --live)."),
                 QStringLiteral("ms"), QStringLiteral("300000")),
             QCommandLineOption(QStringLiteral("ruler"),
                                QStringLiteral("Show 0-based (x, y) rulers (default).")),
             QCommandLineOption({QStringLiteral("no-ruler"), QStringLiteral("plain")},
                                QStringLiteral("Print plain frame without rulers.")),
             QCommandLineOption(QStringLiteral("no-frame"),
                                QStringLiteral("Omit initial frame from output.")),
             QCommandLineOption(QStringLiteral("styled"),
                                QStringLiteral("Include --- Styles --- in initial output.")),
             QCommandLineOption(QStringLiteral("scrollback"),
                                QStringLiteral("Include Inline scrollback in initial output.")),
             QCommandLineOption(QStringLiteral("tree"),
                                QStringLiteral("Include QML Item tree in initial output.")),
             QCommandLineOption(QStringLiteral("include-hidden"),
                                QStringLiteral("Include hidden subtrees in --tree.")),
             QCommandLineOption(
                 QStringLiteral("live"),
                 QStringLiteral("Run in foreground and render live ANSI frames to this terminal "
                                "while accepting session commands from other terminals.")),
             QCommandLineOption(QStringLiteral("json"), QStringLiteral("Emit compact JSON.")),
         }) {
        parser.addOption(option);
    }

    QStringList parseInput;
    parseInput << QStringLiteral("qmltui-runner session start") << args;
    if (!parser.parse(parseInput)) return cliError(parser.errorText(), hasJsonFlag(args));
    const bool jsonOut = parser.isSet(QStringLiteral("json"));
    const bool liveMode = parser.isSet(QStringLiteral("live"));
    if (parser.isSet(QStringLiteral("help"))) {
        QTextStream(stdout) << parser.helpText();
        return 0;
    }
    if (liveMode && jsonOut)
        return cliError(QStringLiteral("--live cannot be combined with --json."), jsonOut);

    const QStringList positional = parser.positionalArguments();
    if (positional.isEmpty())
        return cliError(QStringLiteral("An application executable is required."), jsonOut);

    QString err;
    int columns = 80;
    int rows = 24;
    int opTimeout = 2000;
    int startupTimeout = 5000;
    int idleTimeout = 300000;
    if (!parseIntValue(parser.value(QStringLiteral("columns")), QStringLiteral("--columns"), 1,
                       1000, &columns, &err) ||
        !parseIntValue(parser.value(QStringLiteral("rows")), QStringLiteral("--rows"), 1, 1000,
                       &rows, &err) ||
        !parseIntValue(parser.value(QStringLiteral("operation-timeout-ms")),
                       QStringLiteral("--operation-timeout-ms"), 1, 30000, &opTimeout, &err) ||
        !parseIntValue(parser.value(QStringLiteral("startup-timeout-ms")),
                       QStringLiteral("--startup-timeout-ms"), 1, 300000, &startupTimeout, &err)) {
        return cliError(err, jsonOut);
    }
    const QString rawIdle = parser.value(QStringLiteral("idle-timeout-ms"));
    bool idleOk = false;
    idleTimeout = rawIdle.toInt(&idleOk);
    if (!idleOk || (idleTimeout != 0 && (idleTimeout < 100 || idleTimeout > 3600000))) {
        return cliError(
            QStringLiteral(
                "Invalid --idle-timeout-ms '%1': expected 0 (disabled) or integer in 100..3600000.")
                .arg(rawIdle),
            jsonOut);
    }

    const QString clock = parser.value(QStringLiteral("clock"));
    if (clock != QLatin1String("real") && clock != QLatin1String("virtual"))
        return cliError(QStringLiteral("--clock must be real or virtual."), jsonOut);

    const QString recording = QDir::fromNativeSeparators(parser.value(QStringLiteral("record")));
    if (parser.isSet(QStringLiteral("record")) &&
        (recording.isEmpty() || QDir::isAbsolutePath(recording) ||
         recording.startsWith(QLatin1Char(':')) ||
         recording.split(QLatin1Char('/')).contains(QStringLiteral("..")))) {
        return cliError(
            QStringLiteral("--record must be a relative path without parent traversal."), jsonOut);
    }

    QString sessionId = parser.value(QStringLiteral("name")).trimmed();
    if (parser.isSet(QStringLiteral("name"))) {
        if (!isValidSessionName(sessionId)) {
            return cliError(
                QStringLiteral("Invalid --name '%1': use 1..32 characters of A-Za-z0-9_-.")
                    .arg(sessionId),
                jsonOut);
        }
        const QList<SessionInfo> existing = listActiveSessions();
        for (const SessionInfo& s : existing) {
            if (s.id == sessionId) {
                return cliError(
                    QStringLiteral("Session \"%1\" is already active. Stop it first or choose "
                                   "another --name.")
                        .arg(sessionId),
                    jsonOut);
            }
        }
    } else {
        sessionId = QStringLiteral("s-") + QUuid::createUuid().toString(QUuid::Id128).left(6);
    }

    QString artifacts;
    if (parser.isSet(QStringLiteral("artifacts"))) {
        if (parser.value(QStringLiteral("artifacts")).trimmed().isEmpty())
            return cliError(QStringLiteral("--artifacts must not be empty."), jsonOut);
        artifacts = QFileInfo(parser.value(QStringLiteral("artifacts"))).absoluteFilePath();
        QDir directory(artifacts);
        if (QFileInfo::exists(artifacts)) {
            if (!QFileInfo(artifacts).isDir())
                return cliError(QStringLiteral("Artifact directory must be new or empty: ") +
                                    artifacts,
                                jsonOut);
            const bool nonEmpty = !directory
                                       .entryList(QDir::AllEntries | QDir::Hidden | QDir::System |
                                                  QDir::NoDotAndDotDot)
                                       .isEmpty();
            if (nonEmpty) {
                if (!parser.isSet(QStringLiteral("clean-artifacts")))
                    return cliError(QStringLiteral("Artifact directory must be new or empty: ") +
                                        artifacts,
                                    jsonOut);
                if (!QFileInfo::exists(directory.filePath(QStringLiteral("result.json")))) {
                    return cliError(
                        QStringLiteral(
                            "Refusing to clean non-Runner directory (missing result.json): ") +
                            artifacts,
                        jsonOut);
                }
                if (!directory.removeRecursively())
                    return cliError(QStringLiteral("Cannot clean artifact directory: ") + artifacts,
                                    jsonOut);
            }
        }
        if (!QDir().mkpath(artifacts))
            return cliError(QStringLiteral("Cannot create artifact directory: ") + artifacts,
                            jsonOut);
    } else {
        QTemporaryDir dir(QDir::tempPath() + QStringLiteral("/qmltui-session-XXXXXX"));
        if (!dir.isValid())
            return cliError(QStringLiteral("Cannot create artifact directory."), jsonOut);
        dir.setAutoRemove(false);
        artifacts = dir.path();
    }

    const QString regDir = sessionRegistryDir();
    if (!ensureSessionRegistryDir(regDir))
        return cliError(QStringLiteral("Cannot create session registry directory: ") + regDir,
                        jsonOut);

    QTemporaryDir bootstrapDir;
    if (!bootstrapDir.isValid())
        return cliError(QStringLiteral("Cannot create session bootstrap directory."), jsonOut);
    const QString launchSpecPath = bootstrapDir.filePath(QStringLiteral("launch.json"));
    const QString readyFilePath = bootstrapDir.filePath(QStringLiteral("ready.json"));

    QJsonArray appArgs;
    for (int i = 1; i < positional.size(); ++i)
        appArgs.append(positional.at(i));

    const QString socketName = sessionSocketName(regDir, sessionId);
    QJsonObject include{
        {QStringLiteral("frame"), !parser.isSet(QStringLiteral("no-frame"))},
        {QStringLiteral("styled"), parser.isSet(QStringLiteral("styled"))},
        {QStringLiteral("scrollback"), parser.isSet(QStringLiteral("scrollback"))},
        {QStringLiteral("tree"), parser.isSet(QStringLiteral("tree"))},
        {QStringLiteral("includeHidden"), parser.isSet(QStringLiteral("include-hidden"))}};

    const bool explicitCols = parser.isSet(QStringLiteral("columns"));
    const bool explicitRows = parser.isSet(QStringLiteral("rows"));
    if (liveMode && (!explicitCols || !explicitRows)) {
        const QSize hostSize = queryHostTerminalSize();
        if (hostSize.isValid()) {
            if (!explicitCols) columns = hostSize.width();
            if (!explicitRows) rows = hostSize.height();
        }
    }

    const int effectiveIdleTimeout =
        (liveMode && !parser.isSet(QStringLiteral("idle-timeout-ms"))) ? 0 : idleTimeout;

    const QJsonObject launchSpec{
        {QStringLiteral("id"), sessionId},
        {QStringLiteral("socketName"), socketName},
        {QStringLiteral("sessionDir"), regDir},
        {QStringLiteral("readyFile"), liveMode ? QString() : readyFilePath},
        {QStringLiteral("program"), positional.first()},
        {QStringLiteral("arguments"), appArgs},
        {QStringLiteral("workingDirectory"), QDir::currentPath()},
        {QStringLiteral("clock"), clock},
        {QStringLiteral("columns"), columns},
        {QStringLiteral("rows"), rows},
        {QStringLiteral("explicitColumns"), explicitCols},
        {QStringLiteral("explicitRows"), explicitRows},
        {QStringLiteral("operationTimeoutMs"), opTimeout},
        {QStringLiteral("startupTimeoutMs"), startupTimeout},
        {QStringLiteral("idleTimeoutMs"), effectiveIdleTimeout},
        {QStringLiteral("artifactDir"), artifacts},
        {QStringLiteral("recordPath"), recording},
        {QStringLiteral("include"), include}};

    if (liveMode) return runLiveSessionDaemon(launchSpec);

    QString writeErr;
    if (!writeTextFileAtomic(
            launchSpecPath,
            QString::fromUtf8(QJsonDocument(launchSpec).toJson(QJsonDocument::Compact)),
            &writeErr)) {
        return cliError(QStringLiteral("Cannot write launch spec: ") + writeErr, jsonOut);
    }

    qint64 daemonPid = 0;
    // Detach standard streams and prevent caller handle inheritance so the
    // background daemon never keeps a piped caller's stdout/stderr open.
    const bool launched = startDetachedDaemon(QCoreApplication::applicationFilePath(),
                                              {QStringLiteral("__session-daemon"), launchSpecPath},
                                              QDir::currentPath(), &daemonPid);
    if (!launched)
        return cliError(QStringLiteral("Failed to spawn background session daemon."), jsonOut);

    QDeadlineTimer deadline(startupTimeout + 3000);
    while (!deadline.hasExpired() && !QFileInfo::exists(readyFilePath))
        QThread::msleep(10);

    if (!QFileInfo::exists(readyFilePath)) {
        return cliError(QStringLiteral("Timed out waiting for session daemon to start."), jsonOut);
    }

    QFile readyFile(readyFilePath);
    QJsonObject response;
    if (readyFile.open(QIODevice::ReadOnly))
        response = QJsonDocument::fromJson(readyFile.readAll()).object();

    if (!response.value(QStringLiteral("ok")).toBool()) {
        return cliError(response.value(QStringLiteral("error"))
                            .toString(QStringLiteral("Session daemon failed to start.")),
                        jsonOut);
    }

    if (parser.isSet(QStringLiteral("no-frame"))) response.remove(QStringLiteral("frame"));

    if (jsonOut) {
        QTextStream(stdout) << QString::fromUtf8(
                                   QJsonDocument(response).toJson(QJsonDocument::Compact))
                            << '\n';
    } else {
        const bool showRuler =
            !parser.isSet(QStringLiteral("no-ruler")) && !parser.isSet(QStringLiteral("plain"));
        QTextStream(stdout) << formatInteractiveResponse(
            response, showRuler, parser.isSet(QStringLiteral("styled")),
            parser.isSet(QStringLiteral("scrollback")), parser.isSet(QStringLiteral("tree")));
    }
    return 0;
}

int runSessionList(const QStringList& args) {
    StepOptions opts;
    QStringList pos;
    QString err;
    if (!parseStepArgs(args, &opts, &pos, &err) ||
        !validateAllowedFlags(QStringLiteral("list"), opts.explicitFlags,
                              {QStringLiteral("--json")}, &err)) {
        return cliError(err, opts.json);
    }
    if (!pos.isEmpty())
        return cliError(QStringLiteral("session list takes no positional arguments."), opts.json);

    const QList<SessionInfo> sessions = listActiveSessions();
    if (opts.json) {
        QJsonArray arr;
        for (const SessionInfo& s : sessions) {
            arr.append(QJsonObject{{QStringLiteral("id"), s.id},
                                   {QStringLiteral("default"), s.isDefault},
                                   {QStringLiteral("pid"), s.pid},
                                   {QStringLiteral("daemonPid"), s.daemonPid},
                                   {QStringLiteral("program"), s.program},
                                   {QStringLiteral("clock"), s.clock},
                                   {QStringLiteral("mode"), s.mode},
                                   {QStringLiteral("viewport"), s.viewport},
                                   {QStringLiteral("artifacts"), s.artifacts}});
        }
        QTextStream(stdout) << QString::fromUtf8(
                                   QJsonDocument(QJsonObject{{QStringLiteral("ok"), true},
                                                             {QStringLiteral("sessions"), arr}})
                                       .toJson(QJsonDocument::Compact))
                            << '\n';
        return 0;
    }

    if (sessions.isEmpty()) {
        QTextStream(stdout) << "No active sessions.\n";
        return 0;
    }
    QTextStream out(stdout);
    for (const SessionInfo& s : sessions) {
        const int cols = s.viewport.value(QStringLiteral("columns")).toInt();
        const int rows = s.viewport.value(QStringLiteral("rows")).toInt();
        out << (s.isDefault ? "* " : "  ") << s.id << "  pid=" << s.pid << "  " << cols << 'x'
            << rows << "  " << s.clock << "  " << s.mode << "  " << s.program << '\n';
    }
    return 0;
}

int runSessionStop(const QStringList& args) {
    StepOptions opts;
    QStringList pos;
    QString err;
    if (!parseStepArgs(args, &opts, &pos, &err) ||
        !validateAllowedFlags(QStringLiteral("stop"), opts.explicitFlags,
                              {QStringLiteral("--session"), QStringLiteral("--all"),
                               QStringLiteral("--export"), QStringLiteral("--json")},
                              &err)) {
        return cliError(err, opts.json);
    }
    if (!pos.isEmpty())
        return cliError(QStringLiteral("session stop takes no positional arguments."), opts.json);

    if (opts.stopAll) {
        if (!opts.exportPath.isEmpty()) {
            return cliError(QStringLiteral("--export cannot be combined with --all."), opts.json);
        }
        const QList<SessionInfo> sessions = listActiveSessions();
        QJsonArray stopped;
        QJsonArray failed;
        for (const SessionInfo& s : sessions) {
            QJsonObject reply;
            QString sendErr;
            const bool sent = sendSessionRequest(
                s.socketName, QJsonObject{{QStringLiteral("cmd"), QStringLiteral("stop")}}, &reply,
                &sendErr, 5000);
            if (sent && reply.value(QStringLiteral("ok")).toBool()) {
                stopped.append(s.id);
            } else {
                const QString reason =
                    sent ? reply.value(QStringLiteral("error")).toString() : sendErr;
                failed.append(
                    QJsonObject{{QStringLiteral("id"), s.id}, {QStringLiteral("error"), reason}});
            }
        }
        const bool allOk = failed.isEmpty();
        if (opts.json) {
            QJsonObject outObj{{QStringLiteral("ok"), allOk}, {QStringLiteral("stopped"), stopped}};
            if (!failed.isEmpty()) outObj.insert(QStringLiteral("failed"), failed);
            QTextStream(stdout) << QString::fromUtf8(
                                       QJsonDocument(outObj).toJson(QJsonDocument::Compact))
                                << '\n';
        } else {
            QTextStream(stdout) << "Stopped " << stopped.size() << " session(s).\n";
            for (const QJsonValue& f : failed) {
                const QJsonObject obj = f.toObject();
                QTextStream(stderr)
                    << "qmltui-runner session: failed to stop \""
                    << obj.value(QStringLiteral("id")).toString()
                    << "\": " << obj.value(QStringLiteral("error")).toString() << '\n';
            }
        }
        return allOk ? 0 : 1;
    }

    SessionInfo session;
    if (!resolveActiveSession(opts.sessionId, &session, &err)) return cliError(err, opts.json);

    QJsonObject stopArgs;
    if (!opts.exportPath.isEmpty()) {
        stopArgs.insert(QStringLiteral("exportPath"),
                        QFileInfo(opts.exportPath).absoluteFilePath());
    }
    QJsonObject reply;
    if (!sendSessionRequest(session.socketName,
                            QJsonObject{{QStringLiteral("cmd"), QStringLiteral("stop")},
                                        {QStringLiteral("args"), stopArgs}},
                            &reply, &err, 10000)) {
        return cliError(err, opts.json);
    }

    if (opts.json) {
        QTextStream(stdout) << QString::fromUtf8(
                                   QJsonDocument(reply).toJson(QJsonDocument::Compact))
                            << '\n';
    } else if (!reply.value(QStringLiteral("ok")).toBool()) {
        QTextStream(stderr) << "qmltui-runner session: "
                            << reply.value(QStringLiteral("error")).toString() << '\n';
    } else {
        QTextStream out(stdout);
        out << "Stopped session " << session.id
            << " (artifacts: " << reply.value(QStringLiteral("artifacts")).toString() << ")";
        if (reply.contains(QStringLiteral("exported")))
            out << "; exported: " << reply.value(QStringLiteral("exported")).toString();
        out << '\n';
    }
    return reply.value(QStringLiteral("ok")).toBool() ? 0 : 1;
}

int runSessionExport(const QStringList& args) {
    StepOptions opts;
    QStringList pos;
    QString err;
    if (!parseStepArgs(args, &opts, &pos, &err) ||
        !validateAllowedFlags(
            QStringLiteral("export"), opts.explicitFlags,
            {QStringLiteral("--session"), QStringLiteral("-o"), QStringLiteral("--json")}, &err)) {
        return cliError(err, opts.json);
    }
    if (!pos.isEmpty())
        return cliError(
            QStringLiteral("session export takes no positional arguments; use -o <path>."),
            opts.json);

    SessionInfo session;
    if (!resolveActiveSession(opts.sessionId, &session, &err)) return cliError(err, opts.json);

    QJsonObject reply;
    if (!sendSessionRequest(session.socketName,
                            QJsonObject{{QStringLiteral("cmd"), QStringLiteral("export")}}, &reply,
                            &err, 5000)) {
        return cliError(err, opts.json);
    }

    const QString script = reply.value(QStringLiteral("script")).toString();
    if (!opts.outputPath.isEmpty()) {
        QString writeErr;
        if (!writeTextFileAtomic(opts.outputPath, script, &writeErr))
            return cliError(QStringLiteral("Cannot write %1: %2").arg(opts.outputPath, writeErr),
                            opts.json);
        reply.insert(QStringLiteral("path"), QFileInfo(opts.outputPath).absoluteFilePath());
    }

    if (opts.json) {
        QTextStream(stdout) << QString::fromUtf8(
                                   QJsonDocument(reply).toJson(QJsonDocument::Compact))
                            << '\n';
    } else if (!opts.outputPath.isEmpty()) {
        QTextStream(stdout) << "Exported " << reply.value(QStringLiteral("stepCount")).toInt()
                            << " step(s) to " << reply.value(QStringLiteral("path")).toString()
                            << '\n';
    } else {
        QTextStream(stdout) << script;
    }
    return 0;
}

int runSessionStep(const QString& verb, const QStringList& args) {
    StepOptions defaults;
    ParsedSessionStep step;
    QString err;
    if (!buildSessionStepRequest(verb, args, defaults, &step, &err))
        return cliError(err, step.options.json);

    SessionInfo session;
    if (!resolveActiveSession(step.options.sessionId, &session, &err))
        return cliError(err, step.options.json);

    const int socketTimeout = step.options.timeoutMs > 0 ? step.options.timeoutMs + 10000 : 60000;
    QJsonObject reply;
    if (!sendSessionRequest(session.socketName, step.request, &reply, &err, socketTimeout))
        return cliError(err, step.options.json);

    if (step.options.json) {
        QTextStream(stdout) << QString::fromUtf8(
                                   QJsonDocument(reply).toJson(QJsonDocument::Compact))
                            << '\n';
    } else if (verb == QLatin1String("tree") && reply.value(QStringLiteral("ok")).toBool()) {
        QTextStream(stdout) << formatItemTree(reply.value(QStringLiteral("tree")).toObject())
                            << '\n';
    } else if (verb == QLatin1String("logs") && reply.value(QStringLiteral("ok")).toBool()) {
        const QJsonArray logs = reply.value(QStringLiteral("logs")).toArray();
        QTextStream out(stdout);
        if (logs.isEmpty()) {
            out << "(no logs)\n";
        } else {
            for (const QJsonValue& entry : logs)
                out << entry.toString() << '\n';
        }
    } else {
        QTextStream(stdout) << formatInteractiveResponse(
            reply, step.options.showRuler, step.showStyled, step.showScrollback, step.showTree);
    }

    return reply.value(QStringLiteral("ok")).toBool() ? 0 : 1;
}

int runSessionWatchCli(const QStringList& args) {
    StepOptions opts;
    QStringList pos;
    QString err;
    if (!parseStepArgs(args, &opts, &pos, &err) ||
        !validateAllowedFlags(QStringLiteral("watch"), opts.explicitFlags,
                              {QStringLiteral("--session")}, &err)) {
        return cliError(err);
    }
    if (!pos.isEmpty())
        return cliError(QStringLiteral("Usage: qmltui-runner session watch [--session <id>]"));

    SessionInfo session;
    if (!resolveActiveSession(opts.sessionId, &session, &err)) return cliError(err);
    return runSessionWatch(session);
}

#endif // QMLTUI_RUNNER_HAS_SESSION_DAEMON

} // namespace

int runSessionCli(const QStringList& args) {
    if (args.isEmpty() || args.first() == QLatin1String("help") ||
        args.first() == QLatin1String("--help") || args.first() == QLatin1String("-h")) {
        printSessionHelp();
        return 0;
    }

#if !QMLTUI_RUNNER_HAS_SESSION_DAEMON
    return cliError(QStringLiteral(
        "qmltui-runner was built without Qt6::Network, so persistent `session` daemons are "
        "unavailable. Use `qmltui-runner --interactive -- <app>` for direct stdio NDJSON driving, "
        "or rebuild with Qt6::Network installed."));
#else
    const QString verb = args.first();
    const QStringList rest = args.mid(1);
    if (verb == QLatin1String("start")) return runSessionStart(rest);
    if (verb == QLatin1String("connect") || verb == QLatin1String("shell"))
        return runSessionConnect(rest, /*defaultBatch=*/false);
    if (verb == QLatin1String("batch")) return runSessionConnect(rest, /*defaultBatch=*/true);
    if (verb == QLatin1String("watch")) return runSessionWatchCli(rest);
    if (verb == QLatin1String("list")) return runSessionList(rest);
    if (verb == QLatin1String("stop")) return runSessionStop(rest);
    if (verb == QLatin1String("export")) return runSessionExport(rest);
    return runSessionStep(verb, rest);
#endif
}

} // namespace qmltui::runner_cli
