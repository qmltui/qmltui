# TableView — Sorting and Filtering

Shows how `TableView` binds to a C++ `QAbstractTableModel` (`ProcessTableModel`) wrapped in a `QSortFilterProxyModel` (`ProcessProxy`), alongside a second `TableView` driven by a static 2D JavaScript array.

- **External sorting and filtering**: `TableView` emits `headerClicked(column)` and `rowActivated(row)` without owning sort or filter state itself; `ProcessProxy` updates its sort column/order and maps proxy rows back to source PIDs.
- **Connected grid borders**: Uses `showOuterBorder: true`, `showColumnGrid: true`, and `gridStyle: TableView.Double` / `TableView.Single` to draw connected box-drawing intersections.
- **Quit path**: `Escape` clears `filterInput.text` first and quits when the filter is empty; `Q` quits when `filterInput` does not have active focus.

```bash
cmake --build build --target qmltui_table_view_demo
./build/examples/recipes/table_view/qmltui_table_view_demo
ctest --test-dir build -R '^tableview_proxy_smoke$' --output-on-failure
```
