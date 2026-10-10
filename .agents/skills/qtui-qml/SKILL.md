---
name: qtui-qml
description: Applies Qtui retained-mode terminal QML conventions. Covers allowed imports (no QtQuick), pragmas and typing, C++ models with required-property delegates, Frame vs Stream renderer mode, integer cell geometry, border styles, and the keyboard quit path. Use whenever writing, reviewing, or refactoring Qtui QML files; do not use generic Qt Quick rules.
---

# Qtui QML Guidelines

Qtui is a C++20, Qt 6.8+ retained-mode terminal UI framework built on `QtQml`
and a custom text-grid renderer. It does not use Qt Quick, a display server, or
pixel rasterization. These rules describe how Qtui QML is written in this
repository. When a rule and an existing file disagree, match the file's style
for small edits and follow the rule for new files.

The authoritative type list is the module registration in the source tree, not
this document. Check `src/Qtui/Core/items`, `src/Qtui/Core/engine`, `src/Qtui/Controls`,
`src/Qtui/Image`, `src/Qtui/TerminalHost`, and `docs/qml_api.md`
before using a type or property you have not seen in a nearby file.

## 1. Imports

Never import `QtQuick` or any `QtQuick.*` module. Qtui registers its own
`Item`, `Rectangle`, `Text`, and layout types; a `QtQuick` import only works
because later imports shadow it, and it silently adds a Qt Quick runtime
dependency. `AGENTS.md` forbids it.

```qml
// Qtui modules
import Qtui                 // Terminal, Item, Rectangle, Text, views, layouts, Canvas
import Qtui.Controls        // Button, Panel, CheckBox, Slider, Dialog, TabBar, Toast, ...
import Qtui.Image           // Image, AnimatedSprite
import Qtui.TerminalHost    // TerminalView (PTY, Linux/macOS only, optional build)

// Qt foundational modules
import QtQml                // Timer, Component, QtObject, Connections, Binding
import QtQml.Models         // ListModel, DelegateModel
import Qt.labs.qmlmodels    // TableModel, DelegateChooser
```

`import Qtui` does not re-export `QtQml`. Add `import QtQml` when the file uses
`Timer`, `Component`, `QtObject`, `Connections`, or `Binding`. Add
`import QtQml.Models` when using `ListModel` or `DelegateModel`.

## 2. Pragmas and Typing

Start new files with `pragma ComponentBehavior: Bound`. It resolves ids and
properties at compile time, lets `qmlcachegen` compile more bindings to C++,
and is what makes `required property` delegates reliable. Many existing files,
including the Controls module, predate this pragma; do not add it to a file as a
drive-by change unless the file is otherwise being reworked.

Prefer concrete property types. `var` forces dynamic JS values and blocks
compilation of the bindings that use it.

```qml
// Prefer
property int counter: 0
property string labelText: "Status"
property PodModel activePod          // QObject-derived type: no pointer syntax

// Acceptable when no better type exists
property var thresholds: []          // small JS array configured from QML
property var tableModel: null        // model injected by the parent
```

Keep heavy data out of QML. Datasets, logs, parsing, filtering, and network IO
belong in a C++ `QAbstractItemModel` or `QAbstractTableModel`. In delegates,
declare `required property` for each model role so the engine binds roles
directly instead of through dynamic lookup:

```qml
TableView {
    model: podTableModel
    delegate: Text {
        required property string podName
        required property int restartCount
        text: podName
    }
}
```

Express state as declarative bindings. An imperative assignment in a handler
replaces the binding permanently, so use it only for one-shot actions.

```qml
// Avoid: destroys the binding
onClicked: statusText.text = computeStatus()

// Prefer
text: root.isActive ? "Active" : "Idle"
```

## 3. Terminal Mode and Scrollback

`Terminal.mode` defaults to `Terminal.FullScreen`, which fills the terminal
and uses the alternate screen. `Terminal.Inline` uses the normal buffer,
terminal width, and visible content height, capped at terminal rows minus one.

All visible children participate in the UI in either mode. Ordinary stacking
order applies; use layouts to arrange siblings. Hidden children do not
contribute to inline height. Do not vertically fill a content-sized inline
root: establish child height explicitly or implicitly. Root dimensions are
session-managed; put borders and fixed-size geometry on inner rectangles.

```qml
Terminal {
    mode: Terminal.Inline

    Scrollback {
        id: history
    }

    Column {
        width: parent.width
        Text { text: "Ready" }
        TextInput {
            width: parent.width
            height: 1
            focus: true
        }
    }
}
```

`Scrollback` is nonvisual and has no anchors or geometry. Its creation or
destruction never changes mode. It accepts text through `print()`, snapshots
items/data through `append()`, or renders model entries through a delegate.
Accepted output is queued during full-screen mode and committed when inline
resumes or the session stops. The shared queue is bounded to 1 MiB; rejection
emits `outputRejected(reason)`, and `print()` returns false.

Change `mode` for presentation; bind ordinary child `visible` and focus state
for page switching. There are no nested terminals or automatic takeover pages.
Always include the application quit path described below.

## 4. Geometry, Layout, and Borders

All geometry is in integer character cells: `width`, `height`, `x`, `y`,
`spacing`, and margins are columns and rows, never fractional pixels.

`Row` and `Column` are the primary containers and are used throughout the
repository. `RowLayout`, `ColumnLayout`, and `GridLayout` exist for the
attached `Layout.*` properties when children need fill or stretch behaviour.
Use anchors for positioning an item inside its parent; do not anchor a child
that a `Row`, `Column`, or `*Layout` is already positioning.

Keep borders compatible with Qt Quick's `Rectangle` API. The grouped `border`
property belongs exclusively to `Rectangle` and Rectangle-derived controls such
as `Panel`; its style is a string:

```qml
Panel {
    title: "Pod Metrics"
    border.style: "double"      // "none", "single", "double", "bold", "ascii"
    border.color: "#555555"
}
```

Non-Rectangle types, including `Paragraph`, `TableView`, `TreeView`,
`Sparkline`, and `TabBar`, do not expose `border`, `borderStyle`,
or `borderColor`. Wrap one in a `Rectangle` or `Panel` and inset it by one cell
when a decorative border is needed. For a connected table frame, use
`TableView.showOuterBorder` with `showColumnGrid` and `showRowGrid`; these
independent switches default to `false`. Its `gridStyle` is `"single"`,
`"double"`, or `"ascii"`. Column widths include padding but exclude separator
cells. `headerAlign` overrides heading alignment without changing body text.
TableView does not expose the Rectangle `border` group:

```qml
Rectangle {
    border.style: "single"
    border.color: "cyan"

    TableView {
        anchors.fill: parent
        anchors.margins: 1
    }
}
```

`Panel` exposes `title`, `titleAlignment`, and `titleColor` in addition to the
inherited `Rectangle.border` group.

Every Qtui color property is a `string` (`"#rrggbb"` or a named color), not a
Qt Quick `color`. Without `QtQuick` there is no color value-type provider, so
a `QColor` property cannot be assigned from QML; new C++ items must expose
colors as `QString`.

## 5. Keyboard Focus and Quit Path

Terminal users navigate only by keyboard. A root `Terminal` does not map `Q`
or `Escape` to exit; every application and example must provide a reachable
quit path. The repository idiom is a `Keys.onPressed` handler on the root that
accepts the event:

```qml
Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
}
```

Do not assume `Ctrl+C` terminates the process: Qtui raw mode disables
terminal-generated control signals. `TextInput` and `TextArea` use `Ctrl+C`
for copying. Applications must provide an explicit quit path—normally
contextual `Escape` and `Q` or `Ctrl+Q` outside editable input. Handle `Ctrl+C`
explicitly only when the application's interaction model permits it.

Preserve contextual `Escape` behaviour. In order: close an open modal or popup,
cancel an active operation or clear a search input, and quit only when nothing
else consumed the key. When a text input has focus, do not treat `Q` as quit.

```qml
Keys.onPressed: event => {
    if (event.key !== Qt.Key_Escape)
        return
    if (helpModal.visible)
        helpModal.visible = false
    else if (filterInput.text.length > 0)
        filterInput.text = ""
    else
        Qt.quit()
    event.accepted = true
}
```

## 6. Common Type Map

This summary is a map, not the contract. Verify names against the module
sources listed at the top.

Admission rule: a type enters `Qtui.Controls` only if it needs engine
knowledge an application cannot reproduce in QML, or is universal in terminal
toolkits. Status badges, calendars, and banner text are recipes under
`examples/components/`, not library types; copy them into your module.

**`import Qtui`** (C++ in `src/Qtui/Core/items` and `src/Qtui/Core/engine`)
`Terminal`, `Scrollback`, `Item`, `Rectangle`, `Text`, `Paragraph`, `TextInput`, `TextArea`,
`TableView`, `TreeView`, `ListView`, `GridView`, `ScrollView`, `LogView`,
`Canvas`, `CanvasRectangle`, `Circle`, `Line`, `Point`, `Polygon`,
`Row`, `Column`, `RowLayout`, `ColumnLayout`, `GridLayout`, `Repeater`,
`Loader`, `MouseArea`, `Shortcut`, `Keys` (attached), `State`,
`Transition`, `PropertyChanges`, `Gradient`, `GradientStop`, `Easing`,
`Animation`, `PropertyAnimation`, `NumberAnimation`, `ColorAnimation`.

`Behavior` is available only with `QTUI_ENABLE_BEHAVIOR=ON`.

**`import Qtui.Controls`** (QML and C++ in `src/Qtui/Controls`)
`Button`, `ButtonGroup`, `CheckBox`, `RadioButton`, `Switch`, `Slider`,
`ProgressBar`, `BusyIndicator`, `TabBar`, `StackView`, `StackLayout`, `Panel`,
`Dialog`, `Popup`, `Toast`, `Sparkline`. `ToolTip` is attached-only. There is
no palette singleton: colours are set per item, and an inherited palette
property is a Plan 06 section 7 proposal, not a shipped type.

**`import Qtui.Image`**
`Image`, `AnimatedSprite`.

**`import Qtui.TerminalHost`**
`TerminalView`.

## 7. Checklist

Before finishing a Qtui QML file:

1. No `import QtQuick` or `QtQuick.*` anywhere.
2. `import QtQml` present if the file uses `Timer`, `Component`, `QtObject`,
   `Connections`, or `Binding`.
3. `import QtQml.Models` present if the file uses `ListModel` or
   `DelegateModel`.
4. New files start with `pragma ComponentBehavior: Bound`.
5. Properties use concrete types where one exists.
6. Model delegates declare `required property` for the roles they read.
7. Inline roots set `mode: Terminal.Inline`; visible children have a finite
   content height. `Scrollback` has no geometry.
8. Grouped `border` belongs to `Rectangle` and its derived controls. TableView
   uses `showOuterBorder`, `showColumnGrid`, `showRowGrid`, and `gridStyle` for
   connected table lines; wrap other borderless types when needed.
9. A reachable keyboard quit path exists and does not fire while a text input
   has focus.
10. Visible behaviour is verified against the committed terminal frame using the
   `qtui-dev-loop` skill, not only against a QML property.

After changing either repository skill, run
`python3 .agents/skills/qtui-qml/check-skills.py --build-dir build` to catch stale
links, example targets, type-map entries, and whitespace errors.
