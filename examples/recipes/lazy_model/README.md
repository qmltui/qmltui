# Lazy Model

A `QAbstractItemModel` (`LazyDirectoryModel`) that populates directory branches on demand when expanded in `TreeView`.

- **Incremental loading**: `hasChildren()` returns `true` for unloaded directory nodes while `rowCount()` is still `0`, and `canFetchMore()` / `fetchMore()` insert child rows the first time a branch is expanded—matching the `QFileSystemModel` contract.
- **Telemetry**: `fetchCount` and `loadedCount` properties expose how many `fetchMore()` calls `TreeView` has issued as the user expands nodes with `Enter` or `Right`.

```bash
cmake --build build --target qmltui_lazy_model_demo
./build/examples/recipes/lazy_model/qmltui_lazy_model_demo
ctest --test-dir build -R '^lazy_model_smoke$' --output-on-failure
```
