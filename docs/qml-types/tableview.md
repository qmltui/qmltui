---
title: TableView
kind: qml
import: QmlTui
since: "0.4"
inherits: Item
cpp: qmltui::TableView
category: data views
summary: Virtualized table with connected borders and configurable row and column grids.
---

# TableView QML type

Displays tabular models with aligned cells, custom delegates, and connected terminal borders. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import QmlTui` |
| **Since** | QmlTui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `columns` | `list<var>` | `[]` | List of column descriptors: `{ title, key, role, width, stretch, align, headerAlign, padding, leftPadding, rightPadding, color, background }`. |
| `model` | `var` |  | Holds the data model providing tabular entries: 2D array, TableModel, or QAbstractItemModel. |
| `selectedRow` | `int` |  | Holds the index of the currently selected row, or -1 if no row is selected. |
| `currentRow` | `int` |  | Read-only. Holds the index of the currently active row. |
| `currentColumn` | `int` |  | Holds the index of the currently active column. |
| `alternatingRows` | `bool` |  | Whether alternating rows use alternatingRowBackgroundColor. |
| `stretchLastColumn` | `bool` | `false` | Automatically stretch the last column to fill remaining interior width. |
| `headerColor` | `color` |  | Holds the foreground text color used for column headers. |
| `headerBackgroundColor` | `color` |  | Holds the background color used for column headers. |
| `selectionColor` | `color` |  | Holds the foreground text color for the selected row. |
| `selectionKeepsCellColors` | `bool` | `false` | Keep the model's own foreground and background on the selected row, so a table whose colours carry meaning does not lose them on the row being inspected. |
| `selectionBackgroundColor` | `color` |  | Holds the background highlight color for the selected row. |
| `textColor` | `color` |  | Holds the default text color for data cells. |
| `rowBackgroundColor` | `color` |  | Holds the default background color for table rows. |
| `alternatingRowBackgroundColor` | `color` |  | Holds the background color for alternating table rows. |
| `gridColor` | `color` |  | Color used for borders and grid lines; falls back to `textColor`. |
| `showColumnGrid` | `bool` | `false` | Whether to draw column separators. |
| `showRowGrid` | `bool` | `false` | Draw dividers between visible data rows. |
| `showOuterBorder` | `bool` | `false` | Draw a connected outer frame at the view bounds. |
| `gridStyle` | `GridStyle` | `TableView.Single` | Style of all borders and grid lines: `TableView.Single`, `TableView.Double`, or `TableView.Ascii`. |
| `columnWidthProvider` | `var` |  | Optional JS callback (column) => width returning cell count per column. |
| `cellFormat` | `var` |  | JS callback `(row, column, value) => Span \| list<Span> \| { text, spans, color, backgroundColor, bold, dim, italic, underline, reversed, align, padding, leftPadding, rightPadding }`. |
| `delegate` | `Component` |  | Holds an optional custom component for rendering cells. |
| `rowDelegate` | `Component` |  | Holds an optional custom component for rendering entire rows. |

## Signals

| Signal | Description |
| :--- | :--- |
| `rowActivated(int index)` | Emitted when a row is activated by pressing Enter or clicking. |
| `headerClicked(int column)` | Emitted when a column header is clicked. |

## Methods

| Method | Description |
| :--- | :--- |
| `void forceLayout()` | Recalculates column widths, layout geometry, and visible rows immediately. |

## Enumeration TableView.GridStyle

| Value | Description |
| :--- | :--- |
| `TableView.Single` | Draws connected borders and grid separators using single-line box-drawing characters (`┌`, `┬`, `┼`, `─`, `│`). |
| `TableView.Double` | Draws connected borders and grid separators using double-line box-drawing characters (`╔`, `╦`, `╬`, `═`, `║`). |
| `TableView.Ascii` | Draws connected borders and grid separators using ASCII characters (`+`, `-`, `|`). |

## Detailed Description

`showColumnGrid`, `showRowGrid`, and `showOuterBorder` are independent and default
to `false`. Enable all three for a complete box-art table. The header underline
is always present when a header is shown. `gridStyle` accepts `TableView.Single`,
`TableView.Double`, and `TableView.Ascii`. Structural lines, including the header underline, use `gridColor` (falling back to `textColor`)
and the terminal background; `headerColor` applies to heading text only.

### Columns and sizing

Column `width` and `columnWidthProvider` results count content cells, including
padding, **excluding** separators. Each column separator adds one cell; a frame
adds two cells to the total width. Width zero hides a column. Fixed columns
neither stretch to fill unused space nor wrap; excess content is clipped.
Model-defined columns without descriptors or a width provider divide the
interior width after reserving separator cells.

`align` sets cell and header alignment (`"left"`, `"center"`, or `"right"`).
`headerAlign` overrides only the heading, allowing centered headings over
left-aligned data. Padding remains inside the content width.

### Column colours

`color` and `background` let the view say how a column looks, instead of the
model carrying `Qt::ForegroundRole` for every cell. Either takes a colour:

```qml
columns: [
    { title: "Time", width: 14, color: "#8a94a6" },
    { title: "Rate", width: 8, align: "right", color: "#ffb86c" },
]
```

A column that states a colour uses it for every cell and the matching role is
not read at all, so this costs nothing per cell.

Either may instead be a function, called per cell as `(row, column, value)` and
returning a colour, for a column whose ink depends on the row:

```qml
{ title: "Time", width: 14,
  color: (row) => model.isError(row) ? "#dc4c4c" : "#8a94a6" }
```

A function that returns nothing leaves the cell to the model's role, so a view
can decide some rows and defer on the rest. Returned colours are parsed once
and reused, so repeating a handful of values across many cells is cheap.

A column colour wins over `Qt::ForegroundRole` and `Qt::BackgroundRole`: those
roles are the compatibility path for models ported from Qt Widgets, and an
explicit view-side rule outranks them. `cellFormat` still runs afterwards and
overrides both. Neither applies to a selected row unless
`selectionKeepsCellColors` is set.

A frame reserves one line at the top and bottom. A header reserves two interior
lines. Data rows are one line tall, with one extra line **between** visible rows
when `showRowGrid` is enabled. There is no trailing row divider. Empty space
below the data remains inside the frame and keeps its column separators. Five body lines fit three rows and two
dividers; four body lines fit two rows, one divider, and one unused line.

Zero-sized views draw nothing. A one-cell-wide or one-cell-high frame becomes a
straight line; a `1×1` frame becomes a cross. Headers and content are clipped to
the remaining interior.

The implicit width is the sum of the visible column widths plus separator and
frame cells, so a `TableView` inside a `Row` or `Column` sizes itself to its
columns. The implicit height is `0`: give the view a `height` or anchors, or it
draws nothing.

### Delegates and interaction

Cell delegates receive one content rectangle; row delegates receive the full
interior row width. The view owns their geometry and clips each subtree to one
line. Row delegates may span columns, with grid separators drawn over their
content. Both delegate modes retain connected borders and row dividers.
Selection and alternating backgrounds fill the content cells of the resolved
columns only; they do not extend across unused width to the right of the last
column, and never cover structural lines.

Header content clicks emit `headerClicked`. Data content clicks select and
activate the corresponding model row. Borders, separators, header underlines,
and empty body space are not activation targets. Up/Down move one row;
PageUp/PageDown move by the number of visible data rows.

The viewport is remembered rather than derived from the selection: moving the
selection within the visible rows does not scroll, and the view moves only far
enough to bring the selection back into range when it would fall outside. A
view that has not been scrolled stays at the top, so rows inserted above it
become visible instead of pushing it down; once scrolled, the top line keeps
showing the same row even as rows are added or removed above it. Resizing and
grid changes keep the selected row visible whenever a row fits.

This API replaces `showGrid` with `showColumnGrid`; there is no compatibility
alias in QML or C++. Widths no longer include separators, and the view no longer
has a built-in `40×10` implicit size.

### Example

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    TableView {
        width: 28
        height: 7
        showColumnGrid: true
        showRowGrid: true
        showOuterBorder: true
        columns: [
            { title: "Service", width: 15, padding: 1 },
            { title: "Status", width: 10, padding: 1, headerAlign: "center" }
        ]
        model: [["api", "ready"], ["worker", "busy"]]
    }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
}
```

### Terminal Preview

```text
┌───────────────┬──────────┐
│ Service       │  Status  │
├───────────────┼──────────┤
│ api           │ ready    │
├───────────────┼──────────┤
│ worker        │ busy     │
└───────────────┴──────────┘
```

## Usage guide

Set `gridStyle` to `TableView.Single`, `TableView.Double`, or `TableView.Ascii`
to select the box-drawing character set (`showGrid` has been removed in favor of
`showColumnGrid`, `showRowGrid`, and `showOuterBorder`).

`delegate` follows Qt Quick's cell-scoped shape. `row`, `column`, `current`, and
`selected` identify the represented cell. QmlTui's `columns` descriptors continue
to provide terminal column titles, keys, widths, and alignment.

`rowDelegate` retains the deprecated QmlTui 0.1 row-scoped delegate contract for
migration; it remains supported with no scheduled removal release. When both properties are set, `delegate` wins and
QmlTui emits a warning.

Selection properties are `currentRow` (read-only alias of `selectedRow`),
`selectedRow`, and `currentColumn`. See the
[TableView example](../../examples/recipes/table_view/Main.qml).

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
not change QmlTui's Qt 6.8 requirement or require a `QtQuick` import. QmlTui does not
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

Column descriptor maps accept `title`, `key`, `role`, `width`, `stretch`, and
`alignment` (or the compatibility spelling `align`). Setting `stretch: true` on
a descriptor stretches that column to absorb remaining interior width (or use
`stretchLastColumn: true` to stretch the final visible column). When `key` or
`role` is set on a descriptor, `TableView` reads that property from JavaScript
row maps or resolves the matching role name in `QAbstractItemModel::roleNames()`
on source column zero (allowing multi-role `ListModel` and `QAbstractListModel`
instances to populate multiple table columns). The default width is 12 cells. When no
descriptors are provided, a C++ model supplies the column count through
`columnCount()`, titles through `headerData(section, Qt::Horizontal)`, and a
default alignment through `Qt::TextAlignmentRole` on the header; width is
shared evenly across the columns. The header row appears only when at least
one section returns a string title. JavaScript array models without
descriptors keep the compatibility column count of five.

`columnWidthProvider(column)` is evaluated once per needed column when geometry
is invalidated, not once per row. Return a finite non-negative integer: zero
hides a column, while a negative number or `undefined` falls back to its
descriptor width. Invalid widths use 12 cells and produce a QML diagnostic.
Geometry is refreshed after size, descriptor, provider, or structural model
changes. Call `forceLayout()` when values captured by an unchanged provider
function change. This API also marks the view for repaint.

`cellFormat(row, column, value)` applies only to built-in painting. It may return
a `QmlTui.span(...)`, a `list<Span>` (or array of span objects), or a descriptor
object with optional keys `text`, `spans`, `color`, `backgroundColor`, `bold`,
`dim`, `italic`, `underline`, `reversed`, `align`, `padding`, `leftPadding`, and
`rightPadding`. Model cells in built-in painting may also supply a `Span` or
`QList<Span>` directly through `Qt::DisplayRole` (or a 2D array / row map),
allowing multi-span cells to be painted without delegate items or JavaScript
callbacks; model-supplied `Span` colors respect `selectionKeepsCellColors` on
the selected row, while `cellFormat` results always override selection colors.
Custom delegates own their styling. Callback exceptions produce diagnostics and
fall back to the unformatted value or descriptor width. Repeated identical
callback errors are suppressed until success or callback replacement; the
remembered set is bounded.

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

### Gallery Example

![TableView recorded demo](../assets/casts/components/table_view.gif)

See [`examples/gallery/examples/TableView.qml`](../../examples/gallery/examples/TableView.qml) (`./build/examples/gallery/qmltui_gallery --example TableView`, or open in the [WebAssembly Playground](../playground.md?example=TableView.qml)).
