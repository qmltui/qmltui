# Color

`qamui::Color` is Qamui's QtGui-free public color value. It is available in C++
and as the constructible QML value type `rgba`; QML string assignments to Qamui
color properties continue to work.

Construct a color from `#RGB`, `#RRGGBB`, `#AARRGGBB`, `transparent`, an SVG
named color, or an `ansi:` palette entry. Parsing ignores surrounding
whitespace, and named colors are case-insensitive. An unrecognized value
produces an invalid color.

## Palette entries and the terminal theme

A terminal draws the sixteen ANSI palette slots with whatever colors the user
has themed them to. A color that is a palette *index* is encoded as that index
at every color depth, so the user's theme applies; a color that is literal RGB
is encoded as RGB. Three spellings let you choose:

| Spelling | Result | Rule |
|---|---|---|
| `"red"` | index 9 | An SVG name whose RGB is exactly an xterm default palette entry becomes that index, so it follows the terminal theme. |
| `"ansi:red"` | index 1 | ANSI's own numbering. |
| `"#ff0000"` | literal RGB | Exact color, no theme. |

The two name spellings deliberately use different naming authorities, so they
do not always agree. Bare names are CSS: `red` is `#ff0000`, which is palette
slot 9. Under `ansi:` the eight color words are the ANSI ones, SGR 30-37:

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
under the prefix; use them without it.

SVG names that are not an exact palette match, such as `orange`, stay literal
RGB. `toString()` reports a palette color by its canonical CSS name (index 9 is
`red`), `default` and `transparent` by name, and any other color as hex.

## `default` versus `transparent`

Both end up as the terminal's own color — the encoder emits SGR 39 for a
foreground and SGR 49 for a background, so the role decides which. They differ
in whether the cell is painted at all:

| Value | Painted? | Use it for |
|---|---|---|
| `"default"` | Yes | A surface that should follow the terminal's own color *and* hide what it covers, such as a popup or a dialog. |
| `"transparent"` | No | Letting whatever is behind show through. This is a `Rectangle`'s color when none is set. |

A `Rectangle` whose color is `"default"` fills its area, so it occludes the
content beneath it while the terminal still chooses the shade. One whose color
is `"transparent"` writes nothing at all. `Popup` uses `"default"`, which is
why a popup follows the terminal background instead of a hard-coded color.

`Color::isTransparent()` distinguishes the two. Transparency is an instruction
to the painter rather than a color, so it has no representation in a cell:
`Color::paintedArgb32()`, and every path that writes a cell, turns it into the
terminal default.

Use `Qamui.rgba(red, green, blue, alpha)` in QML to construct a color from
normalized channels. Finite channel values are clamped to `0.0` through `1.0`.

```qml
import QtQml
import Qamui

QtObject {
    property rgba accent: Qamui.rgba(0.2, 0.4, 0.6, 1.0)
}
```

The equivalent C++ factory is `Color::fromRgba()`. Use `isValid()`, `argb32()`,
the normalized `red()`/`green()`/`blue()`/`alpha()` channels, and `toString()`
to inspect a color. `Cell::setForeground()`/`setBackground()` and the
`TextGridBuffer` drawing helpers accept `Color` directly. `ColorMapper` remains
an internal compatibility helper and is not part of the installed API.
