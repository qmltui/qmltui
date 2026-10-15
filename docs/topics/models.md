# Models in QmlTui Views

QmlTui views accept the same model kinds as Qt Quick: an integer count, a
JavaScript array or list of maps, and any `QAbstractItemModel`, including
`QStandardItemModel`, `QStringListModel`, proxies such as
`QSortFilterProxyModel`, and your own list, table, or tree models. This page
states what each view does with a C++ model, so a model written for Qt Widgets
or Qt Quick behaves predictably in the terminal.

Everything below is exercised by `tests/Core/test_model_contracts.cpp` (the
view by model matrix), `tests/Core/test_tree_view.cpp`, and
`tests/Core/test_table_view_hardening.cpp`.

## Support matrix

| Capability | ListView | GridView | Repeater | TableView | TreeView | Scrollback |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| Flat rows, named roles as delegate properties | yes | yes | yes | yes | yes | yes |
| Live updates (`dataChanged`, insert, remove, move, reset) | yes | yes | yes | yes | yes | append and reset only |
| Only the viewport's delegates exist (virtualized) | yes | yes | no, eager by contract | yes | yes | n/a, writes rows once |
| Model columns | column 0 | column 0 | column 0 | all, via `index(row, column)` | column 0 | column 0 |
| `headerData()` drives titles and alignment | n/a | n/a | n/a | yes, when `columns` is unset | n/a | n/a |
| Nested rows (tree models) | via `rootIndex` | via `rootIndex` | via `rootIndex` | top level only | yes | top level only |
| Lazy population (`canFetchMore` / `fetchMore`) | on scroll | on scroll | eagerly, all pages | on selection scroll | on expand | when caught up |
| Standard item roles (`ForegroundRole`, `CheckStateRole`, ...) | via `StandardItemDelegate` | via `StandardItemDelegate` | via `StandardItemDelegate` | built-in painter and `StandardItemDelegate` | built-in painter and `StandardItemDelegate` | as named values |

Every view also exposes the standard styling roles and item flags to
delegates under Widgets-style names; see "Standard item roles" below.

## Delegate properties

Every C++ model row provides these to its delegate, both as context
properties and as required properties:

- `index`: the row within the level shown.
- `model`: a map of all named roles. `model.display` is the `Qt::DisplayRole`
  value when the model uses Qt's default role names.
- `modelData`: the same map for C++ models; the raw value for arrays.
- One property per role name from `roleNames()`.

`TableView` cell delegates add `row`, `column`, `current`, and `selected`.
`TreeView` adds `treeView`, `isTreeNode`, `row`, `column`, `current`, `selected`, `depth`, `expanded`, and `hasChildren`.

These reach a delegate only through properties it declares in QML. A C++
property of the root type, `Text.text`, `Rectangle.color`, `Item.enabled`, is
never written, so a role named `color` cannot clobber a `Text` delegate's
color binding; declare `required property string color` to receive it.

## Showing one level of a tree model

`ListView`, `GridView`, and `Repeater` show a single level. Bound to a tree
model they show the top level and emit one warning:

```
qmltui::ListView: model has nested rows; this view shows one level.
Use TreeView, or set rootIndex to show a subtree.
```

Set `rootIndex` to show the children of one node instead, exactly like
`DelegateModel.rootIndex` in Qt Quick. The index is held persistently, so
inserts above it do not retarget the view:

```qml
ListView {
    model: fileSystemModel
    rootIndex: fileSystemModel.index(2, 0) // children of the third top-level row
    delegate: Text { required property string display; text: display }
}
```

Use `TreeView` to show several levels at once.

## TableView columns from the model

With no `columns` descriptors and a C++ model, `TableView` takes its column
count from `columnCount()` and its titles from
`headerData(section, Qt::Horizontal)`. Width is shared evenly. A header row is
shown only when at least one section returns a string; the default
`headerData()` of `QAbstractItemModel` returns section numbers, which do not
count. `Qt::TextAlignmentRole` on a header section becomes the default
alignment for that column's cells. `headerDataChanged` repaints the header.

Explicit `columns` descriptors always win, and JavaScript array models keep the
historical default of five columns.

## Lazily populated models

Views call `canFetchMore()` and `fetchMore()` the way `QTreeView` and
`QListView` do:

- `ListView`, `GridView`, and `TableView` fetch when the materialized window,
  including overscan, reaches the end of the loaded rows, and keep fetching
  until the viewport is filled or the model has no more.
- `Repeater` instantiates every row, so it also loads every page up front.
- `TreeView` fetches a branch the first time it is expanded. A branch that
  reports `hasChildren()` while `rowCount()` is still zero shows an expander
  and fills in when `rowsInserted` arrives, which is the `QFileSystemModel`
  contract.
- `Scrollback` asks for the next page once it has written every loaded row.

A model that fetches asynchronously simply emits `rowsInserted` later; the view
resumes from there. See `examples/topics/lazy_model/` for a directory tree
that loads on expansion and counts its fetches.

## TreeView state across model changes

For C++ models, expansion is keyed by `QPersistentModelIndex`, not by display
text. Renaming a node, inserting siblings above it, sorting through a proxy,
and moving rows keep it expanded and keep the current node current.
`dataChanged` updates the affected visible nodes in place without rebuilding
the tree or re-creating delegates, except when a node changes shape, such as a
leaf gaining children, or when an expanded branch is renamed, which changes
every descendant's path.

Two limits follow from Qt's own semantics. `modelReset` invalidates all
persistent indexes, so everything is collapsed after a reset. A
`QSortFilterProxyModel` invalidates the indexes of rows it filters out, so a
branch that is filtered away and later returns comes back collapsed.

## Standard item roles

Qt Widgets styles items through numeric roles that have no entry in
`roleNames()`: `Qt::ForegroundRole`, `Qt::BackgroundRole`,
`Qt::TextAlignmentRole`, `Qt::CheckStateRole`, `Qt::DecorationRole`, plus the
`Qt::ItemIsEnabled` and `Qt::ItemIsUserCheckable` flags. `QStyledItemDelegate`
paints them; Qt Quick views ignore them. QmlTui does what Widgets does.

For every C++ model row, views add these delegate properties when the model
has not claimed the name itself in `roleNames()`:

| Name | Value | Source |
| :--- | :--- | :--- |
| `foreground`, `background` | color string such as `#ff0000` or `red`, empty when unset | `ForegroundRole`, `BackgroundRole` as `QColor`, `QBrush`, or string |
| `textAlignment` | `left`, `center`, `right`, or empty | `TextAlignmentRole` |
| `checkState` | `Qt.Unchecked`, `Qt.PartiallyChecked`, `Qt.Checked`, or `-1` when the item has no check state | `CheckStateRole` |
| `decoration` | glyph string; images are dropped | `DecorationRole` when it is a string or character |
| `enabled`, `checkable` | booleans | `flags()` |
| `modelIndex` | the `QModelIndex`, for `setData()` calls from QML; present for checkable items | the view |

A property is added when a delegate is created only if it carries a value:
a set color, a non-left alignment, a check state, a glyph, a disabled or
checkable item. Rows without styling cost nothing extra. When a row's data
changes or a pooled delegate is rebound to another row, every name is
published, including cleared ones, so read them with a fallback
(`model.foreground || textColor`) rather than as required properties.

`StandardItemDelegate` from `QmlTui.Controls` renders them: decoration glyph,
`[x]`, `[ ]`, or `[-]` for checkable items, colored and aligned text, dimmed
text for disabled items, and reversed text for the current row. Its
`toggle()` writes `Qt::CheckStateRole` back through the model's `setData()`,
on Space when the delegate has focus and on mouse click.

```qml
// A delegate for an application-provided C++ model:
ListView {
    model: standardItemModel
    delegate: StandardItemDelegate {}
}
```

The built-in painters of `TableView` and `TreeView` apply the same roles, so
`TableView { model: m }` without a delegate matches the delegate's look. The
selected row keeps the view's selection colors.
`examples/components/standard_item_delegate/` shows one styled
`QStandardItemModel` in all three views, with Space toggling a check box
through the model. When Qt Widgets is available it also builds
`qmltui_standard_item_delegate_widgets`, the same model in `QListView`,
`QTableView`, and `QTreeView` with their default `QStyledItemDelegate`, for a
side-by-side comparison (`QMLTUI_BUILD_WIDGETS_REFERENCE` turns it off).

`QColor` and `QBrush` are decoded through their `QDataStream` form, which
QtGui registers on the metatype, so QmlTuiCore still does not link QtGui. Only
RGB colors decode; HSV, HSL, CMYK, and extended-RGB colors are treated as
unset. Models that store colors as strings work in any process.

`QStandardItemModel` does not name these roles either, so
`setItemRoleNames()` is not required; a model that does name, for example,
`foreground` keeps its own value under that name.

## Scrollback is append-only

`Scrollback` renders each model row once into the terminal's scrollback and
never rewrites emitted output. `dataChanged` on an emitted row is therefore
not reflected, `rowsRemoved` only adjusts the consumed-row counter, and
`modelReset` starts consuming from row zero again. Use `ListView` or
`TableView` for rows that change after they are shown.

## What is deliberately not interpreted

- `QIcon` and `QPixmap` in `Qt::DecorationRole`: terminals draw glyphs, not
  images. A `QString` or `QChar` decoration is drawn as a leading glyph. Although it is possible to add this in the future using the Image QML type in Kitty/Sixel-capable terminals.
- Gradient and texture brushes: only a brush's solid color is used.
- `Qt::FontRole`: font families and sizes have no terminal meaning. Use the
  `bold`, `italic`, and `underline` role names, or a delegate.
- `Qt::ToolTipRole`, `Qt::StatusTipRole`, `Qt::WhatsThisRole`: exposed by name
  as Qt does, not rendered.

## Model and delegate compatibility

All views accept ready inline components and components that become ready
asynchronously. Delegate creation preserves the component's lexical QML context,
including `pragma ComponentBehavior: Bound`. A delegate root that is not a
`QmlTui.Item`, or a component that enters the error state, is rejected with a QML
warning instead of being partially installed.

For map and `QAbstractItemModel` data, named roles are available both as required
delegate properties and through `model`. `modelData` contains the row or cell
value described in the table below. `dataChanged` updates only the roles named by
the model when the model supplies such a list.

| Type | Standard delegate properties | `modelData` | QmlTui-specific behavior |
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
`canFetchMore()`/`fetchMore()`. See the [Support matrix](#support-matrix) above for
the full view-by-capability breakdown.
