#include "MouseArea.h"
#include "Terminal.h"

namespace qtui {

MouseArea::MouseArea(QObject *parent) : Item(parent) {
    connect(this, &Item::parentChanged, this, &MouseArea::updateMotionRegistration);
}

MouseArea::~MouseArea() {
    if (m_motionTerminal)
        m_motionTerminal->setMouseMotionRequested(this, false);
}

void MouseArea::setHoverEnabled(bool enabled) {
    if (m_hoverEnabled == enabled) return;
    m_hoverEnabled = enabled;
    updateMotionRegistration();
    emit hoverEnabledChanged();
}

void MouseArea::updateMotionRegistration() {
    Terminal* requestedTerminal = m_hoverEnabled ? terminal() : nullptr;
    if (m_motionTerminal == requestedTerminal) return;
    if (m_motionTerminal)
        m_motionTerminal->setMouseMotionRequested(this, false);
    m_motionTerminal = requestedTerminal;
    if (m_motionTerminal)
        m_motionTerminal->setMouseMotionRequested(this, true);
}

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
