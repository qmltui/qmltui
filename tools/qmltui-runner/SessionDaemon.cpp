#include "SessionDaemon.h"
#include "ChildSession.h"
#include "Formatters.h"
#include "TerminalSize.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QPointer>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>
#include <QTimer>

#if QMLTUI_RUNNER_HAS_SESSION_DAEMON
#include <QLocalServer>
#include <QLocalSocket>
#endif

#include <algorithm>
#include <csignal>
#include <cstdio>
#include <functional>
#include <memory>

namespace qmltui::runner_cli {

namespace {

QString sanitizeForFilename(const QString& raw) {
    QString out = raw;
    static const QRegularExpression bad(QStringLiteral("[^A-Za-z0-9_-]+"));
    out.replace(bad, QStringLiteral("_"));
    if (out.isEmpty()) out = QStringLiteral("user");
    return out.left(24);
}

bool writeJsonAtomic(const QString& path, const QJsonObject& obj, QString* error = nullptr) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    const QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool writeTextAtomic(const QString& path, const QString& text, QString* error = nullptr) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    const QByteArray data = text.toUtf8();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text) || file.write(data) != data.size() ||
        !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

QJsonObject readJsonFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

QString readTextTrimmed(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(file.readAll()).trimmed();
}

} // namespace

QString sessionRegistryDir() {
    const QString overrideDir = qEnvironmentVariable("QMLTUI_RUNNER_SESSION_DIR").trimmed();
    if (!overrideDir.isEmpty()) return QFileInfo(overrideDir).absoluteFilePath();
    const QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR").trimmed();
    if (!runtimeDir.isEmpty() && QFileInfo(runtimeDir).isDir()) {
        return QDir(runtimeDir).filePath(QStringLiteral("qmltui-runner-sessions"));
    }
    const QString user = sanitizeForFilename(
        qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME", QStringLiteral("default"))));
    return QDir(QDir::tempPath()).filePath(QStringLiteral("qmltui-runner-sessions-%1").arg(user));
}

bool ensureSessionRegistryDir(const QString& path) {
    if (!QDir().mkpath(path)) return false;
    (void)QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                          QFileDevice::ExeOwner);
    return true;
}

QString sessionSocketName(const QString& registryDir, const QString& sessionId) {
    const QByteArray hash =
        QCryptographicHash::hash(registryDir.toUtf8(), QCryptographicHash::Sha256).toHex().left(10);
    return QStringLiteral("qmltui-rs-%1-%2").arg(QString::fromLatin1(hash), sessionId);
}

bool isValidSessionName(const QString& name) {
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9_-]{0,31}$"));
    return pattern.match(name).hasMatch();
}

#if QMLTUI_RUNNER_HAS_SESSION_DAEMON

bool sendSessionRequest(const QString& socketName, const QJsonObject& request,
                        QJsonObject* response, QString* error, int timeoutMs) {
    QLocalSocket socket;
    socket.connectToServer(socketName, QIODevice::ReadWrite);
    if (!socket.waitForConnected((std::min)(timeoutMs, 2000))) {
        if (error) *error = socket.errorString();
        return false;
    }

    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (socket.write(payload) != payload.size() || !socket.waitForBytesWritten(2000)) {
        if (error) *error = QStringLiteral("Failed to send request: %1").arg(socket.errorString());
        return false;
    }

    QByteArray buffer;
    QDeadlineTimer deadline((std::max)(1000, timeoutMs));
    while (!deadline.hasExpired()) {
        const int slice = static_cast<int>(
            (std::min<qint64>)(200, (std::max<qint64>)(1, deadline.remainingTime())));
        if (socket.bytesAvailable() > 0 || socket.waitForReadyRead(slice)) {
            buffer.append(socket.readAll());
            const qsizetype nl = buffer.indexOf('\n');
            if (nl >= 0) {
                QJsonParseError parseErr;
                const QJsonDocument doc = QJsonDocument::fromJson(buffer.left(nl), &parseErr);
                if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
                    if (error)
                        *error = QStringLiteral("Malformed daemon response: %1")
                                     .arg(parseErr.errorString());
                    return false;
                }
                if (response) *response = doc.object();
                return true;
            }
        }
        if (socket.state() == QLocalSocket::UnconnectedState) {
            buffer.append(socket.readAll());
            const qsizetype nl = buffer.indexOf('\n');
            if (nl >= 0) {
                const QJsonDocument doc = QJsonDocument::fromJson(buffer.left(nl));
                if (doc.isObject()) {
                    if (response) *response = doc.object();
                    return true;
                }
            }
            if (error)
                *error = QStringLiteral("Session daemon closed the connection unexpectedly.");
            return false;
        }
    }

    if (error) *error = QStringLiteral("Timed out waiting for session daemon response.");
    return false;
}

QList<SessionInfo> listActiveSessions() {
    QList<SessionInfo> active;
    const QString regDirPath = sessionRegistryDir();
    QDir regDir(regDirPath);
    if (!regDir.exists()) return active;

    const QString defaultId = readTextTrimmed(regDir.filePath(QStringLiteral("default")));
    const QStringList files = regDir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    for (const QString& fileName : files) {
        const QString metaPath = regDir.filePath(fileName);
        const QJsonObject meta = readJsonFile(metaPath);
        const QString id = meta.value(QStringLiteral("id")).toString();
        const QString sock = meta.value(QStringLiteral("socketName")).toString();
        if (id.isEmpty() || sock.isEmpty()) {
            QFile::remove(metaPath);
            continue;
        }

        QJsonObject pingReply;
        QString pingErr;
        if (!sendSessionRequest(
                sock,
                QJsonObject{{QStringLiteral("cmd"), QStringLiteral("ping")},
                            {QStringLiteral("args"), QJsonObject{{QStringLiteral("peek"), true}}}},
                &pingReply, &pingErr, 1500) ||
            !pingReply.value(QStringLiteral("ok")).toBool()) {
            // Stale session metadata left behind by a crashed or killed daemon.
            QFile::remove(metaPath);
            QLocalServer::removeServer(sock);
            continue;
        }

        SessionInfo info;
        info.id = id;
        info.socketName = sock;
        info.pid = pingReply.value(QStringLiteral("pid"))
                       .toInteger(meta.value(QStringLiteral("pid")).toInteger());
        info.daemonPid = meta.value(QStringLiteral("daemonPid")).toInteger();
        info.program = meta.value(QStringLiteral("program")).toString();
        info.clock = pingReply.value(QStringLiteral("clock"))
                         .toString(meta.value(QStringLiteral("clock")).toString());
        info.mode = pingReply.value(QStringLiteral("mode"))
                        .toString(meta.value(QStringLiteral("mode")).toString());
        info.artifacts = meta.value(QStringLiteral("artifacts")).toString();
        info.viewport = pingReply.value(QStringLiteral("viewport")).toObject();
        if (info.viewport.isEmpty())
            info.viewport = meta.value(QStringLiteral("viewport")).toObject();
        info.isDefault = (id == defaultId);
        active.append(info);
    }

    // If the recorded default session died, clear the stale default pointer.
    if (!defaultId.isEmpty()) {
        const bool defaultAlive =
            std::any_of(active.cbegin(), active.cend(),
                        [&](const SessionInfo& s) { return s.id == defaultId; });
        if (!defaultAlive) QFile::remove(regDir.filePath(QStringLiteral("default")));
    }
    return active;
}

bool resolveActiveSession(const QString& requestedId, SessionInfo* out, QString* error) {
    const QList<SessionInfo> sessions = listActiveSessions();
    if (!requestedId.isEmpty()) {
        for (const SessionInfo& s : sessions) {
            if (s.id == requestedId) {
                if (out) *out = s;
                return true;
            }
        }
        if (error) {
            *error =
                QStringLiteral("No active session named \"%1\". Run `qmltui-runner session list`.")
                    .arg(requestedId);
        }
        return false;
    }

    if (sessions.isEmpty()) {
        if (error) {
            *error = QStringLiteral(
                "No active sessions. Start one with `qmltui-runner session start -- <app>`.");
        }
        return false;
    }
    for (const SessionInfo& s : sessions) {
        if (s.isDefault) {
            if (out) *out = s;
            return true;
        }
    }
    if (sessions.size() == 1) {
        if (out) *out = sessions.constFirst();
        return true;
    }
    QStringList names;
    for (const SessionInfo& s : sessions)
        names << s.id;
    if (error) {
        *error = QStringLiteral("Multiple sessions are active (%1); specify --session <name>.")
                     .arg(names.join(QStringLiteral(", ")));
    }
    return false;
}

namespace {

volatile std::sig_atomic_t g_liveSignalStop = 0;

void handleLiveStopSignal(int) {
    g_liveSignalStop = 1;
}

void writeStdoutBytes(const QByteArray& bytes) {
    if (bytes.isEmpty()) return;
    std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stdout);
    std::fflush(stdout);
}

void enterLiveScreen(const QString& title) {
    QByteArray seq("\033[?1049h\033[?25l\033[2J\033[H");
    if (!title.isEmpty()) seq += "\033]0;" + title.toUtf8() + "\033\\";
    writeStdoutBytes(seq);
}

void leaveLiveScreen() {
    writeStdoutBytes(QByteArrayLiteral("\033[0m\033[?25h\033[?1049l"));
}

void paintLiveFrameEvent(const QJsonObject& ev) {
    const QByteArray ansi =
        QByteArray::fromBase64(ev.value(QStringLiteral("ansi")).toString().toLatin1());
    if (ansi.isEmpty()) return;
    const QByteArray sbAnsi =
        QByteArray::fromBase64(ev.value(QStringLiteral("scrollbackAnsi")).toString().toLatin1());
    writeStdoutBytes(renderLiveAnsiFrame(ansi, sbAnsi));
}

int runSessionDaemonCore(const QJsonObject& spec, bool foregroundLive) {
    const QString sessionId = spec.value(QStringLiteral("id")).toString();
    const QString socketName = spec.value(QStringLiteral("socketName")).toString();
    const QString regDirPath = spec.value(QStringLiteral("sessionDir")).toString();
    const QString readyFile = spec.value(QStringLiteral("readyFile")).toString();
    const int idleTimeoutMs =
        spec.value(QStringLiteral("idleTimeoutMs")).toInt(foregroundLive ? 0 : 300000);

    const auto writeReadyIfPresent = [&](const QJsonObject& obj) {
        if (readyFile.isEmpty()) return;
        // If `session start` already timed out and destroyed its bootstrap
        // directory, do not recreate it as an orphaned temp folder.
        if (!QFileInfo(QFileInfo(readyFile).absolutePath()).isDir()) return;
        writeJsonAtomic(readyFile, obj);
    };

    const auto failStartup = [&](const QString& msg) {
        if (!readyFile.isEmpty()) {
            writeReadyIfPresent(
                QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), msg}});
        } else {
            QTextStream(stderr) << "qmltui-runner session: " << msg << '\n';
        }
        return foregroundLive ? 2 : 1;
    };

    QLocalServer server;
    server.setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(socketName);
    if (!server.listen(socketName)) {
        return failStartup(QStringLiteral("Cannot listen on session socket %1: %2")
                               .arg(socketName, server.errorString()));
    }

    ChildSession::LaunchConfig cfg;
    cfg.program = spec.value(QStringLiteral("program")).toString();
    for (const QJsonValue& arg : spec.value(QStringLiteral("arguments")).toArray())
        cfg.arguments.append(arg.toString());
    cfg.workingDirectory = spec.value(QStringLiteral("workingDirectory")).toString();
    cfg.clock = spec.value(QStringLiteral("clock")).toString(QStringLiteral("real"));
    cfg.columns = spec.value(QStringLiteral("columns")).toInt(80);
    cfg.rows = spec.value(QStringLiteral("rows")).toInt(24);
    cfg.operationTimeoutMs = spec.value(QStringLiteral("operationTimeoutMs")).toInt(2000);
    cfg.startupTimeoutMs = spec.value(QStringLiteral("startupTimeoutMs")).toInt(5000);
    cfg.artifactDir = spec.value(QStringLiteral("artifactDir")).toString();
    cfg.recordPath = spec.value(QStringLiteral("recordPath")).toString();
    cfg.liveStream = foregroundLive;

    ChildSession child;
    QJsonObject readyInfo;
    QString startError;
    if (!child.start(cfg, &readyInfo, &startError)) {
        server.close();
        QLocalServer::removeServer(socketName);
        return failStartup(startError);
    }

    // If startup requested optional styled/scrollback/tree views, fetch them once now.
    const QJsonObject startInclude = spec.value(QStringLiteral("include")).toObject();
    if (!foregroundLive && (startInclude.value(QStringLiteral("styled")).toBool() ||
                            startInclude.value(QStringLiteral("scrollback")).toBool() ||
                            startInclude.value(QStringLiteral("tree")).toBool())) {
        const QJsonObject extra =
            child.executeCommand(QStringLiteral("frame"), {}, startInclude, cfg.operationTimeoutMs);
        for (const auto& key : {QStringLiteral("styled"), QStringLiteral("scrollback"),
                                QStringLiteral("scrollbackStyled"),
                                QStringLiteral("scrollbackRowCount"), QStringLiteral("tree")}) {
            if (extra.contains(key)) readyInfo.insert(key, extra.value(key));
        }
    }

    ensureSessionRegistryDir(regDirPath);
    const QString metaPath = QDir(regDirPath).filePath(sessionId + QStringLiteral(".json"));
    const QString defaultPath = QDir(regDirPath).filePath(QStringLiteral("default"));

    const QJsonObject metadata{
        {QStringLiteral("id"), sessionId},
        {QStringLiteral("socketName"), socketName},
        {QStringLiteral("pid"), child.pid()},
        {QStringLiteral("daemonPid"), QCoreApplication::applicationPid()},
        {QStringLiteral("program"), cfg.program},
        {QStringLiteral("clock"), child.clock()},
        {QStringLiteral("mode"), child.mode()},
        {QStringLiteral("viewport"), child.viewport()},
        {QStringLiteral("artifacts"), child.artifactDirectory()},
        {QStringLiteral("live"), foregroundLive},
        {QStringLiteral("idleTimeoutMs"), idleTimeoutMs},
        {QStringLiteral("startedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    writeJsonAtomic(metaPath, metadata);
    writeTextAtomic(defaultPath, sessionId);

    readyInfo.insert(QStringLiteral("session"), sessionId);
    readyInfo.insert(QStringLiteral("daemonPid"), QCoreApplication::applicationPid());
    writeReadyIfPresent(readyInfo);

    bool screenEntered = false;
    if (foregroundLive) {
        g_liveSignalStop = 0;
        std::signal(SIGINT, handleLiveStopSignal);
        std::signal(SIGTERM, handleLiveStopSignal);
        enterLiveScreen(QStringLiteral("qmltui-runner session: %1").arg(sessionId));
        screenEntered = true;
        if (!child.lastLiveFrameEvent().isEmpty()) paintLiveFrameEvent(child.lastLiveFrameEvent());
    }

    struct PendingRequest {
        QPointer<QLocalSocket> client;
        QJsonObject request;
    };

    QEventLoop loop;
    bool shuttingDown = false;
    bool handlingRequest = false;
    bool stopRequested = false;
    bool childLiveStreamEnabled = foregroundLive;
    bool desiredLiveStream = foregroundLive;
    QSize pendingHostResize;
    QList<QLocalSocket*> watchers;
    QList<PendingRequest> pendingRequests;

    const auto sendReplyAndClose = [](QLocalSocket* client, const QJsonObject& reply) {
        if (!client || client->state() != QLocalSocket::ConnectedState) return;
        const QByteArray out = QJsonDocument(reply).toJson(QJsonDocument::Compact) + '\n';
        client->write(out);
        client->flush();
        client->waitForBytesWritten(2000);
        client->disconnectFromServer();
        client->deleteLater();
    };

    const auto broadcastToWatchers = [&](const QJsonObject& ev) {
        if (watchers.isEmpty()) return;
        const QByteArray line = QJsonDocument(ev).toJson(QJsonDocument::Compact) + '\n';
        for (QLocalSocket* w : std::as_const(watchers)) {
            if (w && w->state() == QLocalSocket::ConnectedState) {
                w->write(line);
                w->flush();
            }
        }
    };

    const auto cleanupAndQuit = [&] {
        if (shuttingDown) return;
        shuttingDown = true;
        while (!pendingRequests.isEmpty()) {
            PendingRequest leftover = pendingRequests.takeFirst();
            sendReplyAndClose(leftover.client, QJsonObject{{QStringLiteral("ok"), false},
                                                           {QStringLiteral("exited"), true},
                                                           {QStringLiteral("error"),
                                                            QStringLiteral("Session has ended.")}});
        }
        broadcastToWatchers(QJsonObject{{QStringLiteral("event"), QStringLiteral("exited")},
                                        {QStringLiteral("session"), sessionId}});
        for (QLocalSocket* w : std::as_const(watchers)) {
            if (w) {
                w->waitForBytesWritten(200);
                w->disconnectFromServer();
                w->deleteLater();
            }
        }
        watchers.clear();
        if (screenEntered) {
            leaveLiveScreen();
            screenEntered = false;
            QTextStream(stderr) << "qmltui-runner session: live session \"" << sessionId
                                << "\" ended (artifacts: " << child.artifactDirectory() << ")\n";
        }
        QFile::remove(metaPath);
        if (readTextTrimmed(defaultPath) == sessionId) QFile::remove(defaultPath);
        server.close();
        QLocalServer::removeServer(socketName);
        QTimer::singleShot(0, &loop, &QEventLoop::quit);
    };

    QObject::connect(&child, &ChildSession::liveFrameReceived, &loop, [&](const QJsonObject& ev) {
        if (foregroundLive && screenEntered) paintLiveFrameEvent(ev);
        broadcastToWatchers(ev);
    });

    QTimer idleTimer;
    idleTimer.setSingleShot(true);
    QObject::connect(&idleTimer, &QTimer::timeout, &loop, [&] {
        if (handlingRequest) {
            if (idleTimeoutMs > 0) idleTimer.start(idleTimeoutMs);
            return;
        }
        (void)child.stop();
        cleanupAndQuit();
    });
    if (idleTimeoutMs > 0) idleTimer.start(idleTimeoutMs);

    const bool explicitCols = spec.value(QStringLiteral("explicitColumns")).toBool(false);
    const bool explicitRows = spec.value(QStringLiteral("explicitRows")).toBool(false);
    bool followHostResize = foregroundLive && (!explicitCols || !explicitRows);
    QSize lastHostSize(cfg.columns, cfg.rows);
    int resizePollTicks = 0;

    // Serializes all child-process IPC commands one at a time so concurrent
    // CLI invocations never re-enter ChildSession::executeCommand()'s event loop.
    std::function<void()> processQueue;
    processQueue = [&] {
        if (handlingRequest || shuttingDown) return;
        handlingRequest = true;

        while (!shuttingDown) {
            if (stopRequested) {
                handlingRequest = false;
                (void)child.stop();
                cleanupAndQuit();
                return;
            }

            if (!foregroundLive && desiredLiveStream != childLiveStreamEnabled &&
                child.isRunning()) {
                const bool target = desiredLiveStream;
                childLiveStreamEnabled = target;
                (void)child.executeCommand(QStringLiteral("setLiveStream"),
                                           QJsonObject{{QStringLiteral("enabled"), target}}, {},
                                           3000);
                if (target && !child.lastLiveFrameEvent().isEmpty()) {
                    broadcastToWatchers(child.lastLiveFrameEvent());
                }
                continue;
            }

            if (pendingHostResize.isValid() && followHostResize && child.isRunning()) {
                const QSize target = pendingHostResize;
                pendingHostResize = QSize();
                lastHostSize = target;
                (void)child.executeCommand(QStringLiteral("resize"),
                                           QJsonObject{{QStringLiteral("columns"), target.width()},
                                                       {QStringLiteral("rows"), target.height()},
                                                       {QStringLiteral("record"), false}},
                                           QJsonObject{{QStringLiteral("frame"), false}},
                                           cfg.operationTimeoutMs);
                continue;
            }

            if (pendingRequests.isEmpty()) break;
            PendingRequest item = pendingRequests.takeFirst();
            if (!item.client || item.client->state() != QLocalSocket::ConnectedState) continue;

            const QJsonObject& req = item.request;
            const QString cmd = req.value(QStringLiteral("cmd")).toString();
            const QJsonObject args = req.value(QStringLiteral("args")).toObject();
            const QJsonObject include = req.value(QStringLiteral("include")).toObject();
            const int timeoutMs = req.value(QStringLiteral("timeoutMs")).toInt(-1);

            if (idleTimeoutMs > 0) idleTimer.start(idleTimeoutMs);

            QJsonObject reply;
            bool shouldQuit = false;
            if (cmd == QLatin1String("stop")) {
                const QString exportPath = args.value(QStringLiteral("exportPath")).toString();
                if (!exportPath.isEmpty()) {
                    const QString exportedScript = child.exportScenarioJs();
                    QString writeErr;
                    if (!writeTextAtomic(exportPath, exportedScript, &writeErr)) {
                        reply = QJsonObject{{QStringLiteral("ok"), false},
                                            {QStringLiteral("error"),
                                             QStringLiteral("Cannot write exported scenario %1: %2")
                                                 .arg(exportPath, writeErr)}};
                    }
                }
                if (!reply.contains(QStringLiteral("ok"))) {
                    reply = child.stop();
                    reply.insert(QStringLiteral("session"), sessionId);
                    if (!exportPath.isEmpty()) reply.insert(QStringLiteral("exported"), exportPath);
                    shouldQuit = true;
                }
            } else {
                if (cmd == QLatin1String("resize")) {
                    followHostResize = false;
                    pendingHostResize = QSize();
                }
                reply = child.executeCommand(cmd, args, include, timeoutMs);
                reply.insert(QStringLiteral("session"), sessionId);
                reply.insert(QStringLiteral("artifacts"), child.artifactDirectory());
                if (reply.value(QStringLiteral("exited")).toBool()) shouldQuit = true;
            }

            if (idleTimeoutMs > 0 && !shouldQuit) idleTimer.start(idleTimeoutMs);

            sendReplyAndClose(item.client, reply);

            if (shouldQuit || !child.isRunning()) {
                handlingRequest = false;
                cleanupAndQuit();
                return;
            }
        }

        handlingRequest = false;
        if (stopRequested) {
            (void)child.stop();
            cleanupAndQuit();
        } else if (!child.isRunning()) {
            cleanupAndQuit();
        }
    };

    QTimer signalTimer;
    if (foregroundLive) {
        signalTimer.setInterval(50);
        QObject::connect(&signalTimer, &QTimer::timeout, &loop, [&] {
            if (g_liveSignalStop != 0) {
                signalTimer.stop();
                stopRequested = true;
                if (!handlingRequest) {
                    (void)child.stop();
                    cleanupAndQuit();
                }
                return;
            }
            if (followHostResize && !shuttingDown && child.isRunning() &&
                (++resizePollTicks % 3 == 0)) {
                const QSize host = queryHostTerminalSize();
                if (host.isValid()) {
                    const QSize target(explicitCols ? lastHostSize.width() : host.width(),
                                       explicitRows ? lastHostSize.height() : host.height());
                    if (target != lastHostSize && target != pendingHostResize) {
                        pendingHostResize = target;
                        if (!handlingRequest) processQueue();
                    }
                }
            }
        });
        signalTimer.start();
    }

    QObject::connect(&child, &ChildSession::childExited, &loop, [&](int, QProcess::ExitStatus) {
        if (!handlingRequest) cleanupAndQuit();
    });

    const auto dispatchClientRequest = [&](QLocalSocket* client, const QByteArray& rawLine) {
        QJsonParseError parseErr;
        const QJsonDocument doc = QJsonDocument::fromJson(rawLine.trimmed(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            sendReplyAndClose(
                client,
                QJsonObject{
                    {QStringLiteral("ok"), false},
                    {QStringLiteral("error"),
                     QStringLiteral("Invalid request JSON: %1").arg(parseErr.errorString())}});
            return;
        }

        const QJsonObject req = doc.object();
        const QString cmd = req.value(QStringLiteral("cmd")).toString();
        const QJsonObject args = req.value(QStringLiteral("args")).toObject();

        // `ping` reads cached daemon/child metadata with zero child IPC so
        // `session list` stays instant even while another command is running.
        if (cmd == QLatin1String("ping")) {
            const bool isPeekPing = args.value(QStringLiteral("peek")).toBool(false);
            if (!isPeekPing && idleTimeoutMs > 0) idleTimer.start(idleTimeoutMs);
            QJsonObject reply = child.executeCommand(QStringLiteral("info"));
            reply.insert(QStringLiteral("session"), sessionId);
            reply.insert(QStringLiteral("daemonPid"), QCoreApplication::applicationPid());
            reply.insert(QStringLiteral("live"), foregroundLive);
            reply.insert(QStringLiteral("idleTimeoutMs"), idleTimeoutMs);
            sendReplyAndClose(client, reply);
            return;
        }

        if (cmd == QLatin1String("watch")) {
            watchers.append(client);
            QObject::connect(client, &QLocalSocket::disconnected, &loop, [&, client] {
                watchers.removeAll(client);
                client->deleteLater();
                if (watchers.isEmpty() && !foregroundLive && child.isRunning() && !shuttingDown) {
                    desiredLiveStream = false;
                    if (!handlingRequest) processQueue();
                }
            });
            const QJsonObject initial = child.lastLiveFrameEvent();
            if (!initial.isEmpty()) {
                const QByteArray bytes =
                    QJsonDocument(initial).toJson(QJsonDocument::Compact) + '\n';
                client->write(bytes);
                client->flush();
            }
            if (!childLiveStreamEnabled) {
                desiredLiveStream = true;
                if (!handlingRequest) processQueue();
            }
            return;
        }

        pendingRequests.append(PendingRequest{QPointer<QLocalSocket>(client), req});
        if (!handlingRequest) processQueue();
    };

    QObject::connect(&server, &QLocalServer::newConnection, &loop, [&] {
        while (QLocalSocket* client = server.nextPendingConnection()) {
            auto buffer = std::make_shared<QByteArray>();
            auto handled = std::make_shared<bool>(false);
            auto* readTimeout = new QTimer(client);
            readTimeout->setSingleShot(true);

            const auto finishRead = [&, client, buffer, handled, readTimeout](bool fromDisconnect) {
                if (*handled) return;
                buffer->append(client->readAll());
                const qsizetype nl = buffer->indexOf('\n');
                if (nl < 0 && !fromDisconnect) return;
                *handled = true;
                readTimeout->stop();
                const QByteArray line = nl >= 0 ? buffer->left(nl) : *buffer;
                if (line.trimmed().isEmpty() && fromDisconnect) {
                    client->deleteLater();
                    return;
                }
                dispatchClientRequest(client, line);
            };

            QObject::connect(client, &QLocalSocket::readyRead, client,
                             [finishRead] { finishRead(false); });
            QObject::connect(client, &QLocalSocket::disconnected, client,
                             [finishRead] { finishRead(true); });
            QObject::connect(readTimeout, &QTimer::timeout, client, [&, client, handled] {
                if (*handled) return;
                *handled = true;
                sendReplyAndClose(
                    client,
                    QJsonObject{{QStringLiteral("ok"), false},
                                {QStringLiteral("error"),
                                 QStringLiteral("Timed out reading request from client.")}});
            });
            readTimeout->start(3000);
            if (client->bytesAvailable() > 0) finishRead(false);
        }
    });

    loop.exec();
    if (screenEntered) leaveLiveScreen();
    return 0;
}

} // namespace

int runSessionDaemon(const QString& launchSpecPath) {
    const QJsonObject spec = readJsonFile(launchSpecPath);
    QFile::remove(launchSpecPath);
    if (spec.isEmpty()) {
        QTextStream(stderr) << "qmltui-runner: invalid session daemon launch specification.\n";
        return 2;
    }
    return runSessionDaemonCore(spec, /*foregroundLive=*/false);
}

int runLiveSessionDaemon(const QJsonObject& launchSpec) {
    return runSessionDaemonCore(launchSpec, /*foregroundLive=*/true);
}

int runSessionWatch(const SessionInfo& session) {
    QLocalSocket socket;
    socket.connectToServer(session.socketName, QIODevice::ReadWrite);
    if (!socket.waitForConnected(2000)) {
        QTextStream(stderr) << "qmltui-runner session: cannot attach to \"" << session.id
                            << "\": " << socket.errorString() << '\n';
        return 2;
    }
    const QByteArray req = QByteArrayLiteral("{\"cmd\":\"watch\"}\n");
    if (socket.write(req) != req.size() || !socket.waitForBytesWritten(2000)) {
        QTextStream(stderr) << "qmltui-runner session: failed to subscribe to \"" << session.id
                            << "\": " << socket.errorString() << '\n';
        return 2;
    }

    g_liveSignalStop = 0;
    std::signal(SIGINT, handleLiveStopSignal);
    std::signal(SIGTERM, handleLiveStopSignal);
    enterLiveScreen(QStringLiteral("qmltui-runner watch: %1").arg(session.id));

    QEventLoop loop;
    QByteArray buffer;
    QObject::connect(&socket, &QLocalSocket::readyRead, &loop, [&] {
        buffer.append(socket.readAll());
        qsizetype nl = -1;
        while ((nl = buffer.indexOf('\n')) >= 0) {
            const QByteArray line = buffer.left(nl).trimmed();
            buffer.remove(0, nl + 1);
            if (line.isEmpty()) continue;
            const QJsonDocument doc = QJsonDocument::fromJson(line);
            if (!doc.isObject()) continue;
            const QJsonObject obj = doc.object();
            const QString ev = obj.value(QStringLiteral("event")).toString();
            if (ev == QLatin1String("frame")) {
                paintLiveFrameEvent(obj);
            } else if (ev == QLatin1String("exited")) {
                loop.quit();
                return;
            }
        }
    });
    QObject::connect(&socket, &QLocalSocket::disconnected, &loop, &QEventLoop::quit);

    QTimer signalTimer;
    signalTimer.setInterval(50);
    QObject::connect(&signalTimer, &QTimer::timeout, &loop, [&] {
        if (g_liveSignalStop != 0) loop.quit();
    });
    signalTimer.start();

    loop.exec();
    signalTimer.stop();
    socket.disconnectFromServer();
    leaveLiveScreen();
    return 0;
}

#endif // QMLTUI_RUNNER_HAS_SESSION_DAEMON

} // namespace qmltui::runner_cli
