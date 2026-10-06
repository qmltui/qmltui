# TerminalDevice

## 1. Class Overview

`TerminalDevice` is the platform-neutral output side of the terminal session. It tracks raw-mode state, toggles cursor visibility, queries dimensions, and owns an ordered output queue that can drain from the Qt event loop when the terminal is slow. All operating-system calls are delegated to a [`TerminalBackend`](TerminalBackend.md) chosen at construction: `PosixTerminalBackend` on Linux and macOS, `Win32ConsoleBackend` on Windows.

`TerminalDevice` is used by `TerminalEngine`, which also reaches the backend through `backend()` to receive input bytes and resize notifications.

## 2. Project Structure and Dependencies

- **Header**: `src/Qtui/primitives/TerminalDevice.h`
- **Implementation**: `src/Qtui/primitives/TerminalDevice.cpp`
- **Dependencies**: `<string>`, `<functional>`, `<memory>`, and `TerminalBackend.h`. No platform headers; those live in the backend implementations.

## 3. Class Hierarchy and Role

- **Base Class**: None.
- **Role**: Output queue and raw-mode bookkeeping over a `TerminalBackend`.

## 9. Public Methods

#### TerminalBackend* backend() const
The platform backend owned by this device. `TerminalEngine` connects to its `inputReceived()` and `resized()` signals.

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

#### int getWidth() const / int getHeight() const
Current terminal size in cells from the backend, falling back to 80 by 24 when unknown.

#### bool setAsyncOutput(bool enabled, int timeoutMs = 2000)
Switches to a non-blocking output channel drained from the event loop. Requires a `QCoreApplication` and a backend with `supportsAsyncOutput()`; on Windows this returns false and output stays synchronous.

#### bool flushBlocking(int timeoutMs = 2000) / bool finishOutput(int timeoutMs = 2000)
Deliver queued bytes within a deadline. `finishOutput()` also queues cursor and style restoration and restores the output channel; it is idempotent and called from the destructor.

## 11. Ownership and Lifecycle

`TerminalDevice` uses RAII to automatically call `disableRawMode()` and `showCursor()` in its destructor upon application shutdown.

## 12. Thread Safety

Single-threaded. Must be invoked from the thread managing terminal I/O.

## 15. External Communication

- **OS layer**: Only through `TerminalBackend`. See [TerminalBackend](TerminalBackend.md) for the POSIX and Windows mechanisms.

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
