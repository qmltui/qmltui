# Rectangle

`qmltui::Rectangle` is a visual `Item` that paints a background color or `Gradient` fill and optional Unicode or ASCII box-drawing borders (`Border::Single`, `Border::Double`, `Border::Rounded`, `Border::Ascii`).

- **Header**: `<QmlTui/Core/Items/Rectangle.h>`
- **Inherits**: `qmltui::Item`
- **Target**: `QmlTui::Core`

## Properties (`Q_PROPERTY`)

| Property | Type | Access | Notify Signal | Description |
| :--- | :--- | :--- | :--- | :--- |
| `color` | `qmltui::Color` | read/write | `colorChanged` | Background fill color (`"transparent"` by default). |
| `border` | `qmltui::Border*` | read-only | — | Grouped border configuration (`style`, `color`). |
| `gradient` | `qmltui::Gradient*` | read/write | `gradientChanged` | Optional background gradient fill across the rectangle. |

## Public Methods

- `uint32_t resolvedBackgroundColor() const` — returns the 32-bit ARGB value of `color()`.
- `int leftPadding() const override` (and `rightPadding()`, `topPadding()`, `bottomPadding()`) — adds 1 cell of interior padding on each edge when `border()->style() != Border::None`.

## Example

```cpp
#include <QmlTui/Core/Items/Rectangle.h>

auto *rect = new qmltui::Rectangle(parentItem);
rect->setWidth(24);
rect->setHeight(5);
rect->setColor(qmltui::Color(QStringLiteral("#1e1e1e")));
rect->border()->setStyle(qmltui::Border::Single);
rect->border()->setColor(qmltui::Color(QStringLiteral("cyan")));
```

See the [Rectangle QML type reference](../qml-types/rectangle.md).
