#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

namespace qmltui::runner_cli {

struct SessionInfo {
    QString id;
    QString socketName;
    qint64 pid = 0;
    qint64 daemonPid = 0;
    QString program;
    QString clock;
    QString mode;
    QString artifacts;
    QJsonObject viewport;
    bool isDefault = false;
};

// Returns the directory storing active session metadata files. Respects
// QMLTUI_RUNNER_SESSION_DIR so automated tests stay isolated, preferring
// XDG_RUNTIME_DIR when available.
QString sessionRegistryDir();

// Creates the session registry directory if needed and restricts permissions
// to the current user when possible.
bool ensureSessionRegistryDir(const QString& path);

// Computes a short, collision-resistant QLocalServer name safe across Unix
// domain sockets (sun_path limit) and Windows named pipes.
QString sessionSocketName(const QString& registryDir, const QString& sessionId);

// Validates user-supplied session names (1..32 chars of [A-Za-z0-9_-]).
bool isValidSessionName(const QString& name);

#if QMLTUI_RUNNER_HAS_SESSION_DAEMON
// Entry point for the detached background supervisor process (`__session-daemon`).
int runSessionDaemon(const QString& launchSpecPath);

// Runs a live session daemon in the foreground of the current terminal,
// rendering committed ANSI frames to stdout while accepting session commands
// from other terminals over QLocalServer.
int runLiveSessionDaemon(const QJsonObject& launchSpec);

// Attaches to an already-running session and streams its live ANSI frames to
// stdout until Ctrl+C (detaches without stopping the session) or session exit.
int runSessionWatch(const SessionInfo& session);

// Sends one JSON command to a live session daemon over QLocalSocket.
bool sendSessionRequest(const QString& socketName, const QJsonObject& request,
                        QJsonObject* response, QString* error, int timeoutMs = 60000);

// Lists live sessions in sessionRegistryDir(), automatically pruning dead ones.
QList<SessionInfo> listActiveSessions();

// Resolves an explicit --session name or the default active session.
bool resolveActiveSession(const QString& requestedId, SessionInfo* out, QString* error);
#endif

} // namespace qmltui::runner_cli
