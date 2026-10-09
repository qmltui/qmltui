# QML API reference

Qtui 0.3 provides a terminal-rendered, Qt Quick-style QML API. Its core
model/view/delegate names and delegate contracts are source-compatible with the
documented Qt Quick subset below. Qtui remains a terminal toolkit: geometry is
cell-based, delegates must derive from `Qtui.Item`, and APIs tied to the Qt Quick
scene graph are intentionally not implemented.

## Color values

Qtui provides its own QtGui-free `rgba` value type. Use the `Qtui` singleton
to construct a value from normalized red, green, blue, and alpha channels:

```qml
import QtQml
import Qtui

QtObject {
    property rgba accent: Qtui.rgba(0.2, 0.4, 0.6, 1.0)
}
```

This property contains a `qtui::Color`, not a `QColor`. The built-in QML
`color` type and `Qt.rgba()` belong to QtGui and are unavailable in a
QtCore/QtQml-only Qtui application. String literals remain supported for Qtui
properties whose declared C++ type is `qtui::Color`.

Two kinds of string are accepted, and they behave differently on screen:

- Hex values such as `"#2cde85"` are literal RGB. On truecolor terminals they
  render exactly; on 256- and 16-color terminals they are quantised.
- The sixteen ANSI palette names (`black`, `maroon`, `green`, `olive`, `navy`,
  `purple`, `teal`, `silver`, `gray`, `red`, `lime`, `yellow`, `blue`,
  `magenta`, `cyan`, `white`) and the explicit form `"ansi:N"` are palette
  indices, not RGB. They are emitted as SGR 30-37 and 90-97 at every color
  depth, so the user's terminal theme decides how they look. Other CSS names (`orange`, `steelblue`,
  ...) are literal RGB.
- `default` (also spelled `transparent`) means the terminal's default foreground or background.

## Model and delegate compatibility

All views accept ready inline components and components that become ready
asynchronously. Delegate creation preserves the component's lexical QML context,
including `pragma ComponentBehavior: Bound`. A delegate root that is not a
`Qtui.Item`, or a component that enters the error state, is rejected with a QML
warning instead of being partially installed.

For map and `QAbstractItemModel` data, named roles are available both as required
delegate properties and through `model`. `modelData` contains the row or cell
value described in the table below. `dataChanged` updates only the roles named by
the model when the model supplies such a list.

| Type | Standard delegate properties | `modelData` | Qtui-specific behavior |
| --- | --- | --- | --- |
| `ListView` | `index`, `model`, named roles; attached `ListView.view` and `ListView.isCurrentItem` | Row value or role map | Cell-based scrolling and highlighting; `rootIndex` selects the level of a tree model |
| `GridView` | `index`, `model`, named roles | Row value or role map | Fixed `cellWidth` and `cellHeight`; only the viewport's cells are instantiated; `rootIndex` as ListView |
| `Repeater` | `index`, `model`, named roles | Row value or role map | Inserts delegates into its visual parent; `rootIndex` as ListView |
| `TableView.delegate` | `row`, `column`, `index`, `current`, `selected`, `model`, named roles | Cell value | One delegate instance per cell |
| `TableView.rowDelegate` | `row`, `index`, `model`, named roles | Entire row | Deprecated 0.1 row-scoped compatibility path |
| `TreeView` | `treeView`, `isTreeNode`, `row`, `column`, `current`, `selected`, `depth`, `expanded`, `hasChildren`, `model`, named roles | Node role map | Column 0 is the tree column |
| `Scrollback` | `index`, `model`, named roles | Row role map | Appends rendered rows; existing output is not rewritten |

`currentIndex`, `selectedRow`, `currentRow`, and `currentColumn` use `-1` for no
current item where applicable. Model destruction clears the view safely. Model
replacement disconnects the old model before a new generation is observed.

`ListView`, `GridView`, and `Repeater` show one level of a model. Bound to a
model with nested rows they show the top level and warn once; set `rootIndex`
(a `QModelIndex`, as `DelegateModel.rootIndex` in Qt Quick) to show the
children of one node instead. All views drive lazily populated models through
`canFetchMore()`/`fetchMore()`. The [models guide](guide/models.md) has the
full view-by-capability matrix.

## ListView

Properties include `model`, `delegate`, `orientation`, `spacing`, `currentIndex`,
`highlightColor`, and `highlightBackgroundColor`. `currentIndex` defaults to `-1` and can
be reset to `-1`.

Use the attached API for selection state:

```qml
ListView {
    model: applicationModel
    delegate: Text {
        required property string display
        color: ListView.isCurrentItem ? "black" : "white"
    }
}
```

See the [ListView example](../examples/components/list_view/Main.qml).

## TableView

Grid controls are `showColumnGrid`, `showRowGrid`, and `showOuterBorder`, all
`false` by default. Enable all three for connected box-art tables. `gridStyle`
is `"single"`, `"double"`, or `"ascii"`. Column widths include padding and exclude
separators; `headerAlign` can override header alignment independently of cells.
`showGrid` has been removed. See [TableView](components/tableview.md) for sizing,
clipping, and input details.

`delegate` follows Qt Quick's cell-scoped shape. `row`, `column`, `current`, and
`selected` identify the represented cell. Qtui's `columns` descriptors continue
to provide terminal column titles, keys, widths, and alignment.

`rowDelegate` retains the deprecated Qtui 0.1 row-scoped delegate contract for
migration; it remains supported with no scheduled removal release. When both properties are set, `delegate` wins and
Qtui emits a warning.

Selection properties are `currentRow` (read-only alias of `selectedRow`),
`selectedRow`, and `currentColumn`. See the
[TableView example](../examples/components/table_view/Main.qml).

### Sorting, filtering, and model coordinates

TableView displays the supplied model's order. It does not sort, filter, or
rearrange its input. `headerClicked(int column)` reports a primary-button click
on a visible header cell; the divider is not a header activation target.
Applications connect this signal to their sorting policy.

Use a C++ `QSortFilterProxyModel` for sorting/filtering on the Qt 6.8 baseline.
Give the view the proxy as its `model`, and configure the source model, sort
role, filter role, and filter column in application code. Numeric sort roles
should contain numbers, independently of display strings such as `"12.4%"`.
For role-based list models, the application maps visual columns to sort roles;
a visual column number is not necessarily a source-model column number.

Qt 6.10+ also supplies `SortFilterProxyModel`, `RoleSorter`, and filters through
`import QtQml.Models`; that native QML API is a technology preview. It requires
a `QAbstractItemModel` source, not a JavaScript array. It is optional and does
not change Qtui's Qt 6.8 requirement or require a `QtQuick` import. Qtui does not
provide a competing proxy wrapper.

`selectedRow`, delegate `row`/`column`, and `rowActivated(row)` use the supplied
model's coordinate space. When it is a proxy, translate an activation using
`proxy.mapToSource(proxy.index(row, 0))` in application code before accessing
the underlying record. The demo's `ProcessProxy::activate()` illustrates this.
Array/list inputs remain in their supplied order, so their row indexes directly
address the supplied list.

The current C++ model record follows persistent indexes through insertions,
moves, and proxy reordering. Removing or filtering it out clears selection;
removing the filter does not restore it. Reset, replacement (including array
replacement), and destruction also clear selection. Invalid selections never
emit `rowActivated`. Pointer input updates both row and column. Page keys move
by the visible body-row count.

The existing single-current-cell API is retained: built-in rendering highlights
the selected row, while cell delegates receive `selected == current` for the
current cell. This is not Qt Quick's `ItemSelectionModel` API.

### Column geometry and formatting

Column descriptor maps accept `title`, `key`, `width`, and `alignment` (or the
compatibility spelling `align`). The default width is 12 cells. When no
descriptors are provided, a C++ model supplies the column count through
`columnCount()`, titles through `headerData(section, Qt::Horizontal)`, and a
default alignment through `Qt::TextAlignmentRole` on the header; width is
shared evenly across the columns. The header row appears only when at least
one section returns a string title. JavaScript array models without
descriptors keep the compatibility column count of five. A one-column C++
list model can still supply roles for multiple visual columns through
descriptors, all backed by source column zero.

`columnWidthProvider(column)` is evaluated once per needed column when geometry
is invalidated, not once per row. Return a finite non-negative integer: zero
hides a column, while a negative number or `undefined` falls back to its
descriptor width. Invalid widths use 12 cells and produce a QML diagnostic.
Geometry is refreshed after size, descriptor, provider, or structural model
changes. Call `forceLayout()` when values captured by an unchanged provider
function change. This API also marks the view for repaint.

`cellFormat(row, column, value)` applies only to built-in painting. Its optional
result keys are `text`, `color`, `backgroundColor`, `bold`, and `align`. Custom delegates
own their styling. Callback exceptions produce diagnostics and fall back to
the unformatted value or descriptor width. Repeated identical callback errors
are suppressed until success or callback replacement; the remembered set is
bounded.

For compatibility, array/map cell delegates receive stringified `modelData`;
C++ models with named roles provide a role map, and those without named roles
provide the display value. `rowDelegate.modelData` represents the entire row.

Delegate count is bounded by the viewport and overscan. Updates query only
changed roles of realized source indexes, sharing reads between visual columns
backed by the same source index. Selection-only changes do not query roles.
Offscreen delegates are destroyed rather than pooled. Horizontal scrolling,
independent viewport positioning, typed column objects, and multi-selection
remain outside this API.

### Breaking migration: view-owned sorting removed

Remove TableView's `sortColumn`, `sortOrder`, and `sortByColumn()` usages. Replace
them with application-owned model/proxy sorting and `onHeaderClicked`. Sort
indicators are application presentation; TableView no longer appends arrows.
The sorting demo now uses typed source values, an external C++ proxy, a filter
input, and activation mapped back to the source PID. Header clicks on an
unconnected table have no sorting side effects.

## TreeView

For `QAbstractItemModel`, set `model`; optionally set `rootIndex` to display a
subtree. Terminal-native map trees use `rootNode` and a `children` list. Keeping
these entry points separate avoids overloading Qt Quick's `model` semantics.

Properties include `model`, `rootNode`, `rootIndex`, `delegate`, `currentIndex`,
`currentRow`, `currentColumn`, and `currentItemData`. Changes to the current
node's roles notify `currentItemDataChanged`.

Methods include `expand`, `collapse`, `toggleExpanded`, `isExpanded`, `depth`,
`expandRecursively`, `collapseRecursively`, `expandToIndex`, `expandAll`, and
`collapseAll`. Expansion emits `expanded(row, depth)` or
`collapsed(row, recursively)`.

For C++ models, expansion and the current node are tracked by persistent
index, so renames, inserts above, proxy sorting, and moves preserve them.
`dataChanged` refreshes visible nodes in place without re-creating delegates.
Expanding a branch that reports `canFetchMore()` calls `fetchMore()` on it.
A reset collapses everything, and a proxy that filters a branch out drops its
expansion state, matching Qt's persistent-index semantics.

See the [TreeView example](../examples/components/tree_view/Main.qml).

## Migration from 0.1

- Replace a delegate's bare `isCurrentItem` property with the attached
  `ListView.isCurrentItem` property.
- Replace TreeView's bare `isCurrent` property with `current`; use `selected`
  when the delegate expresses selection styling.
- Move map-shaped tree data from `TreeView.model` to `TreeView.rootNode`.
- A `TableView.delegate` is now instantiated per cell. Rename an unchanged
  row-scoped delegate to `rowDelegate` as a temporary compatibility measure, or
  migrate it to consume `row` and `column`.
- Declare delegate inputs as required properties when using
  `pragma ComponentBehavior: Bound`.

The 0.3 public contract is the API subset documented here, not every Qt
Quick view feature. Qtui does not currently expose scene-graph transitions,
flick physics, reuse-pool callbacks, selection models, or the complete Qt Quick
positioning and item-lookup API.

## Colors and styling

Core items and Controls use the terminal's default foreground and background.
`"default"` delegates to the terminal; named ANSI colors follow its color table,
while hex colors request specific RGB values. `"transparent"` is an alias for
`"default"`; transparent item backgrounds leave the surface underneath visible.
On a `Canvas` this means `setPixelColor(x, y, "transparent")` paints a set pixel
in the terminal's default ink, not a hole; erase a pixel with
`setPixel(x, y, false)` or the whole surface with `clear()`.

There is no framework palette, inherited color scheme, or theme singleton.
Set ordinary properties to customize individual components, or define reusable
QML components for application-wide styling:

```qml
// AppButton.qml
pragma ComponentBehavior: Bound
import Qtui.Controls

Button {
    background: Rectangle {
        color: "#202830"
        border.color: "cyan"
        border.style: parent && parent.height >= 3 ? (parent.activeFocus ? "double" : "single") : "none"
    }
    textColor: "#ffffff"
}
```

| Component | Color overrides |
| :--- | :--- |
| Text, Paragraph, TextInput, TextArea, Canvas, BusyIndicator, Sparkline | `color` |
| Rectangle and rectangle-based controls | `color`, `border.color` |
| Button, CheckBox, RadioButton, Switch, Slider | `textColor` |
| Panel | `titleColor` |
| Dialog | `textColor`, `titleColor` |
| ProgressBar | `color`, optional gradient/threshold colors |
| TabBar | `textColor`, `color`, `selectedTextColor`, `selectionColor` (background) |
| Toast | `textColor`, `color`, `infoColor`, `successColor`, `warningColor`, `errorColor` |
| ToolTip attached properties | `ToolTip.textColor`, `ToolTip.color` |

Focus uses borders or bold text. Disabled text is dimmed using SGR 2, preserving
its configured color. `Text.dim` also exposes this attribute directly. TextInput
hints use the configured text color with dimming unless `placeholderColor` is
set. Rich Paragraph links use underline and inline code uses bold.

TabBar and the built-in TableView/TreeView selection render with reverse video
until selection colors are explicitly supplied. TextInput also supports explicit
selection, selected-text, placeholder, and cursor colors. Custom delegates own
their appearance, including selection styling.

Core `Palette`, `TerminalPalette`, `ResolvedPalette`, `Item.palette`, and Controls
`DarkPalette` are no longer provided. Replace earlier palette bindings with
ordinary color bindings or application-defined components.

### StandardItemDelegate

`Qtui.Controls.StandardItemDelegate` is the terminal counterpart of Qt
Widgets' `QStyledItemDelegate`: a delegate for any C++ model that renders the
standard item roles, decoration glyph, check box, foreground and background
colors, text alignment, and disabled state, plus reversed text for the current
row. It works as the `delegate` of `ListView`, `GridView`, `Repeater`,
`TableView`, and `TreeView`. `toggle()` flips `Qt::CheckStateRole` through
`setData()`; `toggled(checkState)` reports it. `current` binds to
`ListView.isCurrentItem` and is set by the other views. `textColor` styles
rows without a foreground role. The [models guide](guide/models.md) lists the
role names every view provides; see the
[standard_item_delegate example](../examples/components/standard_item_delegate/Main.qml).

## Other core elements

- `Terminal` derives from `Item` and owns the terminal session. `mode` is
  `Terminal.FullScreen` by default, or `Terminal.Inline`. It selects screen
  buffer, rendering, and sizing together, and can change at runtime.
- Read-only `Terminal.capabilities` exposes `kittyKeyboard`, `kittyGraphics`,
  `cellPixelSize`, and `syncOutput`. An asynchronous startup probe refines these
  values without delaying the first frame; changes notify QML bindings.
  When `kittyKeyboard` is true the engine has pushed the kitty keyboard
  protocol's disambiguate flag for the session: Escape arrives immediately,
  Ctrl and Alt chords carry their modifiers, and Ctrl+Enter, Shift+Enter and
  Ctrl+Shift+letter reach `Keys.onPressed` as distinct events. On a legacy
  terminal a lone Escape is delivered after a short window
  (`QTUI_ESCAPE_TIMEOUT_MS`, default 50) and those chords collapse to their
  unmodified keys. `QTUI_KITTY_KEYBOARD=0` keeps the legacy path on a terminal
  that answers the query but misbehaves. Graphics transport support does not
  imply Unicode placeholder support. `cellPixelSize` is invalid when unknown.
- `Image.renderMode` selects how pixels reach the terminal: `HalfBlock` (default
  glyph rendering), `Braille`, `ASCII`, `Kitty` (true pixels via the kitty
  graphics protocol using Unicode placeholders), and `Auto`, the new default.
  `Auto` uses kitty where the terminal supports it and true colour, and falls
  back to `HalfBlock` everywhere else, so the same QML shows real images in
  kitty, Ghostty, WezTerm and Konsole and glyph images in every other terminal.
  Sixel (`Sixel` mode) is also supported, for terminals such as Windows Terminal
  that implement sixel but not kitty; `Auto` prefers kitty, then sixel, then
  half-block. Kitty uploads are freed automatically when the source changes, the
  item is destroyed, or the session ends. Sixel paints at the cursor rather than
  into cells, so it is drawn after the cell diff and cannot sit under other cell
  content the way kitty placeholders can; use it for a dedicated image region.
- All visible children render in ordinary stacking order in both modes. Use
  layouts to arrange them; overlapping children are not automatically stacked
  vertically or treated as pages. Visibility and focus remain application state.
- Full-screen roots fill the terminal. Inline roots use terminal width and
  visible content height, capped at terminal rows minus one cursor row. Hidden
  children do not contribute to height. Vertically filling a content-sized
  root is circular; give inline content an explicit or implicit height.
- `Scrollback` is nonvisual and never changes mode. `print(string)` returns
  whether output was accepted. `append(Item)` snapshots a borrowed item;
  `append(data)` renders a delegate without inserting into the bound model.
  Model inserts append rows, edits do not rewrite committed output, and resets
  start a new output sequence. `clear()` resets the delegate cache and consumed
  position; it does not erase the model, accepted output, or terminal history.
- Accepted scrollback belongs to the session, with a 1 MiB UTF-8 queue shared
  by its output objects. Full-screen mode defers output until inline resumes
  or the session stops. Oversized submissions are rejected in full, retaining
  earlier output; `outputRejected(reason)` and a warning report rejection.
  Output snapshots retain their submission-time formatting across resizing.
- Root `color` is the background. Borders and fixed dimensions belong on inner
  rectangles or panels. Nested `Terminal` roots are unsupported; switch normal
  child visibility and `mode` for a persistent full-screen page.
- `Item` is the visual base type and provides cell geometry, visibility, focus,
  clipping, anchors, and stacking order.
- `ScrollView` provides cell-based scrolling for content larger than its bounds.
- `Loader` creates one `Qtui.Item` from `source` or `sourceComponent`.
- `Row` and `Column` lay out children with integer spacing.
- `Text`, `TextInput`, and `TextArea` provide terminal text rendering and input.
- `MouseArea` provides click, wheel, hover, and drag input.

`Qtui.Controls` contains the controls a user operates or reads as finished
widgets, including `TabBar`, `Toast`, `Sparkline`, and `Popup`. `Canvas` and its shapes are part of `Qtui`. `StatusBadge`, `Calendar`,
and `BigText` are recipes under `examples/components/`.

## Styled text spans

`Text.spans` accepts a `list<span>` built from object literals or
`Qtui.span(text, options)`. Nonempty spans take precedence over `text`; clearing
the list restores it. Content is literal, and newlines create explicit lines
without automatic wrapping. Omitted style inherits, explicit boolean values
override, and `color: "default"` requests the terminal foreground. See
[Text](components/text.md) and [span](components/span.md) for the complete
layout, inheritance, and C++ contracts.
