# TerminalEngine

## 1. Class Overview

`TerminalEngine` is the central runtime coordinator for Qtui applications. It initializes raw terminal I/O via `TerminalDevice`, hooks into Qt's event loop via `QSocketNotifier` for non-blocking stdin input decoding (`InputDecoder`), manages window resize signals (`SIGWINCH`), dispatches keyboard (`KeyEvent`) and mouse events (`MouseEvent`), and manages off-screen double-buffered frame rendering (`Renderer`).

Button events and button-held motion are enabled for drag interactions. Passive
mouse motion is enabled only while a component requests hover tracking, such as
a `MouseArea` with `hoverEnabled` or an attached tooltip with non-empty text.

Developers interact with `TerminalEngine` when setting up top-level application windows or requesting manual frame renders.

## 2. Project Structure and Dependencies

- **Header**: `Terminal/src/engine/TerminalEngine.h`
- **Implementation**: `Terminal/src/engine/TerminalEngine.cpp`
- **Dependencies**: `TerminalDevice.h`, `InputDecoder.h`, `Renderer.h`, `Terminal.h`, `Item.h`.

## 3. Class Hierarchy and Role

- **Base Class**: `QObject`
  - Leverages Qt slots, meta-object invocation, and event loop integration.

## 8. Public Slots and Q_INVOKABLE Methods

#### void requestRender() [Q_INVOKABLE]
Schedules an asynchronous frame re-render on the next event loop tick. Safe to call from QML or C++.

## 9. Public Methods

#### explicit TerminalEngine(Terminal* rootWindow, QObject* parent = nullptr)
Constructs the terminal engine instance bound to the given root `Terminal` window.

#### void setRenderer(Renderer* renderer)
Registers a custom frame renderer strategy.

#### int terminalWidth() const
Returns the current terminal window width in character columns.

#### int terminalHeight() const
Returns the current terminal window height in character rows.

#### void print(const QString& text)
Prints raw text to the terminal device.

#### void append(qtui::Item* item)
Appends a child item to the root window hierarchy.

#### void copyToClipboard(const QString& text)
Copies the given text to the system clipboard, utilizing OSC 52 sequences if configured.

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

Single-threaded. Runs on the main Qt GUI event loop thread.

## 14. Inter-Class Interactions

- **`TerminalDevice`**: Manages low-level TTY raw mode and stdout writes.
- **`InputDecoder`**: Receives stdin bytes from `QSocketNotifier` and emits key/mouse callbacks.
- **`Terminal` / `Item`**: Dispatches key events (`keyPressEvent`) and mouse events (`hitTestItem`, `mousePressEvent`).

## 15. External Communication

- **POSIX Signals**: Captures `SIGWINCH` signal on POSIX to handle terminal window resizing.
- **Standard I/O**: Reads `stdin` asynchronously via `QSocketNotifier` and writes ANSI buffer sequences to `stdout`.

## 16. Usage Example

```cpp
#include "Terminal.h"
#include "TerminalEngine.h"
#include <QGuiApplication>

using namespace qtui;

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    Terminal rootWindow;
    TerminalEngine engine(&rootWindow);

    rootWindow.setWidth(engine.terminalWidth());
    rootWindow.setHeight(engine.terminalHeight());

    return app.exec();
}
```
