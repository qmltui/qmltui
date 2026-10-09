---
title: TableView
kind: qml
import: Qtui
since: "0.4"
inherits: Item
cpp: qtui::TableView
category: data views
summary: Virtualized table with connected borders and configurable row and column grids.
---

# TableView QML type

Displays tabular models with aligned cells, custom delegates, and connected terminal borders. [More...](#detailed-description)

| | |
| :--- | :--- |
| **Import Statement** | `import Qtui` |
| **Since** | Qtui 0.4 |
| **Inherits** | [Item](item.md) |

## Properties

| Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `columns` | `list<var>` | `[]` | List of column descriptors: `{ title, width, align, headerAlign, padding, leftPadding, rightPadding }`. |
| `model` | `var` |  | _TODO_ |
| `selectedRow` | `int` |  | _TODO_ |
| `currentRow` | `int` |  | Read-only. _TODO_ |
| `currentColumn` | `int` |  | _TODO_ |
| `alternatingRows` | `bool` |  | _TODO_ |
| `stretchLastColumn` | `bool` | `false` | Automatically stretch the last column to fill remaining interior width. |
| `headerColor` | `rgba` |  | _TODO_ |
| `headerBackgroundColor` | `rgba` |  | _TODO_ |
| `selectionColor` | `rgba` |  | _TODO_ |
| `selectionBackgroundColor` | `rgba` |  | _TODO_ |
| `textColor` | `rgba` |  | _TODO_ |
| `rowBackgroundColor` | `rgba` |  | _TODO_ |
| `alternatingRowBackgroundColor` | `rgba` |  | _TODO_ |
| `gridColor` | `rgba` |  | Color used for borders and grid lines; falls back to `textColor`. |
| `showColumnGrid` | `bool` | `false` | Whether to draw column separators. |
| `showRowGrid` | `bool` | `false` | Draw dividers between visible data rows. |
| `showOuterBorder` | `bool` | `false` | Draw a connected outer frame at the view bounds. |
| `gridStyle` | `string` | `"single"` | Style of all borders and grid lines: `"single"`, `"double"`, or `"ascii"`. |
| `columnWidthProvider` | `var` |  | _TODO_ |
| `cellFormat` | `var` |  | JS callback `(row, column, value) => ({ text, color, backgroundColor, bold, align, padding, leftPadding, rightPadding })`. |
| `delegate` | `Component` |  | _TODO_ |
| `rowDelegate` | `Component` |  | _TODO_ |

## Signals

| Signal | Description |
| :--- | :--- |
| `rowActivated(int index)` | _TODO_ |
| `headerClicked(int column)` | _TODO_ |

## Methods

| Method | Description |
| :--- | :--- |
| `void forceLayout()` | _TODO_ |

## Detailed Description

`showColumnGrid`, `showRowGrid`, and `showOuterBorder` are independent and default
to `false`. Enable all three for a complete box-art table. The header underline
is always present when a header is shown. `gridStyle` accepts `"single"`,
`"double"`, and `"ascii"`; other values produce a warning and leave the style
unchanged. Structural lines, including the header underline, use `textColor`
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
import Qtui

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
