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
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted NOTIFY acceptedChanged)

public:
    MouseEvent(int x, int y, int button, QObject *parent = nullptr)
        : QObject(parent), m_x(x), m_y(y), m_button(button), m_accepted(false) {}

    int x() const { return m_x; }
    int y() const { return m_y; }
    int button() const { return m_button; }
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
    bool m_accepted;
};

} // namespace qtui
