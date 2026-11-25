#include <QmlTui/Core/Primitives/Win32ConsoleBackend.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <io.h>

#include <QCoreApplication>
#include <QString>
#include <QThread>
#include <QWinEventNotifier>

#include <algorithm>
#include <limits>
#include <mutex>

namespace qmltui {

namespace {

HANDLE toHandle(void* pointer) {
    return static_cast<HANDLE>(pointer);
}

bool isOnApplicationThread() {
    const QCoreApplication* app = QCoreApplication::instance();
    return !app || QThread::currentThread() == app->thread();
}

struct Win32EmergencyState {
    std::mutex mutex;
    const Win32ConsoleBackend* owner = nullptr;
    HANDLE input = INVALID_HANDLE_VALUE;
    HANDLE output = INVALID_HANDLE_VALUE;
    DWORD originalInputMode = 0;
    DWORD originalOutputMode = 0;
    UINT originalInputCodePage = 0;
    UINT originalOutputCodePage = 0;
    bool active = false;
    bool handlerInstalled = false;
    bool keyboardProtocolActive = false;
};

Win32EmergencyState g_win32Emergency;

constexpr char kWin32KittyKeyboardPop[] = "\033[<u";
constexpr char kWin32ResetSequence[] =
    "\033[?2026l\033[?1000l\033[?1002l\033[?1003l\033[?1006l\033[?1004l\033[?2004l\033[?2031l"
    "\033[?25h\033[0m\033]9;4;0\033\\\033]111\a\033]111;\a\033]111;\033\\\033[?1049l";

BOOL WINAPI consoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_CLOSE_EVENT || ctrlType == CTRL_LOGOFF_EVENT ||
        ctrlType == CTRL_SHUTDOWN_EVENT || ctrlType == CTRL_C_EVENT ||
        ctrlType == CTRL_BREAK_EVENT) {
        const std::lock_guard<std::mutex> lock(g_win32Emergency.mutex);
        if (g_win32Emergency.active) {
            DWORD written = 0;
            if (g_win32Emergency.output != INVALID_HANDLE_VALUE) {
                if (g_win32Emergency.keyboardProtocolActive) {
                    WriteFile(g_win32Emergency.output, kWin32KittyKeyboardPop,
                              static_cast<DWORD>(sizeof(kWin32KittyKeyboardPop) - 1), &written,
                              nullptr);
                    g_win32Emergency.keyboardProtocolActive = false;
                }
                WriteFile(g_win32Emergency.output, kWin32ResetSequence,
                          static_cast<DWORD>(sizeof(kWin32ResetSequence) - 1), &written, nullptr);
                SetConsoleMode(g_win32Emergency.output, g_win32Emergency.originalOutputMode);
            }
            if (g_win32Emergency.input != INVALID_HANDLE_VALUE)
                SetConsoleMode(g_win32Emergency.input, g_win32Emergency.originalInputMode);
            if (g_win32Emergency.originalInputCodePage != 0)
                SetConsoleCP(g_win32Emergency.originalInputCodePage);
            if (g_win32Emergency.originalOutputCodePage != 0)
                SetConsoleOutputCP(g_win32Emergency.originalOutputCodePage);
            g_win32Emergency.active = false;
        }
    }
    return FALSE;
}

} // namespace

Win32ConsoleBackend::Win32ConsoleBackend(int outputFd, QObject* parent) : TerminalBackend(parent) {
    m_input = GetStdHandle(STD_INPUT_HANDLE);
    if (outputFd >= 0 && outputFd != 1)
        m_output = reinterpret_cast<HANDLE>(_get_osfhandle(outputFd));
    else
        m_output = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD mode = 0;
    if (toHandle(m_input) != INVALID_HANDLE_VALUE && GetConsoleMode(toHandle(m_input), &mode)) {
        m_originalInputMode = mode;
        m_inputModeValid = true;
    }
    if (toHandle(m_output) != INVALID_HANDLE_VALUE && GetConsoleMode(toHandle(m_output), &mode)) {
        m_originalOutputMode = mode;
        m_outputModeValid = true;
    }
}

Win32ConsoleBackend::~Win32ConsoleBackend() {
    stopInput();
    if (m_rawMode) leaveRawMode();
}

bool Win32ConsoleBackend::enterRawMode() {
    if (m_rawMode) return true;
    if (!m_inputModeValid || !m_outputModeValid || !isOnApplicationThread()) return false;

    DWORD inputMode = m_originalInputMode;
    // No echo, no line buffering, no Ctrl+C processing, no console-side mouse
    // handling or QuickEdit selection: the application owns all of that.
    inputMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT |
                   ENABLE_MOUSE_INPUT | ENABLE_QUICK_EDIT_MODE);
    // Keystrokes as VT sequences, resizes as input records.
    inputMode |= ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
    if (!SetConsoleMode(toHandle(m_input), inputMode)) return false;

    DWORD outputMode = m_originalOutputMode;
    outputMode |=
        ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    if (!SetConsoleMode(toHandle(m_output), outputMode)) {
        SetConsoleMode(toHandle(m_input), m_originalInputMode);
        return false;
    }

    m_originalInputCodePage = GetConsoleCP();
    m_originalOutputCodePage = GetConsoleOutputCP();
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    m_rawMode = true;

    {
        const std::lock_guard<std::mutex> lock(g_win32Emergency.mutex);
        g_win32Emergency.owner = this;
        g_win32Emergency.input = toHandle(m_input);
        g_win32Emergency.output = toHandle(m_output);
        g_win32Emergency.originalInputMode = m_originalInputMode;
        g_win32Emergency.originalOutputMode = m_originalOutputMode;
        g_win32Emergency.originalInputCodePage = m_originalInputCodePage;
        g_win32Emergency.originalOutputCodePage = m_originalOutputCodePage;
        g_win32Emergency.active = true;
        g_win32Emergency.keyboardProtocolActive = false;
        if (!g_win32Emergency.handlerInstalled) {
            if (SetConsoleCtrlHandler(&consoleCtrlHandler, TRUE))
                g_win32Emergency.handlerInstalled = true;
        }
    }
    return true;
}

bool Win32ConsoleBackend::leaveRawMode() {
    if (!m_rawMode) return true;
    {
        const std::lock_guard<std::mutex> lock(g_win32Emergency.mutex);
        if (g_win32Emergency.owner == this) {
            g_win32Emergency.active = false;
            g_win32Emergency.keyboardProtocolActive = false;
            g_win32Emergency.owner = nullptr;
            if (g_win32Emergency.handlerInstalled) {
                SetConsoleCtrlHandler(&consoleCtrlHandler, FALSE);
                g_win32Emergency.handlerInstalled = false;
            }
        }
    }
    bool restored = true;
    if (m_originalInputCodePage != 0) SetConsoleCP(m_originalInputCodePage);
    if (m_originalOutputCodePage != 0) SetConsoleOutputCP(m_originalOutputCodePage);
    if (m_inputModeValid && !SetConsoleMode(toHandle(m_input), m_originalInputMode))
        restored = false;
    if (m_outputModeValid && !SetConsoleMode(toHandle(m_output), m_originalOutputMode))
        restored = false;
    m_rawMode = false;
    return restored;
}

void Win32ConsoleBackend::setKeyboardProtocolActive(bool active) {
    const std::lock_guard<std::mutex> lock(g_win32Emergency.mutex);
    if (g_win32Emergency.owner == this) {
        g_win32Emergency.keyboardProtocolActive = active;
    }
}

TerminalBackend::WriteStatus Win32ConsoleBackend::writeSome(const char* data, size_t size,
                                                            size_t& offset) {
    if (toHandle(m_output) == INVALID_HANDLE_VALUE || m_output == nullptr)
        return WriteStatus::Error;
    while (offset < size) {
        const size_t remaining = size - offset;
        const DWORD chunk = static_cast<DWORD>(
            (std::min)(remaining, static_cast<size_t>((std::numeric_limits<DWORD>::max)())));
        DWORD written = 0;
        if (!WriteFile(toHandle(m_output), data + offset, chunk, &written, nullptr) || written == 0)
            return WriteStatus::Error;
        offset += written;
    }
    return WriteStatus::Done;
}

bool Win32ConsoleBackend::writeAll(std::string& pending, int timeoutMs) {
    Q_UNUSED(timeoutMs); // console writes complete synchronously
    if (pending.empty()) return true;
    size_t offset = 0;
    const WriteStatus status = writeSome(pending.data(), pending.size(), offset);
    pending.erase(0, offset);
    return status == WriteStatus::Done;
}

bool Win32ConsoleBackend::setNonBlockingOutput(bool enabled) {
    return !enabled; // never non-blocking; disabling is trivially satisfied
}

void Win32ConsoleBackend::setWritableNotifications(bool enabled) {
    Q_UNUSED(enabled);
}

QSize Win32ConsoleBackend::size() const {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (toHandle(m_output) == INVALID_HANDLE_VALUE || m_output == nullptr ||
        !GetConsoleScreenBufferInfo(toHandle(m_output), &info))
        return QSize();
    const int columns = info.srWindow.Right - info.srWindow.Left + 1;
    const int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    if (columns <= 0 || rows <= 0) return QSize();
    return QSize(columns, rows);
}

bool Win32ConsoleBackend::isInteractive() const {
    return m_outputModeValid;
}

bool Win32ConsoleBackend::isInputInteractive() const {
    return m_inputModeValid;
}

bool Win32ConsoleBackend::isStderrInteractive() const {
    const HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode = 0;
    return err != INVALID_HANDLE_VALUE && err != nullptr && GetConsoleMode(err, &mode) != 0;
}

void Win32ConsoleBackend::startInput() {
    if (m_inputStarted) return;
    m_inputStarted = true;
    // Redirected input is not a console, and process-wide console input belongs
    // exclusively to the QCoreApplication thread.
    if (!m_inputModeValid || !QCoreApplication::instance() || !isOnApplicationThread()) return;
    m_inputNotifier = std::make_unique<QWinEventNotifier>(toHandle(m_input));
    connect(m_inputNotifier.get(), &QWinEventNotifier::activated, this, [this] { readInput(); });
    m_inputNotifier->setEnabled(true);
}

void Win32ConsoleBackend::stopInput() {
    if (!m_inputStarted) return;
    m_inputStarted = false;
    m_inputNotifier.reset();
}

void Win32ConsoleBackend::readInput() {
    DWORD available = 0;
    if (!GetNumberOfConsoleInputEvents(toHandle(m_input), &available) || available == 0) return;

    QString text;
    if (m_pendingHighSurrogate != 0) {
        text.append(QChar(m_pendingHighSurrogate));
        m_pendingHighSurrogate = 0;
    }
    bool sizeChanged = false;
    INPUT_RECORD records[128];
    while (available > 0) {
        DWORD count = 0;
        const DWORD wanted = (std::min)(available, static_cast<DWORD>(std::size(records)));
        if (!ReadConsoleInputW(toHandle(m_input), records, wanted, &count) || count == 0) break;
        for (DWORD i = 0; i < count; ++i) {
            const INPUT_RECORD& record = records[i];
            switch (record.EventType) {
            case KEY_EVENT: {
                const KEY_EVENT_RECORD& key = record.Event.KeyEvent;
                // With virtual-terminal input every byte of a sequence arrives
                // as its own key-down record; key-up records carry nothing.
                if (!key.bKeyDown || key.uChar.UnicodeChar == 0) break;
                const int repeat = (std::max)(1, static_cast<int>(key.wRepeatCount));
                for (int r = 0; r < repeat; ++r)
                    text.append(QChar(key.uChar.UnicodeChar));
                break;
            }
            case WINDOW_BUFFER_SIZE_EVENT:
                sizeChanged = true;
                break;
            default:
                break; // focus, menu, and mouse records are not used
            }
        }
        if (!GetNumberOfConsoleInputEvents(toHandle(m_input), &available)) break;
    }

    if (!text.isEmpty() && text.back().isHighSurrogate()) {
        m_pendingHighSurrogate = text.back().unicode();
        text.chop(1);
    }

    if (!text.isEmpty()) emit inputReceived(text.toUtf8());
    if (sizeChanged) emit resized();
}

} // namespace qmltui
