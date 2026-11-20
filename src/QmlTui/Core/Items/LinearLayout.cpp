#include <QmlTui/Core/Items/LinearLayout.h>

#include <QmlTui/Core/Items/Layout.h>
#include <QQmlEngine>
#include <QVarLengthArray>
#include <algorithm>
#include <limits>

namespace qmltui {

LinearLayout::LinearLayout(Direction direction, QObject* parent)
    : Item(parent), m_direction(direction) {
    setClip(true);
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}

void LinearLayout::setSpacing(int spacing) {
    spacing = (std::max)(0, spacing);
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

static const LayoutAttached* layoutInfo(Item* item) {
    return qobject_cast<const LayoutAttached*>(qmlAttachedPropertiesObject<Layout>(item, false));
}

int LinearLayout::preferredMainSize(Item* item) const {
    const LayoutAttached* info = layoutInfo(item);
    const int preferred =
        info ? (m_direction == Horizontal ? info->preferredWidth : info->preferredHeight) : -1;
    const int value = preferred >= 0 ? preferred
                                     : (m_direction == Horizontal ? item->implicitWidth()
                                                                  : item->implicitHeight());
    return std::clamp(value, minimumMainSize(item), maximumMainSize(item));
}

int LinearLayout::minimumMainSize(Item* item) const {
    const LayoutAttached* info = layoutInfo(item);
    return info ? (m_direction == Horizontal ? info->minimumWidth : info->minimumHeight) : 0;
}

int LinearLayout::maximumMainSize(Item* item) const {
    const LayoutAttached* info = layoutInfo(item);
    if (!info) return (std::numeric_limits<int>::max)();
    const int minimum = m_direction == Horizontal ? info->minimumWidth : info->minimumHeight;
    return (std::max)(minimum,
                      m_direction == Horizontal ? info->maximumWidth : info->maximumHeight);
}

int LinearLayout::minimumCrossSize(Item* item) const {
    const LayoutAttached* info = layoutInfo(item);
    return info ? (m_direction == Horizontal ? info->minimumHeight : info->minimumWidth) : 0;
}

int LinearLayout::maximumCrossSize(Item* item) const {
    const LayoutAttached* info = layoutInfo(item);
    if (!info) return (std::numeric_limits<int>::max)();
    const int minimum = m_direction == Horizontal ? info->minimumHeight : info->minimumWidth;
    return (std::max)(minimum,
                      m_direction == Horizontal ? info->maximumHeight : info->maximumWidth);
}

int LinearLayout::preferredCrossSize(Item* item) const {
    const LayoutAttached* info = layoutInfo(item);
    const int preferred =
        info ? (m_direction == Horizontal ? info->preferredHeight : info->preferredWidth) : -1;
    const int value = preferred >= 0 ? preferred
                                     : (m_direction == Horizontal ? item->implicitHeight()
                                                                  : item->implicitWidth());
    return std::clamp(value, minimumCrossSize(item), maximumCrossSize(item));
}

void LinearLayout::layoutChildren() {
    QList<Item*> children;
    for (Item* child : childItems()) {
        if (child && child->visible()) children.append(child);
    }
    if (children.isEmpty()) return;

    const int mainAvailable =
        (std::max)(0, (m_direction == Horizontal ? width() - leftPadding() - rightPadding()
                                                 : height() - topPadding() - bottomPadding()) -
                          m_spacing * (static_cast<int>(children.size()) - 1));
    const int crossAvailable =
        (std::max)(0, m_direction == Horizontal ? height() - topPadding() - bottomPadding()
                                                : width() - leftPadding() - rightPadding());

    const qsizetype count = children.size();
    QVarLengthArray<const LayoutAttached*, 32> infos(count);
    QVarLengthArray<int, 32> minSizes(count);
    QVarLengthArray<int, 32> maxSizes(count);
    QVector<int> sizes;
    sizes.reserve(count);
    int used = 0;
    for (qsizetype i = 0; i < count; ++i) {
        Item* child = children[i];
        const LayoutAttached* info = layoutInfo(child);
        infos[i] = info;
        const int minS =
            info ? (m_direction == Horizontal ? info->minimumWidth : info->minimumHeight) : 0;
        const int maxS = info ? (std::max)(minS, m_direction == Horizontal ? info->maximumWidth
                                                                           : info->maximumHeight)
                              : (std::numeric_limits<int>::max)();
        minSizes[i] = minS;
        maxSizes[i] = maxS;
        const int preferred =
            info ? (m_direction == Horizontal ? info->preferredWidth : info->preferredHeight) : -1;
        const int raw = preferred >= 0 ? preferred
                                       : (m_direction == Horizontal ? child->implicitWidth()
                                                                    : child->implicitHeight());
        const int size = std::clamp(raw, minS, maxS);
        sizes.append(size);
        used += size;
    }

    int remaining = mainAvailable - used;
    while (remaining != 0) {
        int totalStretch = 0;
        for (qsizetype i = 0; i < count; ++i) {
            const LayoutAttached* info = infos[i];
            const bool fill =
                info ? (m_direction == Horizontal ? info->fillWidth : info->fillHeight) : false;
            const int stretch =
                info ? (std::max)(1, m_direction == Horizontal ? info->horizontalStretch
                                                               : info->verticalStretch)
                     : 1;
            if ((remaining > 0 && fill && sizes[i] < maxSizes[i]) ||
                (remaining < 0 && sizes[i] > minSizes[i]))
                totalStretch += stretch;
        }
        if (totalStretch == 0) break;

        bool changed = false;
        int remainingStretch = totalStretch;
        for (qsizetype i = 0; i < count && remaining != 0; ++i) {
            const LayoutAttached* info = infos[i];
            const bool fill =
                info ? (m_direction == Horizontal ? info->fillWidth : info->fillHeight) : false;
            if (remaining > 0 && !fill) continue;
            const int limit = remaining > 0 ? maxSizes[i] : minSizes[i];
            if ((remaining > 0 && sizes[i] >= limit) || (remaining < 0 && sizes[i] <= limit))
                continue;
            const int stretch =
                info ? (std::max)(1, m_direction == Horizontal ? info->horizontalStretch
                                                               : info->verticalStretch)
                     : 1;
            int delta = (std::max)(1, std::abs(remaining) * stretch / remainingStretch);
            delta = (std::min)(delta, std::abs(limit - sizes[i]));
            delta = (std::min)(delta, std::abs(remaining));
            if (remaining < 0) delta = -delta;
            sizes[i] += delta;
            remaining -= delta;
            remainingStretch -= stretch;
            changed = true;
        }
        if (!changed) break;
    }

    int position = m_direction == Horizontal ? leftPadding() : topPadding();
    for (qsizetype i = 0; i < count; ++i) {
        Item* child = children[i];
        const LayoutAttached* info = infos[i];
        if (m_direction == Horizontal) {
            const int minH = info ? info->minimumHeight : 0;
            const int maxH =
                info ? (std::max)(minH, info->maximumHeight) : (std::numeric_limits<int>::max)();
            if (info && info->fillHeight) {
                child->setHeight(std::clamp(crossAvailable, minH, maxH));
            } else if (info && info->preferredHeight >= 0) {
                child->setHeight(
                    std::clamp((std::min)(info->preferredHeight, crossAvailable), minH, maxH));
            } else if (info && (info->minimumHeight > 0 ||
                                info->maximumHeight < (std::numeric_limits<int>::max)())) {
                child->setHeight(
                    std::clamp((std::min)(child->height(), crossAvailable), minH, maxH));
            }

            int yPos = topPadding();
            if (info && !info->fillHeight) {
                const int slack = (std::max)(0, crossAvailable - child->height());
                if (info->alignment & Qt::AlignBottom)
                    yPos += slack;
                else if (info->alignment & Qt::AlignVCenter)
                    yPos += slack / 2;
            }

            child->setX(position);
            child->setY(yPos);
            child->setWidth(sizes[i]);
        } else {
            const int minW = info ? info->minimumWidth : 0;
            const int maxW =
                info ? (std::max)(minW, info->maximumWidth) : (std::numeric_limits<int>::max)();
            if (info && info->fillWidth) {
                child->setWidth(std::clamp(crossAvailable, minW, maxW));
            } else if (info && info->preferredWidth >= 0) {
                child->setWidth(
                    std::clamp((std::min)(info->preferredWidth, crossAvailable), minW, maxW));
            } else if (info && (info->minimumWidth > 0 ||
                                info->maximumWidth < (std::numeric_limits<int>::max)())) {
                child->setWidth(std::clamp((std::min)(child->width(), crossAvailable), minW, maxW));
            }

            int xPos = leftPadding();
            if (info && !info->fillWidth) {
                const int slack = (std::max)(0, crossAvailable - child->width());
                if (info->alignment & Qt::AlignRight)
                    xPos += slack;
                else if (info->alignment & Qt::AlignHCenter)
                    xPos += slack / 2;
            }

            child->setX(xPos);
            child->setY(position);
            child->setHeight(sizes[i]);
        }
        position += sizes[i] + m_spacing;
    }
    notifyImplicitSizeIfChanged();
    clearDirty(DirtyFlag::LayoutDirty);
}

int LinearLayout::implicitWidth() const {
    if (m_direction == Horizontal) {
        int value = leftPadding() + rightPadding();
        int count = 0;
        for (Item* child : childItems()) {
            if (!child || !child->visible()) continue;
            value += preferredMainSize(child);
            ++count;
        }
        if (count > 1) value += (count - 1) * m_spacing;
        return value;
    }
    int maxCross = 0;
    for (Item* child : childItems()) {
        if (!child || !child->visible()) continue;
        maxCross = (std::max)(maxCross, preferredCrossSize(child));
    }
    return leftPadding() + rightPadding() + maxCross;
}

int LinearLayout::implicitHeight() const {
    if (m_direction == Vertical) {
        int value = topPadding() + bottomPadding();
        int count = 0;
        for (Item* child : childItems()) {
            if (!child || !child->visible()) continue;
            value += preferredMainSize(child);
            ++count;
        }
        if (count > 1) value += (count - 1) * m_spacing;
        return value;
    }
    int maxCross = 0;
    for (Item* child : childItems()) {
        if (!child || !child->visible()) continue;
        maxCross = (std::max)(maxCross, preferredCrossSize(child));
    }
    return topPadding() + bottomPadding() + maxCross;
}

void LinearLayout::updateLayout() {
    layoutChildren();
}

} // namespace qmltui
