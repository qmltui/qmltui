#pragma once

#include "Item.h"
#include "../engine/MouseEvent.h"

namespace qtui {

class MouseArea : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int mouseX READ mouseX NOTIFY mouseXChanged)
    Q_PROPERTY(int mouseY READ mouseY NOTIFY mouseYChanged)
    Q_PROPERTY(bool hoverEnabled READ hoverEnabled WRITE setHoverEnabled NOTIFY hoverEnabledChanged)

public:
    explicit MouseArea(QObject *parent = nullptr);
    ~MouseArea() override;

    int mouseX() const { return m_mouseX; }
    int mouseY() const { return m_mouseY; }
    bool hoverEnabled() const { return m_hoverEnabled; }
    void setHoverEnabled(bool enabled);

    void setMousePosition(int x, int y);

    void mousePressEvent(MouseEvent *event) override;
    void mouseReleaseEvent(MouseEvent *event) override;
    void mouseMoveEvent(MouseEvent *event) override;

signals:
    void mouseXChanged();
    void mouseYChanged();
    void hoverEnabledChanged();
    void pressed(qtui::MouseEvent *event);
    void released(qtui::MouseEvent *event);
    void clicked(qtui::MouseEvent *event);

private:
    void updateMotionRegistration();

    int m_mouseX = 0;
    int m_mouseY = 0;
    bool m_hoverEnabled = false;
    QPointer<Terminal> m_motionTerminal;
};

} // namespace qtui
