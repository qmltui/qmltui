# MemoryTerminalBackend

## Class Overview

`MemoryTerminalBackend` is an in-memory [`TerminalBackend`](terminalbackend.md). It captures every byte the engine writes and lets a test inject input bytes and terminal resizes, so the whole path from `TerminalEngine` through `InputDecoder` to QML key handlers can be exercised without a pipe, a pseudo terminal, or a console. It is also the reference implementation of the application-facing backend extension point.

- **Header**: `#include <QmlTui/Runner/MemoryTerminalBackend.h>` (target `QmlTui::Runner`)
- **Inherits**: `qmltui::TerminalBackend`

## Public Methods

#### explicit MemoryTerminalBackend(QSize size = QSize(80, 24), QObject* parent = nullptr)
The size reported to the engine until `setSize()` changes it.

#### void feedInput(const QByteArray& bytes)
Emits `inputReceived(bytes)`. Bytes are raw terminal input, for example `"q"` or `"\033[A"`.

#### void setSize(const QSize& size)
Changes the reported size and emits `resized()` when it differs.

#### const std::string& output() const / void clearOutput()
Everything written by the engine so far, including escape sequences.

#### bool rawMode() const / bool inputStarted() const
Whether the engine entered raw mode and started input delivery.

#### void setInteractive(bool)
What `isInteractive()` reports; affects `TerminalCapabilities::detect()`.

## Usage Example

```cpp
static qmltui::MemoryTerminalBackend* g_backend = nullptr;

qmltui::TerminalBackend::setFactory([](int) {
    auto backend = std::make_unique<qmltui::MemoryTerminalBackend>();
    g_backend = backend.get();
    return backend;
});
// ... create the Terminal from QML with output enabled (no QMLTUI_RUNNER_SCRIPT, QMLTUI_RUNNER_EVAL, or QMLTUI_RUNNER_CAPTURE_AFTER_MS in the environment) ...
g_backend->feedInput("q");            // reaches Keys.onPressed
g_backend->setSize(QSize(120, 40));   // Terminal.terminalWidth becomes 120
QVERIFY(g_backend->output().find("\033[") != std::string::npos);
```
