# TerminalCapabilities

## Class Overview

`qmltui::TerminalCapabilities` is a `Q_GADGET` value struct describing the features detected from the host terminal environment and asynchronous VT query replies. Alongside `qmltui::ColorDepth`, it governs how `TextGridBuffer` and `TerminalEngine` encode colors, frame synchronization, keyboard disambiguation, and pixel graphics protocols.

UTF-8 and Unicode extended grapheme cluster segmentation are always active across QmlTui.

## Project Structure and Dependencies

- **Header**: `src/QmlTui/Core/Primitives/TerminalCapabilities.h` (installed as `<QmlTui/Core/Primitives/TerminalCapabilities.h>`)
- **Implementation**: `src/QmlTui/Core/Primitives/TerminalCapabilities.cpp`
- **CMake Target**: `qmltui::core` (`QmlTui::Core`)
- **QML Value Type**: `terminalCapabilities` (exposed via `Terminal.capabilities`)

## Enumerations

### ColorDepth

```cpp
enum class ColorDepth : uint8_t {
    Mono,       // No colour sequences emitted
    Basic16,    // 16-colour ANSI SGR 30-37 / 90-97
    Indexed256, // 256-colour indexed SGR 38;5;n
    TrueColor   // 24-bit RGB SGR 38;2;r;g;b
};
```

## Public Member Variables

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `colorDepth` | `ColorDepth` | `ColorDepth::TrueColor` | Colour encoding resolution used when serializing cells. |
| `isTty` | `bool` | `true` | Whether standard output is an interactive terminal (`isatty`) rather than a pipe or file. |
| `kittyKeyboard` | `bool` | `false` | Whether the terminal supports the Kitty keyboard protocol (`CSI ? u`). |
| `kittyGraphics` | `bool` | `false` | Whether the terminal supports the Kitty graphics protocol. |
| `sixelGraphics` | `bool` | `false` | Whether the terminal supports Sixel raster graphics (reported via DA1 attribute `4`). |
| `syncOutput` | `bool` | `true` | Whether synchronized output (DEC private mode 2026) is used to bracket frames. |
| `termProgram` | `QString` | `""` | Value of the `TERM_PROGRAM` environment variable when set (e.g. `"kitty"`, `"WezTerm"`, `"iTerm.app"`). |
| `cellPixelSize` | `QSize` | `QSize()` | Pixel dimensions (`width x height`) of a single character cell reported by `CSI 16 t`, or invalid if unknown. |
| `mayProbeGraphics` | `bool` | `false` | Internal hint indicating that the terminal is known to parse APC strings safely without printing probe bytes. |

## Public Methods

#### static TerminalCapabilities detect(const QProcessEnvironment& environment = QProcessEnvironment::systemEnvironment(), bool stdoutIsTty = true)
Inspects `NO_COLOR`, `TERM`, `COLORTERM`, `TERM_PROGRAM`, and terminal-specific environment variables (`WT_SESSION`, `WSL_DISTRO_NAME`, `KITTY_WINDOW_ID`, `KONSOLE_VERSION`, `WEZTERM_EXECUTABLE`, `GHOSTTY_RESOURCES_DIR`) to construct initial capability defaults, then applies environment overrides.

#### static void applyEnvironmentOverrides(TerminalCapabilities& capabilities, const QProcessEnvironment& environment = QProcessEnvironment::systemEnvironment())
Applies explicit user/test overrides from `QMLTUI_COLOR_DEPTH` (`mono`|`16`|`256`|`truecolor`), `QMLTUI_SYNC_OUTPUT` (`0`|`1`), `QMLTUI_KITTY_KEYBOARD`, and `QMLTUI_KITTY_GRAPHICS`. Called by `detect()` and re-applied by `TerminalEngine` after VT query replies so environment overrides always win.

#### static QString colorDepthName(ColorDepth depth)
Returns the QML-facing string name for `depth`: `"mono"`, `"16"`, `"256"`, or `"truecolor"`.

#### bool supportsKittyKeyboard() const / bool supportsKittyGraphics() const / bool supportsSixelGraphics() const
#### QSize pixelSize() const / bool supportsSyncOutput() const
#### QString colorDepthName() const / QString terminalProgram() const / bool isTtyStream() const
Read accessors backing the QML `terminalCapabilities` properties.

## Usage Example

```cpp
#include <QmlTui/Core/Primitives/TerminalCapabilities.h>
#include <QDebug>

auto caps = qmltui::TerminalCapabilities::detect();
qDebug() << "Terminal program:" << caps.termProgram
         << "Color depth:" << caps.colorDepthName()
         << "Interactive TTY:" << caps.isTty;
```

See the [QML value type reference](../qml-types/terminalcapabilities.md) for QML usage.
