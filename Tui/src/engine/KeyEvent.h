#pragma once

#include <QObject>
#include <QQmlEngine>

namespace qtui {

class KeyEvent : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("KeyEvent is passed by the engine")

    Q_PROPERTY(int key READ key CONSTANT)
    Q_PROPERTY(QString text READ text CONSTANT)
    Q_PROPERTY(int modifiers READ modifiers CONSTANT)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted NOTIFY acceptedChanged)

public:
    KeyEvent(int key, const QString &text, int modifiers = 0, QObject *parent = nullptr)
        : QObject(parent), m_key(key), m_text(text), m_modifiers(modifiers), m_accepted(false) {}

    int key() const { return m_key; }
    QString text() const { return m_text; }
    int modifiers() const { return m_modifiers; }
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
    int m_key;
    QString m_text;
    int m_modifiers;
    bool m_accepted;
};

} // namespace qtui
