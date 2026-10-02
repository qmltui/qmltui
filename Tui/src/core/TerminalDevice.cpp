#include "TerminalDevice.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>

namespace qtui {

TerminalDevice::TerminalDevice() {
    tcgetattr(STDIN_FILENO, &m_originalTermios);
}

TerminalDevice::~TerminalDevice() {
    disableRawMode();
    showCursor();
}

void TerminalDevice::enableRawMode() {
    if (m_isRawMode) return;

    termios raw = m_originalTermios;
    // Disable echo, canonical mode, extended input processing, and signals
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // Disable software flow control
    raw.c_iflag &= ~(IXON | ICRNL);
    // Disable output processing
    raw.c_oflag &= ~(OPOST);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    m_isRawMode = true;
}

void TerminalDevice::disableRawMode() {
    if (!m_isRawMode) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_originalTermios);
    m_isRawMode = false;
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
    if (::write(STDOUT_FILENO, data.c_str(), data.size()) == -1) {
        // Error handling could be added here
    }
}

int TerminalDevice::getWidth() const {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return 80; // Fallback
    }
    return ws.ws_col;
}

int TerminalDevice::getHeight() const {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_row == 0) {
        return 24; // Fallback
    }
    return ws.ws_row;
}

} // namespace qtui
