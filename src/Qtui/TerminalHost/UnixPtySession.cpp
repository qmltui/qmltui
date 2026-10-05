#include "UnixPtySession.h"

#include <QSocketNotifier>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#ifdef Q_OS_MACOS
#include <util.h>
#else
#include <pty.h>
#endif

namespace qtui {

namespace {
constexpr qsizetype MaxWriteQueue = 1024 * 1024;
}

UnixPtySession::UnixPtySession(QObject* parent) : QObject(parent) {
    connect(&m_process, &QProcess::started, this, &UnixPtySession::started);
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
                readAvailable();
                closeMaster();
                emit finished(exitCode);
            });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            closeMaster();
        if (error != QProcess::UnknownError)
            emit errorOccurred(m_process.errorString());
    });
}

UnixPtySession::~UnixPtySession() {
    if (running())
        signalProcessGroup(SIGKILL);
    closeMaster();
    if (m_process.state() != QProcess::NotRunning)
        m_process.waitForFinished(250);
}

bool UnixPtySession::start(const QString& program, const QStringList& arguments,
                           const QString& workingDirectory,
                           const QProcessEnvironment& environment,
                           int rows, int columns) {
    if (running() || m_masterFd >= 0)
        return false;

    struct winsize size {};
    size.ws_row = static_cast<unsigned short>(qBound(1, rows, 65535));
    size.ws_col = static_cast<unsigned short>(qBound(1, columns, 65535));

    int slaveFd = -1;
    if (::openpty(&m_masterFd, &slaveFd, nullptr, nullptr, &size) < 0) {
        emit errorOccurred(QStringLiteral("openpty failed: %1")
                               .arg(QString::fromLocal8Bit(std::strerror(errno))));
        m_masterFd = -1;
        return false;
    }

    const int flags = ::fcntl(m_masterFd, F_GETFL, 0);
    if (flags >= 0)
        ::fcntl(m_masterFd, F_SETFL, flags | O_NONBLOCK);
    ::fcntl(m_masterFd, F_SETFD, FD_CLOEXEC);

    const int masterFd = m_masterFd;
    m_process.setChildProcessModifier([masterFd, slaveFd] {
        ::setsid();
        ::ioctl(slaveFd, TIOCSCTTY, 0);
        ::tcsetpgrp(slaveFd, ::getpid());
        ::dup2(slaveFd, STDIN_FILENO);
        ::dup2(slaveFd, STDOUT_FILENO);
        ::dup2(slaveFd, STDERR_FILENO);
        if (slaveFd > STDERR_FILENO)
            ::close(slaveFd);
        ::close(masterFd);
    });
    m_process.setProgram(program);
    m_process.setArguments(arguments);
    m_process.setProcessEnvironment(environment);
    if (!workingDirectory.isEmpty())
        m_process.setWorkingDirectory(workingDirectory);

    m_readNotifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
    m_writeNotifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Write, this);
    m_writeNotifier->setEnabled(false);
    connect(m_readNotifier, &QSocketNotifier::activated, this, &UnixPtySession::readAvailable);
    connect(m_writeNotifier, &QSocketNotifier::activated, this, &UnixPtySession::writeAvailable);

    m_process.start();
    ::close(slaveFd);
    return true;
}

void UnixPtySession::terminate() {
    signalProcessGroup(SIGTERM);
}

void UnixPtySession::kill() {
    signalProcessGroup(SIGKILL);
}

bool UnixPtySession::write(const QByteArray& bytes) {
    if (m_masterFd < 0 || bytes.isEmpty())
        return bytes.isEmpty();
    if (bytes.size() > MaxWriteQueue - m_writeQueue.size()) {
        emit errorOccurred(QStringLiteral("PTY write queue limit exceeded"));
        return false;
    }
    m_writeQueue += bytes;
    writeAvailable();
    return true;
}

void UnixPtySession::resize(int rows, int columns) {
    if (m_masterFd < 0)
        return;
    struct winsize size {};
    size.ws_row = static_cast<unsigned short>(qBound(1, rows, 65535));
    size.ws_col = static_cast<unsigned short>(qBound(1, columns, 65535));
    if (::ioctl(m_masterFd, TIOCSWINSZ, &size) < 0 && errno != EBADF)
        emit errorOccurred(QStringLiteral("PTY resize failed: %1")
                               .arg(QString::fromLocal8Bit(std::strerror(errno))));
}

bool UnixPtySession::running() const {
    return m_process.state() != QProcess::NotRunning;
}

qint64 UnixPtySession::processId() const {
    return m_process.processId();
}

void UnixPtySession::readAvailable() {
    if (m_masterFd < 0)
        return;
    char buffer[16384];
    for (;;) {
        const ssize_t count = ::read(m_masterFd, buffer, sizeof(buffer));
        if (count > 0) {
            emit dataReceived(QByteArray(buffer, static_cast<qsizetype>(count)));
            continue;
        }
        if (count == 0 || (count < 0 && errno == EIO)) {
            if (m_readNotifier)
                m_readNotifier->setEnabled(false);
            return;
        }
        if (errno == EINTR)
            continue;
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            emit errorOccurred(QStringLiteral("PTY read failed: %1")
                                   .arg(QString::fromLocal8Bit(std::strerror(errno))));
        return;
    }
}

void UnixPtySession::writeAvailable() {
    while (m_masterFd >= 0 && !m_writeQueue.isEmpty()) {
        const ssize_t count = ::write(m_masterFd, m_writeQueue.constData(),
                                      static_cast<size_t>(m_writeQueue.size()));
        if (count > 0) {
            m_writeQueue.remove(0, static_cast<qsizetype>(count));
            continue;
        }
        if (count < 0 && errno == EINTR)
            continue;
        if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            emit errorOccurred(QStringLiteral("PTY write failed: %1")
                                   .arg(QString::fromLocal8Bit(std::strerror(errno))));
        break;
    }
    if (m_writeNotifier)
        m_writeNotifier->setEnabled(!m_writeQueue.isEmpty());
}

void UnixPtySession::closeMaster() {
    delete m_readNotifier;
    m_readNotifier = nullptr;
    delete m_writeNotifier;
    m_writeNotifier = nullptr;
    if (m_masterFd >= 0) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }
    m_writeQueue.clear();
}

void UnixPtySession::signalProcessGroup(int signal) {
    const qint64 pid = m_process.processId();
    if (pid > 0)
        ::kill(-static_cast<pid_t>(pid), signal);
}

} // namespace qtui
