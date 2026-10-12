# Rectangle

## 1. Class Overview

`Rectangle` is a visual QML item that renders a background color fill and optional single or double borders in the terminal grid.

Developers use `Rectangle` for containers, cards, panels, and borders.

## 2. Project Structure and Dependencies

- **Header**: `Terminal/src/items/Rectangle.h`
- **Implementation**: `Terminal/src/items/Rectangle.cpp`
- **Dependencies**: `Item.h`, `Border.h`, `Color.h`.

## 3. Class Hierarchy and Role

- **Base Class**: `Item`
  - Inherits spatial positioning, sizing, flexbox layout, and rendering interfaces.

## 4. Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `color` | `qamui::Color` | `color` | `setColor` | `colorChanged` | Background color of the rectangle. |
| `border` | `Border*` | `border` | — | — | Attached border configuration (`style`, `color`). |

## 7. Signals

#### void colorChanged()
Emitted when the background color changes.

## 9. Public Methods

#### uint32_t resolvedBackgroundColor() const
Returns the 32-bit ARGB integer value of the background color.

#### Border* border()
Returns a pointer to the attached `Border` object configuring border style and border color.

#### int leftPadding() const [override]
Returns the left padding, accounting for the left border width.

#### int rightPadding() const [override]
Returns the right padding, accounting for the right border width.

#### int topPadding() const [override]
Returns the top padding, accounting for the top border width.

#### int bottomPadding() const [override]
Returns the bottom padding, accounting for the bottom border width.

## 10. Protected Virtual Methods / Event Handlers

#### void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect) const [override]
Paints the background color fill and borders into `buffer`; the framework
traverses children afterward.

## 16. Usage Example

```cpp
#include <Qamui/Core/Items/Rectangle.h>

using namespace qamui;

Rectangle* rect = new Rectangle(parentItem);
rect->setColor("#1e1e1e");
rect->border()->setStyle("single");
rect->border()->setColor("yellow");
```

See the [QML type reference](../qml-types/rectangle.md) for QML usage.
