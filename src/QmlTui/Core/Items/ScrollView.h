#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>

namespace qmltui {

// Clips child items to its padded viewport, shifts them vertically by
// `contentY`, and paints an optional single-column scrollbar overlay on the right.
class QMLTUI_CORE_EXPORT ScrollView : public Item {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ScrollView)
    QML_ELEMENT

    Q_PROPERTY(int contentWidth READ contentWidth WRITE setContentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(
        int contentHeight READ contentHeight WRITE setContentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(int contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
    Q_PROPERTY(int contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
    Q_PROPERTY(bool scrollBarVisible READ scrollBarVisible WRITE setScrollBarVisible NOTIFY
                   scrollBarVisibleChanged)

public:
    explicit ScrollView(QObject* parent = nullptr);
    ~ScrollView() override;

    int contentWidth() const { return m_contentWidth; }
    void setContentWidth(int w);

    int contentHeight() const { return m_contentHeight; }
    void setContentHeight(int h);

    int contentX() const { return m_contentX; }
    void setContentX(int x);

    int contentY() const { return m_contentY; }
    void setContentY(int y);

    bool scrollBarVisible() const { return m_scrollBarVisible; }
    void setScrollBarVisible(bool visible);

    // Explicit `contentWidth` / `contentHeight` if positive, otherwise the bounding right / bottom
    // of visible children.
    int effectiveContentWidth() const;
    int effectiveContentHeight() const;

    QPoint contentOffset() const override { return QPoint(m_contentX, m_contentY); }

    void keyPressEvent(class KeyEvent* event) override;
    void mousePressEvent(class MouseEvent* event) override;
    void wheelEvent(class MouseEvent* event) override;

    // Adjusts `contentX` and `contentY` minimally so `childItem`'s span is within the viewport.
    void ensureChildVisible(Item* childItem);

signals:
    void contentWidthChanged();
    void contentHeightChanged();
    void contentXChanged();
    void contentYChanged();
    void scrollBarVisibleChanged();

private:
    void paintOverlay(TextGridBuffer& buffer, int absX, int absY,
                      const ClipRect& clipRect) const override;
    ClipRect clipRectForChildren(int absX, int absY, const ClipRect& clipRect) const override;

    int m_contentWidth = 0;
    int m_contentHeight = 0;
    int m_contentX = 0;
    int m_contentY = 0;
    bool m_scrollBarVisible = true;
};

} // namespace qmltui
