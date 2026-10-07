#include "Win32ConsoleBackend.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <io.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QString>
#include <QWinEventNotifier>

#include <algorithm>
#include <limits>

namespace qtui {

namespace {

HANDLE toHandle(void* pointer) { return static_cast<HANDLE>(pointer); }

}

Win32ConsoleBackend::Win32ConsoleBackend(int outputFd, QObject* parent)
    : TerminalBackend(parent)
{
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

Win32ConsoleBackend::~Win32ConsoleBackend()
{
    stopInput();
    if (m_rawMode)
        leaveRawMode();
}

bool Win32ConsoleBackend::enterRawMode()
{
    if (m_rawMode)
        return true;
    if (!m_inputModeValid || !m_outputModeValid)
        return false;

    DWORD inputMode = m_originalInputMode;
    // No echo, no line buffering, no Ctrl+C processing, no console-side mouse
    // handling or QuickEdit selection: the application owns all of that.
    inputMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT
                   | ENABLE_MOUSE_INPUT | ENABLE_QUICK_EDIT_MODE);
    // Keystrokes as VT sequences, resizes as input records.
    inputMode |= ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
    if (!SetConsoleMode(toHandle(m_input), inputMode))
        return false;

    DWORD outputMode = m_originalOutputMode;
    outputMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING
                  | DISABLE_NEWLINE_AUTO_RETURN;
    if (!SetConsoleMode(toHandle(m_output), outputMode)) {
        SetConsoleMode(toHandle(m_input), m_originalInputMode);
        return false;
    }

    m_originalInputCodePage = GetConsoleCP();
    m_originalOutputCodePage = GetConsoleOutputCP();
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    m_rawMode = true;
    return true;
}

bool Win32ConsoleBackend::leaveRawMode()
{
    if (!m_rawMode)
        return true;
    bool restored = true;
    if (m_originalInputCodePage != 0)
        SetConsoleCP(m_originalInputCodePage);
    if (m_originalOutputCodePage != 0)
        SetConsoleOutputCP(m_originalOutputCodePage);
    if (m_inputModeValid && !SetConsoleMode(toHandle(m_input), m_originalInputMode))
        restored = false;
    if (m_outputModeValid && !SetConsoleMode(toHandle(m_output), m_originalOutputMode))
        restored = false;
    m_rawMode = false;
    return restored;
}

TerminalBackend::WriteStatus Win32ConsoleBackend::writeSome(const char* data, size_t size, size_t& offset)
{
    if (toHandle(m_output) == INVALID_HANDLE_VALUE || m_output == nullptr)
        return WriteStatus::Error;
    while (offset < size) {
        const size_t remaining = size - offset;
        const DWORD chunk = static_cast<DWORD>(std::min(remaining,
            static_cast<size_t>(std::numeric_limits<DWORD>::max())));
        DWORD written = 0;
        if (!WriteFile(toHandle(m_output), data + offset, chunk, &written, nullptr) || written == 0)
            return WriteStatus::Error;
        offset += written;
    }
    return WriteStatus::Done;
}

bool Win32ConsoleBackend::writeAll(std::string& pending, int timeoutMs)
{
    Q_UNUSED(timeoutMs);   // console writes complete synchronously
    if (pending.empty())
        return true;
    size_t offset = 0;
    const WriteStatus status = writeSome(pending.data(), pending.size(), offset);
    pending.erase(0, offset);
    return status == WriteStatus::Done;
}

bool Win32ConsoleBackend::setNonBlockingOutput(bool enabled)
{
    return !enabled;   // never non-blocking; disabling is trivially satisfied
}

void Win32ConsoleBackend::setWritableNotifications(bool enabled)
{
    Q_UNUSED(enabled);
}

QSize Win32ConsoleBackend::size() const
{
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (toHandle(m_output) == INVALID_HANDLE_VALUE || m_output == nullptr
        || !GetConsoleScreenBufferInfo(toHandle(m_output), &info))
        return QSize();
    const int columns = info.srWindow.Right - info.srWindow.Left + 1;
    const int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    if (columns <= 0 || rows <= 0)
        return QSize();
    return QSize(columns, rows);
}

bool Win32ConsoleBackend::isInteractive() const
{
    return m_outputModeValid;
}

bool Win32ConsoleBackend::isInputInteractive() const
{
    return m_inputModeValid;
}

void Win32ConsoleBackend::startInput()
{
    if (m_inputStarted)
        return;
    m_inputStarted = true;
    // Redirected input is not a console and cannot be waited on this way.
    if (!m_inputModeValid || !QCoreApplication::instance())
        return;
    m_inputNotifier = std::make_unique<QWinEventNotifier>(toHandle(m_input));
    connect(m_inputNotifier.get(), &QWinEventNotifier::activated, this, [this] { readInput(); });
    m_inputNotifier->setEnabled(true);
}

void Win32ConsoleBackend::stopInput()
{
    if (!m_inputStarted)
        return;
    m_inputStarted = false;
    m_inputNotifier.reset();
}

void Win32ConsoleBackend::readInput()
{
    DWORD available = 0;
    if (!GetNumberOfConsoleInputEvents(toHandle(m_input), &available) || available == 0)
        return;

    QString text;
    bool sizeChanged = false;
    INPUT_RECORD records[128];
    while (available > 0) {
        DWORD count = 0;
        const DWORD wanted = std::min<DWORD>(available, static_cast<DWORD>(std::size(records)));
        if (!ReadConsoleInputW(toHandle(m_input), records, wanted, &count) || count == 0)
            break;
        for (DWORD i = 0; i < count; ++i) {
            const INPUT_RECORD& record = records[i];
            switch (record.EventType) {
            case KEY_EVENT: {
                const KEY_EVENT_RECORD& key = record.Event.KeyEvent;
                // With virtual-terminal input every byte of a sequence arrives
                // as its own key-down record; key-up records carry nothing.
                if (!key.bKeyDown || key.uChar.UnicodeChar == 0)
                    break;
                const int repeat = std::max<int>(1, key.wRepeatCount);
                for (int r = 0; r < repeat; ++r)
                    text.append(QChar(key.uChar.UnicodeChar));
                break;
            }
            case WINDOW_BUFFER_SIZE_EVENT:
                sizeChanged = true;
                break;
            default:
                break;   // focus, menu, and mouse records are not used
            }
        }
        if (!GetNumberOfConsoleInputEvents(toHandle(m_input), &available))
            break;
    }

    if (!text.isEmpty())
        emit inputReceived(text.toUtf8());
    if (sizeChanged)
        emit resized();
}

} // namespace qtui
