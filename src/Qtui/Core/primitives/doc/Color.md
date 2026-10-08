# Color

`qtui::Color` is Qtui's QtGui-free public color value. It is available in C++
and as the constructible QML value type `rgba`; QML string assignments to Qtui
color properties continue to work.

Construct a color from `#RGB`, `#RRGGBB`, `#AARRGGBB`, `transparent`, or an SVG
named color. Parsing ignores surrounding whitespace, and named colors are
case-insensitive. An unrecognized value produces an invalid color.

Use `Qtui.rgba(red, green, blue, alpha)` in QML to construct a color from
normalized channels. Finite channel values are clamped to `0.0` through `1.0`.

```qml
import QtQml
import Qtui

QtObject {
    property rgba accent: Qtui.rgba(0.2, 0.4, 0.6, 1.0)
}
```

The equivalent C++ factory is `Color::fromRgba()`. Use `isValid()`, `argb32()`,
the normalized `red()`/`green()`/`blue()`/`alpha()` channels, and `toString()`
to inspect a color. `Cell::setForeground()`/`setBackground()` and the
`TextGridBuffer` drawing helpers accept `Color` directly. `ColorMapper` remains
an internal compatibility helper and is not part of the installed API.
