#include <QmlTui/Core/Engine/State.h>
#include <QmlTui/Core/Engine/PropertyChanges_p.h>

namespace qmltui {

PropertyChanges::PropertyChanges(QObject* parent)
    : QObject(parent), d(std::make_unique<PropertyChangesPrivate>()) {
    d->q = this;
}

PropertyChanges::~PropertyChanges() = default;

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

bool PropertyChanges::isExplicit() const {
    return d->isExplicit;
}

void PropertyChanges::setIsExplicit(bool e) {
    if (d->isExplicit == e) return;
    d->isExplicit = e;
    emit explicitChanged();
}

State::State(QObject* parent) : QObject(parent) {}

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

QQmlListProperty<QObject> State::changes() const {
    return QQmlListProperty<QObject>(const_cast<State*>(this), nullptr, &State::appendChange,
                                     &State::changeCount, &State::changeAt, &State::clearChanges);
}

void State::appendChange(QQmlListProperty<QObject>* list, QObject* obj) {
    auto* self = qobject_cast<State*>(list->object);
    if (self && obj) {
        self->m_changesList.append(obj);
    }
}

qsizetype State::changeCount(QQmlListProperty<QObject>* list) {
    auto* self = qobject_cast<State*>(list->object);
    return self ? self->m_changesList.size() : 0;
}

QObject* State::changeAt(QQmlListProperty<QObject>* list, qsizetype index) {
    auto* self = qobject_cast<State*>(list->object);
    return (self && index >= 0 && index < self->m_changesList.size())
               ? self->m_changesList.at(index).data()
               : nullptr;
}

void State::clearChanges(QQmlListProperty<QObject>* list) {
    auto* self = qobject_cast<State*>(list->object);
    if (self) {
        self->m_changesList.clear();
    }
}

QList<PropertyChanges*> State::propertyChangesList() const {
    QList<PropertyChanges*> result;
    for (const QPointer<QObject>& changeObj : m_changesList) {
        if (changeObj) {
            if (auto* pc = qobject_cast<PropertyChanges*>(changeObj.data())) {
                result.append(pc);
            }
        }
    }
    return result;
}

} // namespace qmltui
