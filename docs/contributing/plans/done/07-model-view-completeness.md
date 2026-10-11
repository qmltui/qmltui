# Plan 07: Any `QAbstractItemModel` Renders Correctly

## 1. Goal

Make the sentence "pass any `QAbstractItemModel` to a Qamui view and it works"
true, and prove it with a test matrix. Today that sentence holds for flat list
and table models in `ListView` and `TableView`. It does not hold for tree
models, lazily populated models, models that carry the standard Qt item roles,
or models whose header data should drive the table columns.

Everything in this plan is view-side work under `src/Qamui/Core/items/`. No public
QML API is removed. New properties follow Qt Quick names where an equivalent
exists.

## 2. Baseline (verified against the tree on 2026-09-11)

What already works, and must keep working (regression scope):

- All six model-backed views accept a `QAbstractItemModel*`: `ListView`,
  `GridView`, `Repeater`, `TableView`, `TreeView`, `Scrollback`.
- `roleNames()` become delegate context properties, plus `index`, `model`,
  `modelData`. `dataChanged` honours the changed-roles list.
- `ListView`, `GridView`, `Repeater`, `TableView` connect `dataChanged`,
  `rowsInserted`, `rowsRemoved`, `rowsMoved`, `layoutChanged`, `modelReset`.
  `TableView` also handles the `columns*` signals.
- `TableView` maps view columns to model columns via `index(row, column)` and
  falls back to column 0 for single-column models (`TableView.cpp:160`).
- `QSortFilterProxyModel` over `QStandardItemModel` is covered by
  `tests/Core/test_table_view_hardening.cpp:196`.
- `ListView` and `TableView` are virtualized (`ListView::rebuildViewport`,
  `TableView::generateDelegateItems`).

Gaps, each with the line that proves it:

| # | Gap | Evidence |
| :---: | :--- | :--- |
| G1 | `TreeView` re-flattens the whole visible tree on every signal; expansion state is keyed by `displayText#row` strings, so a rename or an insert above a node collapses it | `TreeView.cpp:67-90` (all handlers set `m_treeDirty`), `TreeView.cpp:549` (key), `TreeView.h:164` (`QSet<QString>`) |
| G2 | `GridView` creates a delegate for every row of the model, no viewport | `GridView.cpp:301-302` |
| G3 | Standard roles (`Foreground`, `Background`, `TextAlignment`, `CheckState`, `Decoration`, `Font`) are not interpreted by the built-in painters of `TableView` and `TreeView` | only `Qt::DisplayRole` appears in `items/*.cpp` |
| G4 | No view calls `canFetchMore()` / `fetchMore()` | grep of `items/` returns nothing |
| G5 | `TableView` ignores `headerData()`, and with no `columns` set it shows a hard-coded 5 columns regardless of `model->columnCount()` | `TableView.cpp:158`, `TableView.cpp:412-420` |
| G6 | A tree model bound to `ListView`, `GridView`, or `Repeater` silently shows only the top level; nothing warns | `Repeater.cpp:221,247`, `GridView.cpp:222`, `ListView.cpp:511` (`parent().isValid()` early returns) |
| G7 | `Scrollback` never reflects `dataChanged` on rows already emitted | `Scrollback.cpp:63-77` connects only insert, remove, reset |

G7 is by design (output already left the retained buffer) and is closed by
documentation only. G1 to G6 are code.

Constraint that shapes G3: `QamuiCore` links `Qt6::Core` and `Qt6::Qml` only,
never `Qt6::Gui` (`src/Qamui/CMakeLists.txt:156`). `QColor` cannot be named in
Qamui code. It can still be consumed: a `QColor` can only exist in the process
if the application loaded QtGui, and QtGui registers the `QColor` to `QString`
`QVariant` converter, so `variant.toString()` followed by
`qamui::Color::fromString()` works without a link dependency.

---

## 3. Work Packages

Ordered by value per day. Each package is independently mergeable and ships
with its tests. Estimates assume one engineer familiar with the tree.

### WP1: `TableView` header data and column count (G5) — 0.5 day

1. `TableView::columnCount()`: when `m_columns` is empty and `m_cppModel` is
   set, return `m_cppModel->columnCount()` (clamped to at least 1). Keep 5 as
   the fallback for JS list models with no `columns`.
2. `resolveColumns()`: default title comes from
   `headerData(column, Qt::Horizontal, Qt::DisplayRole)` when the model
   provides one, then the existing `Col%1` fallback. An explicit `columns`
   entry still wins, so nothing existing changes appearance.
3. Connect `headerDataChanged` to invalidate geometry (`m_geometryDirty`).
4. Honour `headerData(..., Qt::TextAlignmentRole)` for the header cell and as
   the default cell alignment for that column when the descriptor sets none.

Tests (`tests/Core/test_table_view_hardening.cpp`): a 3-column
`QStandardItemModel` with `setHorizontalHeaderLabels` renders exactly three
titled columns with no `columns` property; changing a label repaints the
header; a list model with `columns: 2` keeps two columns.

### WP2: Nested-model diagnostics and `rootIndex` on flat views (G6) — 0.5 day

1. When a `QAbstractItemModel` is assigned to `ListView`, `GridView`, or
   `Repeater`, check `model->hasChildren(model->index(0, 0))` once (O(1)) and
   emit a single `qmlWarning`: "model has nested rows; <View> shows one level.
   Use TreeView, or set rootIndex to show a subtree."
2. Add `Q_PROPERTY(QModelIndex rootIndex)` to `ListView`, `GridView`, and
   `Repeater`, same name and semantics as `DelegateModel.rootIndex` in Qt
   Quick. All `index(row, 0)` calls take `m_rootIndex` as parent, and the
   `parent().isValid()` guards become `parent == m_rootIndex`. A
   `QPersistentModelIndex` keeps it valid across inserts.

Tests (`tests/Core/test_views_virtualization.cpp` or a new
`test_model_contracts.cpp`, see WP7): warning emitted once for a tree model;
`rootIndex` set to a branch shows that branch's children and follows inserts
under it.

### WP3: `GridView` virtualization (G2) — 1.5 days

Mirror `ListView` exactly rather than inventing a second scheme.

1. Replace `QList<Item*> m_createdItems` (dense) with `QMap<int, Item*>`
   keyed by model index, plus `m_modelCount`, `m_materializedFirst/Last`.
2. `rebuildViewport()`: columns per row is `max(1, width() / cellWidth)`;
   visible rows derive from `contentY()` and `cellHeight`, with the same
   two-row overscan as `ListView`. Only those rows' cells are created; others
   are destroyed or recycled.
3. Reuse `ListView`'s `reuseItems` pool logic through a small shared helper
   (`DelegateUtils` or a new `ViewportRange_p.h`) so both views share one
   range computation. Do not touch `Repeater`: Qt Quick's `Repeater` is eager
   by contract, and ours stays eager.
4. `insertModelRows` / `removeModelRows` / `rowsMoved` shift keys the way
   `ListView::insertModelRows` does (`ListView.cpp:570`).

Tests: extend `test_views_virtualization.cpp` with a `GridView` on a 100 000
row `TestCountModel`; assert created items ≤ visible rows + overscan, that
scrolling materialises new rows and releases old ones, and that `currentIndex`
survives inserts. Add a `GridView` row to `benchmark_views.cpp`.

### WP4: Standard roles in built-in painters (G3) — 1.5 days

Applies to the non-delegate rendering paths of `TableView` (`cellText` and the
cell painter around `TableView.cpp:678`) and `TreeView` (row painter). Delegate
users already receive every named role and are unaffected.

1. New private header `items/StandardRoles_p.h` with one function that reads
   the standard roles for a `QModelIndex` and returns a small struct:
   `text`, `foreground`, `background`, `alignment`, `checkState`,
   `decorationGlyph`, `bold/italic/underline`, `enabled`.
   - `ForegroundRole` / `BackgroundRole`: accept `QMetaType::QString` and any
     type whose `toString()` yields a `#rrggbb` or named colour (this covers
     `QColor` per the Section 2 constraint). `QBrush` is not convertible
     without QtGui and is ignored.
   - `TextAlignmentRole`: `Qt::Alignment` to the existing left/center/right.
   - `CheckStateRole`: render `[x]`, `[ ]`, `[-]` as a prefix; `Qt::ItemIsUserCheckable`
     in `flags()` plus Space toggles via `setData` when the model allows it.
   - `DecorationRole`: a `QString` or `QChar` is drawn as a leading glyph
     (this is how terminal apps carry icons); `QIcon`/`QPixmap` are ignored.
   - `FontRole`: a `QFont` is only readable with QtGui; read `bold`, `italic`,
     `underline` through `QVariant::toString()` parsing is unreliable, so
     instead honour the Qt Quick convention of role names `bold`, `italic`,
     `underline` if present, and document that `FontRole` is not interpreted.
   - `flags()` without `Qt::ItemIsEnabled`: paint dimmed and skip in keyboard
     selection.
2. `TableView`: apply foreground and background per cell; header uses the
   `headerData` alignment from WP1. `alternatingRows` still applies when the
   model gives no background.
3. `TreeView`: apply foreground, decoration glyph, check state per row.

Tests: `QStandardItemModel` with `setForeground(QColor)`, `setCheckable`,
`setTextAlignment`, and a disabled item; assert the styled frame captured by
`TestRunner` shows the colours, the check prefix, right alignment, and the dim
attribute. A second test uses plain string colours to run with no QtGui in
the process.

### WP5: Lazy population (G4) — 1 day

1. Shared helper in `DelegateUtils` or `StandardRoles_p.h`:
   `maybeFetchMore(model, parent, lastMaterializedRow)` that calls
   `fetchMore(parent)` when `canFetchMore(parent)` and the materialised range
   ends within `overscan` rows of `rowCount(parent)`. A per-view `m_fetching`
   flag prevents re-entry; `rowsInserted` re-arms it.
2. `ListView`, `GridView`, `TableView`: call it at the end of the viewport
   rebuild. `Repeater`: call it once on model assignment and after each
   `rowsInserted` until `canFetchMore` is false (eager view, eager fetch).
3. `TreeView`: on expand, if `canFetchMore(idx)` call `fetchMore(idx)`; treat
   `hasChildren(idx) && rowCount(idx) == 0` as "loading" and keep the node
   expandable until `rowsInserted` arrives (this is the `QFileSystemModel`
   contract).
4. `Scrollback`: call `fetchMore` when caught up with `rowCount()`, so a
   paged log model streams to the end.

Tests: a `BatchModel` (`QAbstractListModel`, 500 rows served 50 at a time)
in `test_model_contracts.cpp`. `ListView` with 20 visible rows triggers
exactly one fetch on load and one per page while scrolling; `TreeView` expand
on a lazy branch triggers one fetch and shows the children after
`rowsInserted`. A new example `examples/topics/lazy_model/` with a
hand-written lazy directory model (no `QFileSystemModel`, to avoid a QtGui
dependency in examples that are built headless).

### WP6: `TreeView` incremental updates and stable expansion (G1) — 2.5 days

1. Expansion state for C++ models becomes `QSet<QPersistentModelIndex>`
   (`m_expandedIndexes`), replacing the `displayText#row` keys. The string
   keys remain only for the JS `rootNode` map model. `expandAll()` and
   `collapseAll()` are rewritten on top of the set. Public `expand(path)`
   style API, if any, keeps working by resolving the path to an index once.
2. `dataChanged` no longer sets `m_treeDirty`. It updates `rawData` and the
   delegate properties of the affected visible nodes in place (the same loop
   `Repeater` uses at `Repeater.cpp:218`). Only if `hasChildren` flips for a
   node does it fall back to a re-flatten.
3. `rowsInserted` / `rowsRemoved` / `rowsMoved`: re-flatten only when the
   parent is the root or a currently expanded visible node; otherwise ignore.
   Selection (`m_selectedIndex`) and `m_scrollTop` are preserved by
   translating through a `QPersistentModelIndex` of the current node before
   and after the re-flatten.
4. Delegate reuse: `m_delegateItems` are rebound rather than destroyed when
   the flattened list changes, mirroring `ListView::rebindDelegateItem`.

Tests (`tests/Core/test_tree_view.cpp`, new): rename a node and the subtree
stays expanded; insert a sibling above an expanded node and it stays expanded
and selected; `dataChanged` on a leaf does not move the scroll position or
re-create delegate items (count `QObject` creations); `benchmark_views.cpp`
gains a `TreeView` `dataChanged` case with a 10 000 node model.

### WP7: Acceptance matrix and documentation (all gaps) — 1 day

1. `tests/Core/test_model_contracts.cpp`: one data-driven test that binds each
   of the six views to each of these models and checks first frame, one
   `dataChanged`, one insert, one remove, one `modelReset`:
   `QStringListModel`, `QStandardItemModel` (flat, 3 columns),
   `QStandardItemModel` (tree), `QSortFilterProxyModel` over each,
   `BatchModel` (lazy), and a custom `QAbstractTableModel` with only
   `Qt::DisplayRole`. This is the file the launch claim points at.
2. `docs/topics/models.md` (new, single source per Plan 01): a support matrix
   view × capability (flat rows, columns, tree, lazy, standard roles, header
   data, live updates), the `rootIndex` recipe, the Scrollback append-only
   note (G7), and what is deliberately not interpreted (`QIcon`, `QBrush`,
   `FontRole`).
3. Update `docs/qml-types/index.md` entries for `TableView`, `TreeView`, `GridView`,
   `ListView`, `Repeater` with the new `rootIndex` property and header
   behaviour; add a migration note to `docs/changes/0.4.md` for the
   `TableView` default column count change (5 → `model->columnCount()`).

---

## 3b. Status (2026-09-11)

| Package | State | Where |
| :--- | :--- | :--- |
| WP1 header data | done | `TableView::refreshModelHeader`, tests in `test_table_view_hardening.cpp` |
| WP2 diagnostics + `rootIndex` | done | `DelegateUtils::warnIfNested`, `rootIndex` on ListView/GridView/Repeater, `test_model_contracts.cpp` |
| WP3 GridView viewport | done | `GridView::rebuildViewport`, `test_views_virtualization.cpp`, `benchmark_views.cpp` |
| WP4 standard roles | done | `items/StandardRoles_p.h`, `Controls/StandardItemDelegate.qml`, `tests/Controls/test_standard_roles.cpp`, `examples/components/standard_item_delegate/` |
| WP5 lazy fetch | done | `DelegateUtils::fetchMoreIfNeeded` in all six views, `examples/topics/lazy_model/` |
| WP6 TreeView incremental | done | `m_expandedIndexes`, `handleModelDataChanged`, `test_tree_view.cpp` |
| WP7 matrix + docs | done | `TestModelContracts::matrix`, `docs/topics/models.md`, `qml_api.md`, `migration/0.4.md` |

WP4 shipped as a hybrid rather than painter-only code: the C++ helper reads
the numeric roles and flags, every view publishes them as delegate properties
(`foreground`, `background`, `textAlignment`, `checkState`, `decoration`,
`enabled`, `checkable`, `modelIndex`) when the model has not claimed the name,
`StandardItemDelegate` renders them, and the TableView and TreeView built-in
painters reuse the helper. `QColor` and `QBrush` are decoded from their
`QDataStream` form, so QamuiCore still does not link QtGui; `QStandardItem`
stores brushes, which is why both are handled.

Cost of WP4, measured with `benchmark_views` on the Qt 6.8.6 Release scratch
tree (same machine, two runs each): publishing the standard values as
delegate properties is what costs, not reading the roles. Publishing all
eight names for every row tripled GridView scrolling (257 ms to 912 ms per
1,000 steps over 100k rows) and TreeView flattening (11 ms to 34 ms per 10k
rows). Publishing only informative values on creation, computed without an
intermediate map, brings those back to 270 to 290 ms and 13 to 17 ms, within
run-to-run noise of the baseline; the residual is the five extra `data()`
calls and `flags()` per node, which the Widgets item views pay as well.

Findings while implementing, beyond the plan:

- An example whose QML module lives in a shared static library (one module,
  two executables) cannot find `Qamui.Controls` when launched from Qt Creator
  or a shell: the executable references no Controls C++ symbol, the linker's
  default `--as-needed` drops `libQamuiControls.so`, and with it the embedded
  `Qamui.Controls` qmldir resource; only the scenario runner's
  `QML2_IMPORT_PATH` hides this. Owning the module in each executable, as
  `table_view` and `btop` do, makes Qt record the build tree's QML import path
  for that executable and works on Qt 6.8 and 6.11: Qt writes a `qt.conf`
  with `QmlImports` beside such executables (`_qt_internal_write_qmldir_part`,
  driven by `DEPENDENCIES TARGET`), which `QLibraryInfo` reads from the
  application directory. `lazy_model`, `standard_item_delegate`,
  `cyber_audio`, and `custom_cpp_item` all use it now.

- `ListView` and `GridView` ignored `dataChanged` for delegates declared under
  `pragma ComponentBehavior: Bound`: the handler looked up the row map through
  a per-row `QQmlContext` that bound components do not have. Found by the
  `standard_item_delegate` example, fixed by finding the row map as the
  delegate's child, covered by `TestModelContracts::boundDelegatesFollowDataChanged`.
- `ListView { currentIndex: 0; model: m }` lost the index: the constant was
  applied before the `model` binding resolved and clamped to `-1`. It is now
  kept pending until rows exist, as Qt Quick does and as `TreeView` already
  did; covered by `TestModelContracts::listViewCurrentIndexBeforeModel`.

- `TreeView::expandToIndex()` inserted row-number keys (`"0/2"`) that never
  matched the `name#row` keys used by the flattener, so it never expanded a
  C++ model branch. Fixed by WP6's persistent-index state.
- Delegate reuse across re-flattens already existed in `TreeView` through
  `persistentLookup`; WP6 item 4 needed no new code.
- `TreeView` wrote `text` and `display` onto the delegate root item, so a
  `delegate: Text { text: <binding> }` showed the bare node label. Fixed on
  2026-09-11: all views write model data only to QML-declared delegate
  properties (`DelegateUtils::isDeclaredInQml`), never to a C++ property of
  the root type; TreeView keeps publishing undeclared names as dynamic
  properties. Covered by `TestTreeView::textRootKeepsItsBinding` and
  `TestModelContracts::rolesDoNotOverwriteBuiltInProperties`.
- A `QSortFilterProxyModel` invalidates persistent indexes of filtered-out
  rows, so a branch filtered away and brought back is collapsed. This matches
  Qt Quick and is documented rather than worked around.

## 4. Order and Exit Criteria

| Order | Package | Days | Exit criterion |
| :---: | :--- | :---: | :--- |
| 1 | WP1 header data | 0.5 | 3-column `QStandardItemModel` renders titled columns with no `columns` |
| 2 | WP2 diagnostics + `rootIndex` | 0.5 | Warning fires once; `rootIndex` shows a subtree |
| 3 | WP3 `GridView` viewport | 1.5 | 100 000 rows create ≤ visible + overscan items |
| 4 | WP4 standard roles | 1.5 | Styled frame shows colour, check box, alignment, dim |
| 5 | WP5 lazy fetch | 1 | `BatchModel` fully paged by scrolling; lazy tree expands |
| 6 | WP6 `TreeView` incremental | 2.5 | Rename keeps expansion; `dataChanged` creates no items |
| 7 | WP7 matrix + docs | 1 | `test_model_contracts` green for 6 views × 7 models |

Total: about 8.5 engineer-days, roughly two calendar weeks with review.

WP1 and WP2 are small enough to land before the launch tag and remove the two
most embarrassing surprises (five phantom columns, silent tree truncation).
WP3 to WP7 ship in the weeks after launch as independent PRs.

Done means: `docs/topics/models.md` says "any `QAbstractItemModel`" and
`test_model_contracts` is the evidence, with the three documented exclusions
(`QIcon`/`QPixmap`, `QBrush`, `FontRole`) and the Scrollback append-only rule.

## 5. Non-Goals

- Multi-column `TreeView`. Column 0 only, as today. Tracked for a later plan.
- Editing (`setData` from delegates, `Qt::ItemIsEditable`) beyond the
  check-state toggle in WP4.
- Drag and drop, `QMimeData`, `moveRows` initiated from the view.
- Linking `Qt6::Gui` into `QamuiCore`. Every role decode must work through
  `QVariant` conversions available in a Core-only process.
- A `DelegateModel` / `DelegateChooser` port. `DelegateUtils::resolveDelegate`
  already covers the `choices` pattern in use.
