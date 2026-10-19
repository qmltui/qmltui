# Rectangle

## Class Overview

`Rectangle` is a visual QML item that renders a background color or gradient fill and optional single (`Border::Single`), double (`Border::Double`), rounded (`Border::Rounded`), heavy (`Border::Heavy`), or ASCII (`Border::Ascii`) borders in the terminal grid.

Developers use `Rectangle` for containers, cards, panels, and borders.

## Project Structure and Dependencies

- **Header**: `src/QmlTui/Core/Items/Rectangle.h` (installed as `<QmlTui/Core/Items/Rectangle.h>`)
- **Implementation**: `src/QmlTui/Core/Items/Rectangle.cpp`
- **Dependencies**: `Item.h`, `Border.h`, `Gradient.h`, `Color.h`.

## Class Hierarchy and Role

- **Base Class**: `Item`
  - Inherits spatial positioning, cell-grid sizing, anchoring, padding, and rendering interfaces.

## Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `color` | `qmltui::Color` | `color` | `setColor` | `colorChanged` | Background color of the rectangle (defaults to `"transparent"`). |
| `border` | `qmltui::Border*` | `border` | — | — | Grouped border configuration (`style`, `color`). |
| `gradient` | `qmltui::Gradient*` | `gradient` | `setGradient` | `gradientChanged` | Optional background gradient fill across the rectangle. |

## Signals

#### void colorChanged()
Emitted when the background color changes.

#### void gradientChanged()
Emitted when the background gradient changes.

## Public Methods

#### uint32_t resolvedBackgroundColor() const
Returns the 32-bit ARGB integer value of the background color.

#### Border* border() const
Returns a pointer to the grouped `Border` object configuring border style (`Border::None`, `Border::Single`, `Border::Double`, `Border::Rounded`, `Border::Heavy`, `Border::Ascii`) and border color.

#### Gradient* gradient() const / void setGradient(Gradient* gradient)
Returns or sets the optional `Gradient` used to fill the rectangle background.

#### int leftPadding() const [override]
Returns the left padding, accounting for the left border width (adds 1 cell when `border()->style() != Border::None`).

#### int rightPadding() const [override]
Returns the right padding, accounting for the right border width.

#### int topPadding() const [override]
Returns the top padding, accounting for the top border width.

#### int bottomPadding() const [override]
Returns the bottom padding, accounting for the bottom border width.

## Protected Virtual Methods / Event Handlers

#### void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect) const [override]
Paints the background color or gradient fill and borders into `buffer`; the framework
traverses children afterward.

## Usage Example

```cpp
#include <QmlTui/Core/Items/Rectangle.h>

using namespace qmltui;

Rectangle* rect = new Rectangle(parentItem);
rect->setColor("#1e1e1e");
rect->border()->setStyle(qmltui::Border::Single);
rect->border()->setColor("yellow");
```

See the [QML type reference](../qml-types/rectangle.md) for QML usage.
