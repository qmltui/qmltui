#pragma once

#include <termios.h>
#include <string>

namespace qtui {

class TerminalDevice {
public:
    TerminalDevice();
    ~TerminalDevice();

    void enableRawMode();
    void disableRawMode();

    void hideCursor();
    void showCursor();
    void clearScreen();

    // Flushes a string (ANSI sequence) to stdout
    void write(const std::string& data);

    // Get terminal dimensions
    int getWidth() const;
    int getHeight() const;

private:
    termios m_originalTermios;
    bool m_isRawMode = false;
};

} // namespace qtui
