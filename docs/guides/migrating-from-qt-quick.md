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
| `import QtQml.Models` | `import QtQml.Models` (+ `Qt.labs.qmlmodels` on Qt 6.8) | `ListModel` and `ListElement` live in `QtQml.Models`; `DelegateChooser` lives in `Qt.labs.qmlmodels` on Qt 6.8 (also in `QtQml.Models` on Qt 6.9+) |
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

- **`Text.AutoText` parses subset HTML and ANSI SGR (no inline Markdown)**: `Text.textFormat` exposes `Text.PlainText`, `Text.RichText`, `Text.AnsiText`, and `Text.AutoText` (the default; there is no `MarkdownText` enum in Core—for full CommonMark/GFM documents and inline Markdown blocks, use the [`QmlTui.Markdown`](https://github.com/qmltui/markdown) addon). Under `Text.AutoText`, `Text` parses subset HTML tags (`<b>`, `<i>`, `<u>`, `<s>`, `<del>`, `<reversed>`, `<font color>`, `<a href>`) and ANSI SGR escape sequences simultaneously (see [`Text`](../qml-types/text.md)). Set `textFormat: Text.PlainText` explicitly when displaying literal or untrusted user text so HTML tags and ANSI escape payloads are not interpreted (`Text` leaves printable bytes literal and drops raw zero-advance C0/C1 control characters rather than injecting terminal controls).
- **`TextInput` and `TextArea`**:
  - When `TextInput` or `TextArea` has active focus and a selection, `Ctrl+C` copies the selection and accepts the event; when nothing is selected, `Ctrl+C` bubbles up to ancestor `Keys.onPressed` handlers. Note that [`Shortcut`](../qml-types/shortcut.md) matches *before* focused-item dispatch (suppressing only unmodified printable chords inside text inputs), so a global `Shortcut { sequence: "Ctrl+C" }` will intercept `Ctrl+C` even while text is selected—handle `Ctrl+C` in root `Keys.onPressed` if you want copy-when-selected behavior.
  - `TextInput.editingFinished()` emits on Return/Enter and when active focus is lost (`activeFocus` transitions `true -> false`), matching Qt Quick.
  - Syntax highlighting on [`TextArea`](../qml-types/textarea.md) uses `setHighlightLayer(name, ranges, revision, priority)` / `clearHighlightLayer()` / `clearHighlights()` with [`highlightRange`](../qml-types/highlightrange.md) values rather than `QQuickTextDocument` or a `SyntaxHighlighter` base class (see [`examples/recipes/text_area_highlighting/`](../../examples/recipes/text_area_highlighting/)).

## Layouts (`RowLayout`, `ColumnLayout`, `GridLayout`, `StackLayout`)

- **Auto-clipping**: `RowLayout`, `ColumnLayout`, and `GridLayout` default `clip: true` so overflowing children do not bleed over adjacent panes.
- **Stretch factors**: `Layout.horizontalStretch` and `Layout.verticalStretch` default to `1` (participating equally when `fillWidth` / `fillHeight` is enabled).
- **No `Layout` margin properties**: [`Layout`](../qml-types/layout.md) does not provide `Layout.margins`, `Layout.leftMargin`, `Layout.rightMargin`, `Layout.topMargin`, or `Layout.bottomMargin`. Control whitespace with `spacing` on the layout container, `padding` on a surrounding `Rectangle` or `Panel`, or an explicit spacer `Item`.

## Virtualized Views and Models

- **Uniform cell strides**: [`ListView`](../qml-types/listview.md) measures its first delegate once and reuses that height for every row; [`TableView`](../qml-types/tableview.md) and [`TreeView`](../qml-types/treeview.md) use fixed 1-row strides; [`GridView`](../qml-types/gridview.md) uses explicit `cellWidth` and `cellHeight` (none of `ListView`, `TableView`, or `TreeView` expose a `rowHeight` property). There is no kinetic flick physics, `section`, or `header`/`footer` delegate API.
- **Static JS model cap**: Integer counts and JavaScript arrays are capped at `10,000` entries; use a C++ or Python `QAbstractItemModel` for unbounded datasets (100,000+ rows).
- **Standard Qt Widgets roles work out of the box**: Unlike Qt Quick views, QmlTui views decode `Qt::ForegroundRole`, `Qt::BackgroundRole`, `Qt::TextAlignmentRole`, `Qt::CheckStateRole`, `Qt::DecorationRole` (string/glyph), and `flags()` (`ItemIsEnabled`, `ItemIsUserCheckable`) automatically. See [Models in QmlTui Views](models.md).

## Input, Focus, and Shortcuts

- **`Keys.priority` defaults to `Keys.BeforeItem`**: Unlike Qt Quick (where `Keys.AfterItem` is default), QmlTui runs attached `Keys.onPressed` handlers *before* the item's built-in C++ `keyPressEvent()`. Set `Keys.priority: Keys.AfterItem` when you want a control (`TextInput`, `ListView`, `Button`) to handle keys first.
- **`Keys.forwardTo` routing**: If a key event remains unaccepted after an item's own `Keys` handlers and `keyPressEvent()`, [`Keys.forwardTo`](../qml-types/keys.md) delivers each target's built-in C++ `keyPressEvent()` first and then its `Keys.onPressed` signal (skipping target `Keys.priority` and specific-key signals such as `Keys.onReturnPressed`).
- **`MouseArea` press/release acceptance and bubbling**: [`MouseArea`](../qml-types/mousearea.md) pre-accepts press and release events before emitting `pressed(mouse)` and `released(mouse)`, matching Qt Quick—set `mouse.accepted = false` inside `onPressed` to pass the press and implicit grab up to an ancestor `MouseArea`. Note that unaccepted mouse events bubble strictly along the **parent chain** (not to overlapping sibling items behind in z-order).
- **`Shortcut`**: Accepts both portable key-chord strings (`"Ctrl+Q"`, `"Alt+1"`, `"F5"`, `"Shift+Tab"`) and Qt integer key bitmasks.

## States, Transitions, and `Behavior`

- **Direct `easing` property**: Write `easing: Easing.OutQuad` directly on `NumberAnimation` / `ColorAnimation` / `PropertyAnimation` rather than Qt Quick's grouped `easing.type: Easing.OutQuad`.
- **Public Qt vs. Private Qt builds**: Map-based `PropertyChanges { properties: ({ ... }) }`, `State`, `StateGroup`, and `Transition` work in all builds. Direct-assignment `PropertyChanges { target.prop: val }`, `Behavior on prop`, and binding-preserving restoration require `QMLTUI_ENABLE_PRIVATE_QT=ON` (default when `Qt6::QmlPrivate` is installed). See [Animation and States](animation-and-states.md).
- **Reverse transitions**: Matching `Transition`s animate both forward state changes and reversions back to the base state `""`.

## `Loader` Differences

- **`asynchronous: true`** applies to external `source` URLs (`QQmlComponent::Asynchronous`), not inline `sourceComponent`.
- **`setSource()` is not exposed**: Bind `source` or `sourceComponent` declaratively and pass properties to `loader.item` inside `onLoaded` or via bindings.

## Styling and Colors

- **Terminal-default colors vs. unset sentinels**: Core items and `QmlTui.Controls` default to the terminal's own foreground and background (`"default"`, i.e. [`QmlTui.defaultColor`](../qml-types/qmltui.md), which is a **valid** `qmltui::Color` with `valid === true`). Controls that fall back to reverse-video highlighting (such as `ComboBox.selectionColor`) use `QmlTui.invalidColor` (`valid === false`) as their unset sentinel. There is no `Material`, `Fusion`, or `Universal` style plugin and no global framework `Palette` singleton.
- **Explicit overrides survive focus and disabled states**: Setting `color`, `textColor`, or `borderColor` explicitly keeps your configured colors across focus and disabled transitions (disabled controls still apply dim SGR 2 without clobbering custom colors). Share application-wide colors through reusable QML components or a `pragma Singleton` `Theme.qml` (see [Styling and Colors](styling-and-colors.md)).

## Next Steps

- [Getting Started](../getting-started.md) — build a 3-file standalone CMake project with `Runner` tests.
- [Layouts & Cell Geometry](layouts.md) — anchors, positioners, and `RowLayout` / `ColumnLayout` / `GridLayout`.
- [Input & Focus](input-and-focus.md) — `FocusScope`, `Keys`, `Shortcut`, and contextual `Escape`.
- [Inline Mode & Scrollback](inline-mode.md) — build normal-buffer CLIs and REPLs that emit native terminal scrollback.
