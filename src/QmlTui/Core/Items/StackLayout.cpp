#include <QmlTui/Core/Items/StackLayout.h>

#include <algorithm>

namespace qmltui {

StackLayout::StackLayout(QObject* parent) : Item(parent) {
    auto refresh = [this] {
        const int currentCount = count();
        if (m_lastCount != currentCount) {
            m_lastCount = currentCount;
            emit countChanged();
        }
        layoutChildren();
        notifyImplicitSizeIfChanged();
        markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    };

    connect(this, &Item::childrenChanged, this, refresh);
    connect(this, &Item::widthChanged, this, refresh);
    connect(this, &Item::heightChanged, this, refresh);
    connect(this, &Item::paddingChanged, this, refresh);
    connect(this, &Item::leftPaddingChanged, this, refresh);
    connect(this, &Item::rightPaddingChanged, this, refresh);
    connect(this, &Item::topPaddingChanged, this, refresh);
    connect(this, &Item::bottomPaddingChanged, this, refresh);
}

void StackLayout::setCurrentIndex(int index) {
    if (m_currentIndex == index) return;
    m_currentIndex = index;
    emit currentIndexChanged();
    layoutChildren();
    notifyImplicitSizeIfChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}

void StackLayout::layoutChildren() {
    const QList<Item*>& children = childItems();
    const int contentX = leftPadding();
    const int contentY = topPadding();
    const int contentWidth = (std::max)(0, width() - leftPadding() - rightPadding());
    const int contentHeight = (std::max)(0, height() - topPadding() - bottomPadding());

    for (qsizetype i = 0; i < children.size(); ++i) {
        Item* child = children.at(i);
        if (!child) continue;
        const bool isCurrent = (static_cast<int>(i) == m_currentIndex);
        child->setVisible(isCurrent);
        if (isCurrent) {
            child->setX(contentX);
            child->setY(contentY);
            child->setWidth(contentWidth);
            child->setHeight(contentHeight);
        }
    }
    clearDirty(DirtyFlag::LayoutDirty);
}

int StackLayout::implicitWidth() const {
    const QList<Item*>& children = childItems();
    const int base = leftPadding() + rightPadding();
    if (m_currentIndex >= 0 && m_currentIndex < children.size()) {
        if (const Item* active = children.at(m_currentIndex)) return base + active->implicitWidth();
    }
    return base;
}

int StackLayout::implicitHeight() const {
    const QList<Item*>& children = childItems();
    const int base = topPadding() + bottomPadding();
    if (m_currentIndex >= 0 && m_currentIndex < children.size()) {
        if (const Item* active = children.at(m_currentIndex))
            return base + active->implicitHeight();
    }
    return base;
}

void StackLayout::updateLayout() {
    layoutChildren();
}

} // namespace qmltui
