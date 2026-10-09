---
title: Border
kind: qml
import: Qtui
since: "0.4"
inherits: 
cpp: qtui::Border
category: core
summary: Grouped property configuring the border style and colour of a Rectangle.
---

# Border QML type

Grouped property configuring the border style and colour of a Rectangle. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not instantiable; use as an attached property |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `style` | `string` | `"none"` | Holds the border style glyph set: `"none"`, `"single"` (single-line), `"double"` (double-line), `"ascii"` (`+`, `-`, `|`), or `"rounded"` (single-line with curved corners `╭`, `╮`, `╰`, `╯`). |
| `color` | `rgba` | `"default"` | Holds the colour of the border glyphs. |

## Detailed Description

`Border` is a grouped property available on [Rectangle](rectangle.md). It
controls the box-drawing characters used to frame the item.

Supported styles are:
- `"none"`: no border is rendered.
- `"single"`: standard box-drawing characters (`┌`, `┐`, `└`, `┘`, `─`, `│`).
- `"double"`: double-line box-drawing characters (`╔`, `╗`, `╚`, `╝`, `═`, `║`).
- `"ascii"`: ASCII fallback characters (`+`, `-`, `|`).
- `"rounded"`: single-line border with curved corner glyphs (`╭`, `╮`, `╰`, `╯`).

### Example

```qml
import QtQml
import Qtui

Terminal {
    Rectangle {
        anchors.centerIn: parent
        width: 20
        height: 6
        border.style: "rounded"
        border.color: "cyan"
    }
}
```
