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
| `ListView` | `index`, `model`, named roles; attached `ListView.view` and `ListView.isCurrentItem` | Row value or role map | Cell-based scrolling and highlighting |
| `GridView` | `index`, `model`, named roles | Row value or role map | Fixed `cellWidth` and `cellHeight` |
| `Repeater` | `index`, `model`, named roles | Row value or role map | Inserts delegates into its visual parent |
| `TableView.delegate` | `row`, `column`, `index`, `current`, `selected`, `model`, named roles | Cell value | One delegate instance per cell |
| `TableView.rowDelegate` | `row`, `index`, `model`, named roles | Entire row | Deprecated 0.1 row-scoped compatibility path |
| `TreeView` | `treeView`, `isTreeNode`, `row`, `column`, `current`, `selected`, `depth`, `expanded`, `hasChildren`, `model`, named roles | Node role map | Column 0 is the tree column |
| `Scrollback` | `index`, `model`, named roles | Row role map | Appends rendered rows; existing output is not rewritten |

`currentIndex`, `selectedRow`, `currentRow`, and `currentColumn` use `-1` for no
current item where applicable. Model destruction clears the view safely. Model
replacement disconnects the old model before a new generation is observed.

## ListView

Properties include `model`, `delegate`, `orientation`, `spacing`, `currentIndex`,
`highlightColor`, and `highlightBgColor`. `currentIndex` defaults to `-1` and can
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
compatibility spelling `align`). The default width is 12 cells. The compatibility
column count is five when no descriptors are provided; it is not inferred from
the model. A one-column C++ list model can supply roles for multiple visual
columns, all backed by source column zero.

`columnWidthProvider(column)` is evaluated once per needed column when geometry
is invalidated, not once per row. Return a finite non-negative integer: zero
hides a column, while a negative number or `undefined` falls back to its
descriptor width. Invalid widths use 12 cells and produce a QML diagnostic.
Geometry is refreshed after size, descriptor, provider, or structural model
changes. Call `forceLayout()` when values captured by an unchanged provider
function change. This API also marks the view for repaint.

`cellFormat(row, column, value)` applies only to built-in painting. Its optional
result keys are `text`, `color`, `bgColor`, `bold`, and `align`. Custom delegates
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

## Other core elements

- `Terminal` derives from `Item` and owns the terminal session. `mode` is
  `Terminal.FullScreen` by default, or `Terminal.Inline`. It selects screen
  buffer, rendering, and sizing together, and can change at runtime.
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

`Qtui.Controls` contains the reusable controls, while `Qtui.Extras` contains
terminal-oriented presentation components such as `BigText`, `Gauge`,
`Paragraph`, `Sparkline`, `TabHeader`, and `ToastManager`.
