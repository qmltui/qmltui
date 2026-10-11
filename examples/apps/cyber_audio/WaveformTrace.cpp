#include "WaveformTrace.h"

#include <algorithm>
#include <cmath>

WaveformTrace::WaveformTrace(QObject* parent)
    : qamui::CanvasShape(parent)
{
}

void WaveformTrace::setSamples(const QList<double>& samples)
{
    // Every tick brings new data; skipping the equality check avoids comparing 256
    // doubles only to find they differ.
    m_samples = samples;
    emit samplesChanged();
    requestRepaint();
}

void WaveformTrace::setAmplitude(qreal amplitude)
{
    const qreal clamped = std::clamp(amplitude, 0.0, 1.0);
    if (qFuzzyCompare(m_amplitude, clamped))
        return;
    m_amplitude = clamped;
    emit amplitudeChanged();
    requestRepaint();
}

void WaveformTrace::setSilenceThreshold(qreal threshold)
{
    const qreal clamped = std::max(0.0, threshold);
    if (qFuzzyCompare(m_silenceThreshold, clamped))
        return;
    m_silenceThreshold = clamped;
    emit silenceThresholdChanged();
    requestRepaint();
}

void WaveformTrace::requestRepaint()
{
    if (parentItem())
        parentItem()->markDirty();
}

namespace {

void bresenham(qamui::CanvasPainter& painter, int x0, int y0, int x1, int y1, uint32_t ink)
{
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        painter.setPixelColorHex(x0, y0, ink);      // bounds-checked by the painter
        if (x0 == x1 && y0 == y1)
            break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

} // namespace

void WaveformTrace::draw(qamui::CanvasPainter& painter) const
{
    const int pw = painter.pixelWidth();
    const int ph = painter.pixelHeight();
    const int n = int(m_samples.size());
    if (pw <= 0 || ph <= 0 || n < 2)
        return;

    bool silent = true;
    for (int i = 0; i < n && silent; ++i)
        silent = std::abs(m_samples[i]) < m_silenceThreshold;
    if (silent)
        return;

    const uint32_t ink = resolvedColor(painter);
    const double midY = ph / 2.0;
    const double amp = ph * 0.5 * m_amplitude;
    const double step = double(n - 1) / std::max(1, pw - 1);

    int prevY = int(std::lround(midY));
    for (int x = 0; x < pw; ++x) {
        const double pos = x * step;
        const int i0 = std::min(n - 1, int(pos));
        const int i1 = std::min(n - 1, i0 + 1);
        const double t = pos - i0;
        const double v = m_samples[i0] * (1.0 - t) + m_samples[i1] * t;
        const int py = int(std::lround(midY - v * amp));
        if (x == 0)
            prevY = py;
        bresenham(painter, x == 0 ? 0 : x - 1, prevY, x, py, ink);
        prevY = py;
    }
}
