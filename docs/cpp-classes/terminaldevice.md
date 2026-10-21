# TerminalDevice

`TerminalDevice` manages the output side of a terminal session on top of a [`TerminalBackend`](terminalbackend.md). It tracks raw-mode state, toggles cursor visibility, queries terminal dimensions, and owns an output queue that drains from the Qt event loop when non-blocking I/O is supported.

- **Header**: `src/QmlTui/Core/Primitives/TerminalDevice.h` *(in-tree protocol header; not installed as a public header)*

## Public Methods

- `TerminalBackend *backend() const` — returns the owned platform backend (`TerminalEngine` connects to its `inputReceived()` and `resized()` signals).
- `bool enableRawMode()` / `bool disableRawMode()` — enters or restores raw terminal mode. `~TerminalDevice()` restores raw mode and cursor visibility automatically.
- `void hideCursor()` / `void showCursor()` — queues `\033[?25l` or `\033[?25h`.
- `void clearScreen()` — queues `\033[2J\033[H`.
- `bool write(const std::string &data)` — queues or writes UTF-8 / ANSI output bytes.
- `int getWidth() const` / `int getHeight() const` — returns current terminal dimensions in cells from the backend (falling back to 80×24 when unknown).
- `bool setAsyncOutput(bool enabled, int timeoutMs = 2000)` — switches to a non-blocking output channel drained from the event loop when `backend()->supportsAsyncOutput()` is `true`.
- `bool flushBlocking(int timeoutMs = 2000)` / `bool finishOutput(int timeoutMs = 2000)` — drains queued output bytes within `timeoutMs`. `finishOutput()` also queues cursor/style restoration and restores blocking mode.
