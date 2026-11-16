# BigText

A custom C++ `qmltui::Item` (`BigText.h`, `BigText.cpp`) that renders uppercase ASCII letters, digits, and punctuation as 3x3 block-pixel glyphs (`█`, `▀`, `▄`) directly into `TextGridBuffer`.

- **`BigText`** subclasses `qmltui::Item`, declares `QML_ELEMENT`, and exposes `text`, `color` (defaulting to `Color::Default`), and `backgroundColor` (`Color::Transparent`) properties.
- **Sizing**: Defaults to `implicitWidth` `12` and `implicitHeight` `3`; `setText()` updates `implicitWidth` to `text.length() * 4` (3 glyph columns plus 1 spacing column per character) and calls `markDirty()`.
- **Painting**: `paint()` intersects `clipRect` with the item and buffer bounds, then writes each character's 3x3 glyph cells via `TextGridBuffer::setGrapheme()`.

```bash
cmake --build build --target qmltui_big_text_demo
./build/examples/recipes/big_text/qmltui_big_text_demo
ctest --test-dir build -R '^big_text_smoke$' --output-on-failure
```

Copy `BigText.h` and `BigText.cpp` into your own `qt_add_qml_module(...)` target when you need multi-row banner text.
