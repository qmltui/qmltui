# StandardItemDelegate

Demonstrates rendering a Qt Widgets-style `QStandardItemModel` (`ReleaseChecklistModel`) across `ListView`, `TableView`, and `TreeView` using standard Qt item roles (`Qt::ForegroundRole`, `Qt::BackgroundRole`, `Qt::TextAlignmentRole`, `Qt::CheckStateRole`, `Qt::DecorationRole`, and item flags).

- **`StandardItemDelegate.qml`**: Reusable QML delegate for `ListView` (and `Repeater`) that renders check states (`[x]`, `[ ]`, `[-]`), decoration glyphs, foreground/background colors, alignment, and disabled dimming, and toggles `Qt::CheckStateRole` on `Space` or mouse click via `model.setData()`.
- **Built-in view painters**: `TableView` and `TreeView` interpret the same standard item roles directly without requiring a custom delegate.
- **Widgets comparison**: When `Qt6::Widgets` is available (`QMLTUI_BUILD_WIDGETS_REFERENCE=ON`), `qmltui_standard_item_delegate_widgets` builds a side-by-side `QListView`/`QTableView`/`QTreeView` window bound to the same `ReleaseChecklistModel`.

```bash
cmake --build build --target qmltui_standard_item_delegate_demo
./build/examples/recipes/standard_item_delegate/qmltui_standard_item_delegate_demo
ctest --test-dir build -R '^standard_item_delegate_smoke$' --output-on-failure
```
