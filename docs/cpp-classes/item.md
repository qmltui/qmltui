# Item

`qmltui::Item` is the base visual class for all elements in a QmlTui scene. It provides integer character-cell geometry (`x`, `y`, `z`, `width`, `height`, `implicitWidth`, `implicitHeight`, padding), anchoring, focus navigation (`focus`, `focusPolicy`, `activeFocus`), state machines (`states`, `transitions`, `state`), and virtual layout, painting, and input hooks.

- **Header**: `<QmlTui/Core/Items/Item.h>`
- **Inherits**: `QObject`
- **Target**: `qmltui::core`

## Properties (`Q_PROPERTY`)

| Property | Type | Access | Notify Signal | Description |
| :--- | :--- | :--- | :--- | :--- |
| `x`, `y` | `int` | read/write | `xChanged`, `yChanged` | Position in cells relative to the parent item. |
| `z` | `qreal` | read/write | `zChanged` | Stacking order among siblings (ascending). |
| `width`, `height` | `int` | read/write | `widthChanged`, `heightChanged` | Item dimensions in character columns and rows. |
| `implicitWidth`, `implicitHeight` | `int` | read/write | `implicitWidthChanged`, `implicitHeightChanged` | Natural content size in columns and rows. |
| `visible` | `bool` | read/write | `visibleChanged` | Whether the item and its subtree are painted. |
| `enabled` | `bool` | read/write | `enabledChanged` | Whether the item accepts input events. |
| `focus` | `bool` | read/write | `focusChanged` | Requested focus state within the enclosing focus scope. |
| `activeFocus` | `bool` | read-only | `activeFocusChanged` | Whether the item currently holds active keyboard focus. |
| `focusPolicy` | `Qt::FocusPolicy` | read/write | `focusPolicyChanged` | Focus acceptance policy (`Qt::TabFocus`, `Qt::ClickFocus`, `Qt::StrongFocus`, `Qt::NoFocus`). |
| `parent` | `qmltui::Item*` | read/write | `parentChanged` | Visual parent item (C++ accessors: `parentItem()` / `setParentItem(Item *)`). |
| `anchors` | `qmltui::Anchors*` | read-only | — | Grouped anchor constraints (`fill`, `centerIn`, edges, margins). |
| `left`, `right`, `top`, `bottom`, `horizontalCenter`, `verticalCenter` | `qmltui::AnchorLine*` | read-only | — | Anchor lines for relative positioning. |
| `clip` | `bool` | read/write | `clipChanged` | Clips child painting to this item's bounds. |
| `padding`, `leftPadding`, `rightPadding`, `topPadding`, `bottomPadding` | `int` | read/write | `*PaddingChanged` | Interior cell padding on all or individual edges. |
| `data` | `QQmlListProperty<QObject>` | read-only | — | Default QML property accepting child items and non-visual objects. |
| `children` | `QQmlListProperty<qmltui::Item>` | read-only | `childrenChanged` | Visual child items (C++ accessor: `childrenProperty() const`). |
| `states` | `QQmlListProperty<qmltui::State>` | read-only | — | Declarative states attached to this item. |
| `transitions` | `QQmlListProperty<qmltui::Transition>` | read-only | — | State transitions attached to this item. |
| `state` | `QString` | read/write | `stateChanged` | Name of the active state. |

In addition to the property notification signals above, `Item` emits `ancestryChanged()` when the item or any visual ancestor changes parent, and `dirty()` when a visual invalidation occurs.

## `ClipRect`

```cpp
struct ClipRect {
    int x = 0;
    int y = 0;
    int width = 99999;
    int height = 99999;

    bool isEmpty() const;
    bool contains(int cx, int cy) const;
    ClipRect intersected(const ClipRect &other) const;
};
```

## Public Methods

#### `Item *parentItem() const` / `virtual void setParentItem(Item *parentItem)`
Returns or sets the visual parent `Item` (backing the `parent` QML property).

#### `Q_INVOKABLE void forceActiveFocus()`
Grants active keyboard focus to this item and updates its enclosing focus scope chain.

#### `void markDirty()`
Marks this item dirty and schedules a frame render on the owning `TerminalEngine`.

#### `QPoint absolutePosition() const`
Returns the item's `(x, y)` cell coordinates relative to the root `Terminal`.

#### `const QList<Item *> &childItems() const`
Returns the visual child items in declaration order.

#### `const QList<Item *> &stackingChildren() const`
Returns the visual child items sorted by ascending `z`.

#### `bool isEffectivelyEnabled() const`
Returns `true` if this item and all of its visual ancestors have `enabled() == true`.

#### `Terminal *terminal() const`
Returns the root `Terminal` for this item's tree, or `nullptr` if unattached.

#### `void evaluateStateConditions()`
Evaluates `when` conditions on the item's declarative `states`.

#### `virtual void keyPressEvent(KeyEvent *event)`
Called when a key is pressed while the item has active focus. Call `event->setAccepted(true)` to stop propagation.

#### `virtual void mousePressEvent(MouseEvent *event)`
#### `virtual void mouseReleaseEvent(MouseEvent *event)`
#### `virtual void mouseMoveEvent(MouseEvent *event)`
#### `virtual void wheelEvent(MouseEvent *event)`
Public virtual mouse and wheel event handlers. Event coordinates (`event->x()`, `event->y()`) are always delivered in this item's local cell space. Passive motion without a held button is delivered only while hover tracking is active (`MouseArea.hoverEnabled` or attached `ToolTip`).

## Protected Virtual Hooks

#### `virtual void updateLayout()`
Runs during the pre-paint layout pass when the item is marked `LayoutDirty` (for example after `setWidth()`, `setHeight()`, or implicit size changes). Geometry changes made in `updateLayout()` take effect in the same frame.

#### `virtual void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) const`
Paints this item's own content into `buffer`. The engine traverses children afterward. `paint()` is `const` and must not mutate item state; store lazy layout caches in `mutable` members.

#### `virtual void paintOverlay(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) const`
Paints foreground chrome (such as a scrollbar or focus frame) after all children have painted.

#### `virtual ClipRect clipRectForChildren(int absX, int absY, const ClipRect &clipRect) const`
Returns the clipping rectangle applied to this item's children: intersects `clipRect` with this item's bounds when `clip()` is `true`, or returns `clipRect` unchanged.

## Subclassing Example

```cpp
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

class BoxItem : public qmltui::Item {
    Q_OBJECT
public:
    explicit BoxItem(QObject *parent = nullptr) : qmltui::Item(parent) {
        setImplicitWidth(10);
        setImplicitHeight(3);
    }

protected:
    void paint(qmltui::TextGridBuffer &buffer, int absX, int absY,
               const ClipRect &clipRect) const override {
        Q_UNUSED(clipRect)
        buffer.drawBox(absX, absY, width(), height(), qmltui::Color(QStringLiteral("green")));
    }
};
```

See [Creating Custom Items](../guides/custom-items.md) and the [Item QML type reference](../qml-types/item.md).
