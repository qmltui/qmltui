#pragma once

#include <QObject>
#include <QQmlEngine>

namespace qtui {

class MouseEvent : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("MouseEvent is passed by the engine")

public:
    enum class Type : quint8 {
        Press,
        Release,
        Move,
        Wheel,
    };
    Q_ENUM(Type)

    Q_PROPERTY(Type type READ type CONSTANT)
    Q_PROPERTY(int x READ x CONSTANT)
    Q_PROPERTY(int y READ y CONSTANT)
    Q_PROPERTY(int button READ button CONSTANT) // 0: Left, 1: Middle, 2: Right, etc.
    Q_PROPERTY(int modifiers READ modifiers CONSTANT)
    Q_PROPERTY(bool motion READ isMotion CONSTANT)
    Q_PROPERTY(int wheelDeltaX READ wheelDeltaX CONSTANT)
    Q_PROPERTY(int wheelDeltaY READ wheelDeltaY CONSTANT)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted NOTIFY acceptedChanged)

public:
    MouseEvent(Type type, int x, int y, int button, int modifiers = 0,
               int wheelDeltaX = 0, int wheelDeltaY = 0, QObject *parent = nullptr)
        : QObject(parent), m_type(type), m_x(x), m_y(y), m_button(button),
          m_modifiers(modifiers),
          m_wheelDeltaX(wheelDeltaX), m_wheelDeltaY(wheelDeltaY),
          m_accepted(false) {}

    Type type() const { return m_type; }
    int x() const { return m_x; }
    int y() const { return m_y; }
    int button() const { return m_button; }
    int modifiers() const { return m_modifiers; }
    bool isMotion() const { return m_type == Type::Move; }
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
    Type m_type;
    int m_x;
    int m_y;
    int m_button;
    int m_modifiers;
    int m_wheelDeltaX;
    int m_wheelDeltaY;
    bool m_accepted;
};

} // namespace qtui
