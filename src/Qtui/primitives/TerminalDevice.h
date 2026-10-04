#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

namespace qtui {

class TerminalDevice {
public:
    TerminalDevice();
    ~TerminalDevice();

    bool enableRawMode();
    bool disableRawMode();

    void hideCursor();
    void showCursor();
    void clearScreen();

    // Flushes a string (ANSI sequence) to stdout
    bool write(const std::string& data);

    // Get terminal dimensions
    int getWidth() const;
    int getHeight() const;

private:
#ifdef _WIN32
    DWORD m_originalConsoleMode = 0;
    DWORD m_originalOutputMode = 0;
#else
    termios m_originalTermios;
    bool m_termiosValid = false;
#endif
    bool m_isRawMode = false;
    bool m_hasWrittenOutput = false;
};

} // namespace qtui
