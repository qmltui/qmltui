# TerminalEngine

## 1. Class Overview

`TerminalEngine` is the central runtime coordinator for QmlTui applications. It initializes raw terminal I/O via `TerminalDevice`, hooks into Qt's event loop via `QSocketNotifier` for non-blocking stdin input decoding (`InputDecoder`), manages window resize signals (`SIGWINCH`), dispatches keyboard (`KeyEvent`) and mouse events (`MouseEvent`), and manages off-screen double-buffered frame rendering (`Renderer`).

Button events and button-held motion are enabled for drag interactions. Passive
mouse motion is enabled only while a component requests hover tracking, such as
a `MouseArea` with `hoverEnabled` or an attached tooltip with non-empty text.

Developers interact with `TerminalEngine` when setting up top-level application windows or requesting manual frame renders.

## 2. Project Structure and Dependencies

- **Header**: `src/QmlTui/Core/Engine/TerminalEngine.h`
- **Implementation**: `src/QmlTui/Core/Engine/TerminalEngine.cpp`
- **Dependencies**: `TerminalDevice.h`, `InputDecoder.h`, `Renderer.h`, `Terminal.h`, `Item.h`.

## 3. Class Hierarchy and Role

- **Base Class**: `QObject`
  - Leverages Qt slots, meta-object invocation, and event loop integration.

## 7. Signals

#### void frameRendered(quint64 sequence)
Emitted after a frame is painted and committed, passing the monotonically increasing frame sequence number.

#### void scrollbackCommitted(quint64 totalRows)
Emitted before `frameRendered()` on a frame that committed scrollback rows in Inline mode.

#### void capabilitiesChanged()
Emitted when detected or overridden terminal capabilities change.

#### void outputSessionInvalidated(quint64 generation)
Emitted when the output session generation changes or an output write fails, allowing resource owners to invalidate terminal-side handles.

#### void renderFailed(const QString& message)
Emitted when an immediate frame write or asynchronous output drain fails.

#### void clipboardReadFinished(const QString& text, const QString& code)
Emitted when an OSC 52 clipboard read completes (`code` is empty on success, or an error identifier otherwise).

## 8. Public Slots and Q_INVOKABLE Methods

#### void requestRender() [Q_INVOKABLE]
Schedules an asynchronous frame re-render on the next event loop tick. Safe to call from QML or C++.

## 9. Public Methods

#### explicit TerminalEngine(Terminal* rootWindow, QObject* parent = nullptr)
Constructs the terminal engine instance bound to the given root `Terminal` window.

#### void setRenderer(Renderer* renderer)
Registers a custom frame renderer strategy.

#### void setMaxFrameRate(int framesPerSecond) / int maxFrameRate() const
Sets or returns the upper bound on committed frames per second (`0` disables frame pacing). Defaults to `60` (`QMLTUI_MAX_FPS`), or `0` in test sessions.

#### const TerminalCapabilities& capabilities() const / void setCapabilities(const TerminalCapabilities& capabilities)
Returns the active terminal capabilities or overrides them programmatically (cancelling pending protocol negotiation probes).

#### int terminalWidth() const
Returns the current terminal window width in character columns.

#### int terminalHeight() const
Returns the current terminal window height in character rows.

#### bool submitScrollback(const QString& text)
#### bool submitScrollback(const TextGridBuffer& rows)
Queues text or painted cell rows for scrollback emission in Inline mode (bounded by `ScrollbackQueueLimit`, 1 MiB). Rejects the submission and returns `false` on overflow while retaining already accepted output.

#### QString requestClipboardText(int timeoutMs)
Starts an OSC 52 clipboard read. An empty result means pending; otherwise the
result is an error code. Concurrent requests share the active query and its
deadline. `clipboardReadFinished(text, code)` reports completion: an empty code
means success. Reads require an active interactive session and enabled clipboard
integration. QML applications should use [Clipboard.requestText()](../qml-types/clipboard.md).

#### void cancelClipboardRead()
Cancels a pending read with `Cancelled`. Shutdown and suppressed output cancel
automatically. Replies are validated as Base64-encoded UTF-8 with a 1 MiB decoded
limit; malformed replies fail without becoming keyboard input.

#### void copyToClipboard(const QString& text)
Sends an OSC 52 write request when terminal output is enabled. This does not confirm that the terminal updated its clipboard; the Terminal facade applies clipboardMode.

#### static std::string osc52Sequence(const QString& text)
Generates the ANSI escape sequence for copying the given text to the clipboard using the OSC 52 protocol.

#### void updateBackgroundColor()
Synchronizes the host terminal emulator's window background color via OSC 11 when the root window defines an opaque background in full-screen mode, or restores it via OSC 111 when in inline mode, transparent, or shutting down.

#### static std::string osc11Sequence(const Color& color)
Generates the ANSI escape sequence (`\033]11;#rrggbb\a`) for setting the terminal background color using the OSC 11 protocol.

#### static std::string osc111Sequence()
Generates the ANSI escape sequence (`\033]111\a`) for resetting the terminal background color back to the terminal default using the OSC 111 protocol.

## 11. Ownership and Lifecycle

`TerminalEngine` is owned by `Terminal` or parent `QObject`. On destruction, it restores standard terminal POSIX/Windows mode and shows the cursor.

## 12. Thread Safety

Single-threaded. Runs on the main Qt event loop thread.

## 14. Inter-Class Interactions

- **`TerminalDevice`**: Manages low-level TTY raw mode and stdout writes.
- **`InputDecoder`**: Receives stdin bytes from `QSocketNotifier` and emits key/mouse callbacks.
- **`Terminal` / `Item`**: Dispatches key events (`keyPressEvent`) and mouse events (`hitTestItem`, `mousePressEvent`).

## 15. External Communication

- **POSIX Signals**: Captures `SIGWINCH` signal on POSIX to handle terminal window resizing.
- **Standard I/O**: Reads `stdin` asynchronously via `QSocketNotifier` and writes ANSI buffer sequences to `stdout`.

## 16. Usage Example

```cpp
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QCoreApplication>

using namespace qmltui;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    Terminal rootWindow;
    TerminalEngine engine(&rootWindow);

    rootWindow.setWidth(engine.terminalWidth());
    rootWindow.setHeight(engine.terminalHeight());

    return app.exec();
}
```

## Terminal protocol negotiation and resource output

At the first renderer start of each enabled output session, the engine queries
Kitty keyboard flags, cell pixel size, synchronized output, and primary device
attributes. The Kitty graphics probe is an APC string that a terminal without
APC parsing prints, so it is sent only to terminals known to implement the
protocol (kitty, Ghostty, WezTerm, Konsole, by `TERM_PROGRAM`, `TERM` or their
own variables). Nothing is queried unless both the output and the input are the
terminal: a pipe on either end would receive the probes or leave the replies for
the shell. Recognized replies never become keys. DA1 closes negotiation; a
100 ms timer handles terminals that do not reply. `QMLTUI_QUERY_TIMEOUT_MS`
selects a positive timeout in milliseconds. Late replies are consumed without
changing closed negotiation results. A later negotiation in the same process
(renderer or output-session change) starts from the capabilities already
proven, so a timeout never downgrades them. Resize starts a separate 100 ms
cell-size query. A probe write that fails is reported once, through the
frame that fails with it, and retried on the next frame.

Environment overrides live in `TerminalCapabilities::applyEnvironmentOverrides()`
and are applied to the detected defaults and again after replies, so they win
over what the terminal claims: `QMLTUI_COLOR_DEPTH`, `QMLTUI_SYNC_OUTPUT`,
`QMLTUI_KITTY_KEYBOARD` and `QMLTUI_KITTY_GRAPHICS`, where `0`, `false` and `off`
disable.

Colour depth is derived from `COLORTERM`, `TERM` and `TERM_PROGRAM`. Windows
Terminal (detected by `WT_SESSION`/`WT_PROFILE_ID`, or `WSL_INTEROP`/
`WSL_DISTRO_NAME` under WSL) is treated as true colour even when `TERM` only
advertises 256, because otherwise desaturated true colours quantise to the
256-cube grey ramp and warm tones render as grey. `QMLTUI_COLOR_DEPTH=256`
overrides this.
`setCapabilities()` is a deterministic programmatic override: it cancels pending
negotiation and suppresses future probes for that engine. Protocol detection
alone neither pushes keyboard modes nor uploads images.

The C++ `Terminal::enqueueControlSequence(QByteArray)` hook accepts an entire
resource command batch, up to the remaining space in a 4 MiB engine queue. It
returns false when stopped, output is disabled, or the batch exceeds the budget.
Callers must preserve cursor position and text rendition state and must submit
all upload chunks and their placement together to obtain atomic acceptance.
Commands request rendering and precede the same frame's cells, including when
enqueued during paint. Command-only frames are emitted. Pending commands are
flushed before renderer replacement, output disable, and stop.

`outputSessionGeneration()` and `outputSessionInvalidated(generation)` let
resource owners invalidate terminal-side handles on output lifecycle changes or
write failure. Queue acceptance is not a terminal acknowledgement. Immediate
frame failure and asynchronous output-drain failure report `renderFailed`; the
latter discards pending output and invalidates resource handles. `takeControlSequences()`
is the renderer integration drain and must not be used by application code.
