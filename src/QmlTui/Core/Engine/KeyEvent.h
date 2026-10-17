#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QQmlEngine>

namespace qmltui {

// Keyboard or bracketed-paste event delivered to Item::keyPressEvent() and Keys
// attached signal handlers. Setting accepted to true stops propagation up the
// parent chain and prevents default Tab focus navigation.
class QMLTUI_CORE_EXPORT KeyEvent : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("KeyEvent is passed by the engine")

    Q_PROPERTY(int key READ key CONSTANT)
    Q_PROPERTY(QString text READ text CONSTANT)
    Q_PROPERTY(int modifiers READ modifiers CONSTANT)
    // True when text arrived inside a bracketed-paste sequence (?2004h) rather
    // than as an individual keystroke.
    Q_PROPERTY(bool paste READ isPaste CONSTANT)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted NOTIFY acceptedChanged)

public:
    KeyEvent(int key, const QString &text, int modifiers = 0, bool paste = false,
             QObject *parent = nullptr)
        : QObject(parent), m_key(key), m_text(text), m_modifiers(modifiers),
          m_paste(paste), m_accepted(false) {}
    KeyEvent(int key, const QString &text, int modifiers, QObject *parent)
        : KeyEvent(key, text, modifiers, false, parent) {}

    int key() const { return m_key; }
    QString text() const { return m_text; }
    int modifiers() const { return m_modifiers; }
    bool isPaste() const { return m_paste; }
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
    bool m_paste;
    bool m_accepted;
};

} // namespace qmltui
