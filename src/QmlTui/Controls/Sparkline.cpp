#include <QmlTui/Controls/Sparkline.h>
#include <algorithm>
#include <cmath>

namespace qmltui {

Sparkline::Sparkline(QObject* parent) : Item(parent) {
    setImplicitWidth(20);
    setImplicitHeight(1);
}

void Sparkline::setValues(const QVariant& values) {
    m_valuesVariant = values;
    QVariantList newValues;

    if (values.userType() == QMetaType::fromType<QJSValue>().id()) {
        newValues = values.value<QJSValue>().toVariant().toList();
    } else if (values.canConvert<QVariantList>()) {
        newValues = values.toList();
    }
    if (m_values == newValues) return;
    m_values = std::move(newValues);
    emit valuesChanged();
    markDirty();
}

void Sparkline::setFrom(double from) {
    if (qFuzzyCompare(m_from, from)) return;
    m_from = from;
    emit fromChanged();
    markDirty();
}

void Sparkline::setTo(double to) {
    if (qFuzzyCompare(m_to, to)) return;
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
    m_resolvedColor = Color::resolve(m_color, 0);
    emit colorChanged();
    markDirty();
}

void Sparkline::setBackgroundColor(Color color) {
    if (m_backgroundColor == color) return;
    m_backgroundColor = color;
    m_resolvedBackgroundColor = Color::resolve(m_backgroundColor, 0);
    emit backgroundColorChanged();
    markDirty();
}

void Sparkline::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!m_visible || m_values.isEmpty()) return;

    const int availW = width();
    const int availH = height();
    const int startX = absX;
    const int startY = absY;

    if (availW <= 0 || availH <= 0) return;

    // Determine min/max range
    double computedMin = m_from;
    double computedMax = m_to;

    if (qFuzzyIsNull(m_to)) {
        computedMin = m_values[0].toDouble();
        computedMax = m_values[0].toDouble();
        for (const QVariant& v : m_values) {
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
    int fillOffset = std::max(0, availW - valCount);

    for (int dx = 0; dx < availW; ++dx) {
        if (dx < fillOffset) {
            for (int ry = 0; ry < availH; ++ry) {
                int cy = startY + (availH - 1 - ry); // ry=0 is bottom row
                int cx = startX + dx;

                if (clipRect.contains(cx, cy)) {
                    Cell* targetCell = buffer.tryCellAt(cx, cy);
                    if (!targetCell) continue;
                    Cell& cell = *targetCell;
                    cell.fgColor = m_resolvedColor;
                    cell.bgColor = m_resolvedBackgroundColor;
                    cell.setCharacter(" ");
                    cell.displayWidth = 1;
                    cell.continuation = false;
                }
            }
            continue;
        }

        double val = m_values[valCount - availW + dx].toDouble();

        double pct = std::clamp((val - computedMin) / range, 0.0, 1.0);
        int totalSubCells = static_cast<int>(pct * (totalLevels - 1));
        totalSubCells = std::clamp(totalSubCells, 0, totalLevels - 1);

        int targetRow = totalSubCells / 8;
        int targetLevel = totalSubCells % 8;

        for (int ry = 0; ry < availH; ++ry) {
            int cy = startY + (availH - 1 - ry); // ry=0 is bottom row
            int cx = startX + dx;

            if (clipRect.contains(cx, cy)) {
                Cell* targetCell = buffer.tryCellAt(cx, cy);
                if (!targetCell) continue;
                Cell& cell = *targetCell;
                cell.fgColor = m_resolvedColor;
                cell.bgColor = m_resolvedBackgroundColor;

                if (availH == 1) {
                    // Single row sparkline mode
                    cell.setCharacter(blockGlyphs[targetLevel]);
                } else {
                    // Multi-row chart mode
                    if (ry == targetRow) {
                        cell.setCharacter(blockGlyphs[targetLevel]);
                    } else if (ry < targetRow) {
                        if (m_filled) {
                            cell.setCharacter("\xE2\x96\x88"); // Solid fill
                        } else {
                            cell.setCharacter(" "); // Blank space for clean curve outline
                        }
                    } else {
                        cell.setCharacter(" "); // Blank space above curve
                    }
                }
                cell.displayWidth = 1;
                cell.continuation = false;
            }
        }
    }
}

} // namespace qmltui
