# Creating Custom Items

You can extend Qamui by creating custom C++ classes that inherit from `qamui::Item` and registering them with the QML engine.

## 1. Class Definition
Inherit from `qamui::Item`. Ensure you have the `Q_OBJECT` and `QML_ELEMENT` macros for QML integration.
If your item has custom properties, define them using `Q_PROPERTY`.

```cpp
#pragma once
#include <Qamui/Core/items/Item.h>

namespace qamui {

class CustomBlock : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)

public:
    explicit CustomBlock(QObject *parent = nullptr) : Item(parent) {
        // e.g., setFocusable(true) if your item accepts input
    }
    
    QString label() const { return m_label; }
    void setLabel(const QString& label) {
        if (m_label == label) return;
        m_label = label;
        emit labelChanged();
        markDirty(); // Tells the engine to re-render this item
    }

signals:
    void labelChanged();

private:
    QString m_label;
};

}
```

## 2. Painting
Override the protected `paint(TextGridBuffer&, int absX, int absY, const ClipRect&) const`
hook. The framework calls `paint()`, then traverses children in stacking order,
applies content offsets and clipping, and finally calls `paintOverlay()`. Tree
painting is private framework behavior; custom items must not traverse or paint
their children themselves.

Respect `clipRect` and use `tryCellAt()` when coordinates may be outside the
buffer. `cellAt()` is the fast, asserted accessor for coordinates already known
to be valid.

```cpp
void CustomBlock::paint(TextGridBuffer& buffer, int absX, int absY,
                        const ClipRect& clipRect) const {
    const Color foreground("red");
    for (int i = 0; i < m_label.length(); ++i) {
        const int cx = absX + leftPadding() + i;
        const int cy = absY + topPadding();
        if (clipRect.contains(cx, cy)) {
            if (Cell *cell = buffer.tryCellAt(cx, cy)) {
                cell->setCharacter(m_label.mid(i, 1).toUtf8());
                cell->setForeground(foreground);
            }
        }
    }
}
```

Override `updateLayout()` for work that positions or creates children. Each
frame runs a layout pass over the tree before any painting, so geometry changed
in `updateLayout()` is painted in the same frame. A size change on the item
itself marks it `LayoutDirty` automatically.

`paint()` and `paintOverlay()` are `const`: they write cells and nothing else.
Keep lazily computed layout caches in `mutable` members. Invalidations that
still reach the tree while painting (through pointers to other items) are
retained but do not schedule a frame; run with `QAMUI_DEBUG_PAINT_DIRTY=1` to
have each offending item reported once.

Override the optional protected `paintOverlay()` hook only for content that
must appear above the item's children, such as a scrollbar or focus decoration.

## 3. Event Handling
If your custom component handles input, override event functions:
```cpp
void CustomBlock::keyPressEvent(KeyEvent* event) override {
    if (event->key() == Qt::Key_Enter) {
        qDebug() << "Custom block activated!";
        event->setAccepted(true); // Stop propagation
    }
}
```

## 4. Layout Sizing
It's good practice to set `implicitWidth` and `implicitHeight` based on your content, so `Row` and `Column` layouts can accurately arrange your custom item.

```cpp
void CustomBlock::setLabel(const QString& label) {
    // ...
    setImplicitWidth(label.length() + leftPadding() + rightPadding());
    setImplicitHeight(1 + topPadding() + bottomPadding());
    markDirty();
}
```
