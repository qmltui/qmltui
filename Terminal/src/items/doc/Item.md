# Item

## 1. Class Overview

`Item` is the base visual class for all QML elements in Qtui (equivalent to `QQuickItem` in Qt Quick). It provides spatial geometry (positioning `x`, `y`, sizing `width`, `height`, padding), focus navigation (`focus`, `focusable`, `activeFocus`), flexbox layout hints (`flexGrow`, `flexShrink`, `flexBasis`), declarative state machine integration (`states`, `transitions`), anchoring primitives, and virtual rendering/input event handling contracts.

Developers subclass `Item` when creating new C++ terminal components.

## 2. Project Structure and Dependencies

- **Header**: `Tui/src/items/Item.h`
- **Implementation**: `Tui/src/items/Item.cpp`
- **Dependencies**: `TextGridBuffer.h`, `Anchors.h`, `Border.h`, Qt Core (`QObject`, `QQmlEngine`).

## 3. Class Hierarchy and Role

- **Base Class**: `QObject`
  - Provides parent-child hierarchy, properties, signal/slot communication, and QML meta-object registration.
- **Derived Classes**: `Rectangle`, `Text`, `MouseArea`, `Row`, `Column`, `Terminal`, `ScrollView`, `ListView`, etc.

## 4. Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `x` | `int` | `x` | `setX` | `xChanged` | X coordinate relative to parent item. |
| `y` | `int` | `y` | `setY` | `yChanged` | Y coordinate relative to parent item. |
| `width` | `int` | `width` | `setWidth` | `widthChanged` | Width of the item in terminal columns. |
| `height` | `int` | `height` | `setHeight` | `heightChanged` | Height of the item in terminal rows. |
| `implicitWidth` | `int` | `implicitWidth` | `setImplicitWidth` | `implicitWidthChanged` | Natural width of the item based on content. |
| `implicitHeight` | `int` | `implicitHeight` | `setImplicitHeight` | `implicitHeightChanged` | Natural height of the item based on content. |
| `visible` | `bool` | `visible` | `setVisible` | `visibleChanged` | Controls whether the item is rendered. |
| `focus` | `bool` | `focus` | `setFocus` | `focusChanged` | Requested focus state. |
| `activeFocus` | `bool` | `activeFocus` | — | `activeFocusChanged` | Read-only property indicating if the item has active input focus. |
| `focusable` | `bool` | `focusable` | `setFocusable` | `focusableChanged` | Whether the item participates in Tab focus cycling. |
| `clip` | `bool` | `clip` | `setClip` | `clipChanged` | Enables clipping of child rendering to item bounds. |
| `padding` | `int` | `padding` | `setPadding` | `paddingChanged` | Uniform inner padding for all sides. |
| `leftPadding` | `int` | `leftPadding` | `setLeftPadding` | `leftPaddingChanged` | Inner left padding. |
| `rightPadding` | `int` | `rightPadding` | `setRightPadding` | `rightPaddingChanged` | Inner right padding. |
| `topPadding` | `int` | `topPadding` | `setTopPadding` | `topPaddingChanged` | Inner top padding. |
| `bottomPadding` | `int` | `bottomPadding` | `setBottomPadding` | `bottomPaddingChanged` | Inner bottom padding. |
| `flexGrow` | `double` | `flexGrow` | `setFlexGrow` | `flexGrowChanged` | Flexbox grow factor. |
| `flexShrink` | `double` | `flexShrink` | `setFlexShrink` | `flexShrinkChanged` | Flexbox shrink factor. |
| `flexBasis` | `int` | `flexBasis` | `setFlexBasis` | `flexBasisChanged` | Flexbox initial size. |

## 7. Signals

#### void xChanged()
Emitted when `x` coordinate changes.

#### void yChanged()
Emitted when `y` coordinate changes.

#### void widthChanged()
Emitted when `width` changes.

#### void heightChanged()
Emitted when `height` changes.

#### void visibleChanged()
Emitted when visibility toggles.

#### void focusChanged()
Emitted when focus state changes.

#### void activeFocusChanged()
Emitted when active keyboard focus changes.

#### void dirty()
Emitted to request a frame re-render when item visual state changes.

## 9. Public Methods

#### void markDirty()
Notifies the parent item and rendering engine that this item needs to be redrawn.

#### QPoint absolutePosition() const
Calculates and returns the absolute (x, y) coordinates of the item relative to the root window.

## 10. Protected Virtual Methods / Event Handlers

#### void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect) [override]
Renders the visual content of the item and its children into the target grid buffer.

#### void keyPressEvent(KeyEvent* event) [virtual]
Called when a key is pressed while the item has focus.

#### void keyReleaseEvent(KeyEvent* event) [virtual]
Called when a key is released while the item has focus.

#### void mousePressEvent(MouseEvent* event) [virtual]
Called when a mouse button is pressed over the item.

#### void mouseReleaseEvent(MouseEvent* event) [virtual]
Called when a mouse button is released over the item.

#### void mouseMoveEvent(MouseEvent* event) [virtual]
Called when the mouse cursor moves over the item.

#### void wheelEvent(MouseEvent* event) [virtual]
Called when mouse wheel scrolling occurs over the item.

## 11. Ownership and Lifecycle

`Item` objects pass `QObject* parent` to the constructor and are parent-owned. Deletion of a parent `Item` deletes all child items.

## 12. Thread Safety

Single-threaded. Must be instantiated and used on the main GUI thread.

## 13. QML Exposure

Registered as `Item` in the `Tui` QML module via `QML_ELEMENT`.

## 16. Usage Example

```cpp
#include "Item.h"
#include "TextGridBuffer.h"

using namespace qtui;

class CustomWidget : public Item {
    Q_OBJECT
public:
    explicit CustomWidget(QObject* parent = nullptr) : Item(parent) {
        setImplicitWidth(10);
        setImplicitHeight(3);
    }

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect) override {
        buffer.drawBox(absX, absY, width(), height());
        Item::render(buffer, absX, absY, clipRect);
    }
};
```
