# TerminalBackend

`TerminalBackend` is the platform abstraction between QmlTui and the operating system's terminal. `TerminalDevice` owns the output queue and raw-mode bookkeeping on top of it, and `TerminalEngine` decodes input bytes and drives the renderers.

- **Header**: `<QmlTui/Core/Primitives/TerminalBackend.h>`
- **Inherits**: `QObject`
- **Target**: `qmltui::core` (`Qt6::Core` only; platform headers stay inside implementation files)

## Built-In Implementations

| Implementation | Platform | Mechanism |
| :--- | :--- | :--- |
| `PosixTerminalBackend` | Linux, macOS, POSIX | `termios` raw mode on `stdin`, `write(2)` and `poll(2)` on the output descriptor, `QSocketNotifier` on `stdin`, a `SIGWINCH` / `SIGCONT` self-pipe bridge, and async-signal-safe terminal restoration on fatal/termination signals (`SIGINT`, `SIGTERM`, `SIGHUP`, `SIGQUIT`, `SIGABRT`, `SIGSEGV`, `SIGBUS`, `SIGFPE`, `SIGILL`) and `SIGTSTP`. |
| `Win32ConsoleBackend` | Windows (Windows Terminal, VT-capable conhost) | `SetConsoleMode` (`ENABLE_VIRTUAL_TERMINAL_INPUT`, `ENABLE_WINDOW_INPUT`, `ENABLE_VIRTUAL_TERMINAL_PROCESSING`), UTF-8 code pages, `WriteFile`, and `ReadConsoleInputW` driven by `QWinEventNotifier`. |
| `WasmTerminalBackend` | Qt for WebAssembly | Emits output bytes via `Module.qmltuiOnOutput` (`EM_JS`) and receives input bytes and resize events through `qmltuiFeedInput` / `qmltuiResize`. |

Input is always delivered as raw VT byte sequences so `InputDecoder` needs no platform-specific logic.

## Public Types and Methods

- `enum class WriteStatus { Done, WouldBlock, Error }` — result of a non-blocking `writeSome()` call.
- `using Factory = std::function<std::unique_ptr<TerminalBackend>(int outputFd)>` — custom backend factory signature.
- `static void setFactory(Factory factory)` / `static const Factory &factory()` — installs or returns the process-wide custom `TerminalBackend` factory (`nullptr` restores the platform default).
- `bool enterRawMode()` / `bool leaveRawMode()` — switches the terminal between line-buffered mode and raw mode. `TerminalDevice` flushes queued output before leaving raw mode.
- `bool needsUtf8Designation() const` — `true` on POSIX terminals that expect `ESC % G` after entering raw mode; `false` on Windows where code pages are used.
- `virtual void setKeyboardProtocolActive(bool active)` — notifies the backend when the Kitty keyboard disambiguation protocol (`CSI > 1 u` / `CSI < u`) is pushed or popped so crash/signal cleanup can pop it if needed.
- `WriteStatus writeSome(const char *data, size_t size, size_t &offset)` — writes as many bytes as the terminal accepts immediately (`Done`, `WouldBlock`, or `Error`).
- `bool writeAll(std::string &pending, int timeoutMs)` — writes all bytes in `pending` within `timeoutMs`, erasing delivered bytes.
- `bool supportsAsyncOutput() const` — `true` on POSIX where output can drain non-blockingly from the event loop; `false` on Windows.
- `bool setNonBlockingOutput(bool enabled)` — toggles non-blocking output mode.
- `void setWritableNotifications(bool enabled)` — enables or disables `writable()` notifications on a non-blocking channel.
- `QSize size() const` — terminal dimensions in cells, or an invalid `QSize` when unknown (`TerminalDevice` falls back to 80×24).
- `bool isInteractive() const` / `virtual bool isInputInteractive() const` — `true` when output (or standard input, respectively; defaults to `isInteractive()`) is attached to a live TTY rather than a pipe or file.
- `void startInput()` / `void stopInput()` — starts and stops delivery of `inputReceived()` and `resized()`.
- `std::unique_ptr<TerminalBackend> createTerminalBackend(int outputFd = -1)` *(free function in `namespace qmltui`)* — creates a backend using `TerminalBackend::factory()` when installed, or the platform default otherwise (`outputFd < 0` selects standard output).

## Signals

| Signal | Description |
| :--- | :--- |
| `writable()` | The non-blocking output descriptor can accept more bytes. |
| `inputReceived(const QByteArray &bytes)` | Raw VT bytes read from the terminal. |
| `resized()` | Terminal geometry changed; query `size()`. |
| `resumed()` | Process resumed after job-control suspension (`SIGCONT`); terminal modes and screen contents must be re-established. |

## Custom Application Backends

`TerminalBackend::setFactory()` installs a custom backend factory for every `TerminalDevice` created afterward. Install it before the first `Terminal` completes; pass `nullptr` to restore the default platform factory.

```cpp
#include <QmlTui/Core/Primitives/TerminalBackend.h>

qmltui::TerminalBackend::setFactory([](int) {
    return std::make_unique<SerialConsoleBackend>(QStringLiteral("/dev/ttyS0"));
});
```

See [`MemoryTerminalBackend`](memoryterminalbackend.md) in `qmltui::runner` for an in-memory reference implementation used in unit tests.

## Windows Notes

- Windows Terminal is the supported runtime target; legacy `conhost.exe` receives whatever its VT implementation supports.
- Raw mode clears `ENABLE_QUICK_EDIT_MODE` and `ENABLE_MOUSE_INPUT` so the console does not intercept mouse clicks; mouse reporting requested through VT sequences arrives as SGR sequences.
- `WINDOW_BUFFER_SIZE_EVENT` records emit `resized()`.
- Console code pages are set to UTF-8 on entering raw mode and restored on exit.
