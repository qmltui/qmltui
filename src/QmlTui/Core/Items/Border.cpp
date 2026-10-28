#include <QmlTui/Core/Items/Border.h>
#include <QmlTui/Core/Items/Item.h>

namespace qmltui {

Border::Border(Item* owner) : QObject(owner), m_owner(owner) {}

Border::~Border() = default;

void Border::setStyle(Style style) {
    if (m_style == style) return;
    const bool paddingChanged = (m_style == None) != (style == None);
    m_style = style;
    emit styleChanged();
    if (m_owner) {
        if (paddingChanged) {
            m_owner->markDirty(Item::DirtyFlag::LayoutDirty | Item::DirtyFlag::SelfDirty);
            emit m_owner->leftPaddingChanged();
            emit m_owner->rightPaddingChanged();
            emit m_owner->topPaddingChanged();
            emit m_owner->bottomPaddingChanged();
        } else {
            m_owner->markDirty();
        }
    }
}

void Border::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    emit colorChanged();
    if (m_owner) {
        m_owner->markDirty();
    }
}

} // namespace qmltui
