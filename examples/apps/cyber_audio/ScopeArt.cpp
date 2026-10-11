#include "ScopeArt.h"

#include <Qamui/Core/primitives/Color.h>
#include <Qamui/Core/primitives/ColorMapper.h>

#include <algorithm>
#include <cmath>
#include <vector>
#include <utility>

using qamui::CanvasPainter;

namespace {

void requestRepaint(qamui::Item* shape)
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
    from = qamui::ColorMapper::toArgb(from);
    to = qamui::ColorMapper::toArgb(to);
    const auto ch = [&](int shift) {
        const int a = (from >> shift) & 0xFF;
        const int b = (to >> shift) & 0xFF;
        return uint32_t(std::clamp(int(std::lround(a + (b - a) * t)), 0, 255)) << shift;
    };
    return (to & 0xFF000000u) | ch(16) | ch(8) | ch(0);
}

uint32_t argbFromString(const QString& text, uint32_t fallback)
{
    const qamui::Color c = qamui::Color::fromString(text);
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
    : qamui::CanvasShape(parent)
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
    : qamui::CanvasShape(parent)
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


NeonNebula::NeonNebula(QObject* parent)
    : qamui::CanvasShape(parent)
{
}

void NeonNebula::setSparkColor(const QString& color)
{
    if (m_sparkColor == color)
        return;
    m_sparkColor = color;
    emit sparkColorChanged();
    requestRepaint(this);
}

void NeonNebula::advance(const QVariantList& bands)
{
    // bandLevels are already attack/release-smoothed, dB-mapped values. Retain
    // their changes relative to a ~1 s baseline instead of smoothing them again.
    // This also reveals attacks in compressed recordings with steady loudness.
    std::array<double, 24> levels{};
    double flux = 0.0;
    for (size_t i = 0; i < levels.size(); ++i) {
        const double value = i < size_t(bands.size()) ? bands[qsizetype(i)].toDouble() : 0.0;
        levels[i] = std::isfinite(value) ? std::clamp(value, 0.0, 1.0) : 0.0;
        flux += std::max(0.0, levels[i] - m_previous[i]);
        m_previous[i] = levels[i];
        const double change = std::clamp((levels[i] - m_average[i]) * 8.0, -1.0, 1.0);
        m_response[i] += (change - m_response[i]) * 0.70;
        m_average[i] += (levels[i] - m_average[i]) * 0.025;
    }
    const auto energy = [&](int first, int last) {
        double sum = 0.0;
        for (int i = first; i < last; ++i)
            sum += levels[size_t(i)] * levels[size_t(i)];
        return std::sqrt(sum / (last - first));
    };
    m_bass = energy(0, 7);
    m_mid = energy(7, 17);
    m_treble = energy(17, 24);
    // Positive spectral flux is an onset envelope, not a tempo/beat estimate.
    m_pulse = std::max(m_pulse * 0.78, std::min(1.0, flux * 18.0 / levels.size()));
    // Sustained sound carries the field forward; attacks accelerate it. Silence
    // stops motion once the short onset envelope has decayed.
    if (m_pulse < 0.001)
        m_pulse = 0.0;
    m_phase += 0.055 * (m_bass + m_mid + m_treble) + 0.10 * m_pulse;
    requestRepaint(this);
}

void NeonNebula::draw(CanvasPainter& painter) const
{
    const int pw = painter.pixelWidth();
    const int ph = painter.pixelHeight();
    if (pw < 4 || ph < 4)
        return;
    const uint32_t ink = resolvedColor(painter);
    const uint32_t spark = argbFromString(m_sparkColor, ink);
    const uint32_t shadow = mixColor(0xFF000000u, ink, 0.22);
    const double energy = std::clamp(m_bass + m_mid, 0.0, 1.0);
    const double spread = 0.55 + m_bass * 0.25 + m_pulse * 0.20;
    const int steps = std::clamp(pw * 2, 80, 800);

    // Two curling arms share a central knot. Offsetting the curves along their
    // normals produces a ribbon, not a stack of sine-wave scope traces.
    const auto point = [&](double t, double strand) {
        // Each part of the arm responds to its own spectral neighborhood.
        const double band = std::abs(t) * (m_response.size() - 1);
        const size_t first = std::min(size_t(band), m_response.size() - 2);
        const double blend = band - first;
        const double response = m_response[first] * (1.0 - blend)
                              + m_response[first + 1] * blend;
        const double angle = (3.2 + m_mid * 1.4) * std::abs(t) + m_phase * 0.45
                           + 0.55 * response
                           + m_mid * 0.45 * std::sin(t * 7.0 - m_phase * 1.7);
        const double radius = t * spread * (0.90 + 0.10 * response);
        const double offset = strand * (0.006 + 0.012 * std::abs(t))
                            * (0.65 + m_treble * 0.65 + m_pulse * 0.45);
        const double x = radius * std::cos(angle) + offset * std::cos(angle + 1.57);
        const double y = radius * std::sin(angle) + offset * std::sin(angle + 1.57);
        return std::pair<int, int>{
            int(std::lround(pw * (0.5 + x * 0.46))),
            int(std::lround(ph * (0.5 + y * 0.43)))};
    };

    // Dim outer strands first, bright inner strands last. Color resolution is
    // still per terminal cell; no framebuffer feedback or unbounded history.
    for (int layer = 8; layer >= 0; --layer) {
        for (int side : {-1, 1}) {
            if (layer == 0 && side == 1)
                continue;
            const double strand = layer * side;
            const double brightness = (1.0 - layer / 11.0) * (0.20 + 0.55 * energy + 0.25 * m_pulse);
            const uint32_t hue = side == 1 && layer > 5 ? spark : ink;
            const uint32_t lineInk = mixColor(shadow, hue, brightness);
            auto previous = point(-1.0, strand);
            for (int i = 1; i <= steps; ++i) {
                const double t = -1.0 + 2.0 * i / steps;
                const auto current = point(t, strand);
                // Outer strands dissolve into dots rather than solid slabs.
                if (layer < 2)
                    bresenham(painter, previous.first, previous.second,
                              current.first, current.second, lineInk);
                else if ((i + layer) % 3 == 0)
                    painter.setPixelColorHex(current.first, current.second, lineInk);
                previous = current;
            }
        }
    }

    // Deterministic drifting motes: no random state in draw(), and a fixed cap
    // on work regardless of playback duration or terminal size.
    const int particles = std::clamp(pw * ph / 100, 12, 160);
    for (int i = 0; i < particles; ++i) {
        const double seed = i * 2.3999632297;
        const double drift = m_phase * (0.65 + (i % 5) * 0.08);
        const double x = 0.5 + (0.32 + 0.14 * m_pulse) * std::sin(seed + drift);
        const double y = 0.5 + (0.30 + 0.13 * m_pulse) * std::sin(seed * 1.73 - drift);
        const double twinkle = 0.5 + 0.5 * std::sin(seed * 3.0 + m_phase * 2.0);
        const double brightness = 0.10 + twinkle * (0.15 + m_treble * 0.75);
        painter.setPixelColorHex(int(x * (pw - 1)), int(y * (ph - 1)),
                                 mixColor(shadow, i % 4 == 0 ? spark : ink, brightness));
    }
}
