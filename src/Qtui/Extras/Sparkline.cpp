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

void Sparkline::setFilled(bool filled) {
    if (m_filled == filled) return;
    m_filled = filled;
    emit filledChanged();
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

    // Unicode block level characters from   to █ (8 sub-cell levels per row, U+2581 to U+2588)
    static const char* blockGlyphs[] = {
        "\xE2\x96\x81", // 0:  
        "\xE2\x96\x82", // 1: ▂
        "\xE2\x96\x83", // 2: ▃
        "\xE2\x96\x84", // 3: ▄
        "\xE2\x96\x85", // 4: ▅
        "\xE2\x96\x86", // 5: ▆
        "\xE2\x96\x87", // 6: ▇
        "\xE2\x96\x88"  // 7: █
    };

    int valCount = static_cast<int>(m_values.size());
    int totalLevels = availH * 8;

    for (int dx = 0; dx < availW; ++dx) {
        double val = computedMin;
        if (valCount >= availW) {
            int idx = valCount - availW + dx;
            val = m_values[idx].toDouble();
        } else {
            int fillOffset = availW - valCount;
            if (dx >= fillOffset) {
                val = m_values[dx - fillOffset].toDouble();
            } else {
                val = m_values[0].toDouble();
            }
        }

        double pct = std::clamp((val - computedMin) / range, 0.0, 1.0);
        int totalSubCells = static_cast<int>(pct * (totalLevels - 1));
        totalSubCells = std::clamp(totalSubCells, 0, totalLevels - 1);

        int targetRow = totalSubCells / 8;
        int targetLevel = totalSubCells % 8;

        for (int ry = 0; ry < availH; ++ry) {
            int cy = startY + (availH - 1 - ry); // ry=0 is bottom row
            int cx = startX + dx;

            if (clipRect.contains(cx, cy)) {
                Cell& cell = buffer.cellAt(cx, cy);
                cell.fgColor = m_resolvedColor;
                cell.bgColor = m_resolvedBgColor;

                if (availH == 1) {
                    // Single row sparkline mode
                    cell.character = QByteArray(blockGlyphs[targetLevel]);
                } else {
                    // Multi-row chart mode
                    if (ry == targetRow) {
                        cell.character = QByteArray(blockGlyphs[targetLevel]);
                    } else if (ry < targetRow) {
                        if (m_filled) {
                            cell.character = QByteArray("\xE2\x96\x88"); // Solid fill
                        } else {
                            cell.character = QByteArray(" "); // Blank space for clean curve outline
                        }
                    } else {
                        cell.character = QByteArray(" "); // Blank space above curve
                    }
                }
                cell.displayWidth = 1;
                cell.continuation = false;
            }
        }
    }
}

} // namespace qtui
