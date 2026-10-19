# Item

## Class Overview

`Item` is the base visual class for all QML elements in QmlTui (equivalent to `QQuickItem` in Qt Quick). It provides spatial geometry (positioning `x`, `y`, `z`, sizing `width`, `height`, padding), focus navigation (`focus`, `focusable`, `activeFocus`), declarative state machine integration (`states`, `transitions`, `state`), anchoring primitives, and virtual rendering/input event handling contracts.

Developers subclass `Item` when creating new C++ terminal components.

## Project Structure and Dependencies

- **Header**: `src/QmlTui/Core/Items/Item.h`
- **Implementation**: `src/QmlTui/Core/Items/Item.cpp`
- **Dependencies**: `TextGridBuffer.h`, `Anchors.h`, `Border.h`, Qt Core (`QObject`, `QQmlEngine`, `QPoint`, `QList`).

## Class Hierarchy and Role

- **Base Class**: `QObject`
  - Provides parent-child hierarchy, properties, signal/slot communication, and QML meta-object registration.
- **Derived Classes**: `Rectangle`, `Text`, `MouseArea`, `Row`, `Column`, `Terminal`, `ScrollView`, `ListView`, etc.

## Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `x` | `int` | `x` | `setX` | `xChanged` | X coordinate relative to parent item. |
| `y` | `int` | `y` | `setY` | `yChanged` | Y coordinate relative to parent item. |
| `z` | `qreal` | `z` | `setZ` | `zChanged` | Visual stacking order. Siblings render by ascending `z`. |
| `width` | `int` | `width` | `setWidth` | `widthChanged` | Width of the item in terminal columns. |
| `height` | `int` | `height` | `setHeight` | `heightChanged` | Height of the item in terminal rows. |
| `implicitWidth` | `int` | `implicitWidth` | `setImplicitWidth` | `implicitWidthChanged` | Natural width of the item based on content. |
| `implicitHeight` | `int` | `implicitHeight` | `setImplicitHeight` | `implicitHeightChanged` | Natural height of the item based on content. |
| `visible` | `bool` | `visible` | `setVisible` | `visibleChanged` | Controls whether the item is rendered. |
| `enabled` | `bool` | `enabled` | `setEnabled` | `enabledChanged` | Controls whether the item receives input events. |
| `focus` | `bool` | `focus` | `setFocus` | `focusChanged` | Requested focus state. |
| `activeFocus` | `bool` | `activeFocus` | — | `activeFocusChanged` | Read-only property indicating if the item has active input focus. |
| `focusable` | `bool` | `focusable` | `setFocusable` | `focusableChanged` | Whether the item participates in Tab focus cycling. |
| `parent` | `qmltui::Item*` | `parentItem` | `setParentItem` | `parentChanged` | Parent visual item. |
| `anchors` | `qmltui::Anchors*` | `anchors` | — | — | Attached anchor configuration object. |
| `left` | `qmltui::AnchorLine*` | `left` | — | — | Left anchor line constraint of the item. |
| `right` | `qmltui::AnchorLine*` | `right` | — | — | Right anchor line constraint of the item. |
| `top` | `qmltui::AnchorLine*` | `top` | — | — | Top anchor line constraint of the item. |
| `bottom` | `qmltui::AnchorLine*` | `bottom` | — | — | Bottom anchor line constraint of the item. |
| `horizontalCenter` | `qmltui::AnchorLine*` | `horizontalCenter` | — | — | Horizontal center anchor line constraint of the item. |
| `verticalCenter` | `qmltui::AnchorLine*` | `verticalCenter` | — | — | Vertical center anchor line constraint of the item. |
| `clip` | `bool` | `clip` | `setClip` | `clipChanged` | Enables clipping of child rendering to item bounds. |
| `padding` | `int` | `padding` | `setPadding` | `paddingChanged` | Uniform inner padding for all sides. |
| `leftPadding` | `int` | `leftPadding` | `setLeftPadding` | `leftPaddingChanged` | Inner left padding. |
| `rightPadding` | `int` | `rightPadding` | `setRightPadding` | `rightPaddingChanged` | Inner right padding. |
| `topPadding` | `int` | `topPadding` | `setTopPadding` | `topPaddingChanged` | Inner top padding. |
| `bottomPadding` | `int` | `bottomPadding` | `setBottomPadding` | `bottomPaddingChanged` | Inner bottom padding. |
| `data` | `QQmlListProperty<QObject>` | `data` | — | — | List of QML visual/logical child objects (default property). |
| `children` | `QQmlListProperty<qmltui::Item>` | `childrenProperty` | — | `childrenChanged` | Read-only list of visual child `Item` objects. |
| `states` | `QQmlListProperty<qmltui::State>` | `states` | — | — | List of states defined on the item. |
| `transitions` | `QQmlListProperty<qmltui::Transition>` | `transitions` | — | — | List of transitions defined on the item. |
| `state` | `QString` | `state` | `setState` | `stateChanged` | Current active state name. |

## Public Types and Member Variables

### ClipRect Struct

| Variable | Type | Description |
|----------|------|-------------|
| `x` | `int` | X coordinate of the clipping bounds (default: `0`). |
| `y` | `int` | `y` coordinate of the clipping bounds (default: `0`). |
| `width` | `int` | Width of the clipping bounds (default: `99999`). |
| `height` | `int` | Height of the clipping bounds (default: `99999`). |

#### bool contains(int cx, int cy) const
Returns `true` if coordinates (cx, cy) are inside the clipping rectangle.

#### ClipRect intersected(const ClipRect& other) const
Returns a new `ClipRect` representing the intersection of this rectangle and the other.

## Signals

#### void childrenChanged()
Emitted when visual child items are added to or removed from `children`.

#### void xChanged()
Emitted when `x` coordinate changes.

#### void yChanged()
Emitted when `y` coordinate changes.

#### void zChanged()
Emitted when `z` order changes.

#### void widthChanged()
Emitted when `width` changes.

#### void heightChanged()
Emitted when `height` changes.

#### void implicitWidthChanged()
Emitted when `implicitWidth` changes.

#### void implicitHeightChanged()
Emitted when `implicitHeight` changes.

#### void visibleChanged()
Emitted when visibility toggles.

#### void enabledChanged()
Emitted when enabled state toggles.

#### void parentChanged()
Emitted when the visual parent changes.

#### void ancestryChanged()
Emitted when this item or any visual ancestor is attached to or detached from a parent, indicating that the resolved `terminal()` may have changed across the subtree.

#### void focusChanged()
Emitted when focus state changes.

#### void activeFocusChanged()
Emitted when active keyboard focus changes.

#### void focusableChanged()
Emitted when focusable state changes.

#### void paddingChanged()
Emitted when uniform padding changes.

#### void leftPaddingChanged()
Emitted when left padding changes.

#### void rightPaddingChanged()
Emitted when right padding changes.

#### void topPaddingChanged()
Emitted when top padding changes.

#### void bottomPaddingChanged()
Emitted when bottom padding changes.

#### void clipChanged()
Emitted when clipping setting changes.

#### void stateChanged()
Emitted when active state changes.

#### void dirty()
Emitted to request a frame re-render when item visual state changes.

## Public Methods

#### explicit Item(QObject *parent = nullptr)
Constructs an `Item` with the optional parent visual item.

#### virtual ~Item()
Destroys the `Item`, cleaning up child items.

#### void markDirty()
Notifies the parent item and rendering engine that this item needs to be redrawn.

#### QPoint absolutePosition() const
Calculates and returns the absolute (x, y) coordinates of the item relative to the root window.

#### const QList<Item *> &childItems() const
Returns the list of visual child items.

#### const QList<Item *> &stackingChildren() const
Returns the list of visual child items sorted by stacking order (`z`).

#### bool isEffectivelyEnabled() const
Returns whether this item is enabled, considering the parent's enabled state hierarchy.

#### Terminal *terminal() const
Returns the root `Terminal` object this item is associated with.

#### void evaluateStateConditions()
Re-evaluates declarative states to update values according to state constraints.

## Protected Virtual Methods / Event Handlers

#### virtual void updateLayout()
Performs layout work when the item has `LayoutDirty` set. The framework runs a
top-down layout pass over the whole tree before the paint pass of each frame,
so geometry changed here is painted in the same frame. `setWidth()`,
`setHeight()`, and effective implicit-size changes mark `LayoutDirty`.
Invalidations raised from `paint()` are retained but do not schedule a frame.

#### virtual void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect) const
Paints only this item's own content. The framework privately traverses children
afterward. The method is `const`: painting writes cells, never item state.
Lazily computed layout caches belong in `mutable` members.

#### virtual void paintOverlay(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect) const
Paints this item's foreground decoration after its children. The default
implementation does nothing.

#### virtual void keyPressEvent(KeyEvent* event)
Called when a key is pressed while the item has focus.

#### virtual void mousePressEvent(MouseEvent* event)
Called when a mouse button is pressed over the item.

#### virtual void mouseReleaseEvent(MouseEvent* event)
Called when a mouse button is released over the item.

#### virtual void mouseMoveEvent(MouseEvent* event)
Called when the mouse cursor moves over the item while a button is held. Passive
hover tracking is requested through `MouseArea.hoverEnabled` or by components
such as attached tooltips.

#### virtual void wheelEvent(MouseEvent* event)
Called when mouse wheel scrolling occurs over the item.

## Ownership and Lifecycle

`Item` objects pass `QObject* parent` to the constructor and are parent-owned. Deletion of a parent `Item` deletes all child items.

## Thread Safety

Single-threaded. Must be instantiated and used on the main GUI thread.

## Usage Example

```cpp
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

using namespace qmltui;

class CustomWidget : public Item {
    Q_OBJECT
public:
    explicit CustomWidget(QObject* parent = nullptr) : Item(parent) {
        setImplicitWidth(10);
        setImplicitHeight(3);
    }

protected:
    void paint(TextGridBuffer &buffer, int absX, int absY,
               const ClipRect& clipRect) const override {
        buffer.drawBox(absX, absY, width(), height(), Color("green"));
    }
};
```

See the [QML type reference](../qml-types/item.md) for QML usage.
