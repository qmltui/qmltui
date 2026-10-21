#pragma once

// Internal: POSIX pseudo-terminal (`openpty`) and child QProcess session manager.

#include <QByteArray>
#include <QObject>
#include <QProcess>

class QSocketNotifier;

namespace qmltui {

// Manages a POSIX pseudo-terminal (`openpty`) and child `QProcess` session,
// bridging non-blocking master-fd reads/writes to Qt signals and forwarding
// window size (`TIOCSWINSZ`) and process-group signals.
class UnixPtySession final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(UnixPtySession)

public:
    explicit UnixPtySession(QObject* parent = nullptr);
    ~UnixPtySession() override;

    // Allocates a PTY pair sized to `rows` x `columns`, attaches the slave as
    // the child process's controlling terminal in a new session (`setsid`), and
    // starts `program`.
    bool start(const QString& program, const QStringList& arguments,
               const QString& workingDirectory, const QProcessEnvironment& environment, int rows,
               int columns);
    // Sends SIGTERM to the child's process group (and `m_process.terminate()`).
    void terminate();
    // Sends SIGKILL to the child's process group (and `m_process.kill()`).
    void kill();
    // Queues `bytes` for non-blocking write to the PTY master fd.
    bool write(const QByteArray& bytes);
    // Updates the PTY window size (`TIOCSWINSZ`), delivering SIGWINCH to the foreground group.
    void resize(int rows, int columns);

    bool running() const;
    qint64 processId() const;

signals:
    void started();
    void dataReceived(const QByteArray& bytes);
    void finished(int exitCode);
    void errorOccurred(const QString& message);

private:
    void readAvailable();
    void writeAvailable();
    void closeMaster();
    void signalProcessGroup(int signal);

    QProcess m_process;
    int m_masterFd = -1;
    QSocketNotifier* m_readNotifier = nullptr;
    QSocketNotifier* m_writeNotifier = nullptr;
    QByteArray m_writeQueue;
};

} // namespace qmltui
