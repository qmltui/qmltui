#include "Transition.h"

namespace qtui {

Transition::Transition(QObject* parent) : QObject(parent) {
}

void Transition::setFrom(const QString& f) {
    if (m_from == f) return;
    m_from = f;
    emit fromChanged();
}

void Transition::setTo(const QString& t) {
    if (m_to == t) return;
    m_to = t;
    emit toChanged();
}

bool Transition::matches(const QString& fromState, const QString& toState) const {
    bool fromOk = (m_from == "*") || (m_from == fromState);
    bool toOk = (m_to == "*") || (m_to == toState);
    return fromOk && toOk;
}

QQmlListProperty<Animation> Transition::animations() {
    return QQmlListProperty<Animation>(
        this,
        nullptr,
        &Transition::appendAnimation,
        &Transition::animationCount,
        &Transition::animationAt,
        &Transition::clearAnimations
    );
}

void Transition::appendAnimation(QQmlListProperty<Animation>* list, Animation* anim) {
    auto* self = qobject_cast<Transition*>(list->object);
    if (self && anim) {
        self->m_animationsList.append(anim);
    }
}

qsizetype Transition::animationCount(QQmlListProperty<Animation>* list) {
    auto* self = qobject_cast<Transition*>(list->object);
    return self ? self->m_animationsList.count() : 0;
}

Animation* Transition::animationAt(QQmlListProperty<Animation>* list, qsizetype index) {
    auto* self = qobject_cast<Transition*>(list->object);
    return self ? self->m_animationsList.at(index) : nullptr;
}

void Transition::clearAnimations(QQmlListProperty<Animation>* list) {
    auto* self = qobject_cast<Transition*>(list->object);
    if (self) {
        self->m_animationsList.clear();
    }
}

} // namespace qtui
