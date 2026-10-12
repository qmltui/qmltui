#include <QmlTui/Core/Engine/Animation.h>
#include <QmlTui/Core/Engine/Clock_p.h>
#include <QmlTui/Core/Primitives/ColorMapper.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QDebug>
#include <QMetaProperty>
#include <QMetaType>
#include <QPointer>
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
#include <QtQml/private/qqmlproperty_p.h>
#endif
#include <algorithm>
#include <cmath>

namespace qmltui {

static double applyEasing(Easing::Type type, double t) {
    switch (type) {
        case Easing::Linear:
            return t;
        case Easing::InQuad:
            return t * t;
        case Easing::OutQuad:
            return t * (2.0 - t);
        case Easing::InOutQuad:
            return t < 0.5 ? 2.0 * t * t : -1.0 + (4.0 - 2.0 * t) * t;
        case Easing::InCubic:
            return t * t * t;
        case Easing::OutCubic: {
            double f = t - 1.0;
            return f * f * f + 1.0;
        }
        case Easing::InOutCubic: {
            if (t < 0.5) return 4.0 * t * t * t;
            double f = 2.0 * t - 2.0;
            return 0.5 * f * f * f + 1.0;
        }
        case Easing::OutBounce: {
            if (t < 1.0 / 2.75) {
                return 7.5625 * t * t;
            } else if (t < 2.0 / 2.75) {
                t -= 1.5 / 2.75;
                return 7.5625 * t * t + 0.75;
            } else if (t < 2.5 / 2.75) {
                t -= 2.25 / 2.75;
                return 7.5625 * t * t + 0.9375;
            } else {
                t -= 2.625 / 2.75;
                return 7.5625 * t * t + 0.984375;
            }
        }
    }
    return t;
}

static void writePropertyBypassingInterceptor(QObject* target, const QString& name, const QVariant& value) {
    if (!target || name.isEmpty()) return;
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    // Dynamic QML properties have no C++ static_metacall. A regular property
    // write would re-enter Behavior and remove the property's source binding.
    const QQmlProperty property(target, name);
    const QVariant converted = property.propertyType() == QMetaType::QString
            && value.metaType() == QMetaType::fromType<Color>()
        ? QVariant(value.value<Color>().toString()) : value;
    QQmlPropertyPrivate::write(property, converted,
        QQmlPropertyData::BypassInterceptor | QQmlPropertyData::DontRemoveBinding);
#else
    const QMetaObject* meta = target->metaObject();
    int idx = meta->indexOfProperty(name.toUtf8().constData());
    if (idx >= 0) {
        QMetaProperty prop = meta->property(idx);
        
        // Find the C++ class QMetaObject defining the property to get its C++ static_metacall
        const QMetaObject* definingMeta = target->metaObject();
        while (definingMeta) {
            const QMetaObject* super = definingMeta->superClass();
            if (!super || idx >= super->propertyCount()) {
                break;
            }
            definingMeta = super;
        }

        if (definingMeta && definingMeta->d.static_metacall) {
            int relativeId = idx;
            if (definingMeta->superClass()) {
                relativeId -= definingMeta->superClass()->propertyCount();
            }

            QMetaType mType = prop.metaType();
            int typeId = mType.id();

            if (typeId == QMetaType::Int) {
                int val = value.toInt();
                void* argv[] = { &val };
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId, argv);
            } else if (typeId == QMetaType::Double) {
                double val = value.toDouble();
                void* argv[] = { &val };
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId, argv);
            } else if (typeId == QMetaType::QString) {
                QString val = (value.userType() == QMetaType::fromType<Color>().id())
                                  ? value.value<Color>().toString()
                                  : value.toString();
                void* argv[] = { &val };
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId, argv);
            } else if (typeId == QMetaType::fromType<Color>().id()) {
                Color col;
                if (value.userType() == typeId) {
                    col = value.value<Color>();
                } else if (value.userType() == QMetaType::QString) {
                    col = Color::fromString(value.toString());
                } else {
                    col = Color::fromArgb32(ColorMapper::fromString(value.toString()));
                }
                void* argv[] = { &col };
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId, argv);
            } else if (typeId == QMetaType::Bool) {
                bool val = value.toBool();
                void* argv[] = { &val };
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId, argv);
            } else {
                void* argv[] = { const_cast<void*>(value.constData()) };
                definingMeta->d.static_metacall(target, QMetaObject::WriteProperty, relativeId, argv);
            }
        } else {
            // Fallback
            prop.write(target, value);
        }
    }
#endif
}

// ==========================================
// Animation Base Class
// ==========================================

Animation::Animation(QObject* parent) : QObject(parent) {
}

Animation::~Animation() {
    AnimationManager::instance()->unregisterAnimation(this);
}

void Animation::setRunning(bool r) {
    if (m_running == r) return;
    m_running = r;
    emit runningChanged();

    if (m_running) {
        AnimationManager::instance()->registerAnimation(this);
        emit started();
    } else {
        AnimationManager::instance()->unregisterAnimation(this);
        emit stopped();
    }
}

void Animation::setLoops(int l) {
    if (m_loops == l) return;
    m_loops = l;
    emit loopsChanged();
}

void Animation::start() {
    setRunning(true);
}

void Animation::stop() {
    setRunning(false);
}

// ==========================================
// PropertyAnimation Class
// ==========================================

PropertyAnimation::PropertyAnimation(QObject* parent) : Animation(parent) {
    connect(this, &Animation::started, this, [this]() {
        // Behavior may stop and restart an animation at an intermediate value.
        // The new interpolation must not inherit the previous elapsed time.
        m_timer.invalidate();
        m_accumulatedTime = 0;
    });
}

void PropertyAnimation::setTarget(QObject* t) {
    if (m_target.data() == t) return;
    m_target = t;
    emit targetChanged();
}

void PropertyAnimation::setProperty(const QString& name) {
    if (m_propertyName == name) return;
    m_propertyName = name;
    emit propertyChanged();
}

void PropertyAnimation::setDuration(int d) {
    if (m_duration == d) return;
    m_duration = (std::max)(0, d);
    emit durationChanged();
}

void PropertyAnimation::setRunning(bool r) {
    if (m_running == r) return;
    if (r && m_duration == 0) {
        writeValue(1.0);
        emit started();
        emit finished();
        return;
    }
    Animation::setRunning(r);
}

void PropertyAnimation::setFrom(const QVariant& f) {
    if (m_from == f) return;
    m_from = f;
    emit fromChanged();
}

void PropertyAnimation::setTo(const QVariant& t) {
    if (m_to == t) return;
    m_to = t;
    emit toChanged();
}

void PropertyAnimation::setEasing(Easing::Type e) {
    if (m_easing == e) return;
    m_easing = e;
    emit easingChanged();
}

void PropertyAnimation::tick(qint64 elapsedMs) {
    if (!m_running) return;

    if (!m_timer.isValid()) {
        m_timer.start();
        m_accumulatedTime = 0;
    } else {
        m_accumulatedTime += elapsedMs;
    }

    if (m_duration <= 0) {
        writeValue(1.0);
        m_timer.invalidate();
        setRunning(false);
        emit finished();
        return;
    }

    bool finishedAnimation = false;
    double t = static_cast<double>(m_accumulatedTime) / m_duration;

    if (t >= 1.0) {
        if (m_loops == -1) {
            // Infinite looping
            m_accumulatedTime = m_accumulatedTime % m_duration;
            t = static_cast<double>(m_accumulatedTime) / m_duration;
        } else if (m_loops > 1) {
            int completedLoops = static_cast<int>(m_accumulatedTime / m_duration);
            if (completedLoops < m_loops) {
                m_accumulatedTime = m_accumulatedTime % m_duration;
                t = static_cast<double>(m_accumulatedTime) / m_duration;
            } else {
                t = 1.0;
                finishedAnimation = true;
            }
        } else {
            t = 1.0;
            finishedAnimation = true;
        }
    }

    double easedT = applyEasing(m_easing, t);

    // Write the eased value using the virtual method
    writeValue(easedT);

    if (finishedAnimation) {
        m_timer.invalidate();
        setRunning(false);
        emit finished();
    }
}

void PropertyAnimation::writeValue(const QVariant& value) {
    if (!m_target || m_propertyName.isEmpty()) return;

    double easedT = value.toDouble();
    double fromVal = m_from.toDouble();
    double toVal = m_to.toDouble();
    double currentVal = fromVal + easedT * (toVal - fromVal);

    QVariant typedVal = currentVal;
    QVariant currentProp = m_target->property(m_propertyName.toUtf8().constData());
    if (currentProp.typeId() == QMetaType::Int) {
        typedVal = QVariant(static_cast<int>(std::round(currentVal)));
    } else if (currentProp.typeId() == QMetaType::Float) {
        typedVal = QVariant(static_cast<float>(currentVal));
    }

    if (currentProp == typedVal) {
        return;
    }

    writePropertyBypassingInterceptor(m_target.data(), m_propertyName, typedVal);
}

// ==========================================
// NumberAnimation Class
// ==========================================

NumberAnimation::NumberAnimation(QObject* parent) : PropertyAnimation(parent) {
}

// ==========================================
// ColorAnimation Class
// ==========================================

ColorAnimation::ColorAnimation(QObject* parent) : PropertyAnimation(parent) {
}

static uint32_t extractColor(const QVariant& var, uint32_t defaultColor = 0xFFFFFFFF) {
    if (var.userType() == QMetaType::fromType<Color>().id()) {
        Color c = var.value<Color>();
        if (c.isValid())
            return c.argb32();
    }
    if (var.userType() == QMetaType::QString) {
        Color c = Color::fromString(var.toString());
        if (c.isValid())
            return c.argb32();
    }
    return ColorMapper::fromString(var.toString(), defaultColor);
}

void ColorAnimation::writeValue(const QVariant& value) {
    if (!m_target || m_propertyName.isEmpty()) return;

    // ANSI-indexed colors carry no RGB; interpolate their palette values.
    uint32_t fromColor = ColorMapper::toArgb(extractColor(m_from, 0xFFFFFFFF));
    uint32_t toColor = ColorMapper::toArgb(extractColor(m_to, 0xFFFFFFFF));

    int fromA = (fromColor >> 24) & 0xFF;
    int fromR = (fromColor >> 16) & 0xFF;
    int fromG = (fromColor >> 8) & 0xFF;
    int fromB = fromColor & 0xFF;

    int toA = (toColor >> 24) & 0xFF;
    int toR = (toColor >> 16) & 0xFF;
    int toG = (toColor >> 8) & 0xFF;
    int toB = toColor & 0xFF;

    double t = value.toDouble();

    int currA = std::clamp(static_cast<int>(fromA + t * (toA - fromA)), 0, 255);
    int currR = std::clamp(static_cast<int>(fromR + t * (toR - fromR)), 0, 255);
    int currG = std::clamp(static_cast<int>(fromG + t * (toG - fromG)), 0, 255);
    int currB = std::clamp(static_cast<int>(fromB + t * (toB - fromB)), 0, 255);

    uint32_t currArgb = (static_cast<uint32_t>(currA) << 24)
                      | (static_cast<uint32_t>(currR) << 16)
                      | (static_cast<uint32_t>(currG) << 8)
                      | static_cast<uint32_t>(currB);

    Color animatedColor = Color::fromArgb32(currArgb);
    QVariant currentProp = m_target->property(m_propertyName.toUtf8().constData());
    if (currentProp.userType() == QMetaType::fromType<Color>().id() && currentProp.value<Color>() == animatedColor)
        return;
    if (currentProp.userType() == QMetaType::QString && currentProp.toString() == animatedColor.toString())
        return;

    writePropertyBypassingInterceptor(m_target.data(), m_propertyName, QVariant::fromValue(animatedColor));
}

// ==========================================
// AnimationManager Class
// ==========================================

AnimationManager::AnimationManager(QObject* parent) : QObject(parent) {
    m_timer = new ClockTimer(this);
    connect(m_timer, &ClockTimer::timeout, this, &AnimationManager::handleTick);
}

AnimationManager* AnimationManager::instance() {
    static AnimationManager manager;
    return &manager;
}

void AnimationManager::registerAnimation(Animation* anim) {
    if (!anim) return;
    if (!m_animations.contains(anim)) {
        m_animations.append(anim);
    }
    if (!m_timer->isActive()) {
        m_timeOriginMs = Clock::instance()->elapsed();
        m_lastTickTime = 0;
        m_timer->start(16); // ~60 FPS
    }
}

void AnimationManager::unregisterAnimation(Animation* anim) {
    m_animations.removeOne(anim);
    if (m_animations.isEmpty()) {
        m_timer->stop();
    }
}

void AnimationManager::handleTick() {
    qint64 current = Clock::instance()->elapsed() - m_timeOriginMs;
    qint64 delta = current - m_lastTickTime;
    m_lastTickTime = current;

    QList<QPointer<Animation>> activeList;
    activeList.reserve(m_animations.size());
    for (Animation* anim : m_animations) {
        activeList.append(anim);
    }
    for (const QPointer<Animation>& anim : activeList) {
        if (anim && anim->running()) {
            anim->tick(delta);
        }
    }
}

} // namespace qmltui
