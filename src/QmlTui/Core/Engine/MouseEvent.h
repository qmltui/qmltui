#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QQmlEngine>

namespace qmltui {

// Mouse press, release, motion, or wheel event delivered to Item mouse
// handlers and MouseArea signals. Coordinates (x, y) are in the receiving
// item's local cell space.
class QMLTUI_CORE_EXPORT MouseEvent : public QObject {
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
    // The button as a Qt::MouseButton value, so QML compares against
    // Qt.LeftButton, Qt.MiddleButton and Qt.RightButton. Qt.NoButton for a
    // wheel event and for motion without a button held.
    Q_PROPERTY(Qt::MouseButton button READ button CONSTANT)
    // The terminal's own button code (SGR low bits: 0 left, 1 middle,
    // 2 right, 3 none), for forwarding to another terminal.
    Q_PROPERTY(int nativeButton READ nativeButton CONSTANT)
    Q_PROPERTY(int modifiers READ modifiers CONSTANT)
    Q_PROPERTY(bool motion READ isMotion CONSTANT)
    Q_PROPERTY(int wheelDeltaX READ wheelDeltaX CONSTANT)
    Q_PROPERTY(int wheelDeltaY READ wheelDeltaY CONSTANT)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted NOTIFY acceptedChanged)

public:
    MouseEvent(Type type, int x, int y, int nativeButton, int modifiers = 0,
               int wheelDeltaX = 0, int wheelDeltaY = 0, QObject *parent = nullptr)
        : QObject(parent), m_type(type), m_x(x), m_y(y),
          m_nativeButton(nativeButton),
          m_button(mapButton(type, nativeButton)),
          m_modifiers(modifiers),
          m_wheelDeltaX(wheelDeltaX), m_wheelDeltaY(wheelDeltaY),
          m_accepted(false) {}

    Type type() const { return m_type; }
    int x() const { return m_x; }
    int y() const { return m_y; }
    Qt::MouseButton button() const { return m_button; }
    int nativeButton() const { return m_nativeButton; }
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
    // A wheel event carries no button (its native code reuses the low bits
    // for the direction); motion carries the held button, 3 meaning none.
    static Qt::MouseButton mapButton(Type type, int native) {
        if (type == Type::Wheel || native < 0) return Qt::NoButton;
        switch (native & 3) {
        case 0: return Qt::LeftButton;
        case 1: return Qt::MiddleButton;
        case 2: return Qt::RightButton;
        default: return Qt::NoButton;
        }
    }

    Type m_type;
    int m_x;
    int m_y;
    int m_nativeButton;
    Qt::MouseButton m_button;
    int m_modifiers;
    int m_wheelDeltaX;
    int m_wheelDeltaY;
    bool m_accepted;
};

} // namespace qmltui
