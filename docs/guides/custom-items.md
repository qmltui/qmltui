# Creating Custom Items

You can extend QmlTui in C++ by subclassing `qmltui::Item` and registering the class with the QML type system.

## Class Definition

Inherit from `qmltui::Item` and add `Q_OBJECT` and `QML_ELEMENT`. Call `markDirty()` whenever a property setter changes the item's visual output:

```cpp
#pragma once
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>

namespace qmltui {

class CustomBlock : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)

public:
    explicit CustomBlock(QObject *parent = nullptr) : Item(parent) {}

    QString label() const { return m_label; }
    void setLabel(const QString &label) {
        if (m_label == label)
            return;
        m_label = label;
        setImplicitWidth(UnicodeText::displayWidth(m_label) + leftPadding() + rightPadding());
        setImplicitHeight(1 + topPadding() + bottomPadding());
        emit labelChanged();
        markDirty();
    }

signals:
    void labelChanged();

protected:
    void paint(TextGridBuffer &buffer, int absX, int absY,
               const ClipRect &clipRect) const override;

private:
    QString m_label;
};

} // namespace qmltui
```

## Painting

Override the protected `paint(TextGridBuffer &, int absX, int absY, const ClipRect &) const` hook. During each frame, the engine calls `paint()`, traverses visible children in stacking order with content offsets and clipping applied, and finally calls `paintOverlay()`. Child traversal is owned by the engine; custom items must not iterate or paint their children directly.

Respect `clipRect` and use `TextGridBuffer::writeText()`, `setGrapheme()`, or `fillRect()` for grapheme-aware cell writes. When accessing individual cells, use `tryCellAt()` if coordinates may fall outside the buffer, or `cellAt()` when coordinates are already bounded by `clipRect`:

```cpp
void CustomBlock::paint(TextGridBuffer &buffer, int absX, int absY,
                        const ClipRect &clipRect) const {
    Q_UNUSED(clipRect)
    const Color foreground("red");
    buffer.writeText(absX + leftPadding(), absY + topPadding(), m_label, foreground);
}
```

Override `updateLayout()` for work that positions or creates children. Each frame runs a layout pass over the tree before painting, so geometry updated in `updateLayout()` is painted in the same frame. A size change on the item itself marks it `LayoutDirty` automatically.

`paint()` and `paintOverlay()` are `const`: they must only write cells to `buffer`. Store lazily computed layout caches in `mutable` members. Invalidations that reach the tree during painting are retained for the next frame without scheduling an immediate repaint; set `QMLTUI_DEBUG_PAINT_DIRTY=1` in the environment to log any item that dirties the tree during paint.

Override `paintOverlay()` only for chrome that must appear above the item's children, such as a scrollbar or focus frame.

## Event Handling

To handle keyboard or mouse input directly in C++, set a focus policy (such as `setFocusPolicy(Qt::StrongFocus)`) and override `keyPressEvent(KeyEvent *event)`:

```cpp
void CustomBlock::keyPressEvent(KeyEvent *event) {
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        event->setAccepted(true);
    }
}
```

## Complete Examples

- [`examples/recipes/custom_cpp_item/`](../../examples/recipes/custom_cpp_item/): Subclassing `qmltui::Item` (`ProgressGauge`), registering with `QML_ELEMENT`, and instantiating from QML.
- [`examples/recipes/big_text/`](../../examples/recipes/big_text/): Custom C++ item painting 3×5 block-pixel banner text into `TextGridBuffer`.
- [`examples/recipes/cpp_only/`](../../examples/recipes/cpp_only/): Constructing and driving a `qmltui::Terminal` and `qmltui::Item` hierarchy directly in C++ without `.qml` files.
