#include <QmlTui/Core/Items/MouseArea.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QMetaMethod>

namespace qmltui {

MouseArea::MouseArea(QObject* parent) : Item(parent) {
    setItemFlag(ItemFlag::IgnoresParentPadding, true);
    connect(this, &Item::parentChanged, this, &MouseArea::updateMotionRegistration);
    // An ancestor's reparenting (a Repeater inserting its finished delegate)
    // changes the resolved terminal without this item's own parentChanged
    // firing; without this the motion registration is missed for good.
    connect(this, &Item::ancestryChanged, this, &MouseArea::updateMotionRegistration);
    connect(this, &Item::visibleChanged, this, &MouseArea::updateContainsMouse);
    connect(this, &Item::enabledChanged, this, &MouseArea::updateContainsMouse);
    connect(this, &Item::visibleChanged, this, &MouseArea::resetPressWhenInert);
    connect(this, &Item::enabledChanged, this, &MouseArea::resetPressWhenInert);
}

MouseArea::~MouseArea() {
    if (m_motionTerminal) m_motionTerminal->setMouseMotionRequested(this, false);
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
        m_hoverConnection = connect(m_motionTerminal, &Terminal::hoveredItemChanged, this,
                                    &MouseArea::updateContainsMouse);
    }
    updateContainsMouse();
}

// The router hit-tests the deepest item under the pointer and publishes it as
// Terminal::hoveredItem; this area contains the mouse when that item is the
// area itself or one of its descendants.
void MouseArea::updateContainsMouse() {
    bool inside = false;
    if (m_hoverEnabled && m_motionTerminal && isEffectivelyVisible() && isEffectivelyEnabled()) {
        if (Item* hovered = m_motionTerminal->hoveredItem()) inside = hovered->isDescendantOf(this);
    }
    if (m_containsMouse == inside) return;
    m_containsMouse = inside;
    QPointer<MouseArea> self(this);
    emit containsMouseChanged();
    if (!self) return;
    if (inside)
        emit entered();
    else
        emit exited();
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

void MouseArea::setAcceptedButtons(Qt::MouseButtons buttons) {
    if (m_acceptedButtons == buttons) return;
    m_acceptedButtons = buttons;
    emit acceptedButtonsChanged();
}

void MouseArea::setPressedButtons(Qt::MouseButtons buttons) {
    if (m_pressedButtons == buttons) return;
    const bool wasPressed = isPressed();
    m_pressedButtons = buttons;
    emit pressedButtonsChanged();
    if (wasPressed != isPressed()) emit pressedChanged();
}

void MouseArea::updateContainsPress(bool inside) {
    const bool value = isPressed() && inside;
    if (m_containsPress == value) return;
    m_containsPress = value;
    emit containsPressChanged();
}

void MouseArea::resetPressWhenInert() {
    if (isEffectivelyVisible() && isEffectivelyEnabled()) return;
    setPressedButtons(Qt::NoButton);
    updateContainsPress(false);
}

void MouseArea::mousePressEvent(MouseEvent* event) {
    if (!(m_acceptedButtons & event->button())) {
        return; // not ours: the event stays unaccepted for items behind
    }
    setMousePosition(event->x(), event->y());
    setPressedButtons(m_pressedButtons | event->button());
    updateContainsPress(true); // a press is hit-tested, so it is inside
    emit pressed(event);
    event->setAccepted(true);
}

void MouseArea::mouseReleaseEvent(MouseEvent* event) {
    const Qt::MouseButton button = event->button();
    // A legacy release reports no button; it ends whatever press is held.
    const bool endsPress = button == Qt::NoButton ? isPressed() : m_pressedButtons.testFlag(button);
    if (!endsPress && !(m_acceptedButtons & button)) {
        return;
    }
    const int ex = event->x();
    const int ey = event->y();
    QPointer<MouseArea> self(this);
    setMousePosition(ex, ey);
    if (!self) return;
    setPressedButtons(button == Qt::NoButton ? Qt::MouseButtons(Qt::NoButton)
                                             : m_pressedButtons & ~Qt::MouseButtons(button));
    if (!self) return;
    if (!isPressed()) {
        updateContainsPress(false);
        if (!self) return;
    }
    emit released(event);
    if (!self) {
        event->setAccepted(true);
        return;
    }
    if (ex >= 0 && ex < width() && ey >= 0 && ey < height()) {
        emit clicked(event);
    }
    event->setAccepted(true);
}

void MouseArea::mouseMoveEvent(MouseEvent* event) {
    const bool buttonMotion = event->isMotion() && event->button() != Qt::NoButton;
    if (!event->isMotion() || buttonMotion || m_hoverEnabled) {
        setMousePosition(event->x(), event->y());
    }
    if (isPressed()) {
        updateContainsPress(event->x() >= 0 && event->x() < width() && event->y() >= 0 &&
                            event->y() < height());
    }
}

void MouseArea::wheelEvent(MouseEvent* event) {
    static const QMetaMethod wheelSignal = QMetaMethod::fromSignal(&MouseArea::wheel);
    if (!isSignalConnected(wheelSignal)) {
        return; // no handler: a scrolling container behind keeps the event
    }
    setMousePosition(event->x(), event->y());
    // Accepted up front, as in Qt Quick; a handler can set accepted = false
    // to let the event through after inspecting it.
    event->setAccepted(true);
    emit wheel(event);
}

} // namespace qmltui
