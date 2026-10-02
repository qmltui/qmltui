# TerminalDevice

## 1. Class Overview

`TerminalDevice` manages low-level POSIX terminal hardware settings via `termios`. It enables and disables raw mode, queries terminal window dimensions (`ioctl TIOCGWINSZ`), toggles cursor visibility, and flushes output strings directly to `stdout`.

`TerminalDevice` is used by `TerminalEngine` to control the terminal TTY environment.

## 2. Project Structure and Dependencies

- **Header**: `Tui/src/core/TerminalDevice.h`
- **Implementation**: `Tui/src/core/TerminalDevice.cpp`
- **Dependencies**: `<termios.h>`, `<unistd.h>`, `<sys/ioctl.h>`, `<string>`.

## 3. Class Hierarchy and Role

- **Base Class**: None.
- **Role**: TTY device manager configuring POSIX raw mode and querying terminal geometry.

## 9. Public Methods

#### void enableRawMode()
Configures the POSIX terminal TTY into raw mode (disables echo, canonical line buffering, and signal signals like Ctrl+C/Ctrl+Z).

#### void disableRawMode()
Restores original `termios` terminal state. Automatically invoked in destructor.

#### void hideCursor()
Writes ANSI control sequence `\033[?25l` to stdout to hide the terminal cursor.

#### void showCursor()
Writes ANSI control sequence `\033[?25h` to stdout to make the terminal cursor visible.

#### void clearScreen()
Writes ANSI control sequence `\033[2J\033[H` to stdout to clear the screen and home the cursor.

#### void write(const std::string& data)
Flushes a string of text or ANSI escape sequence commands directly to the terminal stdout output buffer.

#### int getWidth() const
Queries and returns the current width of the terminal window in character columns.

#### int getHeight() const
Queries and returns the current height of the terminal window in character rows.

## 11. Ownership and Lifecycle

`TerminalDevice` uses RAII to automatically call `disableRawMode()` and `showCursor()` in its destructor upon application shutdown.

## 12. Thread Safety

Single-threaded. Must be invoked from the thread managing terminal I/O.

## 15. External Communication

- **OS / Hardware Layer**: Interacts with POSIX `/dev/tty` and `stdout` using system calls (`read`, `write`, `ioctl`, `tcgetattr`, `tcsetattr`).

## 16. Usage Example

```cpp
#include "TerminalDevice.h"
#include <iostream>

using namespace qtui;

int main() {
    TerminalDevice dev;
    dev.enableRawMode();
    dev.hideCursor();

    dev.write("Terminal Width: " + std::to_string(dev.getWidth()) + "\n");

    dev.showCursor();
    dev.disableRawMode();
    return 0;
}
```
