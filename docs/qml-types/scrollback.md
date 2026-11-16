---
title: Scrollback
kind: qml
import: QmlTui
since: "0.4"
inherits: 
cpp: qmltui::Scrollback
category: core
summary: Manages committing rendered output lines into the terminal's native scrollback buffer.
---

# Scrollback QML type

Manages committing rendered output lines into the terminal's native scrollback buffer. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | none |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `model` | `var` |  | Holds the data model driving scrollback entries. |
| `delegate` | `Component` |  | Holds the visual component template instantiated for each model row. This is the default property. |
| `spacing` | `int` | `0` | Holds the number of blank separator rows inserted between successive entries. |
| `separator` | `Component` |  | Holds an optional component rendered as a horizontal separator line between entries. |
| `reuseItems` | `bool` | `true` | Whether delegate instances are pooled and reused across sequential entries. |

## Signals

| Signal | Description |
| :--- | :--- |
| `outputRejected(string reason)` | Emitted when an output batch exceeds queue capacity or is refused. |

## Methods

| Method | Description |
| :--- | :--- |
| `void append(var data)` | Renders the delegate using the provided data object and commits it to scrollback. |
| `void append(Item item)` | Renders and commits a snapshot of the given Item into scrollback. |
| `void append(span span)` | Renders a styled span (`{ text, color, ... }` or `QmlTui.span()`) and commits it to scrollback (honoring `spacing` and `separator`). |
| `void append(list<span> spans)` | Renders a list of styled spans as a single entry and commits it to scrollback. |
| `bool print(var value)` | Appends `value` directly into the terminal scrollback buffer, bypassing `delegate`, `spacing`, and `separator`. Accepts a plain `string`, a `span` (`QmlTui.span()`), a `{ text, color, ... }` style map, or an array of spans or style maps. Returns `true` on success. |
| `void clear()` | Resets the internal delegate cache and consumed position. |

## Detailed Description

Scrollback is a non-visual element used in `Terminal.Inline` mode to emit
rendered content directly into the terminal emulator's native scrollback buffer.
This enables CLI tools and interactive REPLs to log previous outputs that
remain accessible in the user's terminal history above the active inline interactive UI.

Accepted scrollback belongs to the terminal session with a 1 MiB UTF-8 queue.
In full-screen mode, scrollback output is queued until inline mode resumes.

### Example

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root
    property int stepCount: 1

    mode: Terminal.Inline
    title: "Inline Scrollback Demo"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Scrollback {
        id: history
    }

    Component.onCompleted: {
        history.print([
            { text: "✔ ", color: "green", bold: true },
            { text: "QmlTui Inline Mode initialized in normal terminal buffer" }
        ]);
    }

    Panel {
        width: parent.width
        height: 7
        title: Text {
            text: "Inline Terminal · Scrollback"
            color: "cyan"
            bold: true
        }
        border {
            style: Border.Rounded
            color: "cyan"
        }

        ColumnLayout {
            anchors {
                fill: parent
                leftMargin: 1
                rightMargin: 1
            }
            spacing: 1

            Text {
                text: "Commits lines permanently above the live inline region:"
                dim: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 2

                TextInput {
                    id: logInput
                    Layout.fillWidth: true
                    focus: true
                    text: "Step #" + root.stepCount + " completed"
                    placeholderText: "Message to write to scrollback..."
                    onAccepted: commitBtn.clicked()
                }

                Button {
                    id: commitBtn
                    text: "Write to Scrollback"
                    onClicked: {
                        history.print([
                            { text: "[#" + root.stepCount + "] ", color: "green", bold: true },
                            { text: logInput.text }
                        ]);
                        root.stepCount += 1;
                        logInput.text = "Step #" + root.stepCount + " completed";
                    }
                }
            }
        }
    }
}
```

### Recipe Example

![Inline REPL recorded demo](../assets/casts/components/inline_mode.gif)

See [`examples/recipes/inline_repl/Main.qml`](../../examples/recipes/inline_repl/Main.qml) (`./build/examples/recipes/inline_repl/qmltui_inline_repl_demo`).
