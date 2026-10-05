#include "GradientStop.h"
#include "../primitives/ColorMapper.h"

namespace qtui {

GradientStop::GradientStop(QObject *parent) : QObject(parent) {}

void GradientStop::setPosition(qreal pos) {
    if (qFuzzyCompare(m_position, pos)) return;
    m_position = pos;
    emit positionChanged();
    emit updated();
}

void GradientStop::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    emit colorChanged();
    emit updated();
}

} // namespace qtui
