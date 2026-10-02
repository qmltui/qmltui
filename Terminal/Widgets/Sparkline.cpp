#include "Sparkline.h"
#include <ColorMapper.h>
#include <algorithm>
#include <cmath>

namespace qtui {

Sparkline::Sparkline(QObject *parent) : Item(parent) {
    setImplicitWidth(20);
    setImplicitHeight(1);
}

void Sparkline::setValues(const QVariant &values) {
    m_valuesVariant = values;
    m_values.clear();

    if (values.userType() == QMetaType::fromType<QJSValue>().id()) {
        m_values = values.value<QJSValue>().toVariant().toList();
    } else if (values.canConvert<QVariantList>()) {
        m_values = values.toList();
    }
    emit valuesChanged();
    markDirty();
}

void Sparkline::setMinVal(double minVal) {
    if (qFuzzyCompare(m_minVal, minVal)) return;
    m_minVal = minVal;
    emit minValChanged();
    markDirty();
}

void Sparkline::setMaxVal(double maxVal) {
    if (qFuzzyCompare(m_maxVal, maxVal)) return;
    m_maxVal = maxVal;
    emit maxValChanged();
    markDirty();
}

void Sparkline::setColor(const QString &color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFF00FFFF);
    emit colorChanged();
    markDirty();
}

void Sparkline::setBgColor(const QString &color) {
    if (m_bgColor == color) return;
    m_bgColor = color;
    m_resolvedBgColor = ColorMapper::fromString(m_bgColor, 0x00000000);
    emit bgColorChanged();
    markDirty();
}

void Sparkline::setBorderStyle(const QString &style) {
    if (m_borderStyle == style) return;
    m_borderStyle = style;
    emit borderStyleChanged();
    markDirty();
}

void Sparkline::setBorderColor(const QString &color) {
    if (m_borderColor == color) return;
    m_borderColor = color;
    m_resolvedBorderColor = ColorMapper::fromString(m_borderColor, 0xFF808080);
    emit borderColorChanged();
    markDirty();
}

void Sparkline::render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) {
    if (!m_visible || m_values.isEmpty()) return;

    Item::render(buffer, absX, absY, clipRect);

    int availW = width();
    int availH = height();
    int startX = absX;
    int startY = absY;

    if (m_borderStyle != "none") {
        buffer.drawBox(absX, absY, availW, availH, m_resolvedBorderColor, 0x00000000);
        startX += 1;
        startY += 1;
        availW -= 2;
        availH -= 2;
    }

    if (availW <= 0 || availH <= 0) return;

    // Determine min/max range
    double computedMin = m_minVal;
    double computedMax = m_maxVal;

    if (qFuzzyIsNull(m_maxVal)) {
        computedMin = m_values[0].toDouble();
        computedMax = m_values[0].toDouble();
        for (const QVariant &v : m_values) {
            double d = v.toDouble();
            if (d < computedMin) computedMin = d;
            if (d > computedMax) computedMax = d;
        }
        if (qFuzzyCompare(computedMin, computedMax)) {
            computedMax = computedMin + 1.0;
        }
    }

    double range = computedMax - computedMin;
    if (range <= 0.0) range = 1.0;

    // Unicode block level characters from   to █
    static const char* blockGlyphs[] = {
        " ",          // 0: lowest
        "\xE2\x96\x81", // 1: ▂
        "\xE2\x96\x82", // 2: ▃
        "\xE2\x96\x83", // 3: ▄
        "\xE2\x96\x84", // 4: ▅
        "\xE2\x96\x85", // 5: ▆
        "\xE2\x96\x86", // 6: ▇
        "\xE2\x96\x88"  // 7: █
    };

    int count = std::min(availW, static_cast<int>(m_values.size()));
    int startIndex = static_cast<int>(m_values.size()) - count;

    for (int dx = 0; dx < count; ++dx) {
        int cx = startX + dx;
        int cy = startY + (availH - 1);

        if (clipRect.contains(cx, cy)) {
            double val = m_values[startIndex + dx].toDouble();
            double pct = std::clamp((val - computedMin) / range, 0.0, 1.0);
            int level = static_cast<int>(pct * 7.0);
            level = std::clamp(level, 0, 7);

            Cell& cell = buffer.cellAt(cx, cy);
            cell.fgColor = m_resolvedColor;
            cell.bgColor = m_resolvedBgColor;
            cell.character.fill('\0');
            const char* glyph = blockGlyphs[level];
            for (int k = 0; k < 4 && glyph[k] != '\0'; ++k) {
                cell.character[k] = glyph[k];
            }
        }
    }
}

} // namespace qtui
