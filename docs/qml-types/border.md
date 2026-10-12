---
title: Border
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Border
category: core
summary: Grouped property configuring the border style and colour of a Rectangle.
---

# Border QML type

Grouped property configuring the border style and colour of a Rectangle. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |
| **Instantiation** | Not instantiable; use as an attached property |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `style` | `enumeration` | `Border.None` | Holds the border style: `Border.None`, `Border.Single` (single-line), `Border.Double` (double-line), `Border.Rounded` (single-line with curved corners `╭`, `╮`, `╰`, `╯`), or `Border.Ascii` (`+`, `-`, `|`). |
| `color` | `rgba` | `"default"` | Holds the colour of the border glyphs. |

## Detailed Description

`Border` is a grouped property available on [Rectangle](rectangle.md). It
controls the box-drawing characters used to frame the item.

Supported styles via the `Border.Style` enumeration are:
- `Border.None`: no border is rendered.
- `Border.Single`: standard box-drawing characters (`┌`, `┐`, `└`, `┘`, `─`, `│`).
- `Border.Double`: double-line box-drawing characters (`╔`, `╗`, `╚`, `╝`, `═`, `║`).
- `Border.Rounded`: single-line border with curved corner glyphs (`╭`, `╮`, `╰`, `╯`).
- `Border.Ascii`: ASCII fallback characters (`+`, `-`, `|`).

### Example

```qml
import QtQml
import Qamui

Terminal {
    Rectangle {
        anchors.centerIn: parent
        width: 20
        height: 6
        border.style: Border.Rounded
        border.color: "cyan"
    }
}
```
