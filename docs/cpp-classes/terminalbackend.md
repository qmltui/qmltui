# TerminalBackend

## 1. Class Overview

`TerminalBackend` is the platform seam between Qamui and the operating system's terminal. Everything above it is shared code: `TerminalDevice` owns the output queue and raw-mode bookkeeping, `TerminalEngine` decodes input bytes and drives the renderers. Everything below it is platform code, one class per platform:

| Implementation | Platform | Mechanisms |
| :--- | :--- | :--- |
| `PosixTerminalBackend` | Linux, macOS, other POSIX | `termios` raw mode on standard input, `write(2)` and `poll(2)` on the output descriptor, a `QSocketNotifier` on standard input, a process-wide `SIGWINCH` self-pipe bridge |
| `Win32ConsoleBackend` | Windows (Windows Terminal, VT-capable conhost) | `SetConsoleMode` with `ENABLE_VIRTUAL_TERMINAL_INPUT`, `ENABLE_WINDOW_INPUT`, and `ENABLE_VIRTUAL_TERMINAL_PROCESSING`; UTF-8 code pages; `WriteFile`; `ReadConsoleInputW` driven by a `QWinEventNotifier` on the console input handle |
| `WasmTerminalBackend` | Qt for WebAssembly (any browser, xterm.js on the page) | Output bytes through the `Module.qamuiOnOutput` JS callback (`EM_JS`), key bytes and geometry through the embind entry points `qamuiFeedInput` / `qamuiResize`; sets `TERM`/`COLORTERM` when the environment is empty; synchronous output |

Input is always delivered as raw VT byte sequences, so `InputDecoder` needs no platform knowledge. On Windows the console itself performs that translation once virtual-terminal input is enabled.

## 2. Project Structure and Dependencies

- **Header**: `src/Qamui/Core/Primitives/TerminalBackend.h`, installed as `<Qamui/Core/Primitives/TerminalBackend.h>`
- **Implementations**: `PosixTerminalBackend.{h,cpp}`, `Win32ConsoleBackend.{h,cpp}`, `WasmTerminalBackend.{h,cpp}` (selected in `src/Qamui/Core/CMakeLists.txt` by `WIN32`, `EMSCRIPTEN`, else POSIX)
- **Factory**: `std::unique_ptr<TerminalBackend> createTerminalBackend(int outputFd = -1)` in `TerminalBackend.cpp`
- **Dependencies**: QtCore only. Platform headers are confined to the implementation files; `<windows.h>` never reaches a public header.

## 3. Class Hierarchy and Role

- **Base Class**: `QObject`
- **Role**: Abstract interface. `TerminalDevice` owns one instance and exposes it through `TerminalDevice::backend()`.

## 9. Public Methods

#### bool enterRawMode() / bool leaveRawMode()
Switch the terminal between its normal line-oriented mode and raw mode (no echo, no line buffering, no signal generation, VT sequences for special keys). `TerminalDevice` tracks the current state and flushes queued output before leaving raw mode.

#### bool needsUtf8Designation() const
True when the terminal parser must be told the byte stream is UTF-8 (`ESC % G`) after entering raw mode. POSIX terminals want it; the Windows console uses code pages instead.

#### WriteStatus writeSome(const char* data, size_t size, size_t& offset)
Writes as much as the terminal accepts right now and advances `offset`. Returns `Done`, `WouldBlock` (only possible after `setNonBlockingOutput(true)`), or `Error`.

#### bool writeAll(std::string& pending, int timeoutMs)
Delivers everything in `pending`, erasing delivered bytes, waiting for the terminal as needed but never past `timeoutMs`. False on error or timeout.

#### bool supportsAsyncOutput() const
Whether output can be made non-blocking and drained from the event loop. True on POSIX. False on Windows, where console writes complete synchronously and `TerminalDevice::setAsyncOutput(true)` therefore returns false.

#### bool setNonBlockingOutput(bool enabled)
Toggles the output channel between blocking and non-blocking. Disabling when already blocking succeeds.

#### void setWritableNotifications(bool enabled)
Enables or disables `writable()` for a non-blocking channel.

#### QSize size() const
Terminal size in cells, or an invalid `QSize` when unknown. `TerminalDevice` substitutes 80 by 24.

#### bool isInteractive() const
True when output goes to a real terminal rather than a pipe or file. Feeds `TerminalCapabilities::detect()`.

#### void startInput() / void stopInput()
Begin and end delivery of `inputReceived()` and `resized()`. Idempotent. `TerminalEngine` calls `startInput()` once during construction.

## 10. Signals

| Signal | Meaning |
| :--- | :--- |
| `writable()` | The non-blocking output channel can accept more bytes. |
| `inputReceived(const QByteArray& bytes)` | Raw VT bytes read from the terminal. |
| `resized()` | The terminal geometry changed; call `size()`. |

## 11. Ownership and Lifecycle

Created by `TerminalDevice`'s constructor through `createTerminalBackend()` and destroyed with it. Destruction stops input and restores the output channel; raw mode is restored by `TerminalDevice` before that.

## 12. Thread Safety

Single-threaded. Create and use on the `QCoreApplication` thread. The POSIX `SIGWINCH` bridge is installed while at least one backend is listening and removed with the last one.

## 14. Application-provided backends

`TerminalBackend::setFactory()` installs a factory that replaces the platform backend for every `TerminalDevice` created afterwards. Install it before the first `Terminal` completes; pass `nullptr` to restore the default. Typical uses: a serial console on a device without a tty layer (write to a `QSerialPort`, feed its bytes back through `inputReceived()`), hosting sessions inside another process over a socket or ConPTY, or an in-memory backend in tests. The browser case is covered by the built-in `WasmTerminalBackend`; see [docs/topics/webassembly.md](../topics/webassembly.md).

```cpp
#include <Qamui/Core/Primitives/TerminalBackend.h>

qamui::TerminalBackend::setFactory([](int) {
    return std::make_unique<SerialConsoleBackend>(QStringLiteral("/dev/ttyS0"));
});
```

`qamui::MemoryTerminalBackend` in `Qamui::Runner` is the reference implementation and the test tool: it captures output bytes and lets a test inject input bytes and resizes. See `tests/Core/test_terminal_backend.cpp` for the end-to-end pattern.

This is a C++ extension point and may still change before 1.0, like custom items.

## 15. Windows notes

- Windows Terminal is the target. Legacy `conhost.exe` gets whatever its VT support provides.
- Raw mode clears `ENABLE_QUICK_EDIT_MODE` and `ENABLE_MOUSE_INPUT` so the console does not intercept mouse selection; mouse reporting requested through VT sequences arrives as SGR sequences like on POSIX.
- `WINDOW_BUFFER_SIZE_EVENT` records become `resized()`. Focus and menu records are dropped.
- Code pages are set to UTF-8 while in raw mode and restored on exit.
