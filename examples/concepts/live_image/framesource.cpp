#include "framesource.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

FrameSource::FrameSource(QObject* parent) : QObject(parent) {
    m_timer.setInterval(100);
    connect(&m_timer, &QTimer::timeout, this, &FrameSource::advance);
    m_resizeTimer.setSingleShot(true);
    m_resizeTimer.setInterval(16);
    connect(&m_resizeTimer, &QTimer::timeout, this, [this] {
        m_frame = paintTrace(m_tick, effectiveSize());
        emit frameChanged();
    });
    m_frame = paintTrace(0);
    m_timer.start();
}

void FrameSource::setRunning(bool running) {
    if (running == m_timer.isActive()) return;
    if (running) m_timer.start(); else m_timer.stop();
    emit runningChanged();
}

void FrameSource::setInterval(int ms) {
    if (ms == m_timer.interval() || ms <= 0) return;
    m_timer.setInterval(ms);
    emit intervalChanged();
}

void FrameSource::setFrameSize(const QSize& size) {
    if (size == m_frameSize) return;
    m_frameSize = size;
    emit frameSizeChanged();
    // Width and height can change several times during one relayout. Paint
    // only the latest size, at most once per display interval, even paused.
    if (!m_resizeTimer.isActive()) m_resizeTimer.start();
}

void FrameSource::advance() {
    m_resizeTimer.stop(); // this tick already paints at the latest size
    ++m_tick;
    m_frame = paintTrace(m_tick, effectiveSize());
    emit tickChanged();
    emit frameChanged();   // the bound Image repaints
}

QImage FrameSource::paintTrace(int tick, const QSize& size) {
    // The canvas is opaque. RGB32 uses QPainter's fast raster path rather
    // than the costly straight-alpha path for antialiased thick strokes.
    QImage image(size, QImage::Format_RGB32);
    image.fill(QColor(11, 15, 20));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, size.height() > 64);   // smooth only where pixels are real
    // Grid
    painter.setPen(QColor(30, 41, 59));
    for (int x = 0; x < size.width(); x += 12) painter.drawLine(x, 0, x, size.height());
    painter.drawLine(0, size.height() / 2, size.width(), size.height() / 2);
    // Two traces, the phase advancing with the tick. Line width scales with
    // the resolution so the trace reads the same in cells and in true pixels.
    const double phase = tick * 0.25;
    const int stroke = std::max(1, size.height() / 24);
    auto trace = [&](QColor colour, double frequency, double amplitude, double shift) {
        QPainterPath path;
        // Linear interpolation of A*sin(w*t) has error <= A*w*w/(8*N*N).
        // Keep it below 1/4 pixel, instead of stroking one segment per column
        // even when a large terminal needs only a few hundred curve samples.
        const double pixelAmplitude = amplitude * std::max(0.0, size.height() / 2.0 - 2);
        const int segments = std::clamp(
            int(std::ceil(2 * M_PI * frequency * std::sqrt(pixelAmplitude / 2))),
            1, std::max(1, size.width() - 1));
        for (int point = 0; point <= segments; ++point) {
            const double x = (size.width() - 1) * double(point) / segments;
            const double t = x / double(size.width()) * 2 * M_PI * frequency + phase + shift;
            const double y = size.height() / 2.0 - std::sin(t) * amplitude * (size.height() / 2.0 - 2);
            if (point == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        painter.setPen(QPen(colour, stroke));
        painter.drawPath(path);
    };
    trace(QColor(101, 230, 160), 1.0, 0.9, 0.0);
    trace(QColor(126, 224, 255), 2.0, 0.5, 1.2);
    return image;
}
