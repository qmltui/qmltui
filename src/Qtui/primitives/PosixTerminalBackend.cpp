#include "PosixTerminalBackend.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPointer>
#include <QScopeGuard>
#include <QSet>
#include <QSocketNotifier>
#include <QThread>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace qtui {

namespace {

// SIGWINCH cannot be delivered per object, so one process-wide bridge turns
// the signal into a Qt signal through a self-pipe. It is installed while at
// least one backend is listening for input and removed with the last one.
class SigwinchBridge : public QObject {
    Q_OBJECT
public:
    static SigwinchBridge* instance()
    {
        if (!qApp || QThread::currentThread() != qApp->thread())
            return nullptr;
        static QPointer<SigwinchBridge> bridge;
        if (!bridge)
            bridge = new SigwinchBridge(qApp);
        return bridge.data();
    }

    void addListener(QObject* listener)
    {
        if (m_listeners.contains(listener))
            return;
        m_listeners.insert(listener);
        if (m_listeners.size() == 1)
            install();
    }

    void removeListener(QObject* listener)
    {
        m_listeners.remove(listener);
        if (m_listeners.isEmpty())
            uninstall();
    }

signals:
    void received();

private:
    explicit SigwinchBridge(QObject* parent) : QObject(parent) {}
    ~SigwinchBridge() override { uninstall(); }

    void install()
    {
        if (m_installed)
            return;
        if (::pipe(s_pipe) != 0) {
            s_pipe[0] = s_pipe[1] = -1;
            return;
        }
        for (int fd : s_pipe) {
            int flags = ::fcntl(fd, F_GETFD);
            if (flags != -1)
                ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
            flags = ::fcntl(fd, F_GETFL);
            if (flags != -1)
                ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }
        struct sigaction action {};
        action.sa_handler = &SigwinchBridge::handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        if (::sigaction(SIGWINCH, &action, &m_previousAction) != 0) {
            closePipe();
            return;
        }
        m_installed = true;
        m_notifier = new QSocketNotifier(s_pipe[0], QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated, this, [this] {
            char buffer[128];
            while (::read(s_pipe[0], buffer, sizeof buffer) > 0) {
                // Drain every pending wake-up before notifying once.
            }
            emit received();
        });
    }

    void uninstall()
    {
        if (m_notifier) {
            m_notifier->setEnabled(false);
            delete m_notifier;
            m_notifier = nullptr;
        }
        if (m_installed) {
            ::sigaction(SIGWINCH, &m_previousAction, nullptr);
            m_installed = false;
        }
        closePipe();
    }

    void closePipe()
    {
        for (int& fd : s_pipe) {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }
    }

    static void handler(int)
    {
        const int savedErrno = errno;
        if (s_pipe[1] != -1) {
            const char wake = 1;
            const ssize_t result = ::write(s_pipe[1], &wake, 1);
            (void)result;
        }
        errno = savedErrno;
    }

    static int s_pipe[2];
    QSet<QObject*> m_listeners;
    QSocketNotifier* m_notifier = nullptr;
    struct sigaction m_previousAction {};
    bool m_installed = false;
};

int SigwinchBridge::s_pipe[2] = {-1, -1};

} // namespace

PosixTerminalBackend::PosixTerminalBackend(int outputFd, QObject* parent)
    : TerminalBackend(parent)
    , m_outputFd(outputFd < 0 ? STDOUT_FILENO : outputFd)
    , m_inputFd(STDIN_FILENO)
{
    m_termiosValid = ::tcgetattr(m_inputFd, &m_originalTermios) == 0;
}

PosixTerminalBackend::~PosixTerminalBackend()
{
    stopInput();
    setNonBlockingOutput(false);
}

bool PosixTerminalBackend::enterRawMode()
{
    if (!m_termiosValid)
        return false;
    termios raw = m_originalTermios;
    // Disable echo, canonical mode, extended input processing, and signals.
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // Disable software flow control and bit-stripping.
    raw.c_iflag &= ~(IXON | ICRNL | ISTRIP | INLCR | IGNCR);
    // Ensure 8-bit character size.
    raw.c_cflag |= CS8;
    // Disable output processing.
    raw.c_oflag &= ~OPOST;
    return ::tcsetattr(m_inputFd, TCSAFLUSH, &raw) == 0;
}

bool PosixTerminalBackend::leaveRawMode()
{
    if (!m_termiosValid)
        return false;
    return ::tcsetattr(m_inputFd, TCSAFLUSH, &m_originalTermios) == 0;
}

TerminalBackend::WriteStatus PosixTerminalBackend::writeSome(const char* data, size_t size, size_t& offset)
{
    while (offset < size) {
        const ssize_t written = ::write(m_outputFd, data + offset, size - offset);
        if (written < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return WriteStatus::WouldBlock;
            return WriteStatus::Error;
        }
        if (written == 0)
            return WriteStatus::Error;
        offset += static_cast<size_t>(written);
    }
    return WriteStatus::Done;
}

bool PosixTerminalBackend::writeAll(std::string& pending, int timeoutMs)
{
    if (pending.empty())
        return true;
    if (timeoutMs < 0)
        return false;

    // Even a caller-owned descriptor may have been switched back to blocking
    // mode. Keep every write non-blocking so the deadline also bounds write().
    const int flags = ::fcntl(m_outputFd, F_GETFL);
    if (flags == -1 || ::fcntl(m_outputFd, F_SETFL, flags | O_NONBLOCK) == -1)
        return false;
    const auto restoreFlags = qScopeGuard([&] { ::fcntl(m_outputFd, F_SETFL, flags); });

    QElapsedTimer timer;
    timer.start();
    while (!pending.empty()) {
        const ssize_t written = ::write(m_outputFd, pending.data(),
                                        std::min(pending.size(), size_t(65536)));
        if (written > 0)
            pending.erase(0, static_cast<size_t>(written));
        else if (written == 0 || (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK))
            return false;
        if (!pending.empty()) {
            const qint64 remaining = timeoutMs - timer.elapsed();
            if (remaining <= 0)
                return false;
            pollfd descriptor{m_outputFd, POLLOUT, 0};
            const int ready = ::poll(&descriptor, 1, static_cast<int>(remaining));
            if (ready < 0 && errno == EINTR)
                continue;
            if (ready <= 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)))
                return false;
        }
    }
    return true;
}

bool PosixTerminalBackend::setNonBlockingOutput(bool enabled)
{
    if (enabled) {
        if (m_originalFlags != -1)
            return true;
        const int flags = ::fcntl(m_outputFd, F_GETFL);
        if (flags == -1)
            return false;
        if (::fcntl(m_outputFd, F_SETFL, flags | O_NONBLOCK) == -1)
            return false;
        m_originalFlags = flags;
        return true;
    }
    if (m_originalFlags == -1)
        return true;
    const bool restored = ::fcntl(m_outputFd, F_SETFL, m_originalFlags) != -1;
    m_originalFlags = -1;
    return restored;
}

void PosixTerminalBackend::setWritableNotifications(bool enabled)
{
    if (enabled && !m_writeNotifier) {
        if (!QCoreApplication::instance())
            return;   // nothing to drain the queue from
        m_writeNotifier = std::make_unique<QSocketNotifier>(m_outputFd, QSocketNotifier::Write);
        connect(m_writeNotifier.get(), &QSocketNotifier::activated, this, [this] { emit writable(); });
    }
    if (m_writeNotifier)
        m_writeNotifier->setEnabled(enabled);
}

QSize PosixTerminalBackend::size() const
{
    winsize ws{};
    if (::ioctl(m_outputFd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 || ws.ws_row == 0)
        return QSize();
    return QSize(ws.ws_col, ws.ws_row);
}

bool PosixTerminalBackend::isInteractive() const
{
    return ::isatty(m_outputFd) == 1;
}

void PosixTerminalBackend::startInput()
{
    if (m_inputStarted)
        return;
    m_inputStarted = true;
    if (QCoreApplication::instance()) {
        m_readNotifier = std::make_unique<QSocketNotifier>(m_inputFd, QSocketNotifier::Read);
        connect(m_readNotifier.get(), &QSocketNotifier::activated, this, [this] { readInput(); });
    }
    if (SigwinchBridge* bridge = SigwinchBridge::instance()) {
        connect(bridge, &SigwinchBridge::received, this, &TerminalBackend::resized);
        bridge->addListener(this);
    } else {
        qWarning("TerminalBackend must start input on the QCoreApplication thread.");
    }
}

void PosixTerminalBackend::stopInput()
{
    if (!m_inputStarted)
        return;
    m_inputStarted = false;
    m_readNotifier.reset();
    if (SigwinchBridge* bridge = SigwinchBridge::instance()) {
        disconnect(bridge, nullptr, this, nullptr);
        bridge->removeListener(this);
    }
}

void PosixTerminalBackend::readInput()
{
    char buffer[1024];
    const ssize_t count = ::read(m_inputFd, buffer, sizeof buffer);
    if (count == 0) {
        if (m_readNotifier)
            m_readNotifier->setEnabled(false);
        return;
    }
    if (count < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        if (m_readNotifier)
            m_readNotifier->setEnabled(false);
        qWarning("Terminal input failed: %s", std::strerror(errno));
        return;
    }
    emit inputReceived(QByteArray(buffer, static_cast<qsizetype>(count)));
}

} // namespace qtui

#include "PosixTerminalBackend.moc"
