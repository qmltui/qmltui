# BigText

A custom C++ `qmltui::Item` (`BigText.h`, `BigText.cpp`) that renders uppercase ASCII letters, digits, and punctuation as 3×5 block-pixel glyphs (`█`) directly into `TextGridBuffer`.

- **`BigText`** subclasses `qmltui::Item`, declares `QML_ELEMENT`, and exposes `text`, `color`, and `backgroundColor` properties.
- **Sizing**: `setText()` updates `implicitWidth` (`max(0, len * 4 - 1)`) and `implicitHeight` (`5`) and calls `markDirty()`.
- **Painting**: `paint()` iterates each character's 5×3 bitmap and writes cells constrained by `clipRect`.

```bash
cmake --build build --target qmltui_big_text_demo
./build/examples/recipes/big_text/qmltui_big_text_demo
ctest --test-dir build -R '^big_text_smoke$' --output-on-failure
```

Copy `BigText.h` and `BigText.cpp` into your own `qt_add_qml_module(...)` target when you need multi-row banner text.
