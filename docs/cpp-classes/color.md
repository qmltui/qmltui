# Color

`qmltui::Color` is QmlTui's QtGui-free public color value. It is available in C++
and as the constructible QML value type `color`; QML string assignments to QmlTui
color properties continue to work.

A default-constructed `Color()` (or `Color::Default` / `Color::defaultColor()`)
represents the terminal's default color (`"default"`). Use `Color::Transparent`
(or `Color::transparent()`) for `"transparent"`, `Color::fromAnsiIndex(index)`
for ANSI palette indices `0` through `15`, and `Color::Invalid` (or
`Color::invalid()`) for an explicitly unset or invalid color. Construct a color
from a string with `#RGB`, `#RRGGBB`, `#AARRGGBB`, `default`, `transparent`, an
SVG named color, or an `ansi:` palette entry. Parsing ignores surrounding
whitespace, and named colors are case-insensitive. An unrecognized value
produces `Color::Invalid` (`!isValid()`).

## Palette entries and the terminal theme

A terminal draws the sixteen ANSI palette slots with whatever colors the user
has themed them to. A color that is a palette *index* is encoded as that index
at every color depth, so the user's theme applies; a color that is literal RGB
is encoded as RGB. Bare CSS/SVG color names and hex strings are always literal
RGB, while the `ansi:` prefix (or `QmlTui.ansi()` / `Color::fromAnsiIndex()`)
selects a terminal palette slot:

| Spelling | Result | Rule |
|---|---|---|
| `"red"` | literal RGB (`#ff0000`) | Standard CSS/SVG named color; exact 24-bit RGB, no terminal theme. |
| `"#ff0000"` | literal RGB (`#ff0000`) | Exact 24-bit RGB, no terminal theme. |
| `"ansi:red"` | index 1 | ANSI palette entry (SGR 31/41); follows the user's terminal theme. |
| `"ansi:brightred"` | index 9 | Bright ANSI palette entry (SGR 91/101); follows the user's terminal theme. |

Under `ansi:` the eight color words follow ANSI's own numbering (SGR 30-37 for
indices `0`–`7`, and `bright` for SGR 90-97 indices `8`–`15`):

| Index | `ansi:` name | Index | `ansi:` name |
|---|---|---|---|
| 0 | `black` | 8 | `brightblack`, `gray`, `grey`, `darkgray`, `darkgrey` |
| 1 | `red`, `darkred` | 9 | `brightred` |
| 2 | `green`, `darkgreen` | 10 | `brightgreen` |
| 3 | `yellow`, `darkyellow` | 11 | `brightyellow` |
| 4 | `blue`, `darkblue` | 12 | `brightblue` |
| 5 | `magenta`, `darkmagenta` | 13 | `brightmagenta` |
| 6 | `cyan`, `darkcyan` | 14 | `brightcyan` |
| 7 | `white`, `lightgray`, `lightgrey` | 15 | `brightwhite` |

`ansi:0` through `ansi:15` name the same slots by number. Dashes, underscores
and spaces in the suffix are ignored, so `ansi:bright-red` and `ansi:bright_red`
are both index 9. Words that are CSS-only rather than ANSI — `maroon`, `olive`,
`navy`, `purple`, `teal`, `silver`, `lime`, `fuchsia`, `aqua` — are rejected
under the prefix; without the prefix, they resolve to their CSS RGB values.

`toString()` reports a palette color by its canonical `ansi:` name (`ansi:black`
through `ansi:brightwhite`), `default` and `transparent` by name, and any RGB
color (whether constructed from a hex string or a CSS name) as lowercase hex.

## `default` versus `transparent`

Both end up as the terminal's own color — the encoder emits SGR 39 for a
foreground and SGR 49 for a background, so the role decides which. They differ
in whether the cell is painted at all:

| Value | Painted? | Use it for |
|---|---|---|
| `"default"` | Yes | A surface that should follow the terminal's own color *and* hide what it covers, such as a dialog or an opaque popup background. |
| `"transparent"` | No | Letting whatever is behind show through. This is a `Rectangle`'s color when none is set. |

A `Rectangle` whose color is `"default"` fills its area, so it occludes the
content beneath it while the terminal still chooses the shade. One whose color
is `"transparent"` writes nothing at all. `Dialog` uses a background `Rectangle`
with `"default"`, which is why a dialog follows the terminal background instead
of a hard-coded color. `Popup` itself is an unpainted `Item` container; compose a
`Rectangle` with `"default"` inside it when occlusion is needed.

`Color::isDefault()` and `Color::isTransparent()` distinguish the two. In C++,
prefer `Color::Default` (or `Color()`) and `Color::Transparent`; in QML,
`QmlTui.defaultColor` and `QmlTui.transparent` provide pre-constructed `color`
constants alongside `"default"` and `"transparent"`. Transparency is an instruction
to the painter rather than a color, so it has no representation in a cell:
`Color::paintedArgb32()`, and every path that writes a cell, turns it into the
terminal default.

Use `QmlTui.rgba(red, green, blue, alpha = 1.0)` in QML to construct a color from
normalized channels, `QmlTui.color(name)` to parse a string, or `QmlTui.ansi(index)`
for ANSI palette slot `0` through `15`. Finite channel values are clamped to `0.0`
through `1.0`.

```qml
import QtQml
import QmlTui

QtObject {
    property color accent: QmlTui.rgba(0.2, 0.4, 0.6, 1.0)
}
```

The equivalent C++ factory is `Color::fromRgba()`. Use `isValid()`, `argb32()`,
the normalized `red()`/`green()`/`blue()`/`alpha()` channels, and `toString()`
to inspect a color. `Cell::setForeground()`/`setBackground()` and the
`TextGridBuffer` drawing helpers accept `Color` directly.
