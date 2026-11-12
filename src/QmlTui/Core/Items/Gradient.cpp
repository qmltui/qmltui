#include <QmlTui/Core/Items/Gradient.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <algorithm>
#include <cmath>

namespace qmltui {

Gradient::Gradient(QObject* parent) : QObject(parent) {}

QQmlListProperty<GradientStop> Gradient::stops() const {
    return QQmlListProperty<GradientStop>(const_cast<Gradient*>(this), const_cast<Gradient*>(this),
                                          &Gradient::appendStop, &Gradient::countStops,
                                          &Gradient::stopAt, &Gradient::clearStops);
}

void Gradient::appendStop(QQmlListProperty<GradientStop>* list, GradientStop* stop) {
    auto* self = static_cast<Gradient*>(list->object);
    if (self) self->addStop(stop);
}

void Gradient::addStop(GradientStop* stop) {
    if (!stop) return;
    m_stops.append(stop);
    m_stopsDirty = true;
    connect(stop, &GradientStop::updated, this, [this]() {
        m_stopsDirty = true;
        emit updated();
    });
    connect(stop, &QObject::destroyed, this, [this, stop]() {
        m_stops.removeAll(stop);
        m_stopsDirty = true;
        emit updated();
    });
    emit updated();
}

qsizetype Gradient::countStops(QQmlListProperty<GradientStop>* list) {
    auto* self = static_cast<Gradient*>(list->object);
    return self ? self->m_stops.size() : 0;
}

GradientStop* Gradient::stopAt(QQmlListProperty<GradientStop>* list, qsizetype index) {
    auto* self = static_cast<Gradient*>(list->object);
    return (self && index >= 0 && index < self->m_stops.size()) ? self->m_stops.at(index).data()
                                                                : nullptr;
}

void Gradient::clearStops(QQmlListProperty<GradientStop>* list) {
    auto* self = static_cast<Gradient*>(list->object);
    if (self) self->clearStops();
}

void Gradient::clearStops() {
    for (const auto& stop : m_stops) {
        if (stop) disconnect(stop.data(), nullptr, this, nullptr);
    }
    m_stops.clear();
    m_sortedStops.clear();
    m_stopsDirty = true;
    emit updated();
}

void Gradient::setOffset(qreal offset) {
    if (qFuzzyCompare(m_offset, offset)) return;
    m_offset = offset;
    emit offsetChanged();
    emit updated();
}

void Gradient::setOrientation(Orientation orientation) {
    if (m_orientation == orientation) return;
    m_orientation = orientation;
    emit orientationChanged();
    emit updated();
}

uint32_t Gradient::colorArgbAt(qreal t) const {
    if (m_stopsDirty) {
        m_sortedStops.clear();
        m_sortedStops.reserve(m_stops.size());
        for (const auto& stop : m_stops) {
            if (stop) m_sortedStops.append(stop.data());
        }
        std::sort(m_sortedStops.begin(), m_sortedStops.end(),
                  [](const GradientStop* a, const GradientStop* b) {
                      return a->position() < b->position();
                  });
        m_stopsDirty = false;
    }

    const QList<GradientStop*>& sorted = m_sortedStops;
    if (sorted.isEmpty()) {
        return Color::DefaultTag;
    }
    if (sorted.size() == 1) {
        return sorted.first()->resolvedColor();
    }

    qreal offset = std::fmod(m_offset, 1.0);
    if (offset < 0.0) offset += 1.0;

    qreal pos = std::clamp(t, 0.0, 1.0) + offset;
    if (pos > 1.0) pos -= 1.0;

    if (pos <= sorted.first()->position()) {
        return sorted.first()->resolvedColor();
    }
    if (pos >= sorted.last()->position()) {
        return sorted.last()->resolvedColor();
    }

    for (int i = 0; i < sorted.size() - 1; ++i) {
        qreal p1 = sorted[i]->position();
        qreal p2 = sorted[i + 1]->position();
        if (pos >= p1 && pos <= p2) {
            qreal span = p2 - p1;
            qreal factor = (span > 0.0001) ? (pos - p1) / span : 0.0;
            uint32_t c1 = sorted[i]->resolvedColor();
            uint32_t c2 = sorted[i + 1]->resolvedColor();

            c1 = Color::toArgb(c1);
            c2 = Color::toArgb(c2);
            int r1 = (c1 >> 16) & 0xFF;
            int g1 = (c1 >> 8) & 0xFF;
            int b1 = c1 & 0xFF;

            int r2 = (c2 >> 16) & 0xFF;
            int g2 = (c2 >> 8) & 0xFF;
            int b2 = c2 & 0xFF;

            int r = std::clamp(static_cast<int>(std::round(r1 + factor * (r2 - r1))), 0, 255);
            int g = std::clamp(static_cast<int>(std::round(g1 + factor * (g2 - g1))), 0, 255);
            int b = std::clamp(static_cast<int>(std::round(b1 + factor * (b2 - b1))), 0, 255);

            return 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    return sorted.last()->resolvedColor();
}

} // namespace qmltui
