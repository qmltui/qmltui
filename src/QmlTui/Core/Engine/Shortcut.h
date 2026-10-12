#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QQmlEngine>
#include <QList>

namespace qmltui {

class QMLTUI_CORE_EXPORT Shortcut : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int sequence READ sequence WRITE setSequence NOTIFY sequenceChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit Shortcut(QObject *parent = nullptr);
    ~Shortcut() override;

    int sequence() const { return m_sequence; }
    void setSequence(int sequence);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

signals:
    void sequenceChanged();
    void enabledChanged();
    void activated();

private:
    int m_sequence = 0;
    bool m_enabled = true;
};

const QList<Shortcut*>& activeShortcuts();
void registerShortcut(Shortcut* shortcut);
void unregisterShortcut(Shortcut* shortcut);

} // namespace qmltui
