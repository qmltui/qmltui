#include "framesource.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

FrameSource::FrameSource(QObject* parent) : QObject(parent) {
    m_timer.setInterval(100);
    connect(&m_timer, &QTimer::timeout, this, &FrameSource::advance);
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
    m_frame = paintTrace(m_tick, effectiveSize());   // redraw at the new resolution at once
    emit frameChanged();
}

void FrameSource::advance() {
    ++m_tick;
    m_frame = paintTrace(m_tick, effectiveSize());
    emit tickChanged();
    emit frameChanged();   // the bound Image repaints
}

QImage FrameSource::paintTrace(int tick, const QSize& size) {
    QImage image(size, QImage::Format_ARGB32);
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
        for (int x = 0; x < size.width(); ++x) {
            const double t = x / double(size.width()) * 2 * M_PI * frequency + phase + shift;
            const double y = size.height() / 2.0 - std::sin(t) * amplitude * (size.height() / 2.0 - 2);
            if (x == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        painter.setPen(QPen(colour, stroke));
        painter.drawPath(path);
    };
    trace(QColor(101, 230, 160), 1.0, 0.9, 0.0);
    trace(QColor(126, 224, 255), 2.0, 0.5, 1.2);
    return image;
}
