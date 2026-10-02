#include "State.h"

namespace qtui {

// ==========================================
// PropertyChanges Class
// ==========================================

PropertyChanges::PropertyChanges(QObject* parent) : QObject(parent) {
}

void PropertyChanges::setTarget(QObject* t) {
    if (m_target.data() == t) return;
    m_target = t;
    emit targetChanged();
}

void PropertyChanges::setProperties(const QVariantMap& p) {
    if (m_properties == p) return;
    m_properties = p;
    emit propertiesChanged();
}

void PropertyChanges::setRestoreEntryValues(bool r) {
    if (m_restoreEntryValues == r) return;
    m_restoreEntryValues = r;
    emit restoreEntryValuesChanged();
}

// ==========================================
// State Class
// ==========================================

State::State(QObject* parent) : QObject(parent) {
}

void State::setName(const QString& n) {
    if (m_name == n) return;
    m_name = n;
    emit nameChanged();
}

void State::setWhen(bool w) {
    if (m_when == w) return;
    m_when = w;
    emit whenChanged();
}

QQmlListProperty<QObject> State::changes() {
    return QQmlListProperty<QObject>(
        this,
        nullptr,
        &State::appendChange,
        &State::changeCount,
        &State::changeAt,
        &State::clearChanges
    );
}

void State::appendChange(QQmlListProperty<QObject>* list, QObject* obj) {
    auto* self = qobject_cast<State*>(list->object);
    if (self && obj) {
        self->m_changesList.append(obj);
    }
}

qsizetype State::changeCount(QQmlListProperty<QObject>* list) {
    auto* self = qobject_cast<State*>(list->object);
    return self ? self->m_changesList.count() : 0;
}

QObject* State::changeAt(QQmlListProperty<QObject>* list, qsizetype index) {
    auto* self = qobject_cast<State*>(list->object);
    return self ? self->m_changesList.at(index) : nullptr;
}

void State::clearChanges(QQmlListProperty<QObject>* list) {
    auto* self = qobject_cast<State*>(list->object);
    if (self) {
        self->m_changesList.clear();
    }
}

} // namespace qtui
