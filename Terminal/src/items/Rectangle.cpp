#include "Rectangle.h"
#include "../core/ColorMapper.h"

namespace qtui {

Rectangle::Rectangle(QObject *parent) : Item(parent), m_border(this) {
}

Rectangle::~Rectangle() = default;

void Rectangle::setColor(const QString &color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedBgColor = ColorMapper::fromString(m_color, 0x00000000);
    emit colorChanged();
    markDirty();
}

int Rectangle::leftPadding() const {
    int p = Item::leftPadding();
    if (m_border.style() != "none") p += 1;
    return p;
}

int Rectangle::rightPadding() const {
    int p = Item::rightPadding();
    if (m_border.style() != "none") p += 1;
    return p;
}

int Rectangle::topPadding() const {
    int p = Item::topPadding();
    if (m_border.style() != "none") p += 1;
    return p;
}

int Rectangle::bottomPadding() const {
    int p = Item::bottomPadding();
    if (m_border.style() != "none") p += 1;
    return p;
}

void Rectangle::render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!visible()) return;

    // Resolve anchors first (done by Item::render too, but we need correct width/height now)
    anchors()->resolve();

    int w = width();
    int h = height();

    // Render background if specified and not transparent
    if ((m_resolvedBgColor >> 24) != 0) {
        for (int dy = 0; dy < h; ++dy) {
            int cy = absY + dy;
            for (int dx = 0; dx < w; ++dx) {
                int cx = absX + dx;
                if (clipRect.contains(cx, cy)) {
                    if (cx >= 0 && cx < buffer.width() && cy >= 0 && cy < buffer.height()) {
                        Cell& cell = buffer.cellAt(cx, cy);
                        cell.bgColor = m_resolvedBgColor;
                        cell.character = " ";
                        cell.displayWidth = 1;
                        cell.continuation = false;
                    }
                }
            }
        }
    }

    // Render border if enabled
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
            cell.character = QByteArray(utf8Str);
            cell.displayWidth = 1;
            cell.continuation = false;
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

    // Delegate to base class to render children and layout anchors
    Item::render(buffer, absX, absY, clipRect);
}

} // namespace qtui
