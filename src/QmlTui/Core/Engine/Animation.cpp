#include <QmlTui/Core/Engine/Animation.h>
#include <QmlTui/Core/Engine/Clock_p.h>
#include <QmlTui/Core/Engine/QmlPrivateBridge_p.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QMetaProperty>
#include <QMetaType>
#include <QThreadStorage>
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

Animation::Animation(QObject* parent) : QObject(parent) {}

Animation::~Animation() {
    if (m_running) {
        if (AnimationManager* mgr = AnimationManager::existingInstance()) {
            mgr->unregisterAnimation(this);
        }
    }
}

void Animation::setRunning(bool r) {
    if (m_running == r) return;
    m_running = r;
    emit runningChanged();

    if (m_running) {
        AnimationManager::instance()->registerAnimation(this);
        emit started();
    } else {
        if (AnimationManager* mgr = AnimationManager::existingInstance()) {
            mgr->unregisterAnimation(this);
        }
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

PropertyAnimation::PropertyAnimation(QObject* parent) : Animation(parent) {
    connect(this, &Animation::started, this, [this]() {
        // Behavior may stop and restart an animation at an intermediate value.
        // The new interpolation must not inherit the previous elapsed time.
        m_initialized = false;
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
    const int clamped = d < 0 ? 0 : d;
    if (m_duration == clamped) return;
    m_duration = clamped;
    emit durationChanged();
}

void PropertyAnimation::setRunning(bool r) {
    if (m_running == r) return;
    if (r && m_duration == 0) {
        QPointer<PropertyAnimation> self(this);
        m_running = false;
        emit runningChanged();
        if (!self) return;
        writeValue(1.0);
        if (!self) return;
        emit started();
        if (!self) return;
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

    if (!m_initialized) {
        m_initialized = true;
        m_accumulatedTime = 0;
    } else {
        m_accumulatedTime += elapsedMs;
    }

    QPointer<PropertyAnimation> self(this);
    if (m_duration <= 0) {
        writeValue(1.0);
        if (!self) return;
        m_initialized = false;
        setRunning(false);
        if (!self) return;
        emit finished();
        return;
    }

    bool finishedAnimation = false;
    double t = static_cast<double>(m_accumulatedTime) / m_duration;

    if (t >= 1.0) {
        if (m_loops < 0) {
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
    writeValue(easedT);
    if (!self) return;

    if (finishedAnimation) {
        m_initialized = false;
        setRunning(false);
        if (!self) return;
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

NumberAnimation::NumberAnimation(QObject* parent) : PropertyAnimation(parent) {}

ColorAnimation::ColorAnimation(QObject* parent) : PropertyAnimation(parent) {}

static uint32_t extractColor(const QVariant& var, uint32_t defaultColor = 0xFFFFFFFF) {
    if (var.userType() == QMetaType::fromType<Color>().id()) {
        Color c = var.value<Color>();
        if (c.isValid()) return c.argb32();
    }
    if (var.userType() == QMetaType::QString) {
        Color c = Color::fromString(var.toString());
        if (c.isValid()) return c.argb32();
    }
    return Color::resolve(var.toString(), defaultColor);
}

static Color extractExactColor(const QVariant& var, uint32_t defaultColor = 0xFFFFFFFF) {
    if (var.userType() == QMetaType::fromType<Color>().id()) {
        return var.value<Color>();
    }
    if (var.userType() == QMetaType::QString) {
        return Color::fromString(var.toString());
    }
    return Color::fromArgb32(Color::resolve(var.toString(), defaultColor));
}

void ColorAnimation::writeValue(const QVariant& value) {
    if (!m_target || m_propertyName.isEmpty()) return;

    const double t = value.toDouble();
    Color animatedColor;
    if (t >= 1.0) {
        // Commit the exact destination Color so Transparent, Invalid, Default,
        // and ANSI palette tags survive completion without RGB loss.
        animatedColor = extractExactColor(m_to, 0xFFFFFFFF);
    } else {
        // ANSI-indexed colors carry no RGB; interpolate their palette values.
        const uint32_t fromColor = Color::toArgb(extractColor(m_from, 0xFFFFFFFF));
        const uint32_t toColor = Color::toArgb(extractColor(m_to, 0xFFFFFFFF));

        const int fromA = (fromColor >> 24) & 0xFF;
        const int fromR = (fromColor >> 16) & 0xFF;
        const int fromG = (fromColor >> 8) & 0xFF;
        const int fromB = fromColor & 0xFF;

        const int toA = (toColor >> 24) & 0xFF;
        const int toR = (toColor >> 16) & 0xFF;
        const int toG = (toColor >> 8) & 0xFF;
        const int toB = toColor & 0xFF;

        const int currA = std::clamp(static_cast<int>(fromA + t * (toA - fromA)), 0, 255);
        const int currR = std::clamp(static_cast<int>(fromR + t * (toR - fromR)), 0, 255);
        const int currG = std::clamp(static_cast<int>(fromG + t * (toG - fromG)), 0, 255);
        const int currB = std::clamp(static_cast<int>(fromB + t * (toB - fromB)), 0, 255);

        const uint32_t currArgb =
            (static_cast<uint32_t>(currA) << 24) | (static_cast<uint32_t>(currR) << 16) |
            (static_cast<uint32_t>(currG) << 8) | static_cast<uint32_t>(currB);
        animatedColor = Color::fromArgb32(currArgb);
    }

    QVariant currentProp = m_target->property(m_propertyName.toUtf8().constData());
    if (currentProp.userType() == QMetaType::fromType<Color>().id() &&
        currentProp.value<Color>() == animatedColor)
        return;
    if (currentProp.userType() == QMetaType::QString &&
        currentProp.toString() == animatedColor.toString())
        return;

    writePropertyBypassingInterceptor(m_target.data(), m_propertyName,
                                      QVariant::fromValue(animatedColor));
}

AnimationManager::AnimationManager(QObject* parent) : QObject(parent) {
    m_timer = new ClockTimer(this);
    connect(m_timer, &ClockTimer::timeout, this, &AnimationManager::handleTick);
}

namespace {

QThreadStorage<AnimationManager*>& animationManagerStorage() {
    static QThreadStorage<AnimationManager*> managers;
    return managers;
}

} // namespace

AnimationManager* AnimationManager::instance() {
    QThreadStorage<AnimationManager*>& managers = animationManagerStorage();
    if (!managers.hasLocalData()) {
        managers.setLocalData(new AnimationManager());
    }
    return managers.localData();
}

AnimationManager* AnimationManager::existingInstance() {
    QThreadStorage<AnimationManager*>& managers = animationManagerStorage();
    return managers.hasLocalData() ? managers.localData() : nullptr;
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
