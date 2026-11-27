#pragma once

#include "ScenarioExporter.h"

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QVector>
#include <memory>

namespace qmltui::runner_cli {

// Supervises a single Runner-linked child process in interactive NDJSON pipe
// mode, separating framed Runner responses from app stdout/stderr logs,
// tracking exit/crash state, and recording successful steps for .js export.
class ChildSession final : public QObject {
    Q_OBJECT

public:
    struct LaunchConfig {
        QString program;
        QStringList arguments;
        QString workingDirectory;
        QString clock = QStringLiteral("real");
        int columns = 80;
        int rows = 24;
        int operationTimeoutMs = 2000;
        int startupTimeoutMs = 5000;
        QString artifactDir;
        QString recordPath;
        bool liveStream = false;
        QProcessEnvironment environment;
    };

    explicit ChildSession(QObject* parent = nullptr);
    ~ChildSession() override;

    // Launches the target application, verifies the activation handshake, and
    // waits for Runner's initial "ready" frame event.
    bool start(const LaunchConfig& config, QJsonObject* readyInfo, QString* error);

    // Executes one interactive command (either locally on the supervisor such
    // as "logs"/"export"/"info" or forwarded to the live child Runner).
    QJsonObject executeCommand(const QString& cmd, const QJsonObject& args = {},
                               const QJsonObject& include = {}, int timeoutMs = -1);

    // Cleanly asks the child to close, flushes any recording/artifacts, and
    // terminates the process if it does not exit promptly.
    QJsonObject stop();

    // Renders recorded successful mutating/assertion steps as a standalone
    // Runner JavaScript scenario file (function main(app)).
    QString exportScenarioJs() const;

    bool isRunning() const;
    qint64 pid() const { return m_pid; }
    QString artifactDirectory() const { return m_artifactDir; }
    QString clock() const { return m_clock; }
    QString mode() const { return m_mode; }
    QJsonObject viewport() const { return m_viewport; }
    QStringList allLogs() const { return m_allLogs; }
    QJsonObject lastLiveFrameEvent() const { return m_lastLiveFrameEvent; }

signals:
    void childExited(int exitCode, QProcess::ExitStatus exitStatus);
    void liveFrameReceived(const QJsonObject& frameEvent);

private:
    void drainStdout();
    void drainStderr();
    void processStdoutLine(const QByteArray& line);
    void appendLogLine(const QString& line);
    void recordStepIfExportable(const QString& cmd, const QJsonObject& args);
    bool waitForEventOrReply(int expectedId, int timeoutMs, QJsonObject* outMessage);

    QProcess m_process;
    std::unique_ptr<QTemporaryDir> m_controlDir;
    QByteArray m_ipcPrefix;
    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
    QVector<QJsonObject> m_incomingMessages;
    QStringList m_allLogs;
    QStringList m_pendingLogs;
    QVector<RecordedStep> m_history;

    QString m_artifactDir;
    QString m_clock = QStringLiteral("real");
    QString m_mode = QStringLiteral("AltScreen");
    QString m_lastFrame;
    QJsonObject m_lastLiveFrameEvent;
    QJsonObject m_viewport;
    QJsonObject m_lastFrameStats;
    int m_lastFrameCount = 0;
    int m_initialColumns = 80;
    int m_initialRows = 24;
    int m_operationTimeoutMs = 2000;
    int m_nextRequestId = 1;
    qint64 m_pid = 0;
    bool m_exited = false;
    int m_exitCode = 0;
    QProcess::ExitStatus m_exitStatus = QProcess::NormalExit;
    bool m_stopping = false;
};

// Runs qmltui-runner --interactive directly over stdin/stdout NDJSON without
// requiring a background session daemon or Qt6::Network.
int runInteractiveStdioMode(const ChildSession::LaunchConfig& config);

} // namespace qmltui::runner_cli
