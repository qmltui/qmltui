#include "TerminalDevice.h"
#include <cerrno>
#include <iostream>
#include <algorithm>
#include <limits>

#ifndef _WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace qtui {

TerminalDevice::TerminalDevice() {
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
    disableRawMode();
    showCursor();
    write("\033[0m"); // Reset text formatting and colors back to terminal defaults
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
    // Disable software flow control
    raw.c_iflag &= ~(IXON | ICRNL);
    // Disable output processing
    raw.c_oflag &= ~(OPOST);

    m_isRawMode = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
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

bool TerminalDevice::write(const std::string& data) {
#ifdef _WIN32
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput == INVALID_HANDLE_VALUE) return false;
    size_t totalWritten = 0;
    while (totalWritten < data.size()) {
        const size_t remaining = data.size() - totalWritten;
        const DWORD chunkSize = static_cast<DWORD>((std::min)(remaining,
            static_cast<size_t>((std::numeric_limits<DWORD>::max)())));
        DWORD written = 0;
        if (!WriteFile(hOutput, data.data() + totalWritten, chunkSize, &written, nullptr)
            || written == 0) {
            return false;
        }
        totalWritten += written;
    }
    return true;
#else
    size_t totalWritten = 0;
    const char* ptr = data.c_str();
    size_t remaining = data.size();
    while (remaining > 0) {
        ssize_t bytesWritten = ::write(STDOUT_FILENO, ptr + totalWritten, remaining);
        if (bytesWritten < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (bytesWritten == 0) return false;
        totalWritten += bytesWritten;
        remaining -= bytesWritten;
    }
    return true;
#endif
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
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
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
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_row == 0) {
        return 24; // Fallback
    }
    return ws.ws_row;
#endif
}

} // namespace qtui
