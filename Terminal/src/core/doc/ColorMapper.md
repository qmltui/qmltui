# ColorMapper

## 1. Class Overview

`ColorMapper` is a static helper class that parses color strings (hex values like `"#FF0000"`, `"#333"`, or named colors like `"red"`, `"blue"`, `"green"`, `"black"`, `"white"`) and converts them into 32-bit ARGB `uint32_t` integer values.

Developers use `ColorMapper` when mapping QML string color properties into internal cell rendering formats.

## 2. Project Structure and Dependencies

- **Header**: `Terminal/src/core/ColorMapper.h`
- **Implementation**: `Terminal/src/core/ColorMapper.cpp`
- **Dependencies**: Qt Core (`QString`, `QHash`).

## 9. Public Methods

#### static uint32_t fromString(const QString& colorString, uint32_t defaultColor = 0xFFFFFFFF)
Parses a color string into a 32-bit ARGB integer. Returns `defaultColor` if `colorString` is empty, invalid, or unparseable. Supports `#RRGGBB`, `#AARRGGBB`, `#RGB` hex formats, and standard named colors (e.g., `"transparent"`, `"black"`, `"white"`, `"red"`, `"blue"`, `"green"`).

## 16. Usage Example

```cpp
#include "ColorMapper.h"
#include <iostream>

using namespace qtui;

int main() {
    uint32_t red = ColorMapper::fromString("#FF0000");
    uint32_t blue = ColorMapper::fromString("blue");

    std::cout << "Red ARGB: 0x" << std::hex << red << std::endl;
    return 0;
}
```
