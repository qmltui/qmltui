# QtMessageRouting

## 1. Class Overview

`QtMessageRouting` provides a unified routing pipeline for Qt diagnostics (`qDebug`, `qInfo`, `qWarning`, `qCritical`, `qFatal`) in QmlTui applications. It cleanly decouples **immediate application diagnostic logging** (e.g., writing to a log file or telemetry engine) from **deferred terminal screen presentation** (preventing log text from garbling full-screen alternate-screen terminal sessions).

Scope:
- Intercepts all messages flowing through Qt's global message handler (`qInstallMessageHandler`).
- Does **not** intercept libc `printf`, `std::cout`, or raw file descriptor writes (which bypass Qt).
- Pure C++20 and `Qt6::Core`; introduces no Qt Quick or QML dependencies.

## 2. Project Structure and Dependencies

- **Header**: `<QmlTui/Core/Engine/QtMessageRouting.h>`
- **Namespace**: `qmltui`
- **Dependencies**: `Qt6::Core`

## 3. Architecture and Defaults

```
                          Qt emits log message (qDebug, qWarning, ...)
                                               │
                                ┌──────────────┴──────────────┐
                                ▼                             ▼
                       [ Diagnostic Sink ]           [ Presentation Sink ]
                     (Immediate / File Logger)      (Deferred Terminal Replay)
                                │                             │
                     Delivered immediately           Is alternate screen active?
                     • Synchronous on caller thread  ├── NO  ──► Delivered immediately
                     • Never buffered or dropped     └── YES ──► Buffered up to 500 entries,
                                                                 replayed after terminal restored
```

### Default Behavior (Unconfigured)
When an application does not explicitly call `QtMessageRouting::install()`:
- No application diagnostic sink is registered.
- Presentation defaults to `stderrPresentationSink()`.
- Outside alternate-screen sessions, messages are presented to `stderr` immediately.
- During alternate-screen sessions, messages are buffered (up to 500 entries, dropping oldest on overflow) and flushed to `stderr` after the terminal restores normal mode and exits raw mode.

### Explicit Configuration
An application registers an explicit configuration via `QtMessageRouting::install()`:
- **Diagnostic Sink**: Required. Receives every message synchronously on the emitting thread, unbounded and unbuffered.
- **Presentation Sink**: Optional. If omitted, terminal presentation is disabled. If provided (e.g. `stderrPresentationSink()` or custom callback), messages outside alternate-screen sessions present immediately, while alternate-screen messages buffer until clean restoration.

## 4. API Reference

### Struct `QtMessage`
An owning value snapshot captured upon router receipt:

| Field | Type | Description |
| :--- | :--- | :--- |
| `type` | `QtMsgType` | Qt message severity (`QtDebugMsg`, `QtInfoMsg`, `QtWarningMsg`, `QtCriticalMsg`, `QtFatalMsg`). |
| `message` | `QString` | Original message text. |
| `timestampUtc` | `QDateTime` | UTC timestamp recorded at receipt. |
| `category` | `QByteArray` | Qt logging category name. |
| `file` | `QByteArray` | Source code file name (copied immediately). |
| `function` | `QByteArray` | Function name (copied immediately). |
| `line` | `int` | Source code line number. |
| `formattedText` | `QString` | Snapshot pre-formatted at receipt via `qFormatLogMessage`. |

#### `const QString &formattedMessage() const noexcept`
Returns the emission-time pre-formatted message text according to Qt's log formatting pattern.

---

### Class `QtMessageRouting`

#### `[[nodiscard]] static std::unique_ptr<QtMessageRouting> install(QtMessageSink diagnosticSink, QtMessageSink presentationSink = {})`
Installs an explicit routing configuration.
- Returns a scoped RAII token (`std::unique_ptr<QtMessageRouting>`).
- Noncopyable and nonmovable.
- Only one explicit registration is allowed per process at any time. Duplicate installation attempts or installations while another registration is retiring return `nullptr`.
- May be called before or after `QCoreApplication`.

#### `static QtMessageSink stderrPresentationSink()`
Returns a callback that writes `msg.formattedMessage()` to `stderr` and flushes it.
> [!NOTE]
> If an application or user redirects `stderr` (e.g. `./app 2> errors.log`), this sink writes to that redirected stream. QmlTui does not open `/dev/tty` or bypass operating system file descriptors.

#### `void reset()`
Idempotently unregisters the token:
1. Rejects any new callbacks for this registration.
2. Discards pending presentation records owned by this registration.
3. Waits for active callback deliveries on worker threads to complete before returning.
4. Restores default routing behavior.

---

## 5. Threading, Ordering, and Safety

- **FIFO Guarantee**: Records are assigned monotonic 64-bit sequence numbers under the router mutex. A record becomes eligible for presentation only after its synchronous diagnostic callback finishes. A serialized drainer delivers presentation messages in strict sequence order.
- **Callback Invocations**: Invoked outside the router mutex. Callbacks must be thread-safe.
- **Session Precedence**: Terminal restoration, output flushing, and raw-mode exit strictly precede presentation replay in all engine shutdown and disable paths.
- **Recursion Guard**: If a callback recursively logs via Qt on the same thread, the recursion is diverted to an emergency `stderr` fallback to prevent deadlocks or infinite loops.
- **Exception Isolation**: Exceptions escaping one sink do not prevent delivery to the other sink or escape into Qt's message handler.
- **Fatal Messages (`QtFatalMsg`)**: Synchronously bypass buffering and are delivered immediately to both sinks before returning to Qt for process termination (`abort()`).

---

## 6. C++ Integration Example

```cpp
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QmlTui/Core/Engine/QtMessageRouting.h>
#include <QmlTui/Core/Items/Terminal.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Application-owned log file
    QFile logFile("application.log");
    logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    QTextStream logStream(&logFile);

    // Install routing BEFORE constructing terminal engines
    auto routing = qmltui::QtMessageRouting::install(
        [&](const qmltui::QtMessage &msg) {
            logStream << "[" << msg.timestampUtc.toString(Qt::ISODate) << "] "
                      << msg.formattedMessage() << "\n";
            logStream.flush();
        },
        qmltui::QtMessageRouting::stderrPresentationSink()
    );

    // Terminal engine and QML runtime
    qmltui::Terminal terminal;
    // ... setup and run application ...

    return app.exec();
}
```
