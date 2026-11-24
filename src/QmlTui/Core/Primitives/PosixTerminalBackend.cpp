#include <QmlTui/Core/Primitives/PosixTerminalBackend.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QList>
#include <QPointer>
#include <QScopeGuard>
#include <QSet>
#include <QSocketNotifier>
#include <QThread>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace qmltui {

namespace {

// Signals cannot be delivered per object, so one process-wide bridge turns
// them into Qt signals through a self-pipe. It is installed while at least one
// backend is listening for input and removed with the last one. SIGWINCH is
// handled here; SIGCONT is written into the same pipe by the job-control
// handler below so the main thread learns that the process resumed.
class SignalBridge : public QObject {
    Q_OBJECT
public:
    enum Code : char { WindowChanged = 1, Continued = 2 };

    static SignalBridge* instance() {
        if (!qApp || QThread::currentThread() != qApp->thread()) return nullptr;
        static QPointer<SignalBridge> bridge;
        if (!bridge) bridge = new SignalBridge(qApp);
        return bridge.data();
    }

    // Async-signal-safe: one write(2) on the self-pipe.
    static void notify(Code code) {
        const int fd = s_pipe[1].load(std::memory_order_relaxed);
        if (fd != -1) {
            const char wake = code;
            const ssize_t result = ::write(fd, &wake, 1);
            (void)result;
        }
    }

    void addListener(QObject* listener) {
        if (m_listeners.contains(listener)) return;
        m_listeners.insert(listener);
        if (m_listeners.size() == 1) install();
    }

    void removeListener(QObject* listener) {
        m_listeners.remove(listener);
        if (m_listeners.isEmpty()) uninstall();
    }

    void addInputListener(PosixTerminalBackend* listener) {
        if (!listener || m_inputListeners.contains(listener)) return;
        m_inputListeners.append(listener);
        if (!m_stdinNotifier) {
            m_stdinNotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
            connect(m_stdinNotifier, &QSocketNotifier::activated, this, &SignalBridge::readStdin);
        }
    }

    void removeInputListener(PosixTerminalBackend* listener) {
        m_inputListeners.removeOne(listener);
        if (m_inputListeners.isEmpty() && m_stdinNotifier) {
            m_stdinNotifier->setEnabled(false);
            delete m_stdinNotifier;
            m_stdinNotifier = nullptr;
        }
    }

signals:
    void received();  // SIGWINCH
    void continued(); // SIGCONT after a job-control stop

private:
    explicit SignalBridge(QObject* parent) : QObject(parent) {}
    ~SignalBridge() override {
        if (m_stdinNotifier) {
            m_stdinNotifier->setEnabled(false);
            delete m_stdinNotifier;
            m_stdinNotifier = nullptr;
        }
        uninstall();
    }

    void readStdin() {
        char buffer[1024];
        const ssize_t count = ::read(STDIN_FILENO, buffer, sizeof buffer);
        if (count == 0) {
            if (m_stdinNotifier) m_stdinNotifier->setEnabled(false);
            return;
        }
        if (count < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) return;
            if (m_stdinNotifier) m_stdinNotifier->setEnabled(false);
            qWarning("Terminal input failed: %s", std::strerror(errno));
            return;
        }
        if (!m_inputListeners.isEmpty()) {
            emit m_inputListeners.last()->inputReceived(
                QByteArray(buffer, static_cast<qsizetype>(count)));
        }
    }

    void install() {
        if (m_installed) return;
        int fds[2] = {-1, -1};
        if (::pipe(fds) != 0) {
            s_pipe[0].store(-1, std::memory_order_relaxed);
            s_pipe[1].store(-1, std::memory_order_relaxed);
            return;
        }
        for (int fd : fds) {
            int flags = ::fcntl(fd, F_GETFD);
            if (flags != -1) ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
            flags = ::fcntl(fd, F_GETFL);
            if (flags != -1) ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }
        s_pipe[0].store(fds[0], std::memory_order_release);
        s_pipe[1].store(fds[1], std::memory_order_release);
        struct sigaction action{};
        action.sa_handler = &SignalBridge::handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        if (::sigaction(SIGWINCH, &action, &m_previousAction) != 0) {
            closePipe();
            return;
        }
        m_installed = true;
        m_notifier = new QSocketNotifier(s_pipe[0].load(std::memory_order_relaxed),
                                         QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated, this, [this] {
            char buffer[128];
            bool windowChanged = false;
            bool resumed = false;
            ssize_t count = 0;
            const int readFd = s_pipe[0].load(std::memory_order_relaxed);
            while (readFd != -1 && (count = ::read(readFd, buffer, sizeof buffer)) > 0) {
                // Drain every pending wake-up before notifying once per kind.
                for (ssize_t i = 0; i < count; ++i) {
                    if (buffer[i] == Continued)
                        resumed = true;
                    else
                        windowChanged = true;
                }
            }
            if (resumed) emit continued();
            if (windowChanged) emit received();
        });
    }

    void uninstall() {
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

    void closePipe() {
        for (auto& fd : s_pipe) {
            const int val = fd.exchange(-1, std::memory_order_acq_rel);
            if (val != -1) {
                ::close(val);
            }
        }
    }

    static void handler(int) {
        const int savedErrno = errno;
        notify(WindowChanged);
        errno = savedErrno;
    }

    static std::atomic<int> s_pipe[2];
    QSet<QObject*> m_listeners;
    QList<PosixTerminalBackend*> m_inputListeners;
    QSocketNotifier* m_notifier = nullptr;
    QSocketNotifier* m_stdinNotifier = nullptr;
    struct sigaction m_previousAction{};
    bool m_installed = false;
};

std::atomic<int> SignalBridge::s_pipe[2] = {{-1}, {-1}};

// While a backend is in raw mode the terminal belongs to the application:
// alternate screen, hidden cursor, mouse and paste reporting, no echo. A
// crash, a kill, or Ctrl+Z would otherwise hand the user's shell a terminal
// in that state. These handlers undo it with async-signal-safe calls only
// (write, tcsetattr, sigaction, raise) and then let the signal take its
// default course, so a crash still dumps core and exits with the right status
// and a stop still stops. On SIGCONT raw mode is re-entered in the handler and
// the main thread is told through the bridge so the engine can re-enable its
// modes and repaint.
struct EmergencyState {
    volatile sig_atomic_t active = 0;
    // A kitty keyboard mode is pushed on the current screen and must be
    // popped before the reset sequence leaves it; stacks are per screen.
    volatile sig_atomic_t keyboardProtocol = 0;
    int outputFd = -1;
    int inputFd = -1;
    const PosixTerminalBackend* owner = nullptr;
    termios original{};
    termios raw{};
};
EmergencyState g_emergency;

// Everything TerminalEngine enables, undone in one write. Order: reporting
// modes off, cursor shown, attributes reset, background restored, normal
// screen. Unknown sequences are ignored by terminals that lack a mode.
constexpr char kResetSequence[] = "\033[?1000l\033[?1002l\033[?1003l\033[?1006l\033[?2004l"
                                  "\033[?25h\033[0m\033]111\a\033]111;\a\033]111;\033\\\033[?1049l";
constexpr char kKeyboardPopSequence[] = "\033[<u";

constexpr int kFatalSignals[] = {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT,
                                 SIGTERM, SIGHUP, SIGINT, SIGQUIT};
struct sigaction g_previousFatal[std::size(kFatalSignals)];
struct sigaction g_previousTstp;
struct sigaction g_previousCont;
bool g_handlersInstalled = false;
std::mutex g_emergencyMutex;

bool isOnApplicationThread() {
    const QCoreApplication* app = QCoreApplication::instance();
    return !app || QThread::currentThread() == app->thread();
}

void writeFromSignal(int fd, const char* data, size_t size) {
    // The descriptor may be non-blocking; retry a bounded number of times
    // rather than spin forever in a handler.
    for (int attempt = 0; size > 0 && attempt < 10000; ++attempt) {
        const ssize_t written = ::write(fd, data, size);
        if (written > 0) {
            data += written;
            size -= static_cast<size_t>(written);
            continue;
        }
        if (written < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        break;
    }
}

void restoreTerminalFromSignal() {
    if (!g_emergency.active) return;
    std::atomic_thread_fence(std::memory_order_acquire);
    if (g_emergency.keyboardProtocol) {
        // Pop our own entry only, and only once: after a stop the engine
        // pushes again on resume and re-arms this flag.
        g_emergency.keyboardProtocol = 0;
        writeFromSignal(g_emergency.outputFd, kKeyboardPopSequence,
                        sizeof kKeyboardPopSequence - 1);
    }
    writeFromSignal(g_emergency.outputFd, kResetSequence, sizeof kResetSequence - 1);
    ::tcsetattr(g_emergency.inputFd, TCSANOW, &g_emergency.original);
}

void installTstpHandler();

void fatalHandler(int signo) {
    restoreTerminalFromSignal();
    bool restoredCustom = false;
    for (size_t i = 0; i < std::size(kFatalSignals); ++i) {
        if (kFatalSignals[i] == signo) {
            const struct sigaction& prev = g_previousFatal[i];
            if ((prev.sa_flags & SA_SIGINFO) != 0 ||
                (prev.sa_handler != SIG_DFL && prev.sa_handler != SIG_IGN)) {
                ::sigaction(signo, &prev, nullptr);
                restoredCustom = true;
            }
            break;
        }
    }
    if (!restoredCustom) ::signal(signo, SIG_DFL);
    ::raise(signo); // pending until this handler returns, then the default or previous action
}

void tstpHandler(int) {
    const int savedErrno = errno;
    restoreTerminalFromSignal();
    ::signal(SIGTSTP, SIG_DFL);
    ::raise(SIGTSTP); // the shell sees an ordinary stop with a sane terminal
    errno = savedErrno;
}

void contHandler(int) {
    const int savedErrno = errno;
    if (g_emergency.active) {
        // The shell restored the terminal modes it saved when the job stopped,
        // which are the cooked ones written above. Take raw mode back before
        // the first keystroke, then let the engine re-enable its modes.
        ::tcsetattr(g_emergency.inputFd, TCSANOW, &g_emergency.raw);
        installTstpHandler();
        SignalBridge::notify(SignalBridge::Continued);
    }
    errno = savedErrno;
}

void installTstpHandler() {
    struct sigaction action{};
    action.sa_handler = &tstpHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    ::sigaction(SIGTSTP, &action, nullptr);
}

void installEmergencyHandlers(const PosixTerminalBackend* owner, int inputFd, int outputFd,
                              const termios& original, const termios& raw) {
    const std::lock_guard<std::mutex> lock(g_emergencyMutex);
    g_emergency.active = 0;
    std::atomic_signal_fence(std::memory_order_seq_cst);
    g_emergency.owner = owner;
    g_emergency.inputFd = inputFd;
    g_emergency.outputFd = outputFd;
    g_emergency.original = original;
    g_emergency.raw = raw;
    std::atomic_thread_fence(std::memory_order_release);
    g_emergency.active = 1;
    if (g_handlersInstalled) return; // a second backend in raw mode takes over the state only
    struct sigaction action{};
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    action.sa_handler = &fatalHandler;
    for (size_t i = 0; i < std::size(kFatalSignals); ++i)
        ::sigaction(kFatalSignals[i], &action, &g_previousFatal[i]);
    action.sa_handler = &tstpHandler;
    ::sigaction(SIGTSTP, &action, &g_previousTstp);
    action.sa_handler = &contHandler;
    ::sigaction(SIGCONT, &action, &g_previousCont);
    g_handlersInstalled = true;
}

void uninstallEmergencyHandlers(const PosixTerminalBackend* owner) {
    const std::lock_guard<std::mutex> lock(g_emergencyMutex);
    if (g_emergency.owner != nullptr && g_emergency.owner != owner) return;
    g_emergency.active = 0;
    std::atomic_signal_fence(std::memory_order_seq_cst);
    g_emergency.keyboardProtocol = 0;
    g_emergency.owner = nullptr;
    std::atomic_thread_fence(std::memory_order_release);
    if (!g_handlersInstalled) return;
    for (size_t i = 0; i < std::size(kFatalSignals); ++i)
        ::sigaction(kFatalSignals[i], &g_previousFatal[i], nullptr);
    ::sigaction(SIGTSTP, &g_previousTstp, nullptr);
    ::sigaction(SIGCONT, &g_previousCont, nullptr);
    g_handlersInstalled = false;
}

} // namespace

PosixTerminalBackend::PosixTerminalBackend(int outputFd, QObject* parent)
    : TerminalBackend(parent), m_outputFd(outputFd < 0 ? STDOUT_FILENO : outputFd),
      m_inputFd(STDIN_FILENO) {
    m_termiosValid = ::tcgetattr(m_inputFd, &m_originalTermios) == 0;
}

PosixTerminalBackend::~PosixTerminalBackend() {
    stopInput();
    if (m_rawMode) leaveRawMode();
    setNonBlockingOutput(false);
}

bool PosixTerminalBackend::enterRawMode() {
    if (m_rawMode) return true;
    if (!m_termiosValid || !isOnApplicationThread()) return false;
    termios raw = m_originalTermios;
    // Disable echo, canonical mode, extended input processing, and signals.
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // Disable software flow control and bit-stripping.
    raw.c_iflag &= ~(IXON | ICRNL | ISTRIP | INLCR | IGNCR);
    // Ensure 8-bit character size.
    raw.c_cflag |= CS8;
    // Disable output processing.
    raw.c_oflag &= ~OPOST;
    if (::tcsetattr(m_inputFd, TCSAFLUSH, &raw) != 0) return false;
    m_rawMode = true;
    installEmergencyHandlers(this, m_inputFd, m_outputFd, m_originalTermios, raw);
    return true;
}

void PosixTerminalBackend::setKeyboardProtocolActive(bool active) {
    const std::lock_guard<std::mutex> lock(g_emergencyMutex);
    if (g_emergency.active && g_emergency.owner == this)
        g_emergency.keyboardProtocol = active ? 1 : 0;
}

bool PosixTerminalBackend::leaveRawMode() {
    if (!m_rawMode) return true;
    if (!m_termiosValid) return false;
    m_rawMode = false;
    uninstallEmergencyHandlers(this);
    // TerminalDevice::disableRawMode() already ran a deadline-bounded flush;
    // TCSAFLUSH would block indefinitely in ttywait() on BSD/macOS if the
    // terminal or PTY master has stopped draining output.
    return ::tcsetattr(m_inputFd, TCSANOW, &m_originalTermios) == 0;
}

TerminalBackend::WriteStatus PosixTerminalBackend::writeSome(const char* data, size_t size,
                                                             size_t& offset) {
    while (offset < size) {
        const ssize_t written = ::write(m_outputFd, data + offset, size - offset);
        if (written < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return WriteStatus::WouldBlock;
            return WriteStatus::Error;
        }
        if (written == 0) return WriteStatus::Error;
        offset += static_cast<size_t>(written);
    }
    return WriteStatus::Done;
}

bool PosixTerminalBackend::writeAll(std::string& pending, int timeoutMs) {
    if (pending.empty()) return true;

    // Even a caller-owned descriptor may have been switched back to blocking
    // mode. Keep every write non-blocking so the deadline also bounds write().
    const int flags = ::fcntl(m_outputFd, F_GETFL);
    if (flags == -1 || ::fcntl(m_outputFd, F_SETFL, flags | O_NONBLOCK) == -1) return false;
    const auto restoreFlags = qScopeGuard([&] { ::fcntl(m_outputFd, F_SETFL, flags); });

    QElapsedTimer timer;
    timer.start();
    while (!pending.empty()) {
        const ssize_t written =
            ::write(m_outputFd, pending.data(), (std::min)(pending.size(), size_t(65536)));
        if (written > 0)
            pending.erase(0, static_cast<size_t>(written));
        else if (written == 0 || (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK))
            return false;
        if (!pending.empty()) {
            int pollTimeout = -1;
            if (timeoutMs >= 0) {
                const qint64 remaining = timeoutMs - timer.elapsed();
                if (remaining <= 0) return false;
                pollTimeout = static_cast<int>(remaining);
            }
            pollfd descriptor{m_outputFd, POLLOUT, 0};
            const int ready = ::poll(&descriptor, 1, pollTimeout);
            if (ready < 0 && errno == EINTR) continue;
            if (ready <= 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))) return false;
        }
    }
    return true;
}

bool PosixTerminalBackend::setNonBlockingOutput(bool enabled) {
    if (enabled) {
        if (m_originalFlags != -1) return true;
        const int flags = ::fcntl(m_outputFd, F_GETFL);
        if (flags == -1) return false;
        if (::fcntl(m_outputFd, F_SETFL, flags | O_NONBLOCK) == -1) return false;
        m_originalFlags = flags;
        return true;
    }
    if (m_originalFlags == -1) return true;
    const bool restored = ::fcntl(m_outputFd, F_SETFL, m_originalFlags) != -1;
    m_originalFlags = -1;
    return restored;
}

void PosixTerminalBackend::setWritableNotifications(bool enabled) {
    if (enabled && !m_writeNotifier) {
        if (!QCoreApplication::instance() || !isOnApplicationThread()) return;
        m_writeNotifier = std::make_unique<QSocketNotifier>(m_outputFd, QSocketNotifier::Write);
        connect(m_writeNotifier.get(), &QSocketNotifier::activated, this,
                [this] { emit writable(); });
    }
    if (m_writeNotifier) m_writeNotifier->setEnabled(enabled);
}

QSize PosixTerminalBackend::size() const {
    winsize ws{};
    if (::ioctl(m_outputFd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 || ws.ws_row == 0)
        return QSize();
    return QSize(ws.ws_col, ws.ws_row);
}

bool PosixTerminalBackend::isInteractive() const {
    return ::isatty(m_outputFd) == 1;
}

bool PosixTerminalBackend::isInputInteractive() const {
    return ::isatty(m_inputFd) == 1;
}

bool PosixTerminalBackend::isStderrInteractive() const {
    return ::isatty(STDERR_FILENO) == 1;
}

void PosixTerminalBackend::startInput() {
    if (m_inputStarted) return;
    m_inputStarted = true;
    if (!QCoreApplication::instance() || !isOnApplicationThread()) return;
    if (SignalBridge* bridge = SignalBridge::instance()) {
        connect(bridge, &SignalBridge::received, this, &TerminalBackend::resized);
        connect(bridge, &SignalBridge::continued, this, &TerminalBackend::resumed);
        bridge->addListener(this);
        bridge->addInputListener(this);
    }
}

void PosixTerminalBackend::stopInput() {
    if (!m_inputStarted) return;
    m_inputStarted = false;
    if (SignalBridge* bridge = SignalBridge::instance()) {
        disconnect(bridge, nullptr, this, nullptr);
        bridge->removeInputListener(this);
        bridge->removeListener(this);
    }
}

} // namespace qmltui

#include "PosixTerminalBackend.moc"
