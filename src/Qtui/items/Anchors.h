#pragma once

#include <QtuiExport.h>

#include <QObject>
#include <QPointer>
#include <QQmlEngine>

namespace qtui {

class Item;
class AnchorLine;

class QTUI_CORE_EXPORT Anchors : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Anchors is only available as a grouped property of Item")

    Q_PROPERTY(QObject* fill READ fill WRITE setFill NOTIFY fillChanged)
    Q_PROPERTY(QObject* centerIn READ centerIn WRITE setCenterIn NOTIFY centerInChanged)
    Q_PROPERTY(qtui::AnchorLine* left READ left WRITE setLeft NOTIFY leftChanged)
    Q_PROPERTY(qtui::AnchorLine* right READ right WRITE setRight NOTIFY rightChanged)
    Q_PROPERTY(qtui::AnchorLine* top READ top WRITE setTop NOTIFY topChanged)
    Q_PROPERTY(qtui::AnchorLine* bottom READ bottom WRITE setBottom NOTIFY bottomChanged)
    Q_PROPERTY(qtui::AnchorLine* horizontalCenter READ horizontalCenter WRITE setHorizontalCenter NOTIFY horizontalCenterChanged)
    Q_PROPERTY(qtui::AnchorLine* verticalCenter READ verticalCenter WRITE setVerticalCenter NOTIFY verticalCenterChanged)
    Q_PROPERTY(int margins READ margins WRITE setMargins NOTIFY marginsChanged)
    Q_PROPERTY(int leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged)
    Q_PROPERTY(int rightMargin READ rightMargin WRITE setRightMargin NOTIFY rightMarginChanged)
    Q_PROPERTY(int topMargin READ topMargin WRITE setTopMargin NOTIFY topMarginChanged)
    Q_PROPERTY(int bottomMargin READ bottomMargin WRITE setBottomMargin NOTIFY bottomMarginChanged)

public:
    explicit Anchors(Item* owner);
    ~Anchors() override;

    QObject* fill() const;
    void setFill(QObject* target);

    QObject* centerIn() const;
    void setCenterIn(QObject* target);

    AnchorLine* left() const;
    void setLeft(AnchorLine* target);

    AnchorLine* right() const;
    void setRight(AnchorLine* target);

    AnchorLine* top() const;
    void setTop(AnchorLine* target);

    AnchorLine* bottom() const;
    void setBottom(AnchorLine* target);

    AnchorLine* horizontalCenter() const;
    void setHorizontalCenter(AnchorLine* target);

    AnchorLine* verticalCenter() const;
    void setVerticalCenter(AnchorLine* target);

    int margins() const { return m_margins; }
    void setMargins(int m);

    int leftMargin() const;
    void setLeftMargin(int m);

    int rightMargin() const;
    void setRightMargin(int m);

    int topMargin() const;
    void setTopMargin(int m);

    int bottomMargin() const;
    void setBottomMargin(int m);

    void resolve();

signals:
    void fillChanged();
    void centerInChanged();
    void leftChanged();
    void rightChanged();
    void topChanged();
    void bottomChanged();
    void horizontalCenterChanged();
    void verticalCenterChanged();
    void marginsChanged();
    void leftMarginChanged();
    void rightMarginChanged();
    void topMarginChanged();
    void bottomMarginChanged();

private:
    bool hasAnchors() const;
    // Owner geometry signals are connected on first use so items that never
    // anchor do not pay for a resolve() call on every size change.
    void ensureOwnerConnections();

    Item* m_owner;
    bool m_ownerConnected = false;
    QPointer<Item> m_fillTarget;
    QPointer<Item> m_centerInTarget;
    QPointer<AnchorLine> m_left;
    QPointer<AnchorLine> m_right;
    QPointer<AnchorLine> m_top;
    QPointer<AnchorLine> m_bottom;
    QPointer<AnchorLine> m_horizontalCenter;
    QPointer<AnchorLine> m_verticalCenter;
    int m_margins = 0;
    int m_leftMargin = -1;
    int m_rightMargin = -1;
    int m_topMargin = -1;
    int m_bottomMargin = -1;
    bool m_resolving = false;
};

} // namespace qtui
