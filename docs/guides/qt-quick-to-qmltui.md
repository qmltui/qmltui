# QmlTui for Qt Quick and Widgets Developers

**QmlTui** brings Qt's declarative QML programming model to terminal user interfaces on **Qt 6.8 through 6.12** (C++17, CMake 3.21+). Declare reactive layouts, `QtQml` property bindings, states, signals, and reusable components in QML and implement backend logic in C++ (`QObject`, `QML_ELEMENT`, `QAbstractItemModel`, and `QSortFilterProxyModel`), rendering to a character-cell grid rather than a pixel scene graph. This guide maps `QtQuick` and `QtWidgets` concepts to `QmlTui`.

## Quick Reference: Qt Quick and Widgets vs. QmlTui

| Concept | Qt Quick / Qt Widgets | QmlTui | Key Difference |
| :--- | :--- | :--- | :--- |
| **Application class** | `QGuiApplication` / `QApplication` | `QCoreApplication` | No `QtGui` or display server required for `qmltui::core` and `qmltui::controls` |
| **QML imports** | `import QtQuick` / `import QtQuick.Controls` | `import QmlTui`, `import QmlTui.Controls` | Replaces `import QtQuick` for terminal UIs (and re-exports `QtQml`); do not import `QtQuick` in QmlTui QML files |
| **Root item** | `Window` / `ApplicationWindow` / `QMainWindow` | `Terminal` | Selects `Terminal.AltScreen` (full-screen) or `Terminal.Inline` (CLI scrollback) |
| **Geometry** | Floating-point pixels (`qreal`) | Integer character cells (`int`) | `x`, `y`, `width`, `height`, `spacing`, and `padding` are columns and rows |
| **Borders** | `Rectangle { border.width: 1 }` | `Rectangle { border.style: Border.Single }` | Draws Unicode or ASCII box-drawing glyphs in the outer 1-cell perimeter |
| **Typography** | `font.pixelSize`, `font.family` | `bold`, `dim`, `italic`, `underline` | Host terminal controls font size; QmlTui controls SGR cell attributes |
| **Colors** | `QColor` / `Qt.rgba(...)` / `QPalette` | `"default"` / `"cyan"` / `"#rrggbb"` / `QmlTui.rgba(...)` | Uses `qmltui::Color` (`color`); defaults to the host terminal's palette |
| **Application exit** | Window manager close button (`[X]`) | Explicit `Keys.onPressed` or `Shortcut` calling `Qt.quit()` | Raw-mode terminals do not have a close button; `Ctrl+C` copies in `TextInput` |
| **Item models** | `QAbstractItemModel` + `QStyledItemDelegate` | `ListView`, `TableView`, `TreeView`, `GridView` | Supports both QML `required property` roles and Qt Widgets numeric styling roles |

### Controls and Containers Mapping

| Qt Quick / Qt Quick Controls | QmlTui Equivalent | Module | Notes |
| :--- | :--- | :--- | :--- |
| `Window` / `ApplicationWindow` | [`Terminal`](../qml-types/terminal.md) | `QmlTui` | Root terminal session (`Terminal.AltScreen` or `Terminal.Inline`) |
| `Flickable` / `ScrollView` | [`ScrollView`](../qml-types/scrollview.md) | `QmlTui` | Keyboard and mouse-wheel scrolling viewport |
| `SplitView` | [`RowLayout`](../qml-types/rowlayout.md) / [`ColumnLayout`](../qml-types/columnlayout.md) | `QmlTui` | Use `Layout.fillWidth` / `Layout.fillHeight` and `Layout.preferredWidth` |
| `Button`, `CheckBox`, `RadioButton` | [`Button`](../qml-types/button.md), [`CheckBox`](../qml-types/checkbox.md), [`RadioButton`](../qml-types/radiobutton.md) | `QmlTui.Controls` | Inherit from [`AbstractButton`](../qml-types/abstractbutton.md); group with [`ButtonGroup`](../qml-types/buttongroup.md) |
| `ComboBox`, `SpinBox`, `Slider` | [`ComboBox`](../qml-types/combobox.md), [`SpinBox`](../qml-types/spinbox.md), [`Slider`](../qml-types/slider.md) | `QmlTui.Controls` | Keyboard and mouse interactive value selectors |
| `ProgressBar`, `BusyIndicator` | [`ProgressBar`](../qml-types/progressbar.md), [`BusyIndicator`](../qml-types/busyindicator.md) | `QmlTui.Controls` | Determinate and indeterminate progress indicators |
| `TabBar`, `StackView`, `SwipeView` | [`TabBar`](../qml-types/tabbar.md), [`StackView`](../qml-types/stackview.md), [`StackLayout`](../qml-types/stacklayout.md) | `QmlTui.Controls` / `QmlTui` | Tabbed and stack-based page navigation |
| `Frame` / `GroupBox`, `Dialog`, `Popup`, `ToolTip` | [`Panel`](../qml-types/panel.md), [`Dialog`](../qml-types/dialog.md), [`Popup`](../qml-types/popup.md), [`ToolTip`](../qml-types/tooltip.md) | `QmlTui.Controls` / `QmlTui` | Framed containers, modal dialogs, and overlays |
| `Canvas` / `Shape` | [`Canvas`](../qml-types/canvas.md), [`Sparkline`](../qml-types/sparkline.md) | `QmlTui` / `QmlTui.Controls` | 2×4 Braille, 1×2 half-block, or cell-level drawing |

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

QmlTui is built on `QtQml` and provides its own `Item`, `Rectangle`, `Text`, `MouseArea`, `Loader`, layouts, views, and controls. In QmlTui files, replace `import QtQuick` and `import QtQuick.Controls` with `import QmlTui` and `import QmlTui.Controls`:

<!-- check-docs: ignore -->
```qml no-playground
pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls
```

Like `import QtQuick`, `import QmlTui` automatically re-exports `QtQml` (`Timer`, `Component`, `QtObject`, `Connections`, `Binding`). Add `import QtQml.Models` when using `ListModel` or `DelegateModel`. Do not import `QtQuick` inside a QmlTui QML file (`QQuickItem` and `qmltui::Item` are separate item hierarchies).

### Subtle API Differences to Keep in Mind

| Feature | Qt Quick | QmlTui |
| :--- | :--- | :--- |
| **`Shortcut` binding** | `Shortcut { sequence: "Ctrl+Q" }` | `Shortcut { sequence: "Ctrl+Q" }` or `sequence: Qt.ControlModifier \| Qt.Key_Q` (property is `sequence`, not `key`) |
| **Custom focusable `Item`/`Rectangle`** | `Item { focus: true; activeFocusOnTab: true }` | `Rectangle { focusPolicy: Qt.StrongFocus; focus: true }` (`Item` defaults to `Qt.NoFocus`) |
| **Specific-key `Keys` signals** | `Keys.onEscapePressed` pre-sets `event.accepted = true` | `event.accepted` starts `false`; set `event.accepted = true;` explicitly to stop propagation |
| **Animation easing & infinite loops** | `NumberAnimation { easing.type: Easing.OutQuad; loops: Animation.Infinite }` | `NumberAnimation { easing: Easing.OutQuad; loops: Animation.Infinite }` (`easing` is a direct enum property) |
| **`Behavior` & private Qt ABI** | Built into `QtQuick` | Enabled when `Qt6::QmlPrivate` is installed (`QMLTUI_ENABLE_PRIVATE_QT=ON`, tested across Qt 6.8 to 6.12), coupling shared `QmlTuiCore` binaries to that `Qt 6.x` minor release; pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` for public-Qt-only builds |
| **Imperative `Canvas` pixels** | `onPaint` callback | `Canvas { autoClear: false }` when calling `setPixel()` imperatively (`autoClear` defaults to `true` for shape children) |
| **`console.log()` in full-screen UI** | Writes to stderr immediately | Buffered during `Terminal.AltScreen` when `stderr` is a TTY; redirect `./app 2> debug.log` or set `QMLTUI_LOG_FILE=debug.log` for live streaming |

## Integer Cell Geometry

In `QtQuick`, coordinates and sizes are floating-point pixel values (`qreal`). In QmlTui:

- Every coordinate (`x`, `y`) and dimension (`width`, `height`, `spacing`, `margins`, `padding`) is an integer number of terminal character cells (columns and rows).
- Terminal cells are roughly twice as tall as they are wide (for example, an 80×24 viewport), so horizontal and vertical cell counts are not visually square.
- `anchors` (`anchors.fill`, `anchors.centerIn`, `anchors.left`, `anchors.top`, etc.), positioners (`Row`, `Column`), and layouts (`RowLayout`, `ColumnLayout`, `GridLayout`, `StackLayout`) work like their `QtQuick` counterparts on integer cell bounds. See [Layouts and Cell Geometry](layouts.md).

```qml
pragma ComponentBehavior: Bound
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
| `Border.Ascii` (`"ascii"`) | `+---+` / `|   |` / `+---+` |
| `Border.None` (`"none"`) | No border drawn |

A border occupies 1 cell on each edge: a 10×5 bordered `Rectangle` has an 8×3 interior. `TableView` draws connected table grid intersections through `showOuterBorder`, `showColumnGrid`, `showRowGrid`, and `gridStyle` rather than `Rectangle.border`.

## Colors and Styling

QmlTui does not use `QColor` or a global theme singleton (such as `Material` or `QPalette`):

- Colors use `qmltui::Color` (`color` in QML). Items and controls default to `"default"` (the host terminal's foreground and background).
- Bare CSS/SVG color names (`"red"`, `"blue"`, `"cyan"`, `"orange"`, etc.), hex strings (`"#2cde85"`), and `QmlTui.rgba(r, g, b, a)` specify 24-bit RGB colors, which QmlTui quantizes automatically when running on 256-color or 16-color terminals.
- Explicit `"ansi:..."` colors (`"ansi:cyan"`, `"ansi:red"`, `"ansi:0"`..`"ansi:15"`, or `QmlTui.ansi(index)`) emit palette SGR indices so the user's terminal theme controls their appearance.
- Controls (`Button`, `Panel`, `Dialog`, etc.) expose `background` and `contentItem` delegates for customization. Express shared application styling with QML property bindings and reusable QML components. See [Styling and Colors](styling-and-colors.md).

## Keyboard Quit Path

Desktop windows have a window-manager close button; terminal applications in raw mode do not, and `TextInput` and `TextArea` use `Ctrl+C` for clipboard copy rather than `SIGINT`. A root `Terminal` never exits on `Q` or `Escape` unless the application handles it:

```qml
pragma ComponentBehavior: Bound
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

<!-- check-docs: ignore -->
```qml no-playground
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

In `QtWidgets`, models such as `QStandardItemModel` often provide `Qt::ForegroundRole`, `Qt::BackgroundRole`, `Qt::TextAlignmentRole`, `Qt::CheckStateRole`, `Qt::DecorationRole`, and `flags()` without custom `roleNames()`. (Note that `QAbstractItemModel`, `QAbstractListModel`, `QAbstractTableModel`, and `QSortFilterProxyModel` live in `Qt6::Core`, whereas `QStandardItemModel` lives in `Qt6::Gui`.) While `QtQuick` views ignore those numeric roles, QmlTui views expose them automatically (`foreground`, `background`, `textAlignment`, `checkState`, `decoration`, `enabled`, `checkable`) and `TableView` / `TreeView` built-in painters render them out of the box. See [Models in QmlTui Views](models.md) for the full support matrix.
