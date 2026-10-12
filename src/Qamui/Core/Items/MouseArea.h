#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>

#include <Qamui/Core/Items/Item.h>
#include <Qamui/Core/Engine/MouseEvent.h>

namespace qamui {

class QAMUI_CORE_EXPORT MouseArea : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int mouseX READ mouseX NOTIFY mouseXChanged)
    Q_PROPERTY(int mouseY READ mouseY NOTIFY mouseYChanged)
    // The buttons this area reacts to, Qt Quick style: a press with another
    // button is left unaccepted, so an area behind can take it.
    Q_PROPERTY(Qt::MouseButtons acceptedButtons READ acceptedButtons WRITE setAcceptedButtons NOTIFY acceptedButtonsChanged)
    // True while an accepted button is held after pressing this area. Works
    // on every terminal; unlike containsMouse it needs no motion reporting.
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)
    Q_PROPERTY(Qt::MouseButtons pressedButtons READ pressedButtons NOTIFY pressedButtonsChanged)
    // pressed while the pointer is still inside the area: button-held motion
    // is reported in the default mouse mode, so this tracks a drag out and
    // back without hoverEnabled.
    Q_PROPERTY(bool containsPress READ containsPress NOTIFY containsPressChanged)
    Q_PROPERTY(bool hoverEnabled READ hoverEnabled WRITE setHoverEnabled NOTIFY hoverEnabledChanged)
    // True while the pointer is over this area (or a descendant) and
    // hoverEnabled is set. Hover needs the terminal's any-motion mouse
    // reporting, which the engine turns on while any area asks for it;
    // terminals without it never report hover, so treat it as decoration.
    Q_PROPERTY(bool containsMouse READ containsMouse NOTIFY containsMouseChanged)

public:
    explicit MouseArea(QObject *parent = nullptr);
    ~MouseArea() override;

    int mouseX() const { return m_mouseX; }
    int mouseY() const { return m_mouseY; }
    Qt::MouseButtons acceptedButtons() const { return m_acceptedButtons; }
    void setAcceptedButtons(Qt::MouseButtons buttons);
    bool isPressed() const { return m_pressedButtons != Qt::NoButton; }
    Qt::MouseButtons pressedButtons() const { return m_pressedButtons; }
    bool containsPress() const { return m_containsPress; }
    bool hoverEnabled() const { return m_hoverEnabled; }
    void setHoverEnabled(bool enabled);
    bool containsMouse() const { return m_containsMouse; }

    void setMousePosition(int x, int y);

    void mousePressEvent(MouseEvent *event) override;
    void mouseReleaseEvent(MouseEvent *event) override;
    void mouseMoveEvent(MouseEvent *event) override;
    void wheelEvent(MouseEvent *event) override;

signals:
    void mouseXChanged();
    void mouseYChanged();
    void acceptedButtonsChanged();
    void pressedChanged();
    void pressedButtonsChanged();
    void containsPressChanged();
    void hoverEnabledChanged();
    void containsMouseChanged();
    void entered();
    void exited();
    // The signal shares its name with the bool property, as in Qt Quick:
    // onPressed binds here, `area.pressed` reads the property.
    void pressed(qamui::MouseEvent *event);
    void released(qamui::MouseEvent *event);
    void clicked(qamui::MouseEvent *event);
    // Emitted only while a handler is connected; otherwise the event stays
    // unaccepted so a ScrollView behind keeps scrolling.
    void wheel(qamui::MouseEvent *event);

private:
    void updateMotionRegistration();
    void updateContainsMouse();
    void setPressedButtons(Qt::MouseButtons buttons);
    void updateContainsPress(bool inside);
    // A hidden or disabled area gets no release event: end the press.
    void resetPressWhenInert();

    int m_mouseX = 0;
    int m_mouseY = 0;
    Qt::MouseButtons m_acceptedButtons = Qt::LeftButton;
    Qt::MouseButtons m_pressedButtons = Qt::NoButton;
    bool m_containsPress = false;
    bool m_hoverEnabled = false;
    bool m_containsMouse = false;
    QPointer<Terminal> m_motionTerminal;
    QMetaObject::Connection m_hoverConnection;
};

} // namespace qamui
