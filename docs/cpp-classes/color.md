# Color

`qmltui::Color` is QmlTui's QtGui-free public color value. It is available in C++
and as the constructible QML value type `color`; QML string assignments to QmlTui
color properties continue to work.

- **Header**: `<QmlTui/Core/Primitives/Color.h>`
- **Target**: `QmlTui::Core`
- **QML Value Type**: `color` (`QML_VALUE_TYPE(color)`)

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

## Public Methods and Constants (`qmltui::Color`)

### Constructors and Factories

- `constexpr Color()` / `constexpr Color(Special special)` — constructs `"default"` (`Color::Default`), `"transparent"` (`Color::Transparent`), or an invalid color (`Color::Invalid`).
- `Q_INVOKABLE Color(const QString &value)` / `static Color fromString(const QString &value)` / `static Color fromString(const QString &value, Color defaultColor)` — parses `#RGB`, `#RRGGBB`, `#AARRGGBB`, `"default"`, `"transparent"`, `ansi:<name|0..15>`, or a CSS/SVG color name, returning `Color::Invalid` (or `defaultColor`) when unrecognized.
- `static constexpr Color fromArgb32(quint32 argb)` — wraps a 32-bit ARGB or tagged color value (`DefaultTag = 0x00000000U`, `AnsiTag = 0x01000000U`, `TransparentTag = 0x04000000U`).
- `static constexpr Color fromAnsiIndex(int index)` — constructs an ANSI palette color for `0 <= index < 16`, or `Color::Invalid` otherwise.
- `static Color fromRgba(qreal red, qreal green, qreal blue, qreal alpha = 1.0)` — constructs an RGB color from normalized `[0.0, 1.0]` channels.
- `static constexpr Color defaultColor()` / `static constexpr Color transparent()` / `static constexpr Color invalid()` — convenience factory helpers.

### Inspection and Conversion

- `constexpr bool isValid() const` — `true` unless the color is `Color::Invalid`.
- `constexpr bool isDefault() const` / `static constexpr bool isDefault(quint32 argb)` — `true` when the color represents `"default"`.
- `constexpr bool isTransparent() const` / `static constexpr bool isTransparent(quint32 argb)` — `true` when the color represents `"transparent"`.
- `constexpr bool isAnsiIndex() const` / `static constexpr bool isAnsiIndex(quint32 argb)` — `true` when the color is one of the 16 ANSI palette slots.
- `constexpr int ansiIndex() const` / `static constexpr uint8_t ansiIndex(quint32 argb)` — returns the ANSI palette index (`0`–`15`, or `-1` on the member overload when `!isAnsiIndex()`).
- `constexpr quint32 argb32() const` — returns the raw 32-bit representation (including `AnsiTag` or `TransparentTag`).
- `constexpr quint32 paintedArgb32() const` / `static constexpr quint32 paintedArgb32(quint32 argb)` — normalizes `"transparent"` to `DefaultTag` (`0x00000000U`) for cell storage.
- `Color toRgb() const` / `static quint32 toArgb(quint32 argb)` — resolves ANSI palette indices to xterm's default 24-bit RGB values (identity for RGB colors).
- `uint8_t toIndexed256() const` / `static uint8_t toIndexed256(quint32 argb)` — quantizes the color to an xterm 256-color palette index.
- `uint8_t toBasic16() const` / `static uint8_t toBasic16(quint32 argb)` — quantizes the color to a 16-color ANSI index (`0`–`15`).
- `static constexpr quint32 resolve(Color color, quint32 defaultColor = DefaultTag)` / `static quint32 resolve(const QString &colorString, quint32 defaultColor = DefaultTag)` — returns `color.argb32()` when valid, or `defaultColor` otherwise.
- `qreal red() const`, `qreal green() const`, `qreal blue() const`, `qreal alpha() const` — normalized `[0.0, 1.0]` channels (exposed in QML as `r`, `g`, `b`, `a` alongside `valid`).
- `Q_INVOKABLE QString toString() const` — formats the color as `"default"`, `"transparent"`, `"ansi:<name>"`, or lowercase hex.
- `friend constexpr bool operator==(Color lhs, Color rhs) noexcept` / `friend constexpr bool operator!=(Color lhs, Color rhs) noexcept` — compares two `Color` values for exact equality.
