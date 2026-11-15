# Architecture

QmlTui is a retained-mode layout and rendering engine for terminal applications built on `Qt6::Core` and `Qt6::Qml`. It evaluates a QML item tree into a character-cell grid, diffs consecutive frames, and writes minimal ANSI escape sequences to the terminal while decoding raw terminal input into Qt events.

## Core Loop

Execution runs on the standard Qt event loop (`QCoreApplication`). `TerminalEngine` bridges the event loop and the terminal device.

The root `Terminal` initializes its engine after QML component completion, when parentage and presentation mode are known. C++ applications can initialize it earlier via `engine()`; otherwise initialization runs on the next event-loop turn. The engine connects the rendering lifecycle to terminal I/O and survives presentation-mode switches.

## Rendering Pipeline

Because the component tree stays resident in memory, property bindings mark only affected items dirty rather than rebuilding the UI on every tick. The `Renderer` interface drives terminal output through two implementations:

1. **`AltScreenRenderer`**: Maintains a full-viewport frame in the alternate screen buffer (`\033[?1049h`), diffs consecutive `TextGridBuffer` frames cell by cell, and emits minimal ANSI/SGR updates inside a DEC `?2026` synchronized frame.
2. **`InlineRenderer`**: Keeps the session in the normal screen buffer, commits completed output above the viewport, and redraws only the live bottom region using relative cursor movement.

`Terminal.mode` selects `AltScreen` or `Inline`; ordinary visible root children form the UI in either mode. `Scrollback` submits output to a bounded session queue independently of the active renderer. Inline commits combine pending scrollback output and the live region in a single write. `AltScreen` mode retains queued scrollback output until inline mode resumes or shutdown restores the normal screen.

On each dirty frame, the engine runs a layout pass over the item tree, computes bounds and clip rectangles, and calls each item's `paint()` method to write fixed-size `Cell` values (UTF-8 grapheme bytes, foreground/background colors, and SGR attributes) into `TextGridBuffer`. Grapheme clusters up to 11 bytes are stored inline inside each 24-byte `Cell`; longer clusters (such as multi-person ZWJ emoji sequences) are interned in a process-wide append-only table for the lifetime of the process.

## Signal Handling, Job Control, and Crash Recovery

While a session is in raw mode, `PosixTerminalBackend` installs async-signal-safe handlers so abnormal termination or job suspension never leaves the user's shell in raw mode or the alternate screen:

- **Fatal and termination signals (`SIGINT`, `SIGTERM`, `SIGHUP`, `SIGQUIT`, `SIGABRT`, `SIGSEGV`, `SIGBUS`, `SIGFPE`, `SIGILL`)**: Pops any active Kitty keyboard protocol stack entry (`\033[<u`), disables mouse and bracketed-paste reporting, shows the cursor, resets SGR attributes and terminal colors, exits the alternate screen (`\033[?1049l`), restores the original `termios` state (`tcsetattr`), and re-raises the signal with `SIG_DFL`.
- **Job control (`SIGTSTP` and `SIGCONT`)**: Restores cooked terminal state before stopping on `SIGTSTP` (`Ctrl+Z`), and re-enters raw mode and repaints the screen via a self-pipe bridge on `SIGCONT`.

## Terminal-Safe Qt Diagnostics

`TerminalEngine` installs a process-wide Qt message router (`QtMessageRouting`) while terminal engines exist. While any alternate-screen session is active and `stderr` points to an interactive TTY, non-fatal Qt warnings and `console.log()` messages are buffered in memory so `stderr` writes cannot corrupt ANSI cursor positioning, and replayed on exit. When `stderr` is redirected (`./app 2> debug.log`), when `QMLTUI_LOG_FILE=<path>` is set, or when C++ code registers a sink with `QtMessageRouting::install()`, diagnostics stream immediately. Fatal Qt messages are forwarded immediately before aborting.

## Input Handling and Item Graph

`TerminalDevice` reads raw byte streams from standard input (or the platform console backend) and forwards them to `TerminalEngine`. `InputDecoder` parses UTF-8 text, CSI/SS3 key sequences, Kitty keyboard protocol reports, bracketed paste blocks, and SGR mouse reports into `KeyEvent` and `MouseEvent` objects dispatched along the active focus chain or to the topmost `MouseArea`.

Visual elements inherit from `qmltui::Item`. Each `Item` tracks integer cell geometry (`x`, `y`, `width`, `height`, `implicitWidth`, `implicitHeight`), visibility, focus policy, and child stacking. Property changes call `markDirty()`, scheduling an asynchronous frame on `TerminalEngine`.

## Module Boundaries

QmlTui separates the core scene graph (`qmltui::core`) from interactive controls (`qmltui::controls`) and optional OS/GUI integrations (`qmltui::image` and `qmltui::pty_view`; see [Build Configuration and Installation](../build-configuration.md) for CMake targets and flags).

`QmlTui.Controls` only admits controls that need engine knowledge an application cannot reproduce in QML (such as `ToolTip` overlay positioning or grapheme measurement) or that are standard across terminal toolkits. Domain-specific widgets (`Calendar`, `StatusBadge`, `BigText`, syntax highlighters) live as copyable recipes under [`examples/recipes/`](../../examples/recipes/).

Because `qmltui::core` includes `Rectangle`, `Text`, `TextInput`, `MouseArea`, `Keys`, and layouts, an application can link only `qmltui::core` and define its own controls in QML:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root

    component CoreButton: Rectangle {
        id: button
        property string label: "Submit"
        signal clicked()

        implicitWidth: labelItem.implicitWidth + 4
        implicitHeight: 3
        focusPolicy: Qt.StrongFocus
        focus: true
        border {
            color: activeFocus ? "cyan" : "gray"
            style: Border.Rounded
        }

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Space) {
                button.clicked();
                event.accepted = true;
            }
        }

        Text {
            id: labelItem
            anchors.centerIn: parent
            text: button.label
            bold: button.activeFocus
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                button.focus = true;
                button.clicked();
            }
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        TextInput {
            id: input
            width: 30
            placeholderText: "Type something..."
            focus: true
        }

        CoreButton {
            label: "Submit"
            onClicked: statusText.text = "Entered: " + (input.text || "(empty)")
        }

        Text {
            id: statusText
            text: "Ready"
            dim: true
        }
    }
}
```
