#include "Row.h"

namespace qtui {

Row::Row(QObject *parent) : Item(parent) {
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}

Row::~Row() = default;

void Row::setSpacing(int spacing) {
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
    emit implicitWidthChanged();
    emit implicitHeightChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}

void Row::layoutChildren() {
    int currentX = leftPadding();
    int topPad = topPadding();

    for (Item* childItem : childItems()) {
        if (childItem && childItem->visible()) {
            // Skip layout if the child is anchored (anchors take precedence)
            if (childItem->anchors()->fill() || childItem->anchors()->centerIn()) {
                continue;
            }
            childItem->setX(currentX);
            childItem->setY(topPad);
            currentX += childItem->width() + m_spacing;
        }
    }
    clearDirty(DirtyFlag::LayoutDirty);
}

int Row::implicitWidth() const {
    int w = leftPadding() + rightPadding();
    int visibleCount = 0;
    for (Item* childItem : childItems()) {
        if (childItem && childItem->visible()) {
            if (childItem->anchors()->fill() || childItem->anchors()->centerIn()) {
                continue;
            }
            w += childItem->width();
            visibleCount++;
        }
    }
    if (visibleCount > 1) {
        w += (visibleCount - 1) * m_spacing;
    }
    return w;
}

int Row::implicitHeight() const {
    int h = 0;
    for (Item* childItem : childItems()) {
        if (childItem && childItem->visible()) {
            if (childItem->anchors()->fill() || childItem->anchors()->centerIn()) {
                continue;
            }
            if (childItem->height() > h) {
                h = childItem->height();
            }
        }
    }
    return h + topPadding() + bottomPadding();
}

void Row::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;
    if (dirtyFlags().testFlag(DirtyFlag::LayoutDirty)) layoutChildren();
    Item::render(buffer, absX, absY, clipRect);
}

} // namespace qtui
