# Color

`qtui::Color` is Qtui's QtGui-free public color value. It is available in C++
and as the constructible QML value type `rgba`; QML string assignments to Qtui
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
`red`) and any other color as hex.
