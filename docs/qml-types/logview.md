---
title: LogView
kind: qml
import: Qamui
since: "0.4"
inherits: TextArea
cpp: qamui::LogView
category: data views
summary: Streaming log viewer with history buffer trimming and auto-scroll to tail.
---

# LogView QML type

Streaming log viewer with history buffer trimming and auto-scroll to tail. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | [TextArea](textarea.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `maximumLineCount` | `int` | `10000` | Holds the maximum number of lines retained in the buffer before older lines are trimmed. |
| `followTail` | `bool` | `true` | Whether the view automatically scrolls downward as new lines are appended. |

## Methods

| Method | Description |
| :--- | :--- |
| `void appendLine(string line)` | Appends a single line of log text to the buffer. |
| `void appendText(string text)` | Appends arbitrary text to the buffer, which may contain newlines. |
| `void clear()` | Clears all log lines from the buffer. |
| `void copySelection()` | Copies currently highlighted text to the clipboard. |

## Detailed Description

LogView is a specialized, read-only [TextArea](textarea.md) optimized for viewing
continuous log streams, process outputs, and console messages.

It enforces a bounded memory footprint through `maximumLineCount`, automatically
discarding older lines when the limit is exceeded. When `followTail` is `true`,
the scroll offset stays anchored at the bottom as new lines arrive, but temporarily
pauses auto-scrolling if the user manually scrolls up to inspect history.

### Example

```qml
import QtQml
import Qamui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 40
        height: 8
        border.style: Border.Single

        LogView {
            id: logger
            anchors.fill: parent
            anchors.margins: 1
            followTail: true
            maximumLineCount: 1000

            Component.onCompleted: {
                appendLine("[INFO] Application starting...")
                appendLine("[INFO] Subsystems initialized.")
                appendLine("[DEBUG] Ready for input.")
            }
        }
    }
}
```
