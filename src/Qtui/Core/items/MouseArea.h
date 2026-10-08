#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>
#include <Qtui/Core/engine/MouseEvent.h>

namespace qtui {

class QTUI_CORE_EXPORT MouseArea : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int mouseX READ mouseX NOTIFY mouseXChanged)
    Q_PROPERTY(int mouseY READ mouseY NOTIFY mouseYChanged)
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
    bool hoverEnabled() const { return m_hoverEnabled; }
    void setHoverEnabled(bool enabled);
    bool containsMouse() const { return m_containsMouse; }

    void setMousePosition(int x, int y);

    void mousePressEvent(MouseEvent *event) override;
    void mouseReleaseEvent(MouseEvent *event) override;
    void mouseMoveEvent(MouseEvent *event) override;

signals:
    void mouseXChanged();
    void mouseYChanged();
    void hoverEnabledChanged();
    void containsMouseChanged();
    void entered();
    void exited();
    void pressed(qtui::MouseEvent *event);
    void released(qtui::MouseEvent *event);
    void clicked(qtui::MouseEvent *event);

private:
    void updateMotionRegistration();
    void updateContainsMouse();

    int m_mouseX = 0;
    int m_mouseY = 0;
    bool m_hoverEnabled = false;
    bool m_containsMouse = false;
    QPointer<Terminal> m_motionTerminal;
    QMetaObject::Connection m_hoverConnection;
};

} // namespace qtui
