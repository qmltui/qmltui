#include "TerminalDevice.h"
#include <cerrno>
#include <iostream>

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

void TerminalDevice::enableRawMode() {
    if (m_isRawMode) return;

#ifdef _WIN32
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput != INVALID_HANDLE_VALUE) {
        DWORD mode = m_originalConsoleMode;
        // Disable echo, canonical mode, and signals
        mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        SetConsoleMode(hInput, mode);
    }

    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput != INVALID_HANDLE_VALUE) {
        DWORD mode = m_originalOutputMode;
        // Enable VT100 / ANSI escape processing on Windows console
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
        SetConsoleMode(hOutput, mode);
    }
    m_isRawMode = true;
#else
    if (!m_termiosValid) return;

    termios raw = m_originalTermios;
    // Disable echo, canonical mode, extended input processing, and signals
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // Disable software flow control
    raw.c_iflag &= ~(IXON | ICRNL);
    // Disable output processing
    raw.c_oflag &= ~(OPOST);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    m_isRawMode = true;
#endif
}

void TerminalDevice::disableRawMode() {
    if (!m_isRawMode) return;

#ifdef _WIN32
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput != INVALID_HANDLE_VALUE) {
        SetConsoleMode(hInput, m_originalConsoleMode);
    }
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput != INVALID_HANDLE_VALUE) {
        SetConsoleMode(hOutput, m_originalOutputMode);
    }
    m_isRawMode = false;
#else
    if (!m_termiosValid) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_originalTermios);
    m_isRawMode = false;
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

void TerminalDevice::write(const std::string& data) {
#ifdef _WIN32
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(hOutput, data.c_str(), static_cast<DWORD>(data.size()), &written, NULL);
#else
    size_t totalWritten = 0;
    const char* ptr = data.c_str();
    size_t remaining = data.size();
    while (remaining > 0) {
        ssize_t bytesWritten = ::write(STDOUT_FILENO, ptr + totalWritten, remaining);
        if (bytesWritten < 0) {
            if (errno == EINTR) continue;
            break; // Stop loop on non-retryable error
        }
        totalWritten += bytesWritten;
        remaining -= bytesWritten;
    }
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

