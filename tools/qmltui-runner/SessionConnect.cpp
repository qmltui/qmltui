#include "SessionConnect.h"
#include "Formatters.h"
#include "SessionCommand.h"
#include "SessionDaemon.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <iostream>
#include <string>

namespace qmltui::runner_cli {

#if QMLTUI_RUNNER_HAS_SESSION_DAEMON

namespace {

void printConnectHelp() {
    QTextStream(stdout) << R"(Interactive session console commands:
  press <key> [--modifiers ...]        type <text> / paste <text>
  click <x> <y> / double-click / drag  scroll <x> <y> <dx> <dy>
  click-text <text> [--occurrence N]   click-item <objectName>
  hover <x> <y> / hover-text <text>    resize <cols> <rows>
  advance <ms> / sleep <ms>            wait-text / wait-item / wait-regex
  expect-text / expect-no-text         expect-scrollback / expect-no-scrollback
  eval <js-code> [--no-record]         frame [--diff] [--styled] [--scrollback]
  scrollback [--from-row N] [--styled] tree [--include-hidden]
  cell <x> <y> / find-text / item-rect stats / logs [--all]
  marker <label> / record start|stop   export [-o <file>]
  list                                 List active sessions
  mode on|off|diff|ruler|plain         Change default output after each step
  detach | exit | quit (or Ctrl+D)     Leave console (keeps session running)
  stop [--export <file>]               Stop the session and leave console
)";
}

void printStepReply(const ParsedSessionStep& step, const QJsonObject& reply) {
    if (step.options.json) {
        QTextStream(stdout) << QString::fromUtf8(
                                   QJsonDocument(reply).toJson(QJsonDocument::Compact))
                            << '\n';
        return;
    }
    if (step.verb == QLatin1String("tree") && reply.value(QStringLiteral("ok")).toBool()) {
        QTextStream(stdout) << formatItemTree(reply.value(QStringLiteral("tree")).toObject())
                            << '\n';
        return;
    }
    if (step.verb == QLatin1String("logs") && reply.value(QStringLiteral("ok")).toBool()) {
        const QJsonArray logs = reply.value(QStringLiteral("logs")).toArray();
        QTextStream out(stdout);
        if (logs.isEmpty()) {
            out << "(no logs)\n";
        } else {
            for (const QJsonValue& entry : logs)
                out << entry.toString() << '\n';
        }
        return;
    }
    QTextStream(stdout) << formatInteractiveResponse(reply, step.options.showRuler, step.showStyled,
                                                     step.showScrollback, step.showTree);
}

} // namespace

int runSessionConnect(const QStringList& args, bool defaultBatch) {
    StepOptions defaults;
    defaults.batch = defaultBatch;
    QStringList positional;
    QString err;
    const QString connectVerb = defaultBatch ? QStringLiteral("batch") : QStringLiteral("connect");
    const QStringList allowedConnectFlags = {
        QStringLiteral("--session"),  QStringLiteral("--ruler"),
        QStringLiteral("--no-ruler"), QStringLiteral("--plain"),
        QStringLiteral("--no-frame"), QStringLiteral("--diff"),
        QStringLiteral("--styled"),   QStringLiteral("--scrollback"),
        QStringLiteral("--tree"),     QStringLiteral("--include-hidden"),
        QStringLiteral("--batch"),    QStringLiteral("--continue-on-error"),
        QStringLiteral("--json")};
    if (!parseStepArgs(args, &defaults, &positional, &err) ||
        !validateAllowedFlags(connectVerb, defaults.explicitFlags, allowedConnectFlags, &err)) {
        if (defaults.json) {
            QTextStream(stdout) << QString::fromUtf8(
                                       QJsonDocument(QJsonObject{{QStringLiteral("ok"), false},
                                                                 {QStringLiteral("error"), err}})
                                           .toJson(QJsonDocument::Compact))
                                << '\n';
        } else {
            QTextStream(stderr) << "qmltui-runner session: " << err << '\n';
        }
        return 2;
    }
    if (positional.size() > 1) {
        const QString usage = QStringLiteral(
            "Usage: qmltui-runner session connect [session-id] [--no-frame] [--diff] "
            "[--no-ruler|--plain] [--batch] [--continue-on-error] [--json]");
        if (defaults.json) {
            QTextStream(stdout) << QString::fromUtf8(
                                       QJsonDocument(QJsonObject{{QStringLiteral("ok"), false},
                                                                 {QStringLiteral("error"), usage}})
                                           .toJson(QJsonDocument::Compact))
                                << '\n';
        } else {
            QTextStream(stderr) << usage << '\n';
        }
        return 2;
    }
    if (positional.size() == 1 && defaults.sessionId.isEmpty())
        defaults.sessionId = positional.first();

    SessionInfo session;
    if (!resolveActiveSession(defaults.sessionId, &session, &err)) {
        if (defaults.json) {
            QTextStream(stdout) << QString::fromUtf8(
                                       QJsonDocument(QJsonObject{{QStringLiteral("ok"), false},
                                                                 {QStringLiteral("error"), err}})
                                           .toJson(QJsonDocument::Compact))
                                << '\n';
        } else {
            QTextStream(stderr) << "qmltui-runner session: " << err << '\n';
        }
        return 2;
    }

    const bool interactivePrompt = !defaults.batch && !defaults.json;
    if (interactivePrompt) {
        QTextStream(stderr) << "Connected to session \"" << session.id << "\" (" << session.clock
                            << ", " << session.mode << "). Type `help`, `detach`, or `stop`.\n";
    }

    int lastExitCode = 0;
    QJsonObject viewport = session.viewport;

    std::string rawLine;
    while (true) {
        if (interactivePrompt) {
            const int cols = viewport.value(QStringLiteral("columns")).toInt(80);
            const int rows = viewport.value(QStringLiteral("rows")).toInt(24);
            QTextStream prompt(stderr);
            prompt << session.id << " (" << cols << 'x' << rows << ")> ";
            prompt.flush();
        }
        if (!std::getline(std::cin, rawLine)) break;
        if (!rawLine.empty() && rawLine.back() == '\r') rawLine.pop_back();

        QStringList tokens;
        QString parseErr;
        if (!splitCommandLine(QString::fromUtf8(rawLine), &tokens, &parseErr)) {
            QTextStream(stderr) << "Error: " << parseErr << '\n';
            lastExitCode = 2;
            if (defaults.batch && !defaults.continueOnError) return 2;
            continue;
        }
        if (tokens.isEmpty()) continue;

        const QString verb = tokens.first();
        const QStringList rest = tokens.mid(1);

        if (verb == QLatin1String("help") || verb == QLatin1String("?")) {
            printConnectHelp();
            continue;
        }
        if (verb == QLatin1String("detach") || verb == QLatin1String("exit") ||
            verb == QLatin1String("quit")) {
            break;
        }
        if (verb == QLatin1String("mode")) {
            if (rest.size() != 1) {
                QTextStream(stderr) << "Usage: mode on|off|diff|ruler|plain\n";
                lastExitCode = 2;
                if (defaults.batch && !defaults.continueOnError) return 2;
                continue;
            }
            const QString setting = rest.first();
            if (setting == QLatin1String("on") || setting == QLatin1String("ruler")) {
                defaults.includeFrame = true;
                defaults.showRuler = true;
                defaults.showDiff = false;
            } else if (setting == QLatin1String("plain")) {
                defaults.includeFrame = true;
                defaults.showRuler = false;
                defaults.showDiff = false;
            } else if (setting == QLatin1String("off")) {
                defaults.includeFrame = false;
                defaults.showDiff = false;
            } else if (setting == QLatin1String("diff")) {
                defaults.includeFrame = true;
                defaults.showDiff = true;
            } else {
                QTextStream(stderr) << "Unknown mode \"" << setting
                                    << "\": expected on, off, diff, ruler, or plain.\n";
                lastExitCode = 2;
                if (defaults.batch && !defaults.continueOnError) return 2;
                continue;
            }
            if (interactivePrompt)
                QTextStream(stderr) << "Default frame output set to: " << setting << '\n';
            continue;
        }
        if (verb == QLatin1String("list")) {
            const QList<SessionInfo> active = listActiveSessions();
            QTextStream out(stdout);
            for (const SessionInfo& s : active) {
                const int cols = s.viewport.value(QStringLiteral("columns")).toInt();
                const int rows = s.viewport.value(QStringLiteral("rows")).toInt();
                out << (s.id == session.id ? "* " : "  ") << s.id << "  pid=" << s.pid << "  "
                    << cols << 'x' << rows << "  " << s.clock << "  " << s.mode << "  " << s.program
                    << '\n';
            }
            continue;
        }
        if (verb == QLatin1String("export")) {
            StepOptions expOpts = defaults;
            QStringList expPos;
            err.clear();
            if (!parseStepArgs(rest, &expOpts, &expPos, &err) ||
                !validateAllowedFlags(QStringLiteral("export"), expOpts.explicitFlags,
                                      {QStringLiteral("-o"), QStringLiteral("--json")}, &err) ||
                !expPos.isEmpty()) {
                QTextStream(stderr)
                    << "Error: "
                    << (err.isEmpty() ? QStringLiteral("Usage: export [-o <file>]") : err) << '\n';
                lastExitCode = 2;
                if (defaults.batch && !defaults.continueOnError) return 2;
                continue;
            }
            QJsonObject reply;
            if (!sendSessionRequest(session.socketName,
                                    QJsonObject{{QStringLiteral("cmd"), QStringLiteral("export")}},
                                    &reply, &err, 5000)) {
                QTextStream(stderr) << "Error: " << err << '\n';
                return 2;
            }
            const QString script = reply.value(QStringLiteral("script")).toString();
            if (!expOpts.outputPath.isEmpty()) {
                QString writeErr;
                if (!writeTextFileAtomic(expOpts.outputPath, script, &writeErr)) {
                    QTextStream(stderr)
                        << "Error: cannot write " << expOpts.outputPath << ": " << writeErr << '\n';
                    lastExitCode = 2;
                    if (defaults.batch && !defaults.continueOnError) return 2;
                    continue;
                }
                reply.insert(QStringLiteral("path"),
                             QFileInfo(expOpts.outputPath).absoluteFilePath());
                if (expOpts.json) {
                    QTextStream(stdout)
                        << QString::fromUtf8(QJsonDocument(reply).toJson(QJsonDocument::Compact))
                        << '\n';
                } else {
                    QTextStream(stdout)
                        << "Exported " << reply.value(QStringLiteral("stepCount")).toInt()
                        << " step(s) to " << reply.value(QStringLiteral("path")).toString() << '\n';
                }
            } else if (expOpts.json) {
                QTextStream(stdout)
                    << QString::fromUtf8(QJsonDocument(reply).toJson(QJsonDocument::Compact))
                    << '\n';
            } else {
                QTextStream(stdout) << script;
            }
            continue;
        }
        if (verb == QLatin1String("stop")) {
            StepOptions stopOpts = defaults;
            QStringList stopPos;
            err.clear();
            if (!parseStepArgs(rest, &stopOpts, &stopPos, &err) ||
                !validateAllowedFlags(QStringLiteral("stop"), stopOpts.explicitFlags,
                                      {QStringLiteral("--export"), QStringLiteral("--json")},
                                      &err) ||
                !stopPos.isEmpty()) {
                QTextStream(stderr)
                    << "Error: "
                    << (err.isEmpty() ? QStringLiteral("Usage: stop [--export <file>]") : err)
                    << '\n';
                lastExitCode = 2;
                if (defaults.batch && !defaults.continueOnError) return 2;
                continue;
            }
            QJsonObject stopArgs;
            if (!stopOpts.exportPath.isEmpty()) {
                stopArgs.insert(QStringLiteral("exportPath"),
                                QFileInfo(stopOpts.exportPath).absoluteFilePath());
            }
            QJsonObject reply;
            if (!sendSessionRequest(session.socketName,
                                    QJsonObject{{QStringLiteral("cmd"), QStringLiteral("stop")},
                                                {QStringLiteral("args"), stopArgs}},
                                    &reply, &err, 10000)) {
                QTextStream(stderr) << "Error: " << err << '\n';
                return 2;
            }
            const bool stoppedOk = reply.value(QStringLiteral("ok")).toBool();
            if (stopOpts.json) {
                QTextStream(stdout)
                    << QString::fromUtf8(QJsonDocument(reply).toJson(QJsonDocument::Compact))
                    << '\n';
            } else if (!stoppedOk) {
                QTextStream(stderr)
                    << "Error: " << reply.value(QStringLiteral("error")).toString() << '\n';
            } else {
                QTextStream out(stdout);
                out << "Stopped session " << session.id
                    << " (artifacts: " << reply.value(QStringLiteral("artifacts")).toString()
                    << ")";
                if (reply.contains(QStringLiteral("exported")))
                    out << "; exported: " << reply.value(QStringLiteral("exported")).toString();
                out << '\n';
            }
            if (!stoppedOk) {
                lastExitCode = 1;
                if (defaults.batch && !defaults.continueOnError) return 1;
                continue;
            }
            return defaults.batch ? lastExitCode : 0;
        }

        ParsedSessionStep step;
        if (!buildSessionStepRequest(verb, rest, defaults, &step, &err)) {
            QTextStream(stderr) << "Error: " << err << '\n';
            lastExitCode = 2;
            if (defaults.batch && !defaults.continueOnError) return 2;
            continue;
        }

        const int socketTimeout =
            step.options.timeoutMs > 0 ? step.options.timeoutMs + 10000 : 60000;
        QJsonObject reply;
        if (!sendSessionRequest(session.socketName, step.request, &reply, &err, socketTimeout)) {
            QTextStream(stderr) << "Error: " << err << '\n';
            return 2;
        }

        if (reply.contains(QStringLiteral("viewport")))
            viewport = reply.value(QStringLiteral("viewport")).toObject();

        printStepReply(step, reply);

        const bool ok = reply.value(QStringLiteral("ok")).toBool();
        if (!ok) {
            lastExitCode = 1;
            if (defaults.batch && !defaults.continueOnError) return 1;
        }
        if (reply.value(QStringLiteral("exited")).toBool()) break;
    }

    return defaults.batch ? lastExitCode : 0;
}

#else

int runSessionConnect(const QStringList&, bool) {
    QTextStream(stderr) << "qmltui-runner session: built without Qt6::Network.\n";
    return 2;
}

#endif // QMLTUI_RUNNER_HAS_SESSION_DAEMON

} // namespace qmltui::runner_cli
