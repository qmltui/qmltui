# Model View Performance Profile (2026-09-11)

> [!NOTE]
> **Historical profile**: This document records the pre-0.6 per-phase profile before demand-driven delegate schema inspection, on-demand `QVariantMap` replacement of per-row `QQmlPropertyMap`, and 48-byte `TreeView::VisibleNode` compaction. (`StandardItemDelegate` also later moved from `QmlTui.Controls` to [`examples/recipes/standard_item_delegate/`](../../../examples/recipes/standard_item_delegate/).) For current QmlTui vs. Qt Quick vs. Qt Widgets numbers, see [2026-10-27-view-comparison.md](2026-10-27-view-comparison.md).

Per-phase cost of `ListView`, `GridView`, `TableView`, and `TreeView` over a
C++ model, measured after Plan 07 (header data, `rootIndex`, GridView
virtualization, lazy fetch, TreeView persistent state, standard item roles).
The goal is to say where a frame's time goes and what is worth optimizing.

## Method

- `tests/Core/benchmark_view_phases.cpp` (built with `QMLTUI_BUILD_BENCHMARKS=ON`)
  instantiates each view at 80x24 over a 100,000-row `QAbstractListModel`
  with one `display` role and times four phases with `QElapsedTimer`:
  component creation plus first frame (per created delegate), 1,000 scroll
  steps of one row (`contentY`, `selectedRow`, or `currentIndex`), 1,000
  single-row `dataChanged` events, and 1,000 idle layout and paint passes.
  Every step includes the full layout and paint of the frame. Resident memory
  per delegate comes from `/proc/self/statm` around an eager `Repeater` of
  2,000 rows.
- Delegates: `text` is `Text { required property string display; text: display }`,
  `rect` wraps that in a `Rectangle`, `standard` is `QmlTui.Controls.StandardItemDelegate`,
  `bound` is `text` under `pragma ComponentBehavior: Bound`.
- Attribution: no `perf` or `valgrind` on the host, so
  `tests/tools/sample_stacks.sh` attaches `gdb` 40 times to
  `benchmark_view_phases --loop <View> text 60` built as RelWithDebInfo and
  aggregates inclusive stack frames. Percentages are shares of samples.
- Host: WSL2, 22 cores, Qt 6.8.6 Release (`-O3`), idle machine. Numbers vary
  about 10 percent between runs; a concurrent build doubles them.

## Results, 100,000 rows, 80x24

| View | Delegate | Bound | Delegates | Create us/delegate | Scroll us/step | dataChanged us | Idle layout us | Idle paint us |
| :--- | :--- | :---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ListView | text | no | 26 | 17 | 19 | 11 | 0.6 | 4.1 |
| ListView | text | yes | 26 | 19 | 5 | 8 | 0.3 | 0.4 |
| ListView | rect | no | 26 | 24 | 21 | 14 | 1.2 | 7.3 |
| ListView | standard | no | 26 | 103 | 28 | 20 | 2.2 | 6.3 |
| GridView | text | no | 100 | 18 | 95 | 19 | 2.3 | 16.4 |
| GridView | text | yes | 100 | 16 | 86 | 18 | 2.1 | 16.3 |
| GridView | rect | no | 100 | 28 | 131 | 30 | 4.3 | 24.4 |
| GridView | standard | no | 100 | 112 | 548 | 34 | 8.3 | 25.0 |
| TableView | text | no | 24 | 35 | 85 | 42 | 0.5 | 8.1 |
| TableView | text | yes | 24 | 27 | 75 | 37 | 0.5 | 7.3 |
| TableView | rect | no | 24 | 33 | 90 | 48 | 1.1 | 9.2 |
| TableView | standard | no | 24 | 75 | 142 | 51 | 1.6 | 8.7 |
| TreeView | text | no | 26 | 5,961 | 244 | 19 | 0.5 | 3.8 |
| TreeView | text | yes | 26 | 5,375 | 150 | 16 | 0.5 | 3.9 |
| TreeView | rect | no | 26 | 5,294 | 234 | 23 | 1.0 | 5.9 |
| TreeView | standard | no | 26 | 5,553 | 418 | 32 | 1.8 | 6.1 |

At 10,000 rows TreeView creation drops to about 550 us per delegate and its
scroll step to about 213 us; the other views do not change. Resident memory
per delegate on the eager Repeater: `text` and `rect` are below the
measurement's page granularity (tens of bytes apparent), `standard` about
5.9 KB.

A terminal frame at 60 Hz has a 16,000 us budget. Every idle frame above is
below 30 us, and every scroll step except GridView with the standard delegate
is below 600 us. Nothing here is slow for a terminal application; the
findings below are about scaling headroom and about consistency between the
views.

## Where the time goes (stack samples, scrolling with the `text` delegate)

**TreeView, 244 us per step.** 90 percent of samples are in
`TreeView::generateDelegateItems`, 75 percent in `updateDelegateItem`, and
inside it 42 percent in `QObject::setProperty` by name (22 percent in
`QMetaObject::indexOfProperty`, 15 percent in `strcmp`) plus 20 percent in
`QQmlContext::setContextProperty` and QV4 string allocation. Every scroll
step re-publishes about fifteen properties to every delegate, including the
reused ones whose node did not change. Flattening the model is not on the
scroll path at all; it is the creation cost (5.9 ms per delegate at 100k rows
is 155 ms of one-time flattening spread over 26 delegates).

**GridView, 95 us per step.** 75 percent in `rebuildViewport`, 60 percent in
`createDelegateItem`: `QQmlComponent::beginCreate` 15 percent,
`QQmlPropertyMap::insert` 12 percent, and inside that
`QQmlOpenMetaObject::createProperty` and `QMetaObjectBuilder::toMetaObject`,
plus 10 percent destroying the delegates that scrolled out. GridView has no
reuse pool, so each one-row step creates four delegates and destroys four,
and each new `QQmlPropertyMap` builds its own dynamic meta-object.

**ListView, 19 us per step.** 45 percent in `rebuildViewport` through
`rebindDelegateItem` (20 percent `setPropertyIfExists` with
`indexOfProperty`, 18 percent constructing `QPointer`s, 10 percent
`setContextProperty`, 8 percent `StandardRoles::apply`) and 50 percent in the
paint pass (`Text::paint`, `TextGridBuffer::setGrapheme`). With bound
delegates the context properties disappear and the step costs 5 us, of which
paint is most.

## Findings, ranked by value

1. **TreeView re-publishes unchanged delegate state on every scroll step.**
   Ten times ListView's step cost for the same work. `updateDelegateItem`
   should compare against the node the delegate already shows and write only
   `index`, `row`, `current`, and `selected` when the node is unchanged, and
   look up property indexes once per delegate instead of by name each time.
   Expected: TreeView scroll within two times ListView.

2. **TreeView flattens the whole model and stores a role map per node.**
   O(rows) time and memory on creation and on every structural change: 155 ms
   and 100,000 `QVariantMap`s for a 100k-row flat model, before a single
   delegate exists. Fetching `rawData` lazily for the visible window, or
   keeping only `QPersistentModelIndex`, depth, and expansion per node, would
   make creation independent of model size, as ListView already is.

3. **GridView has no delegate reuse pool.** WP3 deliberately left it out.
   Adding ListView's `reuseItems` and `rebindDelegateItem` would remove the
   create and destroy share, about 70 percent of the step. Expected: GridView
   step near four times ListView's per-delegate rebind, so roughly 20 to
   30 us instead of 95.

4. **One dynamic meta-object per delegate row.** Each `QQmlPropertyMap` used
   as the `model` object builds its own `QQmlOpenMetaObjectType`, so every
   `insert` on a fresh map costs a meta-object rebuild. Reuse (finding 3)
   hides it; a structural fix would share one type per model across rows,
   which needs the private `QQmlOpenMetaObject` API, or would expose `model`
   as a plain `QVariantMap` reassigned on change, as TableView does.

5. **`StandardItemDelegate` costs five to six times a plain `Text`** to create
   (about 100 us) and about 5.9 KB resident per instance: an `Item`, a
   `Rectangle`, a `Text`, a `MouseArea`, and a dozen bindings. Acceptable for
   its purpose; a lighter variant would drop the `MouseArea` in favor of the
   view's own mouse handling and create the `Rectangle` only for rows with a
   background. Its idle paint cost is not higher than `rect`.

6. **Unbound delegates pay for context properties.** ListView's bound step is
   four times cheaper than unbound, and idle paint ten times cheaper. Every
   QmlTui example already uses `pragma ComponentBehavior: Bound`; the docs
   should say plainly that it is also the fast path.

7. **Name-based property writes are the common tax.** `setPropertyIfExists`
   and `QObject::setProperty` resolve names through `indexOfProperty` and
   `strcmp` on every write, visible in all three profiles (8 to 22 percent).
   Caching `QMetaProperty` indexes per delegate component would help every
   view at once.

## After the fixes (same day)

Findings 1, 2, and 3 were implemented the same day, plus the shared
name-lookup tax for TreeView's scroll path (finding 7, TreeView only).

| View | Delegate | Create us/delegate before | after | Scroll us/step before | after |
| :--- | :--- | ---: | ---: | ---: | ---: |
| GridView | text | 18 | 17 | 95 | 58 |
| GridView | standard | 112 | 104 | 548 | 89 |
| TreeView | text | 5,961 | 668 | 244 | 98 |
| TreeView | text, bound | 5,375 | 606 | 150 | 75 |
| TreeView | standard | 5,553 | 518 | 418 | 164 |

What changed:

- **TreeView flattens shape only.** `flattenCppModel` reads no role: a node
  is an index, a depth, `hasChildren`, an expansion flag, and a row-path key
  such as `#2/#0`. Text, path, and role data are loaded on first use by
  `ensureLoaded()`, for the rows that are shown or painted. Node indexes are
  plain `QModelIndex` refreshed on every re-flatten; the current node is
  tracked by one persistent index. `benchmark_views` reports 10k rows
  flattened in 2 ms, down from 13, and 100k in about 15 ms, down from 155.
  The remaining per-row cost is `hasChildren()` and `index()` on the model.
- **TreeView scroll writes position only.** A delegate that still shows the
  same, unchanged node after a re-flatten receives `index`, `row`, `current`,
  and `selected`, written through property indexes cached per delegate
  meta-object, instead of fifteen name-resolved writes. Delegates without
  those declared properties keep receiving them as dynamic properties, as
  before.
- **GridView pools delegates** (`reuseItems`, default on, as ListView). A
  cell that scrolls out is hidden and rebound to the next row that scrolls
  in, so no `QQmlPropertyMap` or meta-object is rebuilt per step. Delegates
  chosen through `choices` are not pooled.
- **ListView bound delegates were never rebound.** `rebindDelegateItem`
  returned early when a delegate had no per-row context, so under
  `pragma ComponentBehavior: Bound` a pooled delegate kept its old row. The
  earlier 5 us "bound scroll step" was this bug; the correct figure is
  about 18 us. Covered by `TestModelContracts::reusedDelegatesShowTheirRow`.

TreeView's scroll step is now about five times ListView's rather than
thirteen; what remains is regenerating the delegate window on each step
(hash lookups, node snapshots) and loading the newly visible row.

## Not a problem

- Idle frames: all views lay out and paint 80x24 in under 30 us. Painting
  itself, `Text::paint` and `setGrapheme`, is the dominant idle cost and is
  proportional to visible cells, as it should be.
- `dataChanged` on a visible row: 8 to 50 us including the repaint.
- TableView: 24 cell delegates, 35 us to create each, 85 us per row scrolled.
  Its delegate records already avoid re-reading unchanged roles.
- The standard-role reads added by Plan 07 WP4: 5 `data()` calls and
  `flags()` per materialized row, about 8 percent of a ListView rebind and
  invisible elsewhere.

## Reproducing

```
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DQMLTUI_BUILD_BENCHMARKS=ON
cmake --build build-bench --target benchmark_view_phases
build-bench/tests/Core/benchmark_view_phases       # table above
build-bench/tests/Core/benchmark_view_phases --rows 10000
# attribution (RelWithDebInfo build, gdb installed):
tests/tools/sample_stacks.sh build-rwdi/tests/Core/benchmark_view_phases TreeView text 40
```
