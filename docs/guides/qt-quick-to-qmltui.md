# QmlTui for Qt Quick and Widgets Developers

Think of **QmlTui** as the **Qt Quick equivalent for terminal user interfaces**: you express the UI in declarative, reactive QML (`QtQml` property bindings, states, signals, and reusable components) and build the backend in C++ (`QObject`, `QML_ELEMENT`, `QAbstractItemModel`, and `QSortFilterProxyModel`), rendering to a character-cell grid rather than a pixel scene graph. This guide explains how `QtQuick` and `QtWidgets` concepts map to `QmlTui`.

## Quick Reference: Qt Quick and Widgets vs. QmlTui

| Concept | Qt Quick / Qt Widgets | QmlTui | Key Difference |
| :--- | :--- | :--- | :--- |
| **Application class** | `QGuiApplication` / `QApplication` | `QCoreApplication` | No `QtGui` or display server required for `qmltui::core` and `qmltui::controls` |
| **QML imports** | `import QtQuick` / `import QtQuick.Controls` | `import QtQml`, `import QmlTui`, `import QmlTui.Controls` | Replaces `import QtQuick` for terminal UIs; alias if both are imported in one file |
| **Root item** | `Window` / `ApplicationWindow` / `QMainWindow` | `Terminal` | Selects `Terminal.AltScreen` (full-screen) or `Terminal.Inline` (CLI scrollback) |
| **Geometry** | Floating-point pixels (`qreal`) | Integer character cells (`int`) | `x`, `y`, `width`, `height`, `spacing`, and `padding` are columns and rows |
| **Borders** | `Rectangle { border.width: 1 }` | `Rectangle { border.style: Border.Single }` | Draws Unicode or ASCII box-drawing glyphs in the outer 1-cell perimeter |
| **Typography** | `font.pixelSize`, `font.family` | `bold`, `dim`, `italic`, `underline` | Host terminal controls font size; QmlTui controls SGR cell attributes |
| **Colors** | `QColor` / `Qt.rgba(...)` / `QPalette` | `"default"` / `"cyan"` / `"#rrggbb"` / `QmlTui.rgba(...)` | Uses `qmltui::Color` (`color`); defaults to the host terminal's palette |
| **Application exit** | Window manager close button (`[X]`) | Explicit `Keys.onPressed` or `Shortcut` calling `Qt.quit()` | Raw-mode terminals do not have a close button; `Ctrl+C` copies in `TextInput` |
| **Item models** | `QAbstractItemModel` + `QStyledItemDelegate` | `ListView`, `TableView`, `TreeView`, `GridView` | Supports both QML `required property` roles and Qt Widgets numeric styling roles |

## Application Entry Point and Shared C++ Backends

A QmlTui application runs on `QCoreApplication` and `QQmlApplicationEngine`:

```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("MyApp", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    return app.exec();
}
```

Because `qmltui::core` and `qmltui::controls` depend only on `Qt6::Core` and `Qt6::Qml`, you can compile the same C++ `QObject` controllers, `QAbstractItemModel` subclasses, and `QSortFilterProxyModel` pipelines into both a desktop GUI executable (`QtQuick` or `QtWidgets`) and a headless `QmlTui` executable for SSH, serial consoles, or container environments. See [`examples/recipes/standard_item_delegate/`](../../examples/recipes/standard_item_delegate/) for a CMake project that renders the same C++ `ReleaseChecklistModel` in both `QmlTui` views and `QtWidgets` views (`QListView`, `QTableView`, `QTreeView`).

## QML Imports

QmlTui is built on `QtQml` and provides its own `Item`, `Rectangle`, `Text`, `MouseArea`, `Loader`, layouts, views, and controls. In QmlTui files, replace `import QtQuick` and `import QtQuick.Controls` with `import QtQml`, `import QmlTui`, and `import QmlTui.Controls`:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls
```

`import QmlTui` does not re-export `QtQml`. Add `import QtQml` when using `Timer`, `Component`, `QtObject`, `Connections`, or `Binding`, and `import QtQml.Models` when using `ListModel` or `DelegateModel`. (If you ever import both `QtQuick` and `QmlTui` in the same QML document, alias one of them, such as `import QmlTui as Tui` or `import QtQuick as Q`, so shared type names like `Item`, `Rectangle`, and `Text` do not collide.)

## Integer Cell Geometry

In `QtQuick`, coordinates and sizes are floating-point pixel values (`qreal`). In QmlTui:

- Every coordinate (`x`, `y`) and dimension (`width`, `height`, `spacing`, `margins`, `padding`) is an integer number of terminal character cells (columns and rows).
- Terminal cells are roughly twice as tall as they are wide (for example, an 80×24 viewport), so horizontal and vertical cell counts are not visually square.
- `anchors` (`anchors.fill`, `anchors.centerIn`, `anchors.left`, `anchors.top`, etc.), positioners (`Row`, `Column`), and layouts (`RowLayout`, `ColumnLayout`, `GridLayout`, `StackLayout`) work like their `QtQuick` counterparts on integer cell bounds. See [Layouts and Cell Geometry](layouts.md).

```qml
import QtQml
import QmlTui

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

QmlTui does not use `QColor` or a global theme singleton (such as `Material` or `QPalette`):

- Colors use `qmltui::Color` (`color` in QML). Items and controls default to `"default"` (the host terminal's foreground and background).
- Bare CSS/SVG color names (`"red"`, `"blue"`, `"cyan"`, `"orange"`, etc.), hex strings (`"#2cde85"`), and `QmlTui.rgba(r, g, b, a)` specify 24-bit RGB colors, which QmlTui quantizes automatically when running on 256-color or 16-color terminals.
- Explicit `"ansi:..."` colors (`"ansi:cyan"`, `"ansi:red"`, `"ansi:0"`..`"ansi:15"`, or `QmlTui.ansi(index)`) emit palette SGR indices so the user's terminal theme controls their appearance.
- Controls (`Button`, `Panel`, `Dialog`, etc.) expose `background` and `contentItem` delegates for customization, and shared application styling is expressed with QML property bindings and reusable QML components. See [Styling and Colors](styling-and-colors.md).

## Keyboard Quit Path

Desktop windows have a window-manager close button; terminal applications in raw mode do not, and `Ctrl+C` is used by `TextInput` and `TextArea` for clipboard copy rather than `SIGINT`. A root `Terminal` never exits on `Q` or `Escape` unless the application handles it:

```qml
import QtQml
import QmlTui

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

When modals, popups, or filter inputs are present, consume `Escape` to close or clear them first, and call `Qt.quit()` only when nothing else handled the key. See [Input and Focus](input-and-focus.md).

## Models, Delegates, and Qt Widgets Roles

`ListView`, `TableView`, `TreeView`, and `GridView` bind directly to `QAbstractItemModel` subclasses, `QSortFilterProxyModel`, and `ListModel`.

### Required-Property Delegates (Qt Quick Style)

Start QML files with `pragma ComponentBehavior: Bound` and declare model roles as `required property` on the delegate:

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

### Standard Numeric Roles (Qt Widgets Style)

In `QtWidgets`, models such as `QStandardItemModel` often provide `Qt::ForegroundRole`, `Qt::BackgroundRole`, `Qt::TextAlignmentRole`, `Qt::CheckStateRole`, `Qt::DecorationRole`, and `flags()` without custom `roleNames()`. While `QtQuick` views ignore those numeric roles, QmlTui views expose them automatically (`foreground`, `background`, `textAlignment`, `checkState`, `decoration`, `enabled`, `checkable`) and `TableView` / `TreeView` built-in painters render them out of the box. See [Models in QmlTui Views](models.md) for the full support matrix.
