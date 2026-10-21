# Custom C++ Item

Minimal recipe showing how to subclass `qmltui::Rectangle` (or `qmltui::Item`) in C++, expose properties with `Q_PROPERTY`, register the type with `QML_ELEMENT`, and paint cells into `TextGridBuffer`.

- **`ProgressBar`** (`progressbar.h`, `progressbar.cpp`) inherits `qmltui::Rectangle` so it keeps `border` and background fill support, then overrides `paint()` to call `Rectangle::paint()` followed by filling the interior width proportional to `value` (`0.0` to `1.0`).
- **Invalidation**: `setValue()` and `setBarColor()` call `markDirty()` when the stored value changes so `TerminalEngine` schedules a frame render.

```bash
cmake --build build --target qmltui_custom_cpp_item_demo
./build/examples/recipes/custom_cpp_item/qmltui_custom_cpp_item_demo
ctest --test-dir build -R '^custom_cpp_item_smoke$' --output-on-failure
```
