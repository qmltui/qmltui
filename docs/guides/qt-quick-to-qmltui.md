# QmlTui for Qt Quick Developers

QmlTui uses Qt's QML language (`QtQml`), property bindings, and `QAbstractItemModel` views, but renders to a character-cell grid rather than a pixel scene graph. This guide summarizes the practical differences when moving from `QtQuick` to `QmlTui`.

## Imports: Do Not Import `QtQuick`

QmlTui is built on `QtQml` and registers its own `Item`, `Rectangle`, `Text`, `MouseArea`, `Loader`, layouts, and controls. Do not import `QtQuick` or `QtQuick.Controls`: importing `QtQuick` pulls in a GUI runtime dependency and collides with QmlTui's visual types.

```qml
import QtQml
import QmlTui
import QmlTui.Controls
```

Note that `import QmlTui` does not re-export `QtQml`; add `import QtQml` when using `Timer`, `Component`, `QtObject`, `Connections`, or `Binding`, and `import QtQml.Models` when using `ListModel` or `DelegateModel`.

## Integer Cell Geometry

In `QtQuick`, coordinates and sizes are floating-point pixel values (`qreal`). In QmlTui:

- Every coordinate (`x`, `y`) and dimension (`width`, `height`, `spacing`, `margins`, `padding`) is an integer number of terminal character cells (columns and rows).
- Terminal cells are roughly twice as tall as they are wide (for example, an 80×24 viewport), so horizontal and vertical cell counts are not visually square.

```qml
Terminal {
    Rectangle {
        x: 2
        y: 1
        width: 30
        height: 5
        border.style: Border.Rounded
    }
}
```

## Box-Drawing Borders

Instead of pixel-width strokes, `Rectangle.border` draws Unicode or ASCII box-drawing characters around the outer 1-cell perimeter of the rectangle:

```qml
Rectangle {
    border {
        style: Border.Single
        color: "cyan"
    }
}
```

| Style | Glyphs |
| :--- | :--- |
| `Border.Single` (`"single"`) | `┌───┐` / `│   │` / `└───┘` |
| `Border.Double` (`"double"`) | `╔═══╗` / `║   ║` / `╚═══╝` |
| `Border.Rounded` (`"rounded"`) | `╭───╮` / `│   │` / `╰───╯` |
| `Border.Heavy` (`"heavy"`) | `┏━━━┓` / `┃   ┃` / `┗━━━┛` |
| `Border.Ascii` (`"ascii"`) | `+---+` / `|   |` / `+---+` |
| `Border.None` (`"none"`) | No border drawn |

A border occupies 1 cell on each edge: a 10×5 bordered `Rectangle` has an 8×3 interior. `TableView` draws connected table grid intersections through `showOuterBorder`, `showColumnGrid`, `showRowGrid`, and `gridStyle` rather than `Rectangle.border`.

## Colors and Styling

QmlTui does not use `QColor` or a global theme singleton (such as `Material` or `Palette`):

- Colors use `qmltui::Color` (`rgba` in QML). Items and controls default to `"default"` (the host terminal's foreground and background).
- Named ANSI colors (`"cyan"`, `"red"`, `"yellow"`, `"gray"`, etc.) emit palette SGR indices so the user's terminal theme controls their appearance.
- Hex strings (`"#2cde85"`) and `QmlTui.rgba(r, g, b, a)` specify 24-bit RGB colors, which QmlTui quantizes automatically when running on 256-color or 16-color terminals.
- Shared styling is expressed with QML property bindings and reusable QML components; see [Styling and Colors](styling-and-colors.md).

## Keyboard Quit Path

Desktop windows have a window-manager close button; terminal applications in raw mode do not, and `Ctrl+C` is used by `TextInput`/`TextArea` for clipboard copy rather than `SIGINT`. A root `Terminal` never exits on `Q` or `Escape` unless the application handles it:

```qml
Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }
}
```

When modals, popups, or filter inputs are present, consume `Escape` to close or clear them first, and call `Qt.quit()` only when nothing else handled the key.

## Models and Delegates

`ListView`, `TableView`, `TreeView`, and `GridView` bind to `QAbstractItemModel` subclasses and `ListModel`. Start QML files with `pragma ComponentBehavior: Bound` and declare model roles as `required property` on the delegate:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

ListView {
    anchors.fill: parent
    model: processModel

    delegate: Item {
        required property string processName
        required property int cpuUsage
        required property int index

        width: ListView.view.width
        height: 1

        Text {
            text: processName + " (" + cpuUsage + "%)"
            color: index % 2 === 0 ? "white" : "gray"
        }
    }
}
```

## Quick Reference

| Qt Quick | QmlTui | Notes |
| :--- | :--- | :--- |
| `import QtQuick` | `import QtQml` + `import QmlTui` | No `QtQuick` or `QtGui` requirement in Core |
| `Item { width: 100.0 }` | `Item { width: 10 }` | Integer character cells |
| `Rectangle { border.width: 1 }` | `Rectangle { border.style: Border.Single }` | 1-cell box-drawing border |
| `font.pixelSize: 14` | `bold`, `dim`, `italic`, `underline` | Terminal controls glyph size; use SGR attributes |
| `Qt.rgba(...)` / `QColor` | `QmlTui.rgba(...)` / `"#rrggbb"` / `"cyan"` | `qmltui::Color` (`rgba` value type) |
| Window close button | Root `Keys.onPressed` / `Shortcut` | Explicit keyboard quit path required |
