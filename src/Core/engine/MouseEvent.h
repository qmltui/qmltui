#pragma once

#include <QObject>
#include <QQmlEngine>

namespace qtui {

class MouseEvent : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("MouseEvent is passed by the engine")

    Q_PROPERTY(int x READ x CONSTANT)
    Q_PROPERTY(int y READ y CONSTANT)
    Q_PROPERTY(int button READ button CONSTANT) // 0: Left, 1: Middle, 2: Right, etc.
    Q_PROPERTY(int modifiers READ modifiers CONSTANT)
    Q_PROPERTY(bool motion READ isMotion CONSTANT)
    Q_PROPERTY(int wheelDeltaX READ wheelDeltaX CONSTANT)
    Q_PROPERTY(int wheelDeltaY READ wheelDeltaY CONSTANT)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted NOTIFY acceptedChanged)

public:
    MouseEvent(int x, int y, int button, int modifiers = 0, bool motion = false,
               int wheelDeltaX = 0, int wheelDeltaY = 0, QObject *parent = nullptr)
        : QObject(parent), m_x(x), m_y(y), m_button(button),
          m_modifiers(modifiers), m_motion(motion),
          m_wheelDeltaX(wheelDeltaX), m_wheelDeltaY(wheelDeltaY),
          m_accepted(false) {}
    MouseEvent(int x, int y, int button, QObject *parent)
        : MouseEvent(x, y, button, 0, false, 0, 0, parent) {}

    int x() const { return m_x; }
    int y() const { return m_y; }
    int button() const { return m_button; }
    int modifiers() const { return m_modifiers; }
    bool isMotion() const { return m_motion; }
    int wheelDeltaX() const { return m_wheelDeltaX; }
    int wheelDeltaY() const { return m_wheelDeltaY; }
    bool isAccepted() const { return m_accepted; }
    void setAccepted(bool accepted) {
        if (m_accepted != accepted) {
            m_accepted = accepted;
            emit acceptedChanged();
        }
    }

signals:
    void acceptedChanged();

private:
    int m_x;
    int m_y;
    int m_button;
    int m_modifiers;
    bool m_motion;
    int m_wheelDeltaX;
    int m_wheelDeltaY;
    bool m_accepted;
};

} // namespace qtui
