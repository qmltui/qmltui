#include "Column.h"

namespace qtui {

Column::Column(QObject *parent) : Item(parent) {
}

Column::~Column() = default;

void Column::setSpacing(int spacing) {
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
    markDirty();
}

void Column::layoutChildren() {
    int currentY = topPadding();
    int leftPad = leftPadding();

    for (Item* childItem : childItems()) {
        if (childItem && childItem->visible()) {
            if (childItem->anchors()->fill() || childItem->anchors()->centerIn()) {
                continue;
            }
            childItem->setX(leftPad);
            childItem->setY(currentY);
            currentY += childItem->height() + m_spacing;
        }
    }
}

int Column::implicitWidth() const {
    int w = 0;
    for (Item* childItem : childItems()) {
        if (childItem && childItem->visible()) {
            if (childItem->anchors()->fill() || childItem->anchors()->centerIn()) {
                continue;
            }
            if (childItem->width() > w) {
                w = childItem->width();
            }
        }
    }
    return w + leftPadding() + rightPadding();
}

int Column::implicitHeight() const {
    int h = topPadding() + bottomPadding();
    int visibleCount = 0;
    for (Item* childItem : childItems()) {
        if (childItem && childItem->visible()) {
            if (childItem->anchors()->fill() || childItem->anchors()->centerIn()) {
                continue;
            }
            h += childItem->height();
            visibleCount++;
        }
    }
    if (visibleCount > 1) {
        h += (visibleCount - 1) * m_spacing;
    }
    return h;
}

void Column::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;
    layoutChildren();
    Item::render(buffer, absX, absY, clipRect);
}

} // namespace qtui
