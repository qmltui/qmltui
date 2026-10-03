#include "progressbar.h"
#include <ColorMapper.h>
#include <algorithm>

namespace qtui {

ProgressBar::ProgressBar(QObject *parent) : Rectangle(parent) {
    setImplicitWidth(20);
    setImplicitHeight(3);
}

void ProgressBar::setValue(double val) {
    double clamped = std::clamp(val, 0.0, 1.0);
    if (qFuzzyCompare(m_value, clamped)) return;
    m_value = clamped;
    emit valueChanged();
    markDirty();
}

void ProgressBar::setBarColor(const QString &color) {
    if (m_barColor == color) return;
    m_barColor = color;
    m_resolvedBarColor = ColorMapper::fromString(m_barColor, 0xFF00FF00);
    emit barColorChanged();
    markDirty();
}

void ProgressBar::render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) {
    if (!m_visible) return;

    // Render borders and backgrounds first
    Rectangle::render(buffer, absX, absY, clipRect);

    int lp = leftPadding();
    int rp = rightPadding();
    int tp = topPadding();
    int bp = bottomPadding();

    int availW = width() - lp - rp;
    int availH = height() - tp - bp;

    if (availW <= 0 || availH <= 0) return;

    // Draw the bar in the middle row of the padding area
    int barY = absY + tp + (availH / 2);

    int fillW = static_cast<int>(m_value * availW);

    // Format percentage string e.g. " 99%"
    QString pctText = QString(" %1%").arg(static_cast<int>(m_value * 100));
    int pctLen = pctText.length();

    for (int dx = 0; dx < availW; ++dx) {
        int cx = absX + lp + dx;
        if (clipRect.contains(cx, barY)) {
            if (cx >= 0 && cx < buffer.width() && barY >= 0 && barY < buffer.height()) {
                Cell& cell = buffer.cellAt(cx, barY);
                if (dx >= availW - pctLen) {
                    int charIdx = dx - (availW - pctLen);
                    cell.character = pctText.mid(charIdx, 1).toUtf8();
                    cell.fgColor = 0xFFFFFFFF; // White percentage text
                } else {
                    if (dx < fillW) {
                        cell.character = "█";
                        cell.fgColor = m_resolvedBarColor;
                    } else {
                        cell.character = "░";
                        cell.fgColor = 0xFF444444;
                    }
                }
            }
        }
    }
}

} // namespace qtui
