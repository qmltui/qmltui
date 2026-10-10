#include <Qtui/Core/items/Row.h>

namespace qtui {

Row::Row(QObject *parent) : Item(parent) {
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}

Row::~Row() = default;

void Row::setSpacing(int spacing) {
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

void Row::layoutChildren() {
    int currentX = leftPadding();
    int topPad = topPadding();

    QHash<const QObject*, Placement> placements;
    m_positioning = true;
    for (Item* childItem : childItems()) {
        if (!childItem) continue;
        // Keep a hidden child's offset so showing it again does not add the
        // padding twice.
        auto it = m_placements.find(childItem);
        if (it != m_placements.end()) {
            placements.insert(childItem, *it);
            m_placements.erase(it);
        }
        if (childItem->visible()) {
            // Skip layout if the child is anchored (anchors take precedence)
            if (childItem->anchors()->fill() || childItem->anchors()->centerIn()) {
                continue;
            }
            auto placed = placements.find(childItem);
            if (placed == placements.end())
                placed = placements.insert(childItem, track(childItem));
            childItem->setX(currentX);
            childItem->setY(placed->ownY + topPad);
            currentX += childItem->width() + m_spacing;
        }
    }
    m_positioning = false;

    // Whatever is left belongs to items that are no longer children.
    for (const Placement &gone : std::as_const(m_placements)) {
        disconnect(gone.yChanged);
        disconnect(gone.destroyed);
    }
    m_placements = std::move(placements);
    clearDirty(DirtyFlag::LayoutDirty);
}

Row::Placement Row::track(Item *childItem) {
    Placement placement;
    placement.ownY = childItem->y();
    placement.yChanged = connect(childItem, &Item::yChanged, this, [this, childItem] {
        if (m_positioning) return;
        auto it = m_placements.find(childItem);
        if (it == m_placements.end()) return;
        it->ownY = childItem->y();
        if (topPadding() != 0)
            markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    });
    placement.destroyed = connect(childItem, &QObject::destroyed, this,
                                  [this](QObject *child) { m_placements.remove(child); });
    return placement;
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

void Row::updateLayout() {
    layoutChildren();
}

} // namespace qtui
