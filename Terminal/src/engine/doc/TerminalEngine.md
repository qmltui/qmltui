# TerminalEngine

## 1. Class Overview

`TerminalEngine` is the central runtime coordinator for Qtui applications. It initializes raw terminal I/O via `TerminalDevice`, hooks into Qt's event loop via `QSocketNotifier` for non-blocking stdin input decoding (`InputDecoder`), manages window resize signals (`SIGWINCH`), dispatches keyboard (`KeyEvent`) and mouse events (`MouseEvent`), and manages off-screen double-buffered frame rendering (`Renderer`).

Developers interact with `TerminalEngine` when setting up top-level application windows or requesting manual frame renders.

## 2. Project Structure and Dependencies

- **Header**: `Tui/src/engine/TerminalEngine.h`
- **Implementation**: `Tui/src/engine/TerminalEngine.cpp`
- **Dependencies**: `TerminalDevice.h`, `InputDecoder.h`, `Renderer.h`, `Terminal.h`, `Item.h`.
- **Build Requirements**: Links against the `Qtui::Core` library target.

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

## 11. Ownership and Lifecycle

`TerminalEngine` is owned by `Terminal` or parent `QObject`. On destruction, it restores standard terminal POSIX mode and shows the cursor.

## 12. Thread Safety

Single-threaded. Runs on the main Qt GUI event loop thread.

## 14. Inter-Class Interactions

- **`TerminalDevice`**: Manages low-level TTY raw mode and stdout writes.
- **`InputDecoder`**: Receives stdin bytes from `QSocketNotifier` and emits key/mouse callbacks.
- **`Terminal` / `Item`**: Dispatches key events (`keyPressEvent`) and mouse events (`hitTestItem`, `mousePressEvent`).

## 15. External Communication

- **POSIX Signals**: Captures `SIGWINCH` signal to handle terminal window resizing.
- **Standard I/O**: Reads `stdin` (fd 0) asynchronously via `QSocketNotifier` and writes ANSI buffer sequences to `stdout` (fd 1).

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
