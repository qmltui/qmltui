#include "MouseArea.h"

namespace qtui {

MouseArea::MouseArea(QObject *parent) : Item(parent) {
}

MouseArea::~MouseArea() = default;

void MouseArea::setMousePosition(int x, int y) {
    bool changed = false;
    if (m_mouseX != x) {
        m_mouseX = x;
        emit mouseXChanged();
        changed = true;
    }
    if (m_mouseY != y) {
        m_mouseY = y;
        emit mouseYChanged();
        changed = true;
    }
    if (changed) {
        markDirty();
    }
}

} // namespace qtui
