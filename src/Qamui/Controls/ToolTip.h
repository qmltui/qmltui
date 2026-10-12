#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>

#include <QObject>
#include <Qamui/Core/Primitives/Color.h>
#include <QQmlEngine>
#include <QPointer>
namespace qamui { class ClockTimer; }

namespace qamui { class Item; class Terminal; class Popup; class Text; class Rectangle; }

namespace qamui::controls {

class QAMUI_CONTROLS_EXPORT ToolTip : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qamui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qamui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
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

    Color textColor() const { return m_textColor; }
    void setTextColor(Color color);

    Color color() const { return m_color; }
    void setColor(Color color);

signals:
    void colorChanged();
    void textColorChanged();
    void textChanged();
    void visibleChanged();
    void delayChanged();
    void timeoutChanged();
    void zChanged();

private:
    Color m_color = "default";
    Color m_textColor = "default";
    void bindTerminal();
    void trackAncestryGeometry();
    void updateAutomaticVisibility();
    void updatePosition();
    void reposition();
    void showNow();
    void updateAppearance();
    void hide();
    bool targetIsActive() const;

    QPointer<qamui::Item> m_target;
    QPointer<qamui::Terminal> m_terminal;
    QList<QMetaObject::Connection> m_geometryConnections;
    QPointer<qamui::Popup> m_popup;
    QPointer<qamui::Rectangle> m_background;
    QPointer<qamui::Text> m_label;
    QString m_text;
    // On the Qamui Clock so delay and timeout are steppable in tests.
    ClockTimer* m_delayTimer = nullptr;
    ClockTimer* m_timeoutTimer = nullptr;
    int m_delay = 700;
    int m_timeout = 5000;
    qreal m_z = 1000;
    bool m_explicitVisible = false;
};

} // namespace qamui::controls

QML_DECLARE_TYPEINFO(qamui::controls::ToolTip, QML_HAS_ATTACHED_PROPERTIES)
