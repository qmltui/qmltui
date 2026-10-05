#pragma once

#include "Item.h"

namespace qtui {

class ScrollView : public Item {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ScrollView)
    QML_ELEMENT

    Q_PROPERTY(int contentHeight READ contentHeight WRITE setContentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(int contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
    Q_PROPERTY(bool scrollBarVisible READ scrollBarVisible WRITE setScrollBarVisible NOTIFY scrollBarVisibleChanged)

public:
    explicit ScrollView(QObject *parent = nullptr);
    ~ScrollView() override;

    int contentHeight() const { return m_contentHeight; }
    void setContentHeight(int h);

    int contentY() const { return m_contentY; }
    void setContentY(int y);

    bool scrollBarVisible() const { return m_scrollBarVisible; }
    void setScrollBarVisible(bool visible);

    int effectiveContentHeight() const;

    QPoint contentOffset() const override { return QPoint(0, m_contentY); }

    void keyPressEvent(class KeyEvent* event) override;
    void mousePressEvent(class MouseEvent* event) override;
    void wheelEvent(class MouseEvent* event) override;
    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

    void ensureChildVisible(Item* childItem);

signals:
    void contentHeightChanged();
    void contentYChanged();
    void scrollBarVisibleChanged();

private:
    void paintOverlay(TextGridBuffer &buffer, int absX, int absY,
                      const ClipRect &clipRect) override;
    ClipRect clipRectForChildren(int absX, int absY,
                                const ClipRect &clipRect) const override;

    int m_contentHeight = 0;
    int m_contentY = 0;
    bool m_scrollBarVisible = true;
};

} // namespace qtui
