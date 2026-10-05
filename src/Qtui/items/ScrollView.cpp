#include "ScrollView.h"
#include "../engine/KeyEvent.h"
#include "../engine/MouseEvent.h"
#include "../primitives/ColorMapper.h"
#include <algorithm>

namespace qtui {

ScrollView::ScrollView(QObject *parent) : Item(parent) {
    setFocusable(true);
    setClip(true);
}

ScrollView::~ScrollView() = default;

int ScrollView::effectiveContentHeight() const {
    if (m_contentHeight > 0) return m_contentHeight;
    int h = 0;
    for (Item* childItem : childItems()) {
        if (childItem && childItem->visible()) {
            h = std::max(h, childItem->y() + childItem->height());
        }
    }
    return h;
}

void ScrollView::setContentHeight(int h) {
    if (m_contentHeight == h) return;
    m_contentHeight = h;
    emit contentHeightChanged();
    
    // Clamp contentY
    int viewportH = height() - topPadding() - bottomPadding();
    int effectiveH = effectiveContentHeight();
    int maxScroll = std::max(0, effectiveH - viewportH);
    if (m_contentY > maxScroll) {
        setContentY(maxScroll);
    }
    markDirty();
}

void ScrollView::setContentY(int y) {
    int viewportH = height() - topPadding() - bottomPadding();
    int effectiveH = effectiveContentHeight();
    int maxScroll = std::max(0, effectiveH - viewportH);
    int clamped = std::clamp(y, 0, maxScroll);
    if (m_contentY == clamped) return;
    m_contentY = clamped;
    emit contentYChanged();
    markDirty();
}

void ScrollView::setScrollBarVisible(bool visible) {
    if (m_scrollBarVisible == visible) return;
    m_scrollBarVisible = visible;
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

constexpr int MouseWheelUp = 64;
constexpr int MouseWheelDown = 65;

void ScrollView::mousePressEvent(MouseEvent* event) {
    if (focusable() && !focus()) {
        setFocus(true);
    }
    if (event->button() == MouseWheelUp) {
        // Scroll Up
        setContentY(m_contentY - 1);
        event->setAccepted(true);
    } else if (event->button() == MouseWheelDown) {
        // Scroll Down
        setContentY(m_contentY + 1);
        event->setAccepted(true);
    }
}

void ScrollView::wheelEvent(MouseEvent* event) {
    if (event->wheelDeltaY() == 0)
        return;
    if (focusable() && !focus())
        setFocus(true);
    setContentY(m_contentY - event->wheelDeltaY());
    event->setAccepted(true);
}

void ScrollView::ensureChildVisible(Item* childItem) {
    if (!childItem) return;
    int viewportH = height() - topPadding() - bottomPadding();
    int childY = childItem->y();
    int childH = childItem->height();
    if (childY < m_contentY) {
        setContentY(childY);
    } else if (childY + childH > m_contentY + viewportH) {
        setContentY(childY + childH - viewportH);
    }
}

void ScrollView::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    Q_UNUSED(buffer);
    Q_UNUSED(absX);
    Q_UNUSED(absY);
    Q_UNUSED(clipRect);
    m_anchors.resolve();
}

void ScrollView::paintOverlay(TextGridBuffer& buffer, int absX, int absY,
                              const ClipRect& clipRect) {
    int w = width();
    int h = height();

    int lp = leftPadding();
    int rp = rightPadding();
    int tp = topPadding();
    int bp = bottomPadding();
    int viewportH = h - tp - bp;
    // Draw vertical scrollbar if content exceeds viewport and scrollBarVisible is true
    if (m_scrollBarVisible) {
        int effectiveH = effectiveContentHeight();
        if (effectiveH > viewportH) {
            int sbH = std::max(1, (viewportH * viewportH) / effectiveH);
            int maxScroll = effectiveH - viewportH;
            int sbPos = (m_contentY * (viewportH - sbH)) / std::max(1, maxScroll);
            int sbX = absX + w - rp; // just inside border / right padding

            for (int dy = 0; dy < viewportH; ++dy) {
                int cy = absY + tp + dy;
                if (clipRect.contains(sbX, cy)) {
                    if (sbX >= 0 && sbX < buffer.width() && cy >= 0 && cy < buffer.height()) {
                        Cell& cell = buffer.cellAt(sbX, cy);
                        cell.fgColor = 0xFF888888;
                        if (dy >= sbPos && dy < sbPos + sbH) {
                            cell.character = "\xE2\x96\x88"; // █
                        } else {
                            cell.character = "\xE2\x96\x91"; // ░
                        }
                    }
                }
            }
        }
    }
}

ClipRect ScrollView::clipRectForChildren(int absX, int absY,
                                        const ClipRect &clipRect) const
{
    const int lp = leftPadding();
    const int rp = rightPadding();
    const int tp = topPadding();
    const int bp = bottomPadding();
    return clipRect.intersected(ClipRect{absX + lp, absY + tp,
                                        width() - lp - rp,
                                        height() - tp - bp});
}

} // namespace qtui
