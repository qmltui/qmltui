# TerminalCapabilities

`qmltui::TerminalCapabilities` is a `Q_GADGET` value struct describing the features detected from the host terminal environment and asynchronous VT query replies. Alongside `qmltui::ColorDepth`, it controls how `TextGridBuffer` and `TerminalEngine` encode colors, frame synchronization, keyboard disambiguation, and image protocols.

- **Header**: `<QmlTui/Core/Primitives/TerminalCapabilities.h>`
- **Target**: `qmltui::core`
- **QML Value Type**: [`terminalCapabilities`](../qml-types/terminalcapabilities.md) (exposed on `Terminal.capabilities`)

## `ColorDepth`

```cpp
enum class ColorDepth : uint8_t {
    Mono,       // No color SGR sequences emitted
    Basic16,    // 16-color ANSI SGR 30-37 / 90-97
    Indexed256, // 256-color indexed SGR 38;5;n
    TrueColor   // 24-bit RGB SGR 38;2;r;g;b
};
```

## Public Fields

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `colorDepth` | `ColorDepth` | `ColorDepth::TrueColor` | Color encoding depth used when serializing cells. |
| `isTty` | `bool` | `true` | Whether standard output is an interactive terminal (`isatty`). |
| `kittyKeyboard` | `bool` | `false` | Whether the terminal supports the Kitty keyboard protocol (`CSI ? u`). |
| `kittyGraphics` | `bool` | `false` | Whether the terminal supports the Kitty graphics protocol. |
| `sixelGraphics` | `bool` | `false` | Whether the terminal supports Sixel graphics (reported via DA1 attribute `4`). |
| `syncOutput` | `bool` | `true` | Whether DEC private mode 2026 synchronized output is used to bracket frames. |
| `termProgram` | `QString` | `""` | Value of `TERM_PROGRAM` when set (for example `"kitty"`, `"WezTerm"`, `"iTerm.app"`). |
| `cellPixelSize` | `QSize` | `QSize()` | Pixel dimensions (`width × height`) of one character cell reported by `CSI 16 t`, or invalid when unknown. |
| `mayProbeGraphics` | `bool` | `false` | Internal hint indicating that the terminal parses APC strings without echoing probe bytes. |

## Public Methods

#### `static TerminalCapabilities detect(const QProcessEnvironment &environment = QProcessEnvironment::systemEnvironment(), bool stdoutIsTty = true)`
Inspects `NO_COLOR`, `TERM`, `COLORTERM`, `TERM_PROGRAM`, and terminal-specific environment variables (`WT_SESSION`, `WSL_DISTRO_NAME`, `KITTY_WINDOW_ID`, `KONSOLE_VERSION`, `WEZTERM_EXECUTABLE`, `GHOSTTY_RESOURCES_DIR`) to construct initial capability defaults, then calls `applyEnvironmentOverrides()`.

#### `static void applyEnvironmentOverrides(TerminalCapabilities &capabilities, const QProcessEnvironment &environment = QProcessEnvironment::systemEnvironment())`
Applies explicit overrides from `QMLTUI_COLOR_DEPTH` (`mono`|`16`|`256`|`truecolor`), `QMLTUI_SYNC_OUTPUT` (`0`|`1`), `QMLTUI_KITTY_KEYBOARD`, and `QMLTUI_KITTY_GRAPHICS`. Re-applied by `TerminalEngine` after VT query replies so environment variables always take precedence.

#### `static QString colorDepthName(ColorDepth depth)`
Returns `"mono"`, `"16"`, `"256"`, or `"truecolor"`.

## Example

```cpp
#include <QmlTui/Core/Primitives/TerminalCapabilities.h>
#include <QDebug>

auto caps = qmltui::TerminalCapabilities::detect();
qDebug() << "Terminal program:" << caps.termProgram
         << "Color depth:" << caps.colorDepthName()
         << "Interactive TTY:" << caps.isTty;
```

See the [terminalCapabilities QML reference](../qml-types/terminalcapabilities.md).
