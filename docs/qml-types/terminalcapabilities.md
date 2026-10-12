---
title: terminalCapabilities
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::TerminalCapabilities
category: core
summary: Describes capabilities detected from the host terminal environment.
---

# terminalCapabilities Value type

Describes capabilities detected from the host terminal environment. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |
| **Instantiation** | Value type, not instantiable |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `kittyKeyboard` | `bool` |  | Read-only. Holds whether the terminal supports the Kitty keyboard protocol with key disambiguation. |
| `kittyGraphics` | `bool` |  | Read-only. Holds whether the terminal supports true-pixel graphics via the Kitty graphics protocol. |
| `sixelGraphics` | `bool` |  | Read-only. Holds whether the terminal supports raster graphics via the Sixel protocol. |
| `cellPixelSize` | `size` |  | Read-only. Holds the pixel dimensions (width x height) of a single character cell, or invalid if unknown. |
| `syncOutput` | `bool` |  | Read-only. Holds whether the terminal supports synchronized output (DEC private mode 2026) for tear-free frame updates. |

## Detailed Description

`terminalCapabilities` is a read-only value type exposed through [Terminal.capabilities](terminal.md).
During application startup, Qamui inspects environment variables (`TERM`, `TERM_PROGRAM`,
`COLORTERM`) and issues asynchronous VT query probes to determine host terminal features.

Applications use these flags to enable rich features conditionally, such as true-pixel
image rendering via Kitty graphics or fine-grained key event detection.

### Example

```qml
import QtQml
import Qamui

Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "Kitty Keyboard: " + (root.capabilities.kittyKeyboard ? "Yes" : "No")
        }
        Text {
            text: "Kitty Graphics: " + (root.capabilities.kittyGraphics ? "Yes" : "No")
        }
        Text {
            text: "Sync Output:    " + (root.capabilities.syncOutput ? "Yes" : "No")
        }
    }
}
```
