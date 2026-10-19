# Creating Custom Items

You can extend QmlTui by creating custom C++ classes that inherit from `qmltui::Item` and registering them with the QML engine.

## 1. Class Definition
Inherit from `qmltui::Item`. Ensure you have the `Q_OBJECT` and `QML_ELEMENT` macros for QML integration.
If your item has custom properties, define them using `Q_PROPERTY`.

```cpp
#pragma once
#include <QmlTui/Core/Items/Item.h>

namespace qmltui {

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

Respect `clipRect` and prefer `TextGridBuffer::writeText()` (or `setGrapheme()` / `fillRect()`) for zero-allocation grapheme cluster painting. When writing individual cells directly, use `tryCellAt()` when coordinates may fall outside the buffer, or `cellAt()` for coordinates already verified against `clipRect`:

```cpp
void CustomBlock::paint(TextGridBuffer& buffer, int absX, int absY,
                        const ClipRect& clipRect) const {
    Q_UNUSED(clipRect)
    const Color foreground("red");
    buffer.writeText(absX + leftPadding(), absY + topPadding(), m_label, foreground);
}
```

Override `updateLayout()` for work that positions or creates children. Each
frame runs a layout pass over the tree before any painting, so geometry changed
in `updateLayout()` is painted in the same frame. A size change on the item
itself marks it `LayoutDirty` automatically.

`paint()` and `paintOverlay()` are `const`: they write cells and nothing else.
Keep lazily computed layout caches in `mutable` members. Invalidations that
still reach the tree while painting (through pointers to other items) are
retained but do not schedule a frame; run with `QMLTUI_DEBUG_PAINT_DIRTY=1` to
have each offending item reported once.

Override the optional protected `paintOverlay()` hook only for content that
must appear above the item's children, such as a scrollbar or focus decoration.

## 3. Event Handling
If your custom component handles input, declare `keyPressEvent(KeyEvent* event) override` in the class header and implement it in your source file:
```cpp
void CustomBlock::keyPressEvent(KeyEvent* event) {
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
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

## 5. C++-Only Applications & Complete Examples

You can also construct and drive a `qmltui::Terminal` and `qmltui::Item` hierarchy directly from C++ without loading any `.qml` files. See:

- [`examples/recipes/custom_cpp_item/`](../../examples/recipes/custom_cpp_item/): Registering a custom `qmltui::Item` subclass (`QML_ELEMENT`) and instantiating it from QML.
- [`examples/recipes/cpp_only/`](../../examples/recipes/cpp_only/): Building a complete QmlTui scene in pure C++20 without QML files.
- [`examples/recipes/big_text/`](../../examples/recipes/big_text/): A reusable component combining custom cell painting and QML properties.
