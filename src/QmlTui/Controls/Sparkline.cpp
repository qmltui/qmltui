#include <QmlTui/Controls/Sparkline.h>
#include <algorithm>

namespace qmltui {

Sparkline::Sparkline(QObject* parent) : Item(parent) {
    setItemFlag(ItemFlag::BoundedSelfPaint);
    setImplicitWidth(20);
    setImplicitHeight(1);
}

void Sparkline::setValues(const QList<qreal>& values) {
    if (m_values == values) return;
    m_values = values;
    emit valuesChanged();
    markDirty();
}

void Sparkline::setFrom(double from) {
    if (m_from == from) return;
    m_from = from;
    emit fromChanged();
    markDirty();
}

void Sparkline::setTo(double to) {
    if (m_to == to) return;
    m_to = to;
    emit toChanged();
    markDirty();
}

void Sparkline::setFilled(bool filled) {
    if (m_filled == filled) return;
    m_filled = filled;
    emit filledChanged();
    markDirty();
}

void Sparkline::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = Color::paintedArgb32(Color::resolve(m_color, Color::DefaultTag));
    emit colorChanged();
    markDirty();
}

void Sparkline::setBackgroundColor(Color color) {
    if (m_backgroundColor == color) return;
    m_backgroundColor = color;
    m_resolvedBackgroundColor =
        Color::paintedArgb32(Color::resolve(m_backgroundColor, Color::DefaultTag));
    emit backgroundColorChanged();
    markDirty();
}

void Sparkline::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!visible() || m_values.isEmpty()) return;
    const bool transparentBg = m_backgroundColor.isTransparent();
    const bool transparentFg = m_color.isTransparent();
    if (transparentFg && transparentBg) return;

    const int availW = width();
    const int availH = height();
    const int startX = absX;
    const int startY = absY;

    if (availW <= 0 || availH <= 0) return;

    double computedMin = m_from;
    double computedMax = m_to;

    if (m_from >= m_to) {
        computedMin = m_values[0];
        computedMax = m_values[0];
        for (const qreal d : m_values) {
            if (d < computedMin) computedMin = d;
            if (d > computedMax) computedMax = d;
        }
        if (qFuzzyCompare(computedMin + 1.0, computedMax + 1.0)) {
            computedMax = computedMin + 1.0;
        }
    }

    double range = computedMax - computedMin;
    if (range <= 0.0) range = 1.0;

    // Unicode block level characters from ▁ to █ (8 sub-cell levels per row, U+2581 to U+2588)
    static const char* blockGlyphs[] = {
        "\xE2\x96\x81", // 0: ▁
        "\xE2\x96\x82", // 1: ▂
        "\xE2\x96\x83", // 2: ▃
        "\xE2\x96\x84", // 3: ▄
        "\xE2\x96\x85", // 4: ▅
        "\xE2\x96\x86", // 5: ▆
        "\xE2\x96\x87", // 6: ▇
        "\xE2\x96\x88"  // 7: █
    };

    const int valCount = static_cast<int>(m_values.size());
    const int totalLevels = availH * 8;
    const int fillOffset = (std::max)(0, availW - valCount);
    const TextAttribute attrs = !isEffectivelyEnabled() ? TextAttribute::Dim : TextAttribute::None;

    for (int dx = 0; dx < availW; ++dx) {
        if (dx < fillOffset) {
            if (transparentBg) continue;
            for (int ry = 0; ry < availH; ++ry) {
                const int cy = startY + (availH - 1 - ry);
                const int cx = startX + dx;
                if (!clipRect.contains(cx, cy)) continue;
                Cell* targetCell = buffer.tryCellAt(cx, cy);
                if (!targetCell) continue;
                buffer.splitWideGlyphSpan(cx, cy, 1);
                Cell& cell = *targetCell;
                cell.fgColor = m_resolvedColor;
                cell.bgColor = m_resolvedBackgroundColor;
                cell.setCharacter(" ");
                cell.displayWidth = 1;
                cell.continuation = false;
                cell.attributes = attrs;
                cell.linkId = 0;
            }
            continue;
        }

        const double val = m_values[valCount - availW + dx];
        const double pct = std::clamp((val - computedMin) / range, 0.0, 1.0);
        int totalSubCells = static_cast<int>(pct * (totalLevels - 1));
        totalSubCells = std::clamp(totalSubCells, 0, totalLevels - 1);

        const int targetRow = totalSubCells / 8;
        const int targetLevel = totalSubCells % 8;

        for (int ry = 0; ry < availH; ++ry) {
            const int cy = startY + (availH - 1 - ry);
            const int cx = startX + dx;
            if (!clipRect.contains(cx, cy)) continue;

            const char* glyph = nullptr;
            if (availH == 1) {
                if (!transparentFg) glyph = blockGlyphs[targetLevel];
            } else if (ry == targetRow) {
                if (!transparentFg) glyph = blockGlyphs[targetLevel];
            } else if (ry < targetRow && m_filled) {
                if (!transparentFg) glyph = "\xE2\x96\x88";
            }

            if (!glyph && transparentBg) continue;

            Cell* targetCell = buffer.tryCellAt(cx, cy);
            if (!targetCell) continue;
            buffer.splitWideGlyphSpan(cx, cy, 1);
            Cell& cell = *targetCell;
            cell.fgColor = m_resolvedColor;
            if (!transparentBg) cell.bgColor = m_resolvedBackgroundColor;
            cell.setCharacter(glyph ? glyph : " ");
            cell.displayWidth = 1;
            cell.continuation = false;
            cell.attributes = attrs;
            cell.linkId = 0;
        }
    }
}

} // namespace qmltui
