# Creating Custom Items

You can extend Qtui by creating custom C++ classes that inherit from `qtui::Item` and registering them with the QML engine. 

## 1. Class Definition
Inherit from `qtui::Item`. Ensure you have the `Q_OBJECT` and `QML_ELEMENT` macros for QML integration.
If your item has custom properties, define them using `Q_PROPERTY`.

```cpp
#pragma once
#include "Item.h"

namespace qtui {

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

## 2. Rendering
Override the `void render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect)` method. 
It is essential to respect the provided `clipRect` bounds when writing to the buffer to ensure scroll views and layouts behave correctly.

```cpp
void CustomBlock::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    // 1. Call base implementation (draws children and background)
    Item::render(buffer, absX, absY, clipRect);
    
    // 2. Intersect with our own bounds if we have `clip` enabled
    ClipRect contentClip = clipRect;
    if (clip()) {
        contentClip = clipRect.intersected(ClipRect{absX, absY, width(), height()});
    }

    // 3. Write characters into the TextGridBuffer
    for (int i = 0; i < m_label.length(); ++i) {
        int cx = absX + leftPadding() + i;
        int cy = absY + topPadding();
        
        if (contentClip.contains(cx, cy)) {
            if (cx >= 0 && cx < buffer.width() && cy >= 0 && cy < buffer.height()) {
                Cell& cell = buffer.cellAt(cx, cy);
                cell.character[0] = m_label[i].toLatin1();
                cell.character[1] = '\0';
                cell.fgColor = 0xFFFF0000; // Red
            }
        }
    }
}
```

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
