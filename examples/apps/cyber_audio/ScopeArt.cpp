#include "ScopeArt.h"

#include "Color.h"
#include "ColorMapper.h"

#include <algorithm>
#include <cmath>
#include <vector>

using qtui::CanvasPainter;

namespace {

void requestRepaint(qtui::Item* shape)
{
    if (shape->parentItem())
        shape->parentItem()->markDirty();
}

void bresenham(CanvasPainter& painter, int x0, int y0, int x1, int y1, uint32_t ink)
{
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        painter.setPixelColorHex(x0, y0, ink);
        if (x0 == x1 && y0 == y1)
            break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Linear interpolation between two ARGB colors (ANSI palette entries are resolved
// to RGB first). Alpha comes from `to` so a transparent fade color still draws.
uint32_t mixColor(uint32_t from, uint32_t to, double t)
{
    from = qtui::ColorMapper::toArgb(from);
    to = qtui::ColorMapper::toArgb(to);
    const auto ch = [&](int shift) {
        const int a = (from >> shift) & 0xFF;
        const int b = (to >> shift) & 0xFF;
        return uint32_t(std::clamp(int(std::lround(a + (b - a) * t)), 0, 255)) << shift;
    };
    return (to & 0xFF000000u) | ch(16) | ch(8) | ch(0);
}

uint32_t argbFromString(const QString& text, uint32_t fallback)
{
    const qtui::Color c = qtui::Color::fromString(text);
    return c.isValid() ? c.argb32() : fallback;
}

bool silent(const QList<double>& samples, double threshold = 0.004)
{
    for (double v : samples)
        if (std::abs(v) >= threshold)
            return false;
    return true;
}

// Sample value at fractional position `pos` (0..n-1) with linear interpolation.
double sampleAt(const QList<double>& s, double pos)
{
    const int n = int(s.size());
    const int i0 = std::clamp(int(pos), 0, n - 1);
    const int i1 = std::min(n - 1, i0 + 1);
    const double t = pos - i0;
    return s[i0] * (1.0 - t) + s[i1] * t;
}

void drawPolyline(CanvasPainter& painter, const QList<double>& s, int x0, int x1,
                  double midY, double amp, uint32_t ink)
{
    const int n = int(s.size());
    const int width = x1 - x0 + 1;
    if (n < 2 || width < 2)
        return;
    const double step = double(n - 1) / (width - 1);
    int prevY = 0;
    for (int x = x0; x <= x1; ++x) {
        const int py = int(std::lround(midY - sampleAt(s, (x - x0) * step) * amp));
        if (x == x0)
            prevY = py;
        bresenham(painter, x == x0 ? x0 : x - 1, prevY, x, py, ink);
        prevY = py;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// PersistenceTrace
// ---------------------------------------------------------------------------

PersistenceTrace::PersistenceTrace(QObject* parent)
    : qtui::CanvasShape(parent)
{
}

void PersistenceTrace::setSamples(const QList<double>& samples)
{
    m_history.push_back(samples);
    while (int(m_history.size()) > std::max(1, m_depth))
        m_history.pop_front();
    emit samplesChanged();
    requestRepaint(this);
}

void PersistenceTrace::setDepth(int depth)
{
    depth = std::clamp(depth, 1, 64);
    if (depth == m_depth)
        return;
    m_depth = depth;
    while (int(m_history.size()) > m_depth)
        m_history.pop_front();
    emit depthChanged();
    requestRepaint(this);
}

void PersistenceTrace::setAmplitude(qreal amplitude)
{
    const qreal clamped = std::clamp(amplitude, 0.0, 1.0);
    if (qFuzzyCompare(m_amplitude, clamped))
        return;
    m_amplitude = clamped;
    emit amplitudeChanged();
    requestRepaint(this);
}

void PersistenceTrace::setFadeColor(const QString& color)
{
    if (m_fadeColor == color)
        return;
    m_fadeColor = color;
    emit fadeColorChanged();
    requestRepaint(this);
}

void PersistenceTrace::draw(CanvasPainter& painter) const
{
    const int pw = painter.pixelWidth();
    const int ph = painter.pixelHeight();
    if (pw <= 1 || ph <= 0 || m_history.empty())
        return;

    bool anySignal = false;
    for (const QList<double>& s : m_history)
        anySignal = anySignal || !silent(s);
    if (!anySignal)
        return;

    const uint32_t ink = resolvedColor(painter);
    const uint32_t fade = argbFromString(m_fadeColor, 0xFF000000u);
    const double midY = ph / 2.0;
    const double amp = ph * 0.5 * m_amplitude;
    const int count = int(m_history.size());
    int index = 0;
    for (const QList<double>& s : m_history) {
        // Oldest frames sit close to the fade color, the newest is the pure ink.
        const double t = count > 1 ? double(index + 1) / count : 1.0;
        drawPolyline(painter, s, 0, pw - 1, midY, amp, mixColor(fade, ink, 0.25 + 0.75 * t));
        ++index;
    }
}

// ---------------------------------------------------------------------------
// RidgePlot
// ---------------------------------------------------------------------------

RidgePlot::RidgePlot(QObject* parent)
    : qtui::CanvasShape(parent)
{
}

void RidgePlot::setSamples(const QList<double>& samples)
{
    m_history.push_back(samples);
    while (int(m_history.size()) > std::max(1, m_depth))
        m_history.pop_front();
    emit samplesChanged();
    requestRepaint(this);
}

void RidgePlot::setDepth(int depth)
{
    depth = std::clamp(depth, 1, 128);
    if (depth == m_depth)
        return;
    m_depth = depth;
    while (int(m_history.size()) > m_depth)
        m_history.pop_front();
    emit depthChanged();
    requestRepaint(this);
}

void RidgePlot::setAmplitude(qreal amplitude)
{
    const qreal clamped = std::clamp(amplitude, 0.0, 1.0);
    if (qFuzzyCompare(m_amplitude, clamped))
        return;
    m_amplitude = clamped;
    emit amplitudeChanged();
    requestRepaint(this);
}

void RidgePlot::setFadeColor(const QString& color)
{
    if (m_fadeColor == color)
        return;
    m_fadeColor = color;
    emit fadeColorChanged();
    requestRepaint(this);
}

void RidgePlot::draw(CanvasPainter& painter) const
{
    const int pw = painter.pixelWidth();
    const int ph = painter.pixelHeight();
    if (pw <= 4 || ph <= 2 || m_history.empty())
        return;

    const uint32_t ink = resolvedColor(painter);
    const uint32_t fade = argbFromString(m_fadeColor, 0xFF1E293Bu);

    // Classic proportions: lines span the middle 80 % of the width, each line may
    // rise up to a third of the height above its baseline, baselines are spread
    // evenly between the bottom and that headroom.
    const int x0 = pw / 10;
    const int x1 = pw - 1 - pw / 10;
    const double rise = ph * 0.33 * m_amplitude;
    const double bottom = ph - 1.0;
    const double top = rise;
    // Keep baselines at least three dots apart so lines stay distinguishable in small
    // panels; only the newest windows that fit are shown.
    const int fitting = 1 + int((bottom - top) / 3.0);
    const int count = std::max(1, std::min(int(m_history.size()), fitting));
    const double stepY = count > 1 ? (bottom - top) / (count - 1) : 0.0;

    // Draw front (newest, lowest) to back. skyline[x] is the highest pixel drawn so
    // far in that column; farther lines only show above it.
    std::vector<int> skyline(size_t(pw), ph);
    int index = 0;
    for (auto it = m_history.rbegin(); it != m_history.rend() && index < count; ++it, ++index) {
        const QList<double>& s = *it;
        const int n = int(s.size());
        if (n < 2)
            continue;
        const double baseline = bottom - index * stepY;
        const double t = count > 1 ? 1.0 - double(index) / (count - 1) : 1.0;   // 1 = front
        const uint32_t lineInk = mixColor(fade, ink, 0.2 + 0.8 * t);
        const double step = double(n - 1) / (x1 - x0);

        int prevY = int(std::lround(baseline));
        for (int x = x0; x <= x1; ++x) {
            const double v = std::abs(sampleAt(s, (x - x0) * step));
            const int py = std::clamp(int(std::lround(baseline - v * rise)), 0, ph - 1);
            const int fromY = x == x0 ? py : prevY;
            // Draw the segment from the previous column, clipped by the skyline.
            const int yLo = std::min(fromY, py);
            const int yHi = std::max(fromY, py);
            for (int y = yLo; y <= yHi; ++y) {
                if (y < skyline[size_t(x)])
                    painter.setPixelColorHex(x, y, lineInk);
            }
            skyline[size_t(x)] = std::min(skyline[size_t(x)], yLo);
            prevY = py;
        }
    }
}
