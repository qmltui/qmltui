#include "Gauge.h"
#include <ColorMapper.h>
#include <Gradient.h>
#include <algorithm>
#include <cmath>

namespace qtui {

Gauge::Gauge(QObject *parent) : Item(parent) {
    setImplicitWidth(30);
    setImplicitHeight(1);
}

void Gauge::setValue(double val) {
    if (qFuzzyCompare(m_value, val)) return;
    // Stored unclamped so value and to may be assigned in any order;
    // paint() clamps the displayed ratio.
    m_value = val;
    emit valueChanged();
    markDirty();
}

void Gauge::setFrom(double from) {
    if (qFuzzyCompare(m_from, from)) return;
    m_from = from;
    emit fromChanged();
    markDirty();
}

void Gauge::setTo(double to) {
    if (qFuzzyCompare(m_to, to)) return;
    m_to = to;
    emit toChanged();
    markDirty();
}

void Gauge::setText(const QString &text) {
    if (m_text == text) return;
    m_text = text;
    emit textChanged();
    markDirty();
}

void Gauge::setColor(Color color) {
    if (m_gaugeColor == color) return;
    m_gaugeColor = color;
    m_resolvedColor = ColorMapper::fromString(m_gaugeColor, 0xFF00FF00);
    emit colorChanged();
    markDirty();
}

void Gauge::setGradient(Gradient *gradient) {
    if (m_gradient == gradient) return;
    if (m_gradient) {
        disconnect(m_gradient, nullptr, this, nullptr);
    }
    m_gradient = gradient;
    if (m_gradient) {
        connect(m_gradient, &Gradient::updated, this, [this]() { markDirty(); });
    }
    emit gradientChanged();
    markDirty();
}

void Gauge::setBackgroundColor(Color color) {
    if (m_backgroundColor == color) return;
    m_backgroundColor = color;
    m_resolvedBackgroundColor = ColorMapper::fromString(m_backgroundColor, 0x00000000);
    emit backgroundColorChanged();
    markDirty();
}

void Gauge::setFillCharacter(const QString &ch) {
    if (m_filledChar == ch || ch.isEmpty()) return;
    m_filledChar = ch;
    emit fillCharacterChanged();
    markDirty();
}

void Gauge::setEmptyCharacter(const QString &ch) {
    if (m_unfilledChar == ch || ch.isEmpty()) return;
    m_unfilledChar = ch;
    emit emptyCharacterChanged();
    markDirty();
}

void Gauge::setShowPercentage(bool show) {
    if (m_showPercentage == show) return;
    m_showPercentage = show;
    emit showPercentageChanged();
    markDirty();
}

void Gauge::setShowBrackets(bool show) {
    if (m_showBrackets == show) return;
    m_showBrackets = show;
    emit showBracketsChanged();
    markDirty();
}

void Gauge::paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) const {
    Q_UNUSED(clipRect);
    if (!m_visible) return;

    const int availW = width();
    const int availH = height();
    const int startX = absX;
    const int startY = absY;

    if (availW <= 0 || availH <= 0) return;

    double pct = (qFuzzyCompare(m_to, m_from) ? 0.0 : std::clamp((m_value - m_from) / (m_to - m_from), 0.0, 1.0));
    QString pctText = m_showPercentage ? QString(" %1%").arg(static_cast<int>(pct * 100.0)) : "";
    int textReservedW = m_text.length() + pctText.length() + (m_showBrackets ? 2 : 0);
    int barW = std::max(1, availW - textReservedW);

    int curX = startX;

    // Render Text
    if (!m_text.isEmpty()) {
        buffer.writeText(curX, startY, m_text, 0xFFFFFFFF, m_resolvedBackgroundColor);
        curX += m_text.length();
    }

    // Render Opening Bracket
    if (m_showBrackets) {
        buffer.writeText(curX, startY, "[", 0xFF808080, m_resolvedBackgroundColor);
        curX += 1;
    }

    // Render Bar Fill
    int filledCells = static_cast<int>(pct * barW);
    bool useGradient = (m_gradient && !m_gradient->stopsList().isEmpty());

    for (int i = 0; i < barW; ++i) {
        if (i < filledCells) {
            uint32_t cellColor = m_resolvedColor;
            if (useGradient) {
                qreal t = (barW > 1) ? static_cast<qreal>(i) / (barW - 1) : 0.0;
                cellColor = m_gradient->colorAt(t);
            }
            buffer.writeText(curX + i, startY, m_filledChar, cellColor, m_resolvedBackgroundColor);
        } else {
            buffer.writeText(curX + i, startY, m_unfilledChar, 0xFF555555, m_resolvedBackgroundColor);
        }
    }
    curX += barW;

    // Render Closing Bracket
    if (m_showBrackets) {
        buffer.writeText(curX, startY, "]", 0xFF808080, m_resolvedBackgroundColor);
        curX += 1;
    }

    // Render Percentage Text
    if (m_showPercentage) {
        uint32_t pctColor = m_resolvedColor;
        if (useGradient) {
            pctColor = m_gradient->colorAt(pct);
        }
        buffer.writeText(curX, startY, pctText, pctColor, m_resolvedBackgroundColor, TextAttribute::Bold);
    }
}

} // namespace qtui
