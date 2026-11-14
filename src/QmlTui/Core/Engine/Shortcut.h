#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariant>

namespace qmltui {

// Application-wide keyboard shortcut evaluated by InputRouter before focused
// item key dispatch. Accepts either a portable key chord string (such as
// "Ctrl+S", "Ctrl+Shift+P", or "F5") or an integer Qt::Key / Qt::KeyboardModifiers
// bitmask (such as Qt.ControlModifier | Qt.Key_S).
class QMLTUI_CORE_EXPORT Shortcut : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Shortcut)
    QML_ELEMENT
    Q_PROPERTY(QVariant sequence READ sequence WRITE setSequence NOTIFY sequenceChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit Shortcut(QObject* parent = nullptr);
    ~Shortcut() override;

    QVariant sequence() const { return m_sequence; }
    void setSequence(const QVariant& sequence);
    void setSequence(int sequence);
    void setSequence(QKeyCombination sequence);
    void setSequence(const QString& sequence);

    int keyCombination() const { return m_keyCombination; }

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

signals:
    void sequenceChanged();
    void enabledChanged();
    void activated();

private:
    QVariant m_sequence;
    int m_keyCombination = 0;
    bool m_enabled = true;
};

const QList<Shortcut*>& activeShortcuts();
void registerShortcut(Shortcut* shortcut);
void unregisterShortcut(Shortcut* shortcut);

} // namespace qmltui
