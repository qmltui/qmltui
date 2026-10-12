---
title: rgba
kind: qml
import: Qamui
since: "0.4"
inherits: 
cpp: qamui::Color
category: core
summary: Terminal color value type supporting ANSI palette indices, RGB hex, and named colors.
---

# rgba Value type

Terminal color value type supporting ANSI palette indices, RGB hex, and named colors. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qamui` |
| **Since** | Qamui 0.4 |
| **Inherits** | none |
| **Instantiation** | Value type, not instantiable |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `r` | `real` |  | Read-only. Holds the red channel component normalized between `0.0` and `1.0`. |
| `g` | `real` |  | Read-only. Holds the green channel component normalized between `0.0` and `1.0`. |
| `b` | `real` |  | Read-only. Holds the blue channel component normalized between `0.0` and `1.0`. |
| `a` | `real` |  | Read-only. Holds the alpha opacity component normalized between `0.0` and `1.0`. |
| `valid` | `bool` |  | Read-only. Holds whether the color value represents a valid parsed color. |

## Detailed Description

`rgba` is the foundational color value type in Qamui. In QML, string literals
automatically convert to `rgba`, supporting:

- Standard named terminal ANSI colors (`"black"`, `"red"`, `"green"`, `"yellow"`,
  `"blue"`, `"magenta"`, `"cyan"`, `"white"`, `"gray"`). These preserve the terminal's
  own user-configured color scheme.
- `"default"`: The host terminal's default foreground (SGR 39) or background (SGR 49).
- `"transparent"`: No ink is painted, allowing underlying content to show through.
- Hexadecimal formats (`"#RGB"`, `"#RRGGBB"`, `"#AARRGGBB"`).

Construct color values dynamically using [Qamui.rgba()](qamui.md).

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
        width: 25
        height: 5
        color: "#2563eb"
        border.style: Border.Single
        border.color: "white"

        Text {
            anchors.centerIn: parent
            text: "Blue Background"
            color: "white"
        }
    }
}
```
