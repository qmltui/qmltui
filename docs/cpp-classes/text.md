# Text

`qmltui::Text` renders single-line or multi-line text strings or structured `Span` runs on the character grid. It supports foreground and background colors, gradients, SGR attributes (`bold`, `dim`, `italic`, `underline`, `reversed`), horizontal and vertical alignment, word wrapping, clickable OSC 8 / RichText hyperlinks (`linkActivated`), and plain, HTML (`RichText`), or ANSI-escaped (`AnsiText`) input formats.

- **Header**: `<QmlTui/Core/Items/Text.h>`
- **Inherits**: `qmltui::Item`
- **Target**: `QmlTui::Core`

## Properties (`Q_PROPERTY`)

| Property | Type | Access | Notify Signal | Description |
| :--- | :--- | :--- | :--- | :--- |
| `spans` | `QList<qmltui::Span>` | read/write | `spansChanged` | Structured text runs; takes precedence over `text` when non-empty. |
| `text` | `QString` | read/write | `textChanged` | Text string displayed when `spans` is empty. |
| `color` | `qmltui::Color` | read/write | `colorChanged` | Foreground color (`"default"` by default). |
| `backgroundColor` | `qmltui::Color` | read/write | `backgroundColorChanged` | Background color of painted character cells (`"transparent"` by default). |
| `bold`, `dim`, `italic`, `underline`, `reversed` | `bool` | read/write | `*Changed` | SGR text attributes (`dim` is also applied automatically when `enabled` is `false`). |
| `horizontalAlignment` | `int` | read/write | `horizontalAlignmentChanged` | `Qt::AlignLeft`, `Qt::AlignHCenter`, or `Qt::AlignRight`. |
| `verticalAlignment` | `int` | read/write | `verticalAlignmentChanged` | `Qt::AlignTop`, `Qt::AlignVCenter`, or `Qt::AlignBottom`. |
| `textFormat` | `TextFormat` | read/write | `textFormatChanged` | `PlainText` (`0`), `RichText` (`1`), `AnsiText` (`2`), or `AutoText` (`3`). |
| `wrapMode` | `WrapMode` | read/write | `wrapModeChanged` | `NoWrap` (`0`), `WordWrap` (`1`), `WrapAnywhere` (`2`), or `Wrap` (`3`). |
| `gradient` | `qmltui::Gradient*` | read/write | `gradientChanged` | Optional foreground gradient interpolated across rendered characters. |

## Public Methods

- `int visualLength(const QString &line) const` — returns the display column width of `line` after stripping markup according to `textFormat()`.
- `void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override` — paints laid-out text and hyperlinks into `buffer`.
- `void mousePressEvent(MouseEvent *event) override` — activates any hyperlink under a left mouse press and emits `linkActivated()`.

## Signals

- `void linkActivated(const QString &link)` — emitted when the user clicks an HTML `<a href="...">` or Markdown `[label](url)` hyperlink.

## Example

```cpp
#include <QmlTui/Core/Items/Text.h>

auto *textItem = new qmltui::Text(parentItem);
textItem->setText(QStringLiteral("Hello <b>World</b>"));
textItem->setTextFormat(qmltui::Text::RichText);
textItem->setWrapMode(qmltui::Text::WordWrap);
textItem->setColor(qmltui::Color(QStringLiteral("green")));
```

See the [Text QML type reference](../qml-types/text.md).
