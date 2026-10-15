#include <QmlTui/Core/Items/Rectangle.h>
#include <QmlTui/Core/Items/Gradient.h>
#include <QmlTui/Core/Primitives/ColorMapper.h>

namespace qmltui {

Rectangle::Rectangle(QObject* parent) : Item(parent), m_border(this) {}

Rectangle::~Rectangle() = default;

void Rectangle::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedBackgroundColor = ColorMapper::fromString(m_color, Color::TransparentTag);
    emit colorChanged();
    markDirty();
}

void Rectangle::setGradient(Gradient* gradient) {
    if (m_gradient.data() == gradient) return;
    if (m_gradient) {
        disconnect(m_gradient.data(), nullptr, this, nullptr);
    }
    m_gradient = gradient;
    if (m_gradient) {
        connect(m_gradient.data(), &Gradient::updated, this, [this]() { markDirty(); });
        connect(m_gradient.data(), &QObject::destroyed, this, [this]() {
            m_gradient = nullptr;
            emit gradientChanged();
            markDirty();
        });
    }
    emit gradientChanged();
    markDirty();
}

int Rectangle::leftPadding() const {
    int p = Item::leftPadding();
    if (m_border.style() != Border::None) p += 1;
    return p;
}

int Rectangle::rightPadding() const {
    int p = Item::rightPadding();
    if (m_border.style() != Border::None) p += 1;
    return p;
}

int Rectangle::topPadding() const {
    int p = Item::topPadding();
    if (m_border.style() != Border::None) p += 1;
    return p;
}

int Rectangle::bottomPadding() const {
    int p = Item::bottomPadding();
    if (m_border.style() != Border::None) p += 1;
    return p;
}

void Rectangle::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!visible()) return;

    int w = width();
    int h = height();

    const bool hasGradient = (m_gradient && !m_gradient->stopsList().isEmpty());
    // Anything but explicit transparency is painted, including the terminal's
    // own default colour: a surface has to be written in order to occlude what
    // it covers, even when the terminal is the one choosing the colour.
    const bool hasSolidBg = !ColorMapper::isTransparent(m_resolvedBackgroundColor);

    // Render background if specified via gradient or solid color
    if (hasGradient || hasSolidBg) {
        const bool isHorizontal = (m_gradient && m_gradient->orientation() == Gradient::Horizontal);
        const int xStart = std::max({0, absX, clipRect.x});
        const int xEnd = std::min({buffer.width(), absX + w, clipRect.x + clipRect.width});
        const int yStart = std::max({0, absY, clipRect.y});
        const int yEnd = std::min({buffer.height(), absY + h, clipRect.y + clipRect.height});
        for (int cy = yStart; cy < yEnd; ++cy) {
            const int dy = cy - absY;
            for (int cx = xStart; cx < xEnd; ++cx) {
                const int dx = cx - absX;
                Cell& cell = buffer.cellAt(cx, cy);
                if (hasGradient) {
                    qreal t = 0.0;
                    if (isHorizontal) {
                        t = (w > 1) ? static_cast<qreal>(dx) / (w - 1) : 0.0;
                    } else {
                        t = (h > 1) ? static_cast<qreal>(dy) / (h - 1) : 0.0;
                    }
                    cell.bgColor = m_gradient->colorAt(t);
                } else {
                    cell.bgColor = m_resolvedBackgroundColor;
                }
                cell.setCharacter(" ");
                cell.displayWidth = 1;
                cell.continuation = false;
                cell.attributes = TextAttribute::None;
            }
        }
    }

    // Render border if enabled
    if (m_border.style() != Border::None && w > 0 && h > 0) {
        uint32_t borderColor = ColorMapper::fromString(m_border.color(), 0xFFFFFFFF);
        const char* tl = "┌";
        const char* tr = "┐";
        const char* bl = "└";
        const char* br = "┘";
        const char* hz = "─";
        const char* vt = "│";

        switch (m_border.style()) {
        case Border::Double:
            tl = "╔";
            tr = "╗";
            bl = "╚";
            br = "╝";
            hz = "═";
            vt = "║";
            break;
        case Border::Ascii:
            tl = "+";
            tr = "+";
            bl = "+";
            br = "+";
            hz = "-";
            vt = "|";
            break;
        case Border::Rounded:
            tl = "╭";
            tr = "╮";
            bl = "╰";
            br = "╯";
            break;
        case Border::Single:
        case Border::None:
            break;
        }

        // Measure the six glyphs once instead of per border cell.
        const QByteArrayView tlGlyph(tl), trGlyph(tr), blGlyph(bl), brGlyph(br), hzGlyph(hz),
            vtGlyph(vt);
        auto paintBorderCell = [&](int dx, int dy, QByteArrayView glyph) {
            int cx = absX + dx;
            int cy = absY + dy;
            if (clipRect.contains(cx, cy) && cx >= 0 && cx < buffer.width() && cy >= 0 &&
                cy < buffer.height()) {
                Cell& cell = buffer.cellAt(cx, cy);
                cell.setCharacter(glyph);
                cell.displayWidth = 1;
                cell.continuation = false;
                cell.fgColor = borderColor;
                cell.attributes = TextAttribute::None;
            }
        };

        paintBorderCell(0, 0, tlGlyph);
        if (w > 1) paintBorderCell(w - 1, 0, trGlyph);
        if (h > 1) paintBorderCell(0, h - 1, blGlyph);
        if (w > 1 && h > 1) paintBorderCell(w - 1, h - 1, brGlyph);

        for (int dx = 1; dx < w - 1; ++dx) {
            paintBorderCell(dx, 0, hzGlyph);
            if (h > 1) paintBorderCell(dx, h - 1, hzGlyph);
        }
        for (int dy = 1; dy < h - 1; ++dy) {
            paintBorderCell(0, dy, vtGlyph);
            if (w > 1) paintBorderCell(w - 1, dy, vtGlyph);
        }
    }

    // Child traversal is performed by Item after this hook returns.
}

} // namespace qmltui
