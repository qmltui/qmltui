#include <Qamui/Core/items/Border.h>
#include <Qamui/Core/items/Item.h>

namespace qamui {

Border::Border(Item* owner) : QObject(owner), m_owner(owner) {
}

Border::~Border() = default;

void Border::setStyle(const QString& style) {
    if (m_style == style) return;
    m_style = style;
    emit styleChanged();
    if (m_owner) {
        m_owner->markDirty();
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

} // namespace qamui
