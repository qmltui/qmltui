#include "ScrollView.h"
#include "../engine/KeyEvent.h"
#include "../engine/MouseEvent.h"
#include "../core/ColorMapper.h"
#include <algorithm>

namespace qtui {

ScrollView::ScrollView(QObject *parent) : Item(parent) {
    setFocusable(true);
    setClip(true);
}

ScrollView::~ScrollView() = default;

void ScrollView::setContentHeight(int h) {
    if (m_contentHeight == h) return;
    m_contentHeight = h;
    emit contentHeightChanged();
    
    // Clamp contentY
    int viewportH = height() - topPadding() - bottomPadding();
    int maxScroll = std::max(0, m_contentHeight - viewportH);
    if (m_contentY > maxScroll) {
        setContentY(maxScroll);
    }
    markDirty();
}

void ScrollView::setContentY(int y) {
    int viewportH = height() - topPadding() - bottomPadding();
    int maxScroll = std::max(0, m_contentHeight - viewportH);
    int clamped = std::clamp(y, 0, maxScroll);
    if (m_contentY == clamped) return;
    m_contentY = clamped;
    emit contentYChanged();
    markDirty();
}

void ScrollView::keyPressEvent(KeyEvent* event) {
    int key = event->key();
    int viewportH = height() - topPadding() - bottomPadding();

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
    }
}

void ScrollView::mousePressEvent(MouseEvent* event) {
    if (event->button() == 64) {
        // Scroll Up
        setContentY(m_contentY - 1);
        event->setAccepted(true);
    } else if (event->button() == 65) {
        // Scroll Down
        setContentY(m_contentY + 1);
        event->setAccepted(true);
    }
}

void ScrollView::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;

    // 1. Resolve anchors
    m_anchors.resolve();

    int w = width();
    int h = height();

    // 2. Draw background (similar to Item::render)
    if ((m_resolvedBgColor >> 24) != 0) {
        for (int dy = 0; dy < h; ++dy) {
            int cy = absY + dy;
            for (int dx = 0; dx < w; ++dx) {
                int cx = absX + dx;
                if (clipRect.contains(cx, cy)) {
                    if (cx >= 0 && cx < buffer.width() && cy >= 0 && cy < buffer.height()) {
                        Cell& cell = buffer.cellAt(cx, cy);
                        cell.bgColor = m_resolvedBgColor;
                    }
                }
            }
        }
    }

    // 3. Draw border if enabled
    if (m_border.style() != "none") {
        uint32_t borderColor = ColorMapper::fromString(m_border.color(), 0xFFFFFFFF);
        const char* tl = "┌"; const char* tr = "┐";
        const char* bl = "└"; const char* br = "┘";
        const char* hz = "─"; const char* vt = "│";

        if (m_border.style() == "double") {
            tl = "╔"; tr = "╗"; bl = "╚"; br = "╝"; hz = "═"; vt = "║";
        } else if (m_border.style() == "ascii") {
            tl = "+"; tr = "+"; bl = "+"; br = "+"; hz = "-"; vt = "|";
        }

        auto setCellChar = [](Cell& cell, const char* utf8Str, uint32_t fg) {
            int len = 0;
            while (utf8Str[len] && len < 3) {
                cell.character[len] = utf8Str[len];
                len++;
            }
            cell.character[len] = '\0';
            cell.fgColor = fg;
        };

        for (int dy = 0; dy < h; ++dy) {
            int cy = absY + dy;
            for (int dx = 0; dx < w; ++dx) {
                int cx = absX + dx;
                if (clipRect.contains(cx, cy)) {
                    if (cx >= 0 && cx < buffer.width() && cy >= 0 && cy < buffer.height()) {
                        bool isBorder = (dx == 0 || dx == w - 1 || dy == 0 || dy == h - 1);
                        if (isBorder) {
                            Cell& cell = buffer.cellAt(cx, cy);
                            if (dx == 0 && dy == 0) setCellChar(cell, tl, borderColor);
                            else if (dx == w - 1 && dy == 0) setCellChar(cell, tr, borderColor);
                            else if (dx == 0 && dy == h - 1) setCellChar(cell, bl, borderColor);
                            else if (dx == w - 1 && dy == h - 1) setCellChar(cell, br, borderColor);
                            else if (dy == 0 || dy == h - 1) setCellChar(cell, hz, borderColor);
                            else if (dx == 0 || dx == w - 1) setCellChar(cell, vt, borderColor);
                        }
                    }
                }
            }
        }
    }

    int lp = leftPadding();
    int rp = rightPadding();
    int tp = topPadding();
    int bp = bottomPadding();
    int viewportH = h - tp - bp;

    // 4. Viewport clipping for children
    ClipRect viewportClip = clipRect.intersected(ClipRect{absX + lp, absY + tp, w - lp - rp, viewportH});

    // 5. Render children offset by -m_contentY
    for (Item* childItem : childItems()) {
        if (childItem) {
            // Check auto-scroll on active focus: if child has focus, make sure it is in view!
            if (childItem->activeFocus()) {
                int childY = childItem->y();
                int childH = childItem->height();
                if (childY < m_contentY) {
                    const_cast<ScrollView*>(this)->setContentY(childY);
                } else if (childY + childH > m_contentY + viewportH) {
                    const_cast<ScrollView*>(this)->setContentY(childY + childH - viewportH);
                }
            }

            childItem->render(buffer, absX + childItem->x(), absY + childItem->y() - m_contentY, viewportClip);
        }
    }

    // 6. Draw vertical scrollbar if content exceeds viewport
    if (m_contentHeight > viewportH) {
        int sbH = std::max(1, (viewportH * viewportH) / m_contentHeight);
        int maxScroll = m_contentHeight - viewportH;
        int sbPos = (m_contentY * (viewportH - sbH)) / std::max(1, maxScroll);
        int sbX = absX + w - rp; // just inside border / right padding

        for (int dy = 0; dy < viewportH; ++dy) {
            int cy = absY + tp + dy;
            if (clipRect.contains(sbX, cy)) {
                if (sbX >= 0 && sbX < buffer.width() && cy >= 0 && cy < buffer.height()) {
                    Cell& cell = buffer.cellAt(sbX, cy);
                    cell.fgColor = 0xFF888888;
                    if (dy >= sbPos && dy < sbPos + sbH) {
                        cell.character[0] = '\xE2'; // Solid block thumb: █ (\u2588)
                        cell.character[1] = '\x96';
                        cell.character[2] = '\x88';
                        cell.character[3] = '\0';
                    } else {
                        cell.character[0] = '\xE2'; // Light shade track: ░ (\u2591)
                        cell.character[1] = '\x96';
                        cell.character[2] = '\x91';
                        cell.character[3] = '\0';
                    }
                }
            }
        }
    }
}

} // namespace qtui
