#include "TerminalDevice.h"
#include <cerrno>
#include <iostream>
#include <algorithm>
#include <limits>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QObject>
#include <QSocketNotifier>
#include <QScopeGuard>

#ifndef _WIN32
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace qtui {

// Owns the write notifier so TerminalDevice itself can stay a plain class.
class TerminalDeviceNotifier : public QObject {
public:
    TerminalDeviceNotifier(TerminalDevice* device, int fd)
        : m_device(device), m_notifier(fd, QSocketNotifier::Write, this)
    {
        m_notifier.setEnabled(false);
        QObject::connect(&m_notifier, &QSocketNotifier::activated, this,
                         [this] { m_device->drain(); });
    }

    void setEnabled(bool enabled) { m_notifier.setEnabled(enabled); }

private:
    TerminalDevice* m_device;
    QSocketNotifier m_notifier;
};

TerminalDevice::TerminalDevice(int outputFd)
#ifdef _WIN32
    : m_outputFd(outputFd < 0 ? 1 : outputFd)
#else
    : m_outputFd(outputFd < 0 ? STDOUT_FILENO : outputFd)
#endif
{
#ifdef _WIN32
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hInput, &m_originalConsoleMode);
    }
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hOutput, &m_originalOutputMode);
    }
#else
    if (tcgetattr(STDIN_FILENO, &m_originalTermios) == 0) {
        m_termiosValid = true;
    }
#endif
}

TerminalDevice::~TerminalDevice() {
    // Destruction must not log to stderr: it can share the stalled terminal.
    // Callers that need a delivery result use finishOutput() before destruction.
    (void)finishOutput();
    disableRawMode();
}

bool TerminalDevice::enableRawMode() {
    if (m_isRawMode) return true;

#ifdef _WIN32
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    bool inputConfigured = false;
    if (hInput != INVALID_HANDLE_VALUE) {
        DWORD mode = m_originalConsoleMode;
        // Disable echo, canonical mode, and signals
        mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        inputConfigured = SetConsoleMode(hInput, mode);
    }

    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    bool outputConfigured = false;
    if (hOutput != INVALID_HANDLE_VALUE) {
        DWORD mode = m_originalOutputMode;
        // Enable VT100 / ANSI escape processing on Windows console
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
        outputConfigured = SetConsoleMode(hOutput, mode);
    }
    m_isRawMode = inputConfigured && outputConfigured;
    return m_isRawMode;
#else
    if (!m_termiosValid) return false;

    termios raw = m_originalTermios;
    // Disable echo, canonical mode, extended input processing, and signals
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // Disable software flow control and bit-stripping
    raw.c_iflag &= ~(IXON | ICRNL | ISTRIP | INLCR | IGNCR);
    // Ensure 8-bit character size
    raw.c_cflag |= (CS8);
    // Disable output processing
    raw.c_oflag &= ~(OPOST);

    m_isRawMode = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
    if (m_isRawMode) {
        write("\033%G"); // Switch terminal device parser into UTF-8 mode
    }
    return m_isRawMode;
#endif
}

bool TerminalDevice::disableRawMode() {
    if (!m_isRawMode) return true;

#ifdef _WIN32
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    const bool inputRestored = hInput != INVALID_HANDLE_VALUE
        && SetConsoleMode(hInput, m_originalConsoleMode);
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    const bool outputRestored = hOutput != INVALID_HANDLE_VALUE
        && SetConsoleMode(hOutput, m_originalOutputMode);
    m_isRawMode = !(inputRestored && outputRestored);
    return !m_isRawMode;
#else
    if (!m_termiosValid) return false;
    // Queued restore sequences must reach the terminal before its mode changes.
    flushBlocking();
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_originalTermios) != 0)
        return false;
    m_isRawMode = false;
    return true;
#endif
}

void TerminalDevice::hideCursor() {
    write("\033[?25l");
}

void TerminalDevice::showCursor() {
    write("\033[?25h");
}

void TerminalDevice::clearScreen() {
    write("\033[2J\033[H");
}

TerminalDevice::WriteStatus TerminalDevice::writeSome(const char* data, size_t size, size_t& offset) {
#ifdef _WIN32
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput == INVALID_HANDLE_VALUE) return WriteStatus::Error;
    while (offset < size) {
        const size_t remaining = size - offset;
        const DWORD chunkSize = static_cast<DWORD>((std::min)(remaining,
            static_cast<size_t>((std::numeric_limits<DWORD>::max)())));
        DWORD written = 0;
        if (!WriteFile(hOutput, data + offset, chunkSize, &written, nullptr) || written == 0)
            return WriteStatus::Error;
        offset += written;
    }
    return WriteStatus::Done;
#else
    while (offset < size) {
        const ssize_t bytesWritten = ::write(m_outputFd, data + offset, size - offset);
        if (bytesWritten < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return WriteStatus::WouldBlock;
            return WriteStatus::Error;
        }
        if (bytesWritten == 0) return WriteStatus::Error;
        offset += static_cast<size_t>(bytesWritten);
    }
    return WriteStatus::Done;
#endif
}

bool TerminalDevice::write(const std::string& data) {
    if (m_outputFinished)
        return false;
    if (data.empty())
        return true;
    m_hasWrittenOutput = true;

    // Preserve ordering: once anything is queued, everything queues.
    if (!m_pending.empty()) {
        m_pending += data;
        setNotifierEnabled(true);
        return true;
    }

    size_t offset = 0;
    const WriteStatus status = writeSome(data.data(), data.size(), offset);
    if (status == WriteStatus::Error)
        return false;
    if (offset < data.size()) {
        m_pending.append(data, offset, std::string::npos);
        setNotifierEnabled(true);
    }
    return true;
}

void TerminalDevice::drain() {
    if (m_pending.empty()) {
        setNotifierEnabled(false);
        return;
    }
    size_t offset = 0;
    const WriteStatus status = writeSome(m_pending.data(), m_pending.size(), offset);
    m_pending.erase(0, offset);
    if (status == WriteStatus::Error) {
        m_pending.clear();
        setNotifierEnabled(false);
        return;
    }
    if (m_pending.empty()) {
        setNotifierEnabled(false);
        if (m_drainedCallback)
            m_drainedCallback();
    }
}

bool TerminalDevice::flushBlocking(int timeoutMs) {
#ifndef _WIN32
    if (m_pending.empty())
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
    while (!m_pending.empty()) {
        const ssize_t written = ::write(m_outputFd, m_pending.data(),
                                        std::min(m_pending.size(), size_t(65536)));
        if (written > 0)
            m_pending.erase(0, static_cast<size_t>(written));
        else if (written == 0 || (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK))
            return false;
        if (!m_pending.empty()) {
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
#else
    size_t offset = 0;
    const WriteStatus status = writeSome(m_pending.data(), m_pending.size(), offset);
    m_pending.erase(0, offset);
    if (status != WriteStatus::Done)
        return false;
#endif
    setNotifierEnabled(false);
    return true;
}

void TerminalDevice::setDrainedCallback(std::function<void()> callback) {
    m_drainedCallback = std::move(callback);
}

void TerminalDevice::setNotifierEnabled(bool enabled) {
    if (m_notifier)
        m_notifier->setEnabled(enabled);
}

bool TerminalDevice::setAsyncOutput(bool enabled, int timeoutMs) {
#ifdef _WIN32
    Q_UNUSED(timeoutMs);
    if (enabled)
        m_outputFinished = false;
    return !enabled;
#else
    if (m_async == enabled)
        return true;
    if (enabled) {
        if (!QCoreApplication::instance())
            return false;   // nothing to drain the queue from
        const int flags = ::fcntl(m_outputFd, F_GETFL);
        if (flags == -1)
            return false;
        if (::fcntl(m_outputFd, F_SETFL, flags | O_NONBLOCK) == -1)
            return false;
        m_originalFlags = flags;
        m_notifier = std::make_unique<TerminalDeviceNotifier>(this, m_outputFd);
        m_async = true;
        m_outputFinished = false;
        if (!m_pending.empty())
            setNotifierEnabled(true);
    } else {
        if (!flushBlocking(timeoutMs))
            return false;
        if (m_originalFlags != -1 && ::fcntl(m_outputFd, F_SETFL, m_originalFlags) == -1)
            return false;
        m_notifier.reset();
        m_originalFlags = -1;
        m_async = false;
    }
    return true;
#endif
}

bool TerminalDevice::finishOutput(int timeoutMs) {
    if (m_outputFinished)
        return m_finishSucceeded;
    // Append directly: a blocking descriptor may already be full. flushBlocking
    // owns the only writes here and applies the deadline to all of them.
    if (m_hasWrittenOutput)
        m_pending += "\033[?2026l\033[?25h\033[0m";
    m_finishSucceeded = flushBlocking(timeoutMs);
    m_notifier.reset();
#ifndef _WIN32
    if (m_originalFlags != -1 && ::fcntl(m_outputFd, F_SETFL, m_originalFlags) == -1)
        m_finishSucceeded = false;
    m_originalFlags = -1;
#endif
    m_pending.clear();
    m_async = false;
    m_hasWrittenOutput = false;
    m_outputFinished = true;
    return m_finishSucceeded;
}

int TerminalDevice::getWidth() const {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOutput, &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
#else
    struct winsize ws;
    if (ioctl(m_outputFd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return 80; // Fallback
    }
    return ws.ws_col;
#endif
}

int TerminalDevice::getHeight() const {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOutput, &csbi)) {
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    return 24;
#else
    struct winsize ws;
    if (ioctl(m_outputFd, TIOCGWINSZ, &ws) == -1 || ws.ws_row == 0) {
        return 24; // Fallback
    }
    return ws.ws_row;
#endif
}

} // namespace qtui
