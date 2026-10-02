#include "Behavior.h"

namespace qtui {

Behavior::Behavior(QObject* parent) : QObject(parent) {
}

void Behavior::setAnimation(Animation* anim) {
    if (m_animation == anim) return;
    m_animation = anim;
    if (m_animation) {
        const QPointer<Animation> observedAnimation = m_animation;
        connect(m_animation, &QObject::destroyed, this, [this, observedAnimation]() {
            if (m_animation && m_animation != observedAnimation)
                return;
            m_animation.clear();
            emit animationChanged();
        });
    }
    emit animationChanged();

    if (m_animation && m_property.isValid()) {
        PropertyAnimation* propAnim = qobject_cast<PropertyAnimation*>(m_animation);
        if (propAnim) {
            propAnim->setTarget(m_property.object());
            propAnim->setProperty(m_property.name());
        }
    }
}

void Behavior::setTarget(const QQmlProperty& property) {
    m_property = property;

    if (m_animation && m_property.isValid()) {
        PropertyAnimation* propAnim = qobject_cast<PropertyAnimation*>(m_animation);
        if (propAnim) {
            propAnim->setTarget(m_property.object());
            propAnim->setProperty(m_property.name());
        }
    }
}

void Behavior::write(const QVariant& value) {
    if (!m_property.isValid()) return;

    QVariant currentVal = m_property.read();
    if (currentVal == value) return;

    if (!m_animation) {
        m_property.write(value);
        return;
    }

    PropertyAnimation* propAnim = qobject_cast<PropertyAnimation*>(m_animation);
    if (propAnim) {
        // Stop currently running animation to reset it smoothly
        if (propAnim->running()) {
            propAnim->stop();
        }

        // Set the target, property name, from value, to value, and start
        propAnim->setTarget(m_property.object());
        propAnim->setProperty(m_property.name());
        propAnim->setFrom(m_property.read()); // Start from current intermediate value
        propAnim->setTo(value);
        propAnim->start();
    } else {
        // Fallback if not a PropertyAnimation
        m_property.write(value);
    }
}

} // namespace qtui
