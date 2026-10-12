#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>

#include <QByteArray>
#include <QObject>
#include <QProcess>

class QSocketNotifier;

namespace qamui {

class QAMUI_TERMINALHOST_EXPORT UnixPtySession final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(UnixPtySession)

public:
    explicit UnixPtySession(QObject* parent = nullptr);
    ~UnixPtySession() override;

    bool start(const QString& program, const QStringList& arguments,
               const QString& workingDirectory, const QProcessEnvironment& environment,
               int rows, int columns);
    void terminate();
    void kill();
    bool write(const QByteArray& bytes);
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

} // namespace qamui

