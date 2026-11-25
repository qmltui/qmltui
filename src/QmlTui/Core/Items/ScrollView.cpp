#include <QmlTui/Core/Items/ScrollView.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <algorithm>

namespace qmltui {

ScrollView::ScrollView(QObject* parent) : Item(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setClip(true);
    setItemFlag(ItemFlag::ScrollbarOverhang, true);
    setItemFlag(ItemFlag::BoundedSelfPaint, true);
}

ScrollView::~ScrollView() = default;

int ScrollView::effectiveContentWidth() const {
    if (m_contentWidth > 0) return m_contentWidth;
    int w = 0;
    for (Item* childItem : childItems()) {
        if (childItem && childItem->visible()) {
            w = (std::max)(w, childItem->x() + childItem->width());
        }
    }
    return w;
}

int ScrollView::effectiveContentHeight() const {
    if (m_contentHeight > 0) return m_contentHeight;
    int h = 0;
    for (Item* childItem : childItems()) {
        if (childItem && childItem->visible()) {
            h = (std::max)(h, childItem->y() + childItem->height());
        }
    }
    return h;
}

void ScrollView::setContentWidth(int w) {
    if (m_contentWidth == w) return;
    m_contentWidth = w;
    emit contentWidthChanged();

    const int viewportW = (std::max)(0, width() - leftPadding() - rightPadding());
    const int effectiveW = effectiveContentWidth();
    const int maxScroll = (std::max)(0, effectiveW - viewportW);
    if (m_contentX > maxScroll) {
        setContentX(maxScroll);
    }
    markDirty();
}

void ScrollView::setContentHeight(int h) {
    if (m_contentHeight == h) return;
    m_contentHeight = h;
    emit contentHeightChanged();

    int viewportH = height() - topPadding() - bottomPadding();
    int effectiveH = effectiveContentHeight();
    int maxScroll = (std::max)(0, effectiveH - viewportH);
    if (m_contentY > maxScroll) {
        setContentY(maxScroll);
    }
    markDirty();
}

void ScrollView::setContentX(int x) {
    const int viewportW = (std::max)(0, width() - leftPadding() - rightPadding());
    const int effectiveW = effectiveContentWidth();
    const int maxScroll = (std::max)(0, effectiveW - viewportW);
    const int clamped = std::clamp(x, 0, maxScroll);
    if (m_contentX == clamped) return;
    m_contentX = clamped;
    invalidateSubtreeBounds();
    emit contentXChanged();
    markDirty();
}

void ScrollView::setContentY(int y) {
    int viewportH = height() - topPadding() - bottomPadding();
    int effectiveH = effectiveContentHeight();
    int maxScroll = (std::max)(0, effectiveH - viewportH);
    int clamped = std::clamp(y, 0, maxScroll);
    if (m_contentY == clamped) return;
    m_contentY = clamped;
    invalidateSubtreeBounds();
    emit contentYChanged();
    markDirty();
}

void ScrollView::setScrollBarVisible(bool visible) {
    if (m_scrollBarVisible == visible) return;
    m_scrollBarVisible = visible;
    invalidateSubtreeBounds();
    emit scrollBarVisibleChanged();
    markDirty();
}

void ScrollView::keyPressEvent(KeyEvent* event) {
    int key = event->key();
    int viewportH = height() - topPadding() - bottomPadding();
    int effectiveH = effectiveContentHeight();

    if (key == Qt::Key_Up) {
        setContentY(m_contentY - 1);
        event->setAccepted(true);
    } else if (key == Qt::Key_Down) {
        setContentY(m_contentY + 1);
        event->setAccepted(true);
    } else if (key == Qt::Key_PageUp) {
        setContentY(m_contentY - viewportH);
        event->setAccepted(true);
    } else if (key == Qt::Key_PageDown) {
        setContentY(m_contentY + viewportH);
        event->setAccepted(true);
    } else if (key == Qt::Key_Home) {
        setContentY(0);
        event->setAccepted(true);
    } else if (key == Qt::Key_End) {
        setContentY(effectiveH - viewportH);
        event->setAccepted(true);
    }
}

void ScrollView::mousePressEvent(MouseEvent* event) {
    Q_UNUSED(event);
    if ((focusPolicy() & Qt::ClickFocus) && !focus()) {
        setFocus(true);
    }
}

void ScrollView::wheelEvent(MouseEvent* event) {
    if (event->wheelDeltaY() == 0) return;
    if ((focusPolicy() & (Qt::ClickFocus | Qt::WheelFocus)) && !focus()) setFocus(true);
    setContentY(m_contentY - event->wheelDeltaY());
    event->setAccepted(true);
}

void ScrollView::ensureChildVisible(Item* childItem) {
    if (!childItem) return;
    const int viewportW = (std::max)(0, width() - leftPadding() - rightPadding());
    const int childX = childItem->x() - leftPadding();
    const int childW = childItem->width();
    if (viewportW > 0) {
        if (childX < m_contentX) {
            setContentX(childX);
        } else if (childX + childW > m_contentX + viewportW) {
            setContentX(childX + childW - viewportW);
        }
    }
    int viewportH = height() - topPadding() - bottomPadding();
    int childY = childItem->y() - topPadding();
    int childH = childItem->height();
    if (childY < m_contentY) {
        setContentY(childY);
    } else if (childY + childH > m_contentY + viewportH) {
        setContentY(childY + childH - viewportH);
    }
}

void ScrollView::paintOverlay(TextGridBuffer& buffer, int absX, int absY,
                              const ClipRect& clipRect) const {
    int w = width();
    int h = height();

    int rp = rightPadding();
    int tp = topPadding();
    int bp = bottomPadding();
    int viewportH = h - tp - bp;
    // Draw vertical scrollbar if content exceeds viewport and scrollBarVisible is true
    if (m_scrollBarVisible) {
        int effectiveH = effectiveContentHeight();
        if (effectiveH > viewportH) {
            int sbH = (std::max)(1, (viewportH * viewportH) / effectiveH);
            int maxScroll = effectiveH - viewportH;
            int sbPos = (m_contentY * (viewportH - sbH)) / (std::max)(1, maxScroll);
            int sbX = absX + w - rp; // just inside border / right padding

            for (int dy = 0; dy < viewportH; ++dy) {
                int cy = absY + tp + dy;
                if (clipRect.contains(sbX, cy)) {
                    if (sbX >= 0 && sbX < buffer.width() && cy >= 0 && cy < buffer.height()) {
                        Cell& cell = buffer.cellAt(sbX, cy);
                        cell.fgColor = 0xFF888888;
                        if (dy >= sbPos && dy < sbPos + sbH) {
                            cell.setCharacter("\xE2\x96\x88"); // █
                        } else {
                            cell.setCharacter("\xE2\x96\x91"); // ░
                        }
                    }
                }
            }
        }
    }
}

ClipRect ScrollView::clipRectForChildren(int absX, int absY, const ClipRect& clipRect) const {
    const int lp = leftPadding();
    const int rp = rightPadding();
    const int tp = topPadding();
    const int bp = bottomPadding();
    return clipRect.intersected(
        ClipRect{absX + lp, absY + tp, width() - lp - rp, height() - tp - bp});
}

} // namespace qmltui
