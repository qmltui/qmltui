#include <Qtui/Core/items/LinearLayout.h>

#include <Qtui/Core/items/Layout.h>
#include <QQmlEngine>
#include <algorithm>

namespace qtui {

LinearLayout::LinearLayout(Direction direction, QObject *parent)
    : Item(parent), m_direction(direction) {
    setClip(true);
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}

void LinearLayout::setSpacing(int spacing) {
    spacing = std::max(0, spacing);
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

static LayoutAttached *layoutInfo(Item *item) {
    return qobject_cast<LayoutAttached*>(
        qmlAttachedPropertiesObject<Layout>(item, true));
}

int LinearLayout::preferredMainSize(Item *item) const {
    LayoutAttached *info = layoutInfo(item);
    const int preferred = m_direction == Horizontal ? info->preferredWidth : info->preferredHeight;
    const int value = preferred >= 0 ? preferred :
        (m_direction == Horizontal ? item->implicitWidth() : item->implicitHeight());
    return std::clamp(value, minimumMainSize(item), maximumMainSize(item));
}

int LinearLayout::minimumMainSize(Item *item) const {
    LayoutAttached *info = layoutInfo(item);
    return m_direction == Horizontal ? info->minimumWidth : info->minimumHeight;
}

int LinearLayout::maximumMainSize(Item *item) const {
    LayoutAttached *info = layoutInfo(item);
    const int minimum = m_direction == Horizontal ? info->minimumWidth : info->minimumHeight;
    return std::max(minimum, m_direction == Horizontal ? info->maximumWidth : info->maximumHeight);
}

void LinearLayout::layoutChildren() {
    QList<Item*> children;
    for (Item *child : childItems()) {
        if (child && child->visible())
            children.append(child);
    }
    if (children.isEmpty()) return;

    const int mainAvailable = std::max(0,
        (m_direction == Horizontal ? width() - leftPadding() - rightPadding()
                                   : height() - topPadding() - bottomPadding()) -
        m_spacing * (static_cast<int>(children.size()) - 1));
    const int crossAvailable = std::max(0,
        m_direction == Horizontal ? height() - topPadding() - bottomPadding()
                                  : width() - leftPadding() - rightPadding());

    QVector<int> sizes;
    sizes.reserve(children.size());
    int used = 0;
    for (Item *child : children) {
        const int size = preferredMainSize(child);
        sizes.append(size);
        used += size;
    }

    int remaining = mainAvailable - used;
    while (remaining != 0) {
        int totalStretch = 0;
        for (int i = 0; i < children.size(); ++i) {
            LayoutAttached *info = layoutInfo(children[i]);
            const bool fill = m_direction == Horizontal ? info->fillWidth : info->fillHeight;
            if ((remaining > 0 && fill && sizes[i] < maximumMainSize(children[i])) ||
                (remaining < 0 && sizes[i] > minimumMainSize(children[i])))
                totalStretch += std::max(1, m_direction == Horizontal
                    ? info->horizontalStretch : info->verticalStretch);
        }
        if (totalStretch == 0) break;

        bool changed = false;
        for (int i = 0; i < children.size() && remaining != 0; ++i) {
            LayoutAttached *info = layoutInfo(children[i]);
            const bool fill = m_direction == Horizontal ? info->fillWidth : info->fillHeight;
            if (remaining > 0 && !fill) continue;
            const int limit = remaining > 0 ? maximumMainSize(children[i]) : minimumMainSize(children[i]);
            if ((remaining > 0 && sizes[i] >= limit) || (remaining < 0 && sizes[i] <= limit))
                continue;
            const int stretch = std::max(1, m_direction == Horizontal
                ? info->horizontalStretch : info->verticalStretch);
            int delta = std::max(1, std::abs(remaining) * stretch / totalStretch);
            delta = std::min(delta, std::abs(limit - sizes[i]));
            delta = std::min(delta, std::abs(remaining));
            if (remaining < 0) delta = -delta;
            sizes[i] += delta;
            remaining -= delta;
            changed = true;
        }
        if (!changed) break;
    }

    int position = m_direction == Horizontal ? leftPadding() : topPadding();
    for (int i = 0; i < children.size(); ++i) {
        Item *child = children[i];
        LayoutAttached *info = layoutInfo(child);
        if (m_direction == Horizontal) {
            child->setX(position);
            child->setY(topPadding());
            child->setWidth(sizes[i]);
            if (info->fillHeight)
                child->setHeight(std::clamp(crossAvailable, info->minimumHeight,
                                            std::max(info->minimumHeight, info->maximumHeight)));
        } else {
            child->setX(leftPadding());
            child->setY(position);
            child->setHeight(sizes[i]);
            if (info->fillWidth)
                child->setWidth(std::clamp(crossAvailable, info->minimumWidth,
                                           std::max(info->minimumWidth, info->maximumWidth)));
        }
        position += sizes[i] + m_spacing;
    }
    clearDirty(DirtyFlag::LayoutDirty);
}

int LinearLayout::implicitWidth() const {
    int value = leftPadding() + rightPadding();
    int count = 0;
    for (Item *child : childItems()) {
        if (!child || !child->visible()) continue;
        value += m_direction == Horizontal ? preferredMainSize(child) : child->implicitWidth();
        ++count;
    }
    if (m_direction == Horizontal && count > 1) value += (count - 1) * m_spacing;
    return value;
}

int LinearLayout::implicitHeight() const {
    int value = topPadding() + bottomPadding();
    int count = 0;
    for (Item *child : childItems()) {
        if (!child || !child->visible()) continue;
        value += m_direction == Vertical ? preferredMainSize(child) : child->implicitHeight();
        ++count;
    }
    if (m_direction == Vertical && count > 1) value += (count - 1) * m_spacing;
    return value;
}

void LinearLayout::updateLayout() {
    layoutChildren();
}

} // namespace qtui
