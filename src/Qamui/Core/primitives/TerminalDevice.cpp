#include <Qamui/Core/primitives/TerminalDevice.h>
#include <Qamui/Core/primitives/TerminalBackend.h>

#include <QCoreApplication>
#include <QObject>
#include <QSize>

namespace qamui {

TerminalDevice::TerminalDevice(int outputFd)
    : m_backend(createTerminalBackend(outputFd))
    , m_outputFd(outputFd < 0 ? 1 : outputFd)
{
    QObject::connect(m_backend.get(), &TerminalBackend::writable, m_backend.get(),
                     [this] { drain(); });
}

TerminalDevice::~TerminalDevice() {
    // Destruction must not log to stderr: it can share the stalled terminal.
    // Callers that need a delivery result use finishOutput() before destruction.
    (void)finishOutput();
    disableRawMode();
}

bool TerminalDevice::enableRawMode() {
    if (m_isRawMode) return true;
    m_isRawMode = m_backend->enterRawMode();
    if (m_isRawMode && m_backend->needsUtf8Designation())
        write("\033%G");   // switch the terminal's parser into UTF-8 mode
    return m_isRawMode;
}

bool TerminalDevice::disableRawMode() {
    if (!m_isRawMode) return true;
    // Queued restore sequences must reach the terminal before its mode changes.
    flushBlocking();
    if (!m_backend->leaveRawMode())
        return false;
    m_isRawMode = false;
    return true;
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
    const TerminalBackend::WriteStatus status = m_backend->writeSome(data.data(), data.size(), offset);
    if (status == TerminalBackend::WriteStatus::Error)
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
    const TerminalBackend::WriteStatus status = m_backend->writeSome(m_pending.data(), m_pending.size(), offset);
    m_pending.erase(0, offset);
    if (status == TerminalBackend::WriteStatus::Error) {
        m_pending.clear();
        setNotifierEnabled(false);
        if (m_outputErrorCallback) m_outputErrorCallback();
        return;
    }
    if (m_pending.empty()) {
        setNotifierEnabled(false);
        if (m_drainedCallback)
            m_drainedCallback();
    }
}

bool TerminalDevice::flushBlocking(int timeoutMs) {
    if (m_pending.empty())
        return true;
    if (!m_backend->writeAll(m_pending, timeoutMs))
        return false;
    setNotifierEnabled(false);
    return true;
}

void TerminalDevice::setOutputErrorCallback(std::function<void()> callback) {
    m_outputErrorCallback = std::move(callback);
}

void TerminalDevice::setDrainedCallback(std::function<void()> callback) {
    m_drainedCallback = std::move(callback);
}

void TerminalDevice::setNotifierEnabled(bool enabled) {
    // Notifications only exist for a non-blocking channel; disabling is always safe.
    if (m_async || !enabled)
        m_backend->setWritableNotifications(enabled);
}

bool TerminalDevice::setAsyncOutput(bool enabled, int timeoutMs) {
    if (!m_backend->supportsAsyncOutput()) {
        if (enabled)
            m_outputFinished = false;
        return !enabled;
    }
    if (m_async == enabled)
        return true;
    if (enabled) {
        if (!QCoreApplication::instance())
            return false;   // nothing to drain the queue from
        if (!m_backend->setNonBlockingOutput(true))
            return false;
        m_async = true;
        m_outputFinished = false;
        if (!m_pending.empty())
            setNotifierEnabled(true);
    } else {
        if (!flushBlocking(timeoutMs))
            return false;
        if (!m_backend->setNonBlockingOutput(false))
            return false;
        m_async = false;
    }
    return true;
}

bool TerminalDevice::finishOutput(int timeoutMs) {
    if (m_outputFinished)
        return m_finishSucceeded;
    // Append directly: a blocking channel may already be full. writeAll owns
    // the only writes here and applies the deadline to all of them.
    if (m_hasWrittenOutput)
        m_pending += "\033[?2026l\033[?25h\033[0m";
    m_finishSucceeded = m_backend->writeAll(m_pending, timeoutMs);
    m_backend->setWritableNotifications(false);
    if (!m_backend->setNonBlockingOutput(false))
        m_finishSucceeded = false;
    m_pending.clear();
    m_async = false;
    m_hasWrittenOutput = false;
    m_outputFinished = true;
    return m_finishSucceeded;
}

int TerminalDevice::width() const {
    const QSize size = m_backend->size();
    return size.width() > 0 ? size.width() : 80;
}

int TerminalDevice::height() const {
    const QSize size = m_backend->size();
    return size.height() > 0 ? size.height() : 24;
}

} // namespace qamui
