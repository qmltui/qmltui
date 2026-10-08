---
title: Dialog
kind: qml
import: Qtui.Controls
since: "0.4"
inherits: Popup
cpp: qtui::Popup
category: overlays and navigation
summary: _TODO_
---

# Dialog QML type

_TODO_ one sentence. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui.Controls` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Popup](popup.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `textColor` | `rgba` | `"default"` | _TODO_ |
| `titleColor` | `rgba` |  | _TODO_ |
| `title` | `string` | `""` | _TODO_ |
| `message` | `string` | `""` | _TODO_ |
| `acceptText` | `string` |  | _TODO_ |
| `rejectText` | `string` |  | _TODO_ |
| `showRejectButton` | `bool` | `true` | _TODO_ |

## Detailed Description

_TODO_

### Example

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    Dialog {
        anchors.centerIn: parent
    }
}
```
