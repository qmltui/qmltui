# Migrating from Qt Quick

QmlTui borrows Qt Quick's declarative QML ergonomics—property bindings, anchors, layouts, states, transitions, and `QAbstractItemModel` views—while running strictly on `QCoreApplication`, `Qt6::Core`, and `Qt6::Qml` without `QtQuick`, `QtGui`, or a scene-graph GPU.

This guide highlights every structural and behavioral difference a Qt Quick developer will encounter when moving to QmlTui.

## Imports and Application Host

| Qt Quick | QmlTui | Notes |
| :--- | :--- | :--- |
| `QGuiApplication` | `QCoreApplication` | No display server or platform plugin required |
| `import QtQuick` | `import QtQml` + `import QmlTui` | Never import `QtQuick` or link `Qt6::Quick` |
| `import QtQuick.Controls` | `import QmlTui.Controls` | Terminal-default controls; no framework theme singleton |
| `import QtQuick.Layouts` | `import QmlTui` | `RowLayout`, `ColumnLayout`, `GridLayout`, `StackLayout`, `Layout` live in `QmlTui` |
| `import QtQml.Models` | `import QtQml.Models` | `ListModel`, `ListElement`, `DelegateChooser` work unchanged |
| `Window` / `ApplicationWindow` | [`Terminal`](../qml-types/terminal.md) | `Terminal.AltScreen` (default) or `Terminal.Inline` |
| Window close button (`X`) | Explicit `Keys.onPressed` / `Shortcut` | Root `Terminal` does not auto-bind `Q` or `Escape` to `Qt.quit()` |

Always begin QML files with `pragma ComponentBehavior: Bound` and provide a reachable keyboard quit path (closing modals or clearing input before quitting on `Escape`).

## Integer Character-Cell Geometry and Border Padding

- **Cells, not pixels**: `x`, `y`, `width`, `height`, `spacing`, margins, and paddings are integer character cells. A typical viewport is `80×24` rather than `1280×720`.
- **Bordered `Rectangle` and `Panel` implicit padding**: When `border.style !== Border.None`, QmlTui automatically adds `1` cell to `leftPadding`, `rightPadding`, `topPadding`, and `bottomPadding`. Because `anchors.fill: parent` respects parent padding, children already sit inside the border:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            event.accepted = true;
            Qt.quit();
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 34
        height: 5
        border {
            style: Border.Rounded
            color: "cyan"
        }

        // Already inset 1 cell from the border; adding margins: 1 would inset 2 cells.
        Text {
            anchors.fill: parent
            text: "Inset automatically by border"
        }
    }
}
```

- **Button inner padding**: `Button` defaults to `leftPadding: 2; rightPadding: 2` (1 border cell + 1 inner space per side), so an explicit width leaves `width - 4` cells for the label unless you override padding.

## Text, Rich Formatting, and Text Inputs

- **`Text.AutoText` parses Markdown and ANSI**: Unlike Qt Quick's HTML-based `AutoText`, `Text.textFormat` defaults to `Text.AutoText` and automatically switches to `MarkdownText` or `AnsiText` when Markdown or ANSI escape sequences are detected. Set `textFormat: Text.PlainText` explicitly when displaying untrusted or literal user text so escape sequences render safely as visible glyphs.
- **`TextInput` and `TextArea`**:
  - `Ctrl+C` copies the active selection when text is selected, and otherwise bubbles up to ancestor `Keys.onPressed` handlers and `Shortcut` items.
  - `TextInput.editingFinished()` emits on Return/Enter and when active focus is lost (`activeFocus` transitions `true -> false`), matching Qt Quick.
  - Syntax highlighting on `TextArea` uses QmlTui's `SyntaxHighlighter` / `highlightRanges` API rather than `QQuickTextDocument`.

## Layouts (`RowLayout`, `ColumnLayout`, `GridLayout`, `StackLayout`)

- **Auto-clipping**: `RowLayout`, `ColumnLayout`, and `GridLayout` default `clip: true` so overflowing children do not bleed over adjacent panes.
- **Stretch factors**: `Layout.horizontalStretch` and `Layout.verticalStretch` default to `1` (participating equally when `fillWidth` / `fillHeight` is enabled).
- **Per-side margins only**: Use `Layout.leftMargin`, `Layout.rightMargin`, `Layout.topMargin`, and `Layout.bottomMargin` (there is no shorthand `Layout.margins` property).

## Virtualized Views and Models

- **Fixed cell stride**: `ListView` (`rowHeight`), `TableView` (`rowHeight`), `TreeView` (`rowHeight`), and `GridView` (`cellWidth`, `cellHeight`) use uniform integer cell strides for O(1) virtualized scrolling. There is no kinetic flick physics, `section`, or `header`/`footer` delegate API.
- **Static JS model cap**: Integer counts and JavaScript arrays are capped at `10,000` entries; use a C++ or Python `QAbstractItemModel` for unbounded datasets (100,000+ rows).
- **Standard Qt Widgets roles work out of the box**: Unlike Qt Quick views, QmlTui views decode `Qt::ForegroundRole`, `Qt::BackgroundRole`, `Qt::TextAlignmentRole`, `Qt::CheckStateRole`, `Qt::DecorationRole` (string/glyph), and `flags()` (`ItemIsEnabled`, `ItemIsUserCheckable`) automatically. See [Models in QmlTui Views](models.md).

## Input, Focus, and Shortcuts

- **`Keys.priority` defaults to `Keys.BeforeItem`**: Unlike Qt Quick (where `Keys.AfterItem` is default), QmlTui runs attached `Keys.onPressed` handlers *before* the item's built-in C++ `keyPressEvent()`. Set `Keys.priority: Keys.AfterItem` when you want a control (`TextInput`, `ListView`, `Button`) to handle keys first.
- **`Keys.forwardTo` limitation**: `Keys.forwardTo` forwards key events to target items' attached `Keys.onPressed` handlers; it does not forward into built-in C++ `keyPressEvent()` implementations (such as `ListView` or `TextInput` navigation). Use `FocusScope` or explicit handlers when coordinating C++ controls.
- **`MouseArea` press/release acceptance**: `MouseArea` pre-accepts press and release events before emitting `pressed(mouse)` and `released(mouse)`, matching Qt Quick—set `mouse.accepted = false` inside `onPressed` to let an underlying `MouseArea` receive the press and implicit grab.
- **`Shortcut`**: Accepts both portable key-chord strings (`"Ctrl+Q"`, `"Alt+1"`, `"F5"`) and Qt integer key bitmasks.

## States, Transitions, and `Behavior`

- **Direct `easing` property**: Write `easing: Easing.OutQuad` directly on `NumberAnimation` / `ColorAnimation` / `PropertyAnimation` rather than Qt Quick's grouped `easing.type: Easing.OutQuad`.
- **Public Qt vs. Private Qt builds**: Map-based `PropertyChanges { properties: ({ ... }) }`, `State`, `StateGroup`, and `Transition` work in all builds. Direct-assignment `PropertyChanges { target.prop: val }`, `Behavior on prop`, and binding-preserving restoration require `QMLTUI_ENABLE_PRIVATE_QT=ON` (default when `Qt6::QmlPrivate` is installed). See [Animation and States](animation-and-states.md).
- **Reverse transitions**: Matching `Transition`s animate both forward state changes and reversions back to the base state `""`.

## `Loader` Differences

- **`asynchronous: true`** applies to external `source` URLs (`QQmlComponent::Asynchronous`), not inline `sourceComponent`.
- **`setSource()` is not exposed**: Bind `source` or `sourceComponent` declaratively and pass properties to `loader.item` inside `onLoaded` or via bindings.

## Styling and Colors

- **Terminal-default colors**: Core items and `QmlTui.Controls` default to the terminal's own foreground and background (`"default"`, an invalid `qmltui::Color`). There is no `Material`, `Fusion`, or `Universal` style plugin and no global framework `Palette` singleton.
- **Explicit overrides survive focus and disabled states**: Setting `color`, `textColor`, or `borderColor` explicitly overrides default focus/disabled styling. Share application-wide colors through reusable QML components or a `pragma Singleton` `Theme.qml` (see [Styling and Colors](styling-and-colors.md)).

## Next Steps

- [Getting Started](../getting-started.md) — build a 3-file standalone CMake project with `Runner` tests.
- [Layouts & Cell Geometry](layouts.md) — anchors, positioners, and `RowLayout` / `ColumnLayout` / `GridLayout`.
- [Input & Focus](input-and-focus.md) — `FocusScope`, `Keys`, `Shortcut`, and contextual `Escape`.
- [Inline Mode & Scrollback](inline-mode.md) — build normal-buffer CLIs and REPLs that emit native terminal scrollback.
