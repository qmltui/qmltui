# ColorMapper (internal)

## 1. Class Overview

`ColorMapper` is an internal bridge from legacy string parsing call sites to
`qmltui::Color`. It is not installed or part of the custom-item API. New code
should include `Color.h` and use `qmltui::Color` directly.

Developers use `ColorMapper` when mapping QML string color properties into internal cell rendering formats.

## 2. Project Structure and Dependencies

- **Header**: `src/QmlTui/Core/Primitives/ColorMapper.h`
- **Implementation**: `src/QmlTui/Core/Primitives/ColorMapper.cpp`
- **Dependencies**: Qt Core (`QString`, `QHash`).

## 9. Public Methods

#### static uint32_t fromString(const QString& colorString, uint32_t defaultColor = 0xFFFFFFFF)
Parses a color string into a 32-bit ARGB integer. Returns `defaultColor` if `colorString` is empty, invalid, or unparseable. Supports `#RRGGBB`, `#AARRGGBB`, `#RGB` hex formats, and standard named colors (e.g., `"transparent"`, `"black"`, `"white"`, `"red"`, `"blue"`, `"green"`).

## 16. Usage Example

```cpp
#include <QmlTui/Core/Primitives/ColorMapper.h>
#include <iostream>

using namespace qmltui;

int main() {
    uint32_t red = ColorMapper::fromString("#FF0000");
    uint32_t blue = ColorMapper::fromString("blue");

    std::cout << "Red ARGB: 0x" << std::hex << red << std::endl;
    return 0;
}
```
