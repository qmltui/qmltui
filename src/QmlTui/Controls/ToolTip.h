#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QmlTui/Core/Items/Border.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QQmlEngine>
#include <QPointer>
namespace qmltui {

class ClockTimer;
class Item;
class Terminal;
class Popup;
class Text;
class Rectangle;

// Attached tooltip overlay (`ToolTip.text`, `ToolTip.visible`, `ToolTip.delay`,
// `ToolTip.timeout`) that automatically opens a non-modal Popup near the target
// item after `delay` ms of hover or active focus, or immediately when `visible`
// is set to true. Supports `color`, `textColor`, `border`, and custom
// `background` / `contentItem` slots.
class QMLTUI_CONTROLS_EXPORT ToolTip : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ToolTip)
    QML_ELEMENT
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qmltui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(qmltui::Border* border READ border CONSTANT)
    Q_PROPERTY(
        qmltui::Item* background READ background WRITE setBackground NOTIFY backgroundChanged)
    Q_PROPERTY(
        qmltui::Item* contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged)
    QML_UNCREATABLE("ToolTip is only available as an attached property")
    QML_ATTACHED(ToolTip)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    // Milliseconds to wait after hover/focus before showing (<= 0 shows immediately).
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged)
    // Milliseconds before an open tooltip auto-hides (<= 0 disables auto-hide).
    Q_PROPERTY(int timeout READ timeout WRITE setTimeout NOTIFY timeoutChanged)
    Q_PROPERTY(qreal z READ z WRITE setZ NOTIFY zChanged)

public:
    explicit ToolTip(QObject* target);
    ~ToolTip() override;
    static ToolTip* qmlAttachedProperties(QObject* object) { return new ToolTip(object); }

    QString text() const { return m_text; }
    void setText(const QString& text);
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

    Border* border() const;

    Item* background() const { return m_background; }
    void setBackground(Item* background);

    Item* contentItem() const { return m_contentItem; }
    void setContentItem(Item* contentItem);

signals:
    void colorChanged();
    void textColorChanged();
    void backgroundChanged();
    void contentItemChanged();
    void textChanged();
    void visibleChanged();
    void delayChanged();
    void timeoutChanged();
    void zChanged();

private:
    Color m_color;
    Color m_textColor;
    void bindTerminal();
    void trackAncestryGeometry();
    void trackBackgroundSignals();
    void trackContentSignals();
    void syncPopupChildren();
    void detachPopupChildren();
    void updateAutomaticVisibility();
    void updatePosition();
    void reposition();
    void showNow();
    void updateAppearance();
    void hide();
    bool targetIsActive() const;
    bool hasContent() const;

    QPointer<qmltui::Item> m_target;
    QPointer<qmltui::Terminal> m_terminal;
    QList<QMetaObject::Connection> m_geometryConnections;
    QList<QMetaObject::Connection> m_backgroundConnections;
    QList<QMetaObject::Connection> m_contentConnections;
    QPointer<qmltui::Popup> m_popup;
    QPointer<qmltui::Rectangle> m_defaultBackground;
    QPointer<qmltui::Text> m_defaultLabel;
    QPointer<qmltui::Item> m_background;
    QPointer<qmltui::Item> m_contentItem;
    QString m_text;
    // On the QmlTui Clock so delay and timeout are steppable in tests.
    ClockTimer* m_delayTimer = nullptr;
    ClockTimer* m_timeoutTimer = nullptr;
    int m_delay = 700;
    int m_timeout = 5000;
    qreal m_z = 1000;
    bool m_explicitVisible = false;
};

} // namespace qmltui

QML_DECLARE_TYPEINFO(qmltui::ToolTip, QML_HAS_ATTACHED_PROPERTIES)
