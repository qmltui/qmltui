# Models in Qtui Views

Qtui views accept the same model kinds as Qt Quick: an integer count, a
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
| Standard item roles (`ForegroundRole`, `CheckStateRole`, ...) | as plain values | as plain values | as plain values | as plain values | as plain values | as plain values |

"As plain values" means the role is exposed to delegates under the name the
model gives it in `roleNames()`, and nothing else. The built-in painters of
`TableView` and `TreeView` read `Qt::DisplayRole` only. Interpreting the
standard styling roles is planned (see `docs/plans/07-model-view-completeness.md`,
work package 4).

## Delegate properties

Every C++ model row provides these to its delegate, both as context
properties and as required properties:

- `index`: the row within the level shown.
- `model`: a map of all named roles. `model.display` is the `Qt::DisplayRole`
  value when the model uses Qt's default role names.
- `modelData`: the same map for C++ models; the raw value for arrays.
- One property per role name from `roleNames()`.

`TableView` cell delegates add `row`, `column`, `current`, and `selected`.
`TreeView` adds `depth`, `expanded`, `hasChildren`, and `treeView`.

## Showing one level of a tree model

`ListView`, `GridView`, and `Repeater` show a single level. Bound to a tree
model they show the top level and emit one warning:

```
qtui::ListView: model has nested rows; this view shows one level.
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
resumes from there. See `examples/concepts/lazy_model/` for a directory tree
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

## Scrollback is append-only

`Scrollback` renders each model row once into the terminal's scrollback and
never rewrites emitted output. `dataChanged` on an emitted row is therefore
not reflected, `rowsRemoved` only adjusts the consumed-row counter, and
`modelReset` starts consuming from row zero again. Use `ListView` or
`TableView` for rows that change after they are shown.

## What is deliberately not interpreted

- `QIcon` and `QPixmap` in `Qt::DecorationRole`: terminals draw glyphs, not
  images. A `QString` or `QChar` decoration will be drawn once work package 4
  lands.
- `QBrush` in `Qt::ForegroundRole` or `Qt::BackgroundRole`: Qtui does not link
  QtGui. A `QColor` can be decoded through its string form when QtGui is in the
  process; a brush cannot.
- `Qt::FontRole`: font families and sizes have no terminal meaning. Use the
  `bold`, `italic`, and `underline` role names, or a delegate.
