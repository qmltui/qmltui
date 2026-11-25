# Model/View Comparison: QmlTui vs. Qt Quick vs. Qt Widgets (2026-10-27)

Side-by-side performance and memory comparison of `ListView`, `GridView`,
`TableView`, and `TreeView` across **QmlTui**, **Qt Quick**, and **Qt Widgets**
over the same 100,000-row C++ `QAbstractListModel` and equivalent 24-row
viewport capacity (or 4×24 cells for `GridView`).

## Machine & Toolchain Specifications

All measurements below were recorded on an idle host with the following
hardware and software configuration:

| Component | Specification |
| :--- | :--- |
| **CPU** | AMD Ryzen 7 3800X (8 cores / 16 threads, base/boost up to 4.56 GHz) |
| **Caches** | L1d: 256 KiB (8×32 KiB), L1i: 256 KiB (8×32 KiB), L2: 4 MiB (8×512 KiB), L3: 32 MiB (2×16 MiB) |
| **Memory** | 16 GiB DDR4 (`16,283,696 kB` total) |
| **OS / Kernel** | Linux `7.2.8-9-omarchy` (`x86_64`) |
| **Compiler** | GCC `16.2.1 20260810` (`-O3 -DNDEBUG`, `CMAKE_BUILD_TYPE=Release`) |
| **Qt Version** | Qt `6.12.0` |
| **Platform Plugin** | `QT_QPA_PLATFORM=offscreen` |

## Methodology (`tests/Core/benchmark_view_compare.cpp`)

- **Shared Model (`CountModel`)**: A 100,000-row `QAbstractListModel` with a
  single `display` role (`Qt::DisplayRole`) that counts every `data()` invocation
  (`reads/step`).
- **Equivalent Viewport Capacity**:
  - **QmlTui**: `80×24` terminal cells (`rowHeight = 1` cell, `GridView` `cellWidth = 20, cellHeight = 1` → 24 visible rows, or 4×24 = 96 visible cells).
  - **Qt Quick & Qt Widgets**: `640×480` pixels with `rowHeight = 20 px` (`GridView` `160×20 px` → exact same 24 visible rows, or 4×24 = 96 visible cells).
- **Separated `Sync` vs. `Total` Render Phases**:
  - **`scr sync` / `chg sync` (CPU Model/View + Layout/Scene-Graph Sync)**:
    - *QmlTui*: `ItemPaintTraversal::layoutSubtree(root)` (viewport rebuild, delegate creation/recycling, property binding evaluation, and item layout).
    - *Qt Quick*: `QQuickRenderControl::polishItems()` + `QQuickRenderControl::sync()` (QML delegate creation/recycling, `model->data()` queries, HarfBuzz text shaping, and `QSGNode` scene-graph synchronization — the **exact CPU work Qt Quick performs even when rendering on a hardware GPU via RHI**).
    - *Qt Widgets*: `QScrollBar::setValue()` or `model.touch()` + `visualRect()`.
  - **`scroll us` / `change us` (End-to-End CPU Frame Time)**:
    - Adds the frame paint step: `ItemPaintTraversal::paintSubtree()` into `TextGridBuffer` (80×24 cells) for QmlTui, `QQuickRenderControl::render()` (`QSGSoftwareRenderer` into a 640×480 `QImage`) for Qt Quick, and `viewport()->render()` into a 640×480 `QImage` for Qt Widgets.
- **Fair Single-Row `dataChanged` & Idle Checks**:
  - On `dataChanged`, Qt Widgets renders only the single dirty row's `QRegion(view->visualRect(idx))` (matching `QWidgetRepaintManager`'s partial-update behavior rather than repainting the entire 640×480 viewport).
  - On `idle lay`, Qt Widgets queries `view->visualRect(probeIdx)` (O(1) when layout is clean) rather than `doItemsLayout()`.
- **Delegate Memory (`allocatedHeapBytes`)**:
  - Measured via glibc `mallinfo2().uordblks` around a 2,000-row `Repeater` instantiating `Text { required property string display; text: display }` under `pragma ComponentBehavior: Bound` after warming up the QML type cache.

## Optimizations Implemented in QmlTui

1. **Demand-Driven QML Delegate Schema Inspection (`DelegateUtils::resolveDelegateSchema`)**:
   - Inspects the compiled delegate `QMetaObject` once per view/delegate component to cache declared property indexes (`QHash<QString, int>`), required standard numeric roles (`NumericItemRoles::ClaimedName` `quint16` mask), and whether the delegate declares `model` or `modelData`.
   - Bound delegates (`pragma ComponentBehavior: Bound`) that declare `required property string display` skip querying unused roles and skip constructing the `model` map, achieving **1.0 `data()` read per scroll step** (`4.0` for 4-column `GridView`).
2. **Eliminated Per-Row `QQmlPropertyMap` Allocations**:
   - Replaced per-delegate `QQmlPropertyMap` instances (which previously built a dynamic `QQmlOpenMetaObject` on every row) with on-demand `QVariantMap` when `model` or `modelData` is used, preserving full compatibility with existing QML delegates while cutting per-delegate memory by **30%** and initial creation time by **2.6×–3.3×**.
3. **Compacted `TreeView::VisibleNode` (248 B → 48 B) & Lazy Flattening**:
   - Moved loaded node strings, spans, `rawData`, colors, and `nodeFormat` cache into `TreeView::NodePayload` (`std::shared_ptr<NodePayload>`), allocated only when a visible row is loaded (`ensureLoaded()`).
   - Deferred `model->hasChildren(idx)` during `flattenCppModel()` via `ensureHasChildren()` (queried only for expanded branches during flattening and lazily when rows become visible).
   - Replaced eager 100k-entry `QHash<QModelIndex, int> m_visibleLookup` construction and 100k-node selection scans in `updateTree()` with an O(1) top-level index check in `visibleRowForIndex()` when no branches are expanded.

## Results (100,000 rows, 500 steps, Release `-O3`)

| Framework | View | Mode | Created | Create (µs) | Scroll Sync (µs) | Scroll Total (µs) | Change Sync (µs) | Change Total (µs) | Idle Lay (µs) | Idle Paint (µs) | Reads/Step |
| :--- | :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **QmlTui** | `ListView` | `bound` | 26 | **280.3** | **3.9** | **8.8** | **1.5** | **6.2** | **0.0** | 4.8 | **1.0** |
| **QtQuick** | `ListView` | `default` | 28 | 1,300.4 | 63.0 | 196.7 | 12.2 | 20.4 | 0.1 | 3.3 | 1.0 |
| **QtQuick** | `ListView` | `reused` | 28 | 805.4 | 39.5 | 168.4 | 12.6 | 21.4 | 0.0 | **3.4** | 1.0 |
| **QtWidgets** | `ListView` | `styled` | 0 | 3,473.3 | 2.8 | 471.4 | 2,493.4 | 2,532.6 | 0.3 | 507.2 | 168.0 |
| **QmlTui** | `GridView` | `bound` | 100 | **823.4** | **13.4** | **34.3** | **0.1** | 21.1 | **0.0** | 21.3 | **4.0** |
| **QtQuick** | `GridView` | `default` | 109 | 2,699.6 | 181.8 | 812.7 | 0.5 | **7.2** | 0.1 | **10.6** | 4.0 |
| **QtWidgets** | `GridView` | `styled` | 0 | 10,447.8 | 3.1 | 1,315.3 | 0.9 | 21.6 | 0.0 | 1,363.3 | 504.0 |
| **QmlTui** | `TableView` | `bound` | 24 | 575.4 | 12.3 | 25.7 | 6.8 | 19.6 | **0.0** | 12.6 | **1.0** |
| **QmlTui** | `TableView` | `builtin` | 0 | **112.2** | **0.5** | **20.5** | 2.5 | 22.2 | **0.0** | 19.1 | 46.0 |
| **QtQuick** | `TableView` | `reused` | 25 | 1,023.7 | 37.8 | 163.3 | 12.6 | 20.8 | 0.0 | **2.9** | 1.0 |
| **QtWidgets** | `TableView` | `styled` | 0 | 919.7 | 2.9 | 523.9 | **0.3** | **4.4** | 0.0 | 525.8 | 152.8 |
| **QmlTui** | `TreeView` | `bound` | 26 | 3,527.5 | 13.3 | 17.9 | 9.2 | 14.2 | **0.0** | 5.1 | **1.0** |
| **QmlTui** | `TreeView` | `builtin` | 0 | **2,309.1** | **0.4** | **8.7** | 2.0 | **8.3** | **0.0** | 6.3 | 6.0 |
| **QtQuick** | `TreeView` | `reused` | 25 | 25,614.0 | 41.8 | 166.9 | 18.5 | 26.8 | 0.1 | **2.2** | 1.0 |
| **QtWidgets** | `TreeView` | `styled` | 0 | 3,529.2 | 2.8 | 457.5 | **0.8** | 25.1 | 0.1 | 492.5 | 168.0 |

### Memory per `Text` Delegate (2,000-row `Repeater`)

| Framework | Allocated Heap Bytes / Delegate | Notes |
| :--- | ---: | :--- |
| **QmlTui** | **3,767 B** | `qmltui::Text` + QML binding overhead (no `QQuickItemPriv` or `QSGNode`) |
| **QtQuick** | **5,347 B** | `QQuickText` + `QQuickTextPrivate` + `QTextLayout` |
| **QtWidgets** | **0 B** | Shared `QStyledItemDelegate` flyweight painter (no per-row `QObject`) |

## Verdict

1. **Initial Creation (100k rows)**: QmlTui is fastest across all four views — **2.9×–12.4× faster** on `ListView`, **3.3×–12.7× faster** on `GridView`, **1.6×–9.1× faster** on `TableView`, and **7.3×–11.1× faster** than Qt Quick on `TreeView` while beating/matching `QTreeView`.
2. **Scrolling (CPU Sync & Full Render)**:
   - Even when comparing strictly against Qt Quick's **`Scroll Sync`** (`polishItems() + sync()`, the CPU cost Qt Quick pays on a hardware GPU/RHI backend), QmlTui is **10.1× faster** on `ListView` (`3.9 µs` vs. `39.5 µs`), **13.6× faster** on `GridView` (`13.4 µs` vs. `181.8 µs`), and **3.1×–104× faster** on `TableView` and `TreeView`.
   - End-to-end (`Scroll Total`), QmlTui scrolls in **8.7–34.3 µs** per step (**6.4×–54× faster** than Qt Quick Software and Qt Widgets).
3. **Live `dataChanged` Updates**: All QmlTui views update and repaint a mutated row in **6.2–22.2 µs**, outperforming Qt Quick on `ListView`, `TableView`, and `TreeView` and avoiding `QListView`'s `2,532.6 µs` 100k-row invalidation penalty under `ScrollPerPixel`.
4. **Memory & Model Reads**: QmlTui uses **30% less heap memory per QML delegate** than Qt Quick (`3,767 B` vs. `5,347 B`), **81% less memory per `TreeView` row** (`48 B` vs. `248 B`), and performs the theoretical minimum **1.0 model read per newly visible row** on bound delegates.

## Reproducing

Building `benchmark_view_compare` requires `-DQMLTUI_BUILD_BENCHMARKS=ON` and optional `Qt6::Gui`, `Qt6::Quick`, and `Qt6::Widgets` modules installed alongside `Qt6::Core` and `Qt6::Qml` (so the benchmark can measure QmlTui side by side against Qt Quick and Qt Widgets):

```bash
cmake -S . -B build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQMLTUI_BUILD_TESTS=ON \
  -DQMLTUI_BUILD_BENCHMARKS=ON \
  -DQMLTUI_BUILD_EXAMPLES=OFF
cmake --build build-release --target benchmark_view_compare -j8
./build-release/tests/Core/benchmark_view_compare --rows 100000 --steps 500 --json view-compare.json
```

For the earlier per-phase stack profile that motivated these optimizations, see the historical [2026-09-11 view phases profile](2026-09-11-view-phases.md).
