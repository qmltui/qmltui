#include <Qtui/Core/items/MouseArea.h>
#include <Qtui/Core/items/Terminal.h>

namespace qtui {

MouseArea::MouseArea(QObject *parent) : Item(parent) {
    connect(this, &Item::parentChanged, this, &MouseArea::updateMotionRegistration);
    connect(this, &Item::visibleChanged, this, &MouseArea::updateContainsMouse);
    connect(this, &Item::enabledChanged, this, &MouseArea::updateContainsMouse);
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
    if (m_motionTerminal) {
        m_motionTerminal->setMouseMotionRequested(this, false);
        disconnect(m_hoverConnection);
    }
    m_motionTerminal = requestedTerminal;
    if (m_motionTerminal) {
        m_motionTerminal->setMouseMotionRequested(this, true);
        m_hoverConnection = connect(m_motionTerminal, &Terminal::hoveredItemChanged,
                                    this, &MouseArea::updateContainsMouse);
    }
    updateContainsMouse();
}

// The router hit-tests the deepest item under the pointer and publishes it as
// Terminal::hoveredItem; this area contains the mouse when that item is the
// area itself or one of its descendants.
void MouseArea::updateContainsMouse() {
    bool inside = false;
    if (m_hoverEnabled && m_motionTerminal && isEffectivelyVisible() && isEffectivelyEnabled()) {
        for (Item* current = m_motionTerminal->hoveredItem(); current; current = current->parentItem()) {
            if (current == this) {
                inside = true;
                break;
            }
        }
    }
    if (m_containsMouse == inside) return;
    m_containsMouse = inside;
    emit containsMouseChanged();
    if (inside) emit entered();
    else emit exited();
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

void MouseArea::mousePressEvent(MouseEvent *event) {
    if (event->button() < 0 || event->button() > 2) {
        return;
    }
    setMousePosition(event->x(), event->y());
    emit pressed(event);
    event->setAccepted(true);
}

void MouseArea::mouseReleaseEvent(MouseEvent *event) {
    if (event->button() < 0 || event->button() > 2) {
        return;
    }
    setMousePosition(event->x(), event->y());
    emit released(event);
    if (event->x() >= 0 && event->x() < width() &&
        event->y() >= 0 && event->y() < height()) {
        emit clicked(event);
    }
    event->setAccepted(true);
}

void MouseArea::mouseMoveEvent(MouseEvent *event) {
    const bool buttonMotion = event->isMotion() && ((event->button() & 3) != 3);
    if (!event->isMotion() || buttonMotion || m_hoverEnabled) {
        setMousePosition(event->x(), event->y());
    }
}

} // namespace qtui
