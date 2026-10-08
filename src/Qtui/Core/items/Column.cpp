#include <Qtui/Core/items/Column.h>

namespace qtui {

Column::Column(QObject *parent) : Item(parent) {
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}

Column::~Column() = default;

void Column::setSpacing(int spacing) {
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
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
    clearDirty(DirtyFlag::LayoutDirty);
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

void Column::updateLayout() {
    layoutChildren();
}

} // namespace qtui
