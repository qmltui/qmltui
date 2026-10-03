#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QPointer>
#include <QTimer>

namespace qtui { class Item; class Terminal; class Popup; class Text; }

namespace qtui::controls {

class ToolTip : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ToolTip is only available as an attached property")
    QML_ATTACHED(ToolTip)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged)
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout NOTIFY timeoutChanged)
    Q_PROPERTY(qreal z READ z WRITE setZ NOTIFY zChanged)

public:
    explicit ToolTip(QObject *target);
    ~ToolTip() override;
    static ToolTip *qmlAttachedProperties(QObject *object) { return new ToolTip(object); }

    QString text() const { return m_text; }
    void setText(const QString &text);
    bool visible() const;
    void setVisible(bool visible);
    int delay() const { return m_delay; }
    void setDelay(int delay);
    int timeout() const { return m_timeout; }
    void setTimeout(int timeout);
    qreal z() const { return m_z; }
    void setZ(qreal z);

signals:
    void textChanged();
    void visibleChanged();
    void delayChanged();
    void timeoutChanged();
    void zChanged();

private:
    void bindTerminal();
    void updateAutomaticVisibility();
    void showNow();
    void hide();
    bool targetIsActive() const;

    QPointer<qtui::Item> m_target;
    QPointer<qtui::Terminal> m_terminal;
    QPointer<qtui::Popup> m_popup;
    QPointer<qtui::Text> m_label;
    QString m_text;
    QTimer m_delayTimer;
    QTimer m_timeoutTimer;
    int m_delay = 700;
    int m_timeout = 5000;
    qreal m_z = 1000;
    bool m_explicitVisible = false;
};

} // namespace qtui::controls

QML_DECLARE_TYPEINFO(qtui::controls::ToolTip, QML_HAS_ATTACHED_PROPERTIES)
