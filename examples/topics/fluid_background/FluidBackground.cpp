#include "FluidBackground.h"
#include <Qamui/Core/Engine/Clock_p.h>
#include <Qamui/Core/Engine/MouseEvent.h>
#include <Qamui/Core/Primitives/TextGridBuffer.h>
#include <algorithm>
#include <cmath>

namespace qamui {

FluidBackground::FluidBackground(QObject* parent) : Item(parent) {
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFF38BDF8);
    m_resolvedSecondaryColor = ColorMapper::fromString(m_secondaryColor, 0xFFEC4899);
    m_resolvedBgColor = ColorMapper::fromString(m_backgroundColor, 0xFF080C14);

    m_timer = new ClockTimer(this);
    m_timer->setInterval(40); // 25 FPS
    connect(m_timer, &ClockTimer::timeout, this, &FluidBackground::onTick);
    connect(this, &Item::visibleChanged, this, &FluidBackground::updateTimer);

    updateTimer();
}

FluidBackground::~FluidBackground() = default;

void FluidBackground::setRunning(bool r) {
    if (m_running == r) return;
    m_running = r;
    emit runningChanged();
    updateTimer();
    markDirty();
}

void FluidBackground::setSpeed(qreal s) {
    if (qFuzzyCompare(m_speed, s)) return;
    m_speed = s;
    emit speedChanged();
}

void FluidBackground::setTime(qreal t) {
    if (qFuzzyCompare(m_time, t)) return;
    m_time = t;
    emit timeChanged();
    markDirty();
}

void FluidBackground::setCharset(const QString& cs) {
    if (m_charset == cs || cs.isEmpty()) return;
    m_charset = cs;
    emit charsetChanged();
    markDirty();
}

void FluidBackground::setColor(Color c) {
    if (m_color == c) return;
    m_color = c;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFF38BDF8);
    emit colorChanged();
    markDirty();
}

void FluidBackground::setSecondaryColor(Color c) {
    if (m_secondaryColor == c) return;
    m_secondaryColor = c;
    m_resolvedSecondaryColor = ColorMapper::fromString(m_secondaryColor, 0xFFEC4899);
    emit secondaryColorChanged();
    markDirty();
}

void FluidBackground::setBackgroundColor(Color c) {
    if (m_backgroundColor == c) return;
    m_backgroundColor = c;
    m_resolvedBgColor = ColorMapper::fromString(m_backgroundColor, 0xFF080C14);
    emit backgroundColorChanged();
    markDirty();
}

void FluidBackground::setInteractive(bool i) {
    if (m_interactive == i) return;
    m_interactive = i;
    emit interactiveChanged();
}

void FluidBackground::setAutoWakes(bool a) {
    if (m_autoWakes == a) return;
    m_autoWakes = a;
    emit autoWakesChanged();
}

void FluidBackground::setColorMode(int m) {
    if (m_colorMode == m) return;
    m_colorMode = m;
    emit colorModeChanged();
    markDirty();
}

void FluidBackground::updateTimer() {
    if (m_running && visible()) {
        if (!m_timer->isActive()) m_timer->start();
    } else {
        if (m_timer->isActive()) m_timer->stop();
    }
}

void FluidBackground::addWake(float normX, float normY, float strength) {
    FluidRipple r;
    r.x = std::clamp(normX, 0.0f, 1.0f);
    r.y = std::clamp(normY, 0.0f, 1.0f);
    r.intensity = strength;
    r.radius = 0.04f;
    r.speed = 0.30f;
    m_ripples.append(r);
    markDirty();
}

void FluidBackground::clearWakes() {
    m_ripples.clear();
    markDirty();
}

void FluidBackground::onTick() {
    const float dt = 0.04f;
    m_time += dt * m_speed;

    // Update existing ripples
    for (int i = m_ripples.size() - 1; i >= 0; --i) {
        FluidRipple& r = m_ripples[i];
        r.radius += r.speed * dt;
        r.intensity *= 0.94f;
        if (r.intensity < 0.02f || r.radius > r.maxRadius) {
            m_ripples.removeAt(i);
        }
    }

    // Auto wake stirring periodically
    if (m_autoWakes) {
        m_autoWakeTimer += dt;
        if (m_autoWakeTimer >= 2.0f) {
            m_autoWakeTimer = 0.0f;
            const float wx = 0.5f + 0.35f * std::sin(m_time * 0.7f);
            const float wy = 0.5f + 0.30f * std::cos(m_time * 0.9f);
            addWake(wx, wy, 0.75f);
        }
    }

    markDirty();
}

void FluidBackground::mousePressEvent(MouseEvent* event) {
    if (m_interactive && width() > 0 && height() > 0) {
        const float u =
            static_cast<float>(event->x() - leftPadding()) / static_cast<float>(width());
        const float v =
            static_cast<float>(event->y() - topPadding()) / static_cast<float>(height());
        addWake(u, v, 1.0f);
        event->setAccepted(true);
    }
}

void FluidBackground::mouseMoveEvent(MouseEvent* event) {
    if (m_interactive && width() > 0 && height() > 0) {
        const float u =
            static_cast<float>(event->x() - leftPadding()) / static_cast<float>(width());
        const float v =
            static_cast<float>(event->y() - topPadding()) / static_cast<float>(height());
        // Soft wake on cursor motion
        addWake(u, v, 0.5f);
        event->setAccepted(true);
    }
}

uint32_t FluidBackground::evaluateColor(float light) const {
    const float t = std::clamp(light, 0.0f, 1.0f);

    auto lerpChannel = [](uint32_t c1, uint32_t c2, float factor) -> uint32_t {
        const uint32_t r1 = (c1 >> 16) & 0xFF;
        const uint32_t g1 = (c1 >> 8) & 0xFF;
        const uint32_t b1 = c1 & 0xFF;

        const uint32_t r2 = (c2 >> 16) & 0xFF;
        const uint32_t g2 = (c2 >> 8) & 0xFF;
        const uint32_t b2 = c2 & 0xFF;

        const uint32_t r = static_cast<uint32_t>(r1 + factor * (static_cast<float>(r2) - r1));
        const uint32_t g = static_cast<uint32_t>(g1 + factor * (static_cast<float>(g2) - g1));
        const uint32_t b = static_cast<uint32_t>(b1 + factor * (static_cast<float>(b2) - b1));

        return 0xFF000000 | (r << 16) | (g << 8) | b;
    };

    if (m_colorMode == Monochrome) {
        const uint32_t lum = static_cast<uint32_t>(t * 255.0f);
        return 0xFF000000 | (lum << 16) | (lum << 8) | lum;
    } else if (m_colorMode == Accent) {
        return lerpChannel(m_resolvedBgColor, m_resolvedColor, t);
    } else { // Gradient mode
        return lerpChannel(m_resolvedColor, m_resolvedSecondaryColor, t);
    }
}

void FluidBackground::paint(TextGridBuffer& buffer, int absX, int absY,
                            const ClipRect& clipRect) const {
    const int w = width() - leftPadding() - rightPadding();
    const int h = height() - topPadding() - bottomPadding();
    if (w <= 0 || h <= 0 || m_charset.isEmpty()) return;

    const int originX = absX + leftPadding();
    const int originY = absY + topPadding();

    // Adjust aspect ratio: terminal cells are ~2x taller than wide
    const float aspect = static_cast<float>(w) / std::max(1.0f, static_cast<float>(h) * 2.0f);
    const int charCount = m_charset.length();

    for (int y = 0; y < h; ++y) {
        const int cellY = originY + y;
        const float v = static_cast<float>(y) / static_cast<float>(h);
        const float py = v - 0.5f;

        for (int x = 0; x < w; ++x) {
            const int cellX = originX + x;
            if (!clipRect.contains(cellX, cellY)) continue;

            const float u = static_cast<float>(x) / static_cast<float>(w);
            const float px = (u - 0.5f) * aspect;

            // Domain warping harmonic folds (procedural fluid equation from Asciify)
            const float qx = px + 0.22f * std::sin(py * 5.2f + m_time * 0.40f * m_speed) +
                             0.15f * std::sin(px * 2.4f - py * 3.1f - m_time * 0.22f * m_speed);
            const float qy = py + 0.19f * std::sin(px * 3.5f + m_time * 0.32f * m_speed);

            const float radius = std::hypot(qx * 0.8f + 0.18f, qy * 1.1f);
            const float folds =
                0.5f + 0.5f * std::sin(radius * 14.0f - qx * 2.8f + std::sin(qy * 5.0f) * 1.4f -
                                       m_time * 0.52f * m_speed);
            const float cloud =
                0.5f + 0.5f * std::sin(qx * 3.6f - qy * 2.9f + m_time * 0.20f * m_speed);

            float light = folds * folds * (0.48f + cloud * 0.32f) - 0.075f;

            // Add wave wakes
            float rippleSum = 0.0f;
            for (const FluidRipple& r : m_ripples) {
                const float rdx = (u - r.x) * aspect;
                const float rdy = v - r.y;
                const float dist = std::hypot(rdx, rdy);
                if (dist < r.radius) {
                    const float envelope = (1.0f - dist / r.radius) * r.intensity;
                    const float wave = std::sin((dist - r.radius) * 36.0f);
                    rippleSum += wave * envelope * 0.55f;
                }
            }

            light = std::clamp(light + rippleSum, 0.0f, 1.0f);

            // Select character from charset ramp
            const int charIndex =
                std::clamp(static_cast<int>(light * (charCount - 1)), 0, charCount - 1);
            const QString cluster = m_charset.mid(charIndex, 1);
            const uint32_t fg = evaluateColor(light);

            buffer.setGrapheme(cellX, cellY, QStringView(cluster), 1, fg, m_resolvedBgColor,
                               TextAttribute::None);
        }
    }
}

} // namespace qamui
