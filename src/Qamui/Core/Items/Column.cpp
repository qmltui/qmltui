#include <Qamui/Core/Items/Column.h>
#include <algorithm>

namespace qamui {

Column::Column(QObject *parent) : Item(parent) {
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}

Column::~Column() = default;

void Column::setSpacing(int spacing) {
    const int sanitized = (std::max)(0, spacing);
    if (m_spacing == sanitized) return;
    m_spacing = sanitized;
    emit spacingChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

void Column::layoutChildren() {
    int currentY = topPadding();
    int leftPad = leftPadding();

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
            if (childItem->anchors()->fill() || childItem->anchors()->centerIn()) {
                continue;
            }
            auto placed = placements.find(childItem);
            if (placed == placements.end())
                placed = placements.insert(childItem, track(childItem));
            childItem->setX(placed->ownX + leftPad);
            childItem->setY(currentY);
            currentY += childItem->height() + m_spacing;
        }
    }
    m_positioning = false;

    // Whatever is left belongs to items that are no longer children.
    for (const Placement &gone : std::as_const(m_placements)) {
        disconnect(gone.xChanged);
        disconnect(gone.destroyed);
    }
    m_placements = std::move(placements);
    clearDirty(DirtyFlag::LayoutDirty);
}

Column::Placement Column::track(Item *childItem) {
    Placement placement;
    placement.ownX = childItem->x();
    placement.xChanged = connect(childItem, &Item::xChanged, this, [this, childItem] {
        if (m_positioning) return;
        auto it = m_placements.find(childItem);
        if (it == m_placements.end()) return;
        it->ownX = childItem->x();
        if (leftPadding() != 0)
            markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    });
    placement.destroyed = connect(childItem, &QObject::destroyed, this,
                                  [this](QObject *child) { m_placements.remove(child); });
    return placement;
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

} // namespace qamui
