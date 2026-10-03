# TerminalDevice

## 1. Class Overview

`TerminalDevice` manages low-level POSIX and Windows terminal hardware settings. On POSIX platforms, it uses `termios` to manage terminal TTY states. It enables and disables raw mode, queries terminal window dimensions (e.g. via `ioctl TIOCGWINSZ`), toggles cursor visibility, and writes output strings directly to `stdout`.

`TerminalDevice` is used by `TerminalEngine` to control the terminal TTY environment.

## 2. Project Structure and Dependencies

- **Header**: `Terminal/src/core/TerminalDevice.h`
- **Implementation**: `Terminal/src/core/TerminalDevice.cpp`
- **Dependencies**: `<string>`, OS-specific headers (`<termios.h>`, `<unistd.h>`, `<sys/ioctl.h>` on POSIX; `<windows.h>` on Windows).

## 3. Class Hierarchy and Role

- **Base Class**: None.
- **Role**: TTY device manager configuring raw mode and querying terminal geometry.

## 9. Public Methods

#### bool enableRawMode()
Configures the terminal TTY into raw mode (disables echo, canonical line buffering, and processing of control signals like Ctrl+C/Ctrl+Z). Returns `true` if raw mode was successfully enabled, otherwise `false`.

#### bool disableRawMode()
Restores the original terminal state. Automatically invoked in the destructor. Returns `true` if raw mode was successfully disabled, otherwise `false`.

#### void hideCursor()
Writes the ANSI control sequence `\033[?25l` to stdout to hide the terminal cursor.

#### void showCursor()
Writes the ANSI control sequence `\033[?25h` to stdout to make the terminal cursor visible.

#### void clearScreen()
Writes the ANSI control sequence `\033[2J\033[H` to stdout to clear the screen and home the cursor.

#### bool write(const std::string& data)
Writes a string of text or ANSI escape sequence commands directly to the terminal stdout output buffer. Returns `true` if successful.

#### int getWidth() const
Queries and returns the current width of the terminal window in character columns.

#### int getHeight() const
Queries and returns the current height of the terminal window in character rows.

## 11. Ownership and Lifecycle

`TerminalDevice` uses RAII to automatically call `disableRawMode()` and `showCursor()` in its destructor upon application shutdown.

## 12. Thread Safety

Single-threaded. Must be invoked from the thread managing terminal I/O.

## 15. External Communication

- **OS / Hardware Layer**: Interacts with POSIX `/dev/tty` or Windows console APIs and `stdout` using system calls (`read`, `write`, `ioctl`, `tcgetattr`, `tcsetattr` on POSIX; console API functions on Windows).

## 16. Usage Example

```cpp
#include "TerminalDevice.h"
#include <iostream>

using namespace qtui;

int main() {
    TerminalDevice dev;
    if (dev.enableRawMode()) {
        dev.hideCursor();
        dev.write("Terminal Width: " + std::to_string(dev.getWidth()) + "\n");
        dev.showCursor();
        dev.disableRawMode();
    }
    return 0;
}
```
