#pragma once

#include "Item.h"
#include "../engine/MouseEvent.h"

namespace qtui {

class MouseArea : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int mouseX READ mouseX NOTIFY mouseXChanged)
    Q_PROPERTY(int mouseY READ mouseY NOTIFY mouseYChanged)

public:
    explicit MouseArea(QObject *parent = nullptr);
    ~MouseArea() override;

    int mouseX() const { return m_mouseX; }
    int mouseY() const { return m_mouseY; }

    void setMousePosition(int x, int y);

signals:
    void mouseXChanged();
    void mouseYChanged();
    void pressed(qtui::MouseEvent *event);
    void released(qtui::MouseEvent *event);
    void clicked(qtui::MouseEvent *event);

private:
    int m_mouseX = 0;
    int m_mouseY = 0;
};

} // namespace qtui
