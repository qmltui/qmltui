#include "Gauge.h"
#include <ColorMapper.h>
#include <algorithm>
#include <cmath>

namespace qtui {

Gauge::Gauge(QObject *parent) : Item(parent) {
    setImplicitWidth(30);
    setImplicitHeight(1);
}

void Gauge::setValue(double val) {
    if (qFuzzyCompare(m_value, val)) return;
    m_value = std::clamp(val, 0.0, m_maxValue);
    emit valueChanged();
    markDirty();
}

void Gauge::setMaxValue(double maxVal) {
    if (qFuzzyCompare(m_maxValue, maxVal) || maxVal <= 0.0) return;
    m_maxValue = maxVal;
    m_value = std::clamp(m_value, 0.0, m_maxValue);
    emit maxValueChanged();
    markDirty();
}

void Gauge::setLabel(const QString &label) {
    if (m_label == label) return;
    m_label = label;
    emit labelChanged();
    markDirty();
}

void Gauge::setGaugeColor(const QString &color) {
    if (m_gaugeColor == color) return;
    m_gaugeColor = color;
    m_resolvedGaugeColor = ColorMapper::fromString(m_gaugeColor, 0xFF00FF00);
    emit gaugeColorChanged();
    markDirty();
}

void Gauge::setBgColor(const QString &color) {
    if (m_bgColor == color) return;
    m_bgColor = color;
    m_resolvedBgColor = ColorMapper::fromString(m_bgColor, 0x00000000);
    emit bgColorChanged();
    markDirty();
}

void Gauge::setFilledChar(const QString &ch) {
    if (m_filledChar == ch || ch.isEmpty()) return;
    m_filledChar = ch;
    emit filledCharChanged();
    markDirty();
}

void Gauge::setUnfilledChar(const QString &ch) {
    if (m_unfilledChar == ch || ch.isEmpty()) return;
    m_unfilledChar = ch;
    emit unfilledCharChanged();
    markDirty();
}

void Gauge::setShowPercentage(bool show) {
    if (m_showPercentage == show) return;
    m_showPercentage = show;
    emit showPercentageChanged();
    markDirty();
}

void Gauge::setBorderStyle(const QString &style) {
    if (m_borderStyle == style) return;
    m_borderStyle = style;
    emit borderStyleChanged();
    markDirty();
}

void Gauge::setBorderColor(const QString &color) {
    if (m_borderColor == color) return;
    m_borderColor = color;
    m_resolvedBorderColor = ColorMapper::fromString(m_borderColor, 0xFF808080);
    emit borderColorChanged();
    markDirty();
}

void Gauge::render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) {
    if (!m_visible) return;

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

    QString pctText = m_showPercentage ? QString(" %1%").arg(static_cast<int>((m_value / m_maxValue) * 100.0)) : "";
    int textReservedW = m_label.length() + pctText.length() + 2; // Bracket frames '[' and ']'
    int barW = std::max(1, availW - textReservedW);

    int curX = startX;

    // Render Label
    if (!m_label.isEmpty()) {
        buffer.writeText(curX, startY, m_label, 0xFFFFFFFF, m_resolvedBgColor);
        curX += m_label.length();
    }

    // Render Opening Bracket
    buffer.writeText(curX, startY, "[", 0xFF808080, m_resolvedBgColor);
    curX += 1;

    // Render Bar Fill
    double pct = std::clamp(m_value / m_maxValue, 0.0, 1.0);
    int filledCells = static_cast<int>(pct * barW);

    for (int i = 0; i < barW; ++i) {
        if (i < filledCells) {
            buffer.writeText(curX + i, startY, m_filledChar, m_resolvedGaugeColor, m_resolvedBgColor);
        } else {
            buffer.writeText(curX + i, startY, m_unfilledChar, 0xFF555555, m_resolvedBgColor);
        }
    }
    curX += barW;

    // Render Closing Bracket
    buffer.writeText(curX, startY, "]", 0xFF808080, m_resolvedBgColor);
    curX += 1;

    // Render Percentage Text
    if (m_showPercentage) {
        buffer.writeText(curX, startY, pctText, m_resolvedGaugeColor, m_resolvedBgColor, TextAttribute::Bold);
    }
}

} // namespace qtui
