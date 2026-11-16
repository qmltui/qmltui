#include <QmlTui/Core/Engine/StateController_p.h>
#include <QmlTui/Core/Engine/State.h>
#include <QmlTui/Core/Engine/PropertyChanges_p.h>
#include <QmlTui/Core/Engine/Transition.h>
#include <QmlTui/Core/Engine/Animation.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QChildEvent>
#include <QQmlInfo>
#include <QQmlProperty>
#include <QQmlContext>
#include <QMetaProperty>

namespace qmltui {

thread_local QMap<CanonicalProperty, StateController*> StateController::s_propertyOwners;

namespace {

struct ResolvedProperty {
    CanonicalProperty key;
    QObject* target = nullptr;
};

ResolvedProperty resolveCanonicalProperty(QObject* target, const QString& name) {
    ResolvedProperty result;
    result.target = target;
    result.key.targetId = reinterpret_cast<quintptr>(target);
    result.key.name = name;

    if (target && !name.isEmpty()) {
        QQmlProperty prop(target, name);
        if (prop.isValid()) {
            QPointer<QObject> aliasTarget;
            QString aliasName;
            if (resolvePropertyAlias(prop, aliasTarget, aliasName) && aliasTarget) {
                result.target = aliasTarget.data();
                result.key.targetId = reinterpret_cast<quintptr>(result.target);
                result.key.name = aliasName;
                return result;
            }
            result.target = prop.object();
            result.key.targetId = reinterpret_cast<quintptr>(result.target);
            result.key.name = prop.name();
        }
    }

    return result;
}

} // namespace

StateController::StateController(QObject* parent) : QObject(parent) {}

StateController::~StateController() {
    releaseAllOwnership();
    for (const QPointer<Animation>& anim : m_runningTransitionAnimations) {
        if (anim) {
            anim->stop();
            delete anim.data();
        }
    }
    m_runningTransitionAnimations.clear();
}

void StateController::releaseAllOwnership() {
    for (const CanonicalProperty& prop : m_ownedProperties) {
        if (s_propertyOwners.value(prop) == this) {
            s_propertyOwners.remove(prop);
        }
    }
    m_ownedProperties.clear();
}

void StateController::watchTarget(QObject* target) {
    if (!target || m_watchedTargets.contains(target)) return;
    m_watchedTargets.insert(target);
    connect(target, &QObject::destroyed, this, &StateController::handleTargetDestroyed);
}

void StateController::handleTargetDestroyed(QObject* obj) {
    m_watchedTargets.remove(obj);
    const quintptr deadId = reinterpret_cast<quintptr>(obj);
    for (qsizetype i = m_runningTransitionAnimations.size() - 1; i >= 0; --i) {
        Animation* anim = m_runningTransitionAnimations.at(i).data();
        if (!anim) {
            m_runningTransitionAnimations.removeAt(i);
            continue;
        }
        auto* pAnim = qobject_cast<PropertyAnimation*>(anim);
        if (pAnim && (!pAnim->target() || pAnim->target() == obj)) {
            m_runningTransitionAnimations.removeAt(i);
            anim->stop();
            delete anim;
        }
    }
    for (auto it = m_baseRecords.begin(); it != m_baseRecords.end();) {
        if (it.key().targetId == deadId)
            it = m_baseRecords.erase(it);
        else
            ++it;
    }
    for (auto it = m_ownedProperties.begin(); it != m_ownedProperties.end();) {
        if (it->targetId == deadId) {
            if (s_propertyOwners.value(*it) == this) s_propertyOwners.remove(*it);
            it = m_ownedProperties.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_appliedCustomProperties.begin(); it != m_appliedCustomProperties.end(); ++it) {
        QSet<CanonicalProperty>& keys = it.value();
        for (auto kit = keys.begin(); kit != keys.end();) {
            if (kit->targetId == deadId)
                kit = keys.erase(kit);
            else
                ++kit;
        }
    }
}

void StateController::childEvent(QChildEvent* event) {
    QObject::childEvent(event);
    if (!event->removed()) return;
    QObject* child = event->child();
    bool removedState = false;
    for (qsizetype i = m_statesList.size() - 1; i >= 0; --i) {
        const QPointer<State>& entry = m_statesList.at(i);
        if (entry.isNull() || static_cast<QObject*>(entry.data()) == child) {
            if (State* liveState = entry.data()) {
                for (PropertyChanges* pc : liveState->propertyChangesList()) {
                    if (pc) {
                        disconnect(pc, nullptr, this, nullptr);
                        m_appliedMaps.remove(pc);
                        m_appliedTargets.remove(pc);
                        m_appliedCustomProperties.remove(pc);
                    }
                }
                disconnect(liveState, nullptr, this, nullptr);
            }
            m_statesList.removeAt(i);
            removedState = true;
        }
    }
    for (qsizetype i = m_transitionsList.size() - 1; i >= 0; --i) {
        const QPointer<Transition>& entry = m_transitionsList.at(i);
        if (entry.isNull() || static_cast<QObject*>(entry.data()) == child) {
            m_transitionsList.removeAt(i);
        }
    }
    if (removedState) evaluateStateConditions();
}

void StateController::setDefaultTarget(QObject* target) {
    m_defaultTarget = target;
}

void StateController::setComponentComplete(bool complete) {
    m_componentComplete = complete;
}

QQmlListProperty<State> StateController::statesProperty() {
    return QQmlListProperty<State>(this, nullptr, &StateController::appendState,
                                   &StateController::stateCount, &StateController::stateAt,
                                   &StateController::clearStates);
}

void StateController::appendState(QQmlListProperty<State>* list, State* state) {
    auto* self = qobject_cast<StateController*>(list->object);
    if (self && state) {
        state->setParent(self);
        self->m_statesList.append(state);
        connect(state, &State::whenChanged, self, &StateController::evaluateStateConditions);

        for (PropertyChanges* pc : state->propertyChangesList()) {
            connect(pc, &PropertyChanges::propertiesChanged, self,
                    [self, pc]() { self->handlePropertiesChanged(pc); });
            connect(pc, &PropertyChanges::targetChanged, self,
                    [self, pc]() { self->handleTargetChanged(pc); });
        }

        self->evaluateStateConditions();
    }
}

qsizetype StateController::stateCount(QQmlListProperty<State>* list) {
    auto* self = qobject_cast<StateController*>(list->object);
    return self ? self->m_statesList.size() : 0;
}

State* StateController::stateAt(QQmlListProperty<State>* list, qsizetype index) {
    auto* self = qobject_cast<StateController*>(list->object);
    return (self && index >= 0 && index < self->m_statesList.size())
               ? self->m_statesList.at(index).data()
               : nullptr;
}

void StateController::clearStates(QQmlListProperty<State>* list) {
    auto* self = qobject_cast<StateController*>(list->object);
    if (self) {
        const QList<QPointer<State>> states = self->m_statesList;
        self->m_statesList.clear();
        for (const QPointer<State>& state : states) {
            if (state) {
                for (PropertyChanges* pc : state->propertyChangesList()) {
                    if (pc) {
                        disconnect(pc, nullptr, self, nullptr);
                        self->m_appliedMaps.remove(pc);
                        self->m_appliedTargets.remove(pc);
                        self->m_appliedCustomProperties.remove(pc);
                    }
                }
                disconnect(state, nullptr, self, nullptr);
                if (state->parent() == self) state->setParent(nullptr);
            }
        }
        self->evaluateStateConditions();
    }
}

QQmlListProperty<Transition> StateController::transitionsProperty() {
    return QQmlListProperty<Transition>(
        this, nullptr, &StateController::appendTransition, &StateController::transitionCount,
        &StateController::transitionAt, &StateController::clearTransitions);
}

void StateController::appendTransition(QQmlListProperty<Transition>* list, Transition* transition) {
    auto* self = qobject_cast<StateController*>(list->object);
    if (self && transition) {
        transition->setParent(self);
        self->m_transitionsList.append(transition);
    }
}

qsizetype StateController::transitionCount(QQmlListProperty<Transition>* list) {
    auto* self = qobject_cast<StateController*>(list->object);
    return self ? self->m_transitionsList.size() : 0;
}

Transition* StateController::transitionAt(QQmlListProperty<Transition>* list, qsizetype index) {
    auto* self = qobject_cast<StateController*>(list->object);
    return (self && index >= 0 && index < self->m_transitionsList.size())
               ? self->m_transitionsList.at(index).data()
               : nullptr;
}

void StateController::clearTransitions(QQmlListProperty<Transition>* list) {
    auto* self = qobject_cast<StateController*>(list->object);
    if (self) {
        const QList<QPointer<Transition>> transitions = self->m_transitionsList;
        self->m_transitionsList.clear();
        for (const QPointer<Transition>& transition : transitions) {
            if (transition && transition->parent() == self) transition->setParent(nullptr);
        }
    }
}

void StateController::stateRemoved(State* state) {
    if (!state) return;
    for (PropertyChanges* pc : state->propertyChangesList()) {
        if (pc) {
            disconnect(pc, nullptr, this, nullptr);
            m_appliedMaps.remove(pc);
            m_appliedTargets.remove(pc);
            m_appliedCustomProperties.remove(pc);
        }
    }
    disconnect(state, nullptr, this, nullptr);
    m_statesList.removeIf(
        [state](const QPointer<State>& s) { return s.isNull() || s.data() == state; });
    evaluateStateConditions();
}

void StateController::transitionRemoved(Transition* transition) {
    if (!transition) return;
    m_transitionsList.removeIf([transition](const QPointer<Transition>& t) {
        return t.isNull() || t.data() == transition;
    });
}

void StateController::evaluateStateConditions() {
    if (!m_componentComplete) return;

    State* activeWhenState = nullptr;
    for (const QPointer<State>& state : m_statesList) {
        State* s = state.data();
        if (s && s->when()) {
            activeWhenState = s;
            break;
        }
    }

    if (activeWhenState) {
        m_isAutomatic = true;
        m_automaticState = activeWhenState->name();
        m_evaluatingWhenConditions = true;
        setState(activeWhenState->name());
        m_evaluatingWhenConditions = false;
    } else {
        if (m_isAutomatic) {
            m_isAutomatic = false;
            m_automaticState.clear();
            m_evaluatingWhenConditions = true;
            setState(QString());
            m_evaluatingWhenConditions = false;
        }
        // If not automatically selected, the user manually set a state:
        // unrelated when condition changes must not clear it!
    }
}

void StateController::setState(const QString& newState) {
    if (!m_evaluatingWhenConditions) {
        m_isAutomatic = false;
        m_automaticState.clear();
    }

    m_pendingState = newState;
    if (m_updatingState) return;

    struct StateGuard {
        bool& flag;
        ~StateGuard() { flag = false; }
    } guard{m_updatingState};
    m_updatingState = true;

    while (m_pendingState) {
        const QString requestedState = std::move(*m_pendingState);
        m_pendingState.reset();
        if (m_stateName != requestedState) {
            applyState(requestedState);
        }
    }
}

void StateController::collectCustomOverrides(
    PropertyChanges* pc, QMap<CanonicalProperty, PropertyOverride>& outOverrides,
    QSet<CanonicalProperty>* outAppliedKeys) {
    if (!pc || !pc->d_func() || !pc->d_func()->hasCustomBindings) return;

    pc->d_func()->decode();
    for (const DecodedBinding& b : pc->d_func()->decodedBindings) {
        QObject* baseTarget = pc->target() ? pc->target() : defaultTarget();
        if (!pc->target() && b.propertyName.contains(QLatin1Char('.'))) {
            const QString firstPart = b.propertyName.section(QLatin1Char('.'), 0, 0);
            if (baseTarget && !firstPart.isEmpty() && !firstPart.at(0).isUpper() &&
                baseTarget->metaObject()->indexOfProperty(firstPart.toUtf8().constData()) < 0) {
                baseTarget = nullptr;
            }
        }
        if (!baseTarget && !b.propertyName.contains(QLatin1Char('.'))) {
            qmlWarning(pc) << "PropertyChanges inside StateGroup requires explicit target "
                              "or ID-qualified property";
            continue;
        }

        QQmlContext* rawContext = qmlContext(pc);
        if (!rawContext && parent()) rawContext = qmlContext(parent());
        if (!rawContext && baseTarget) rawContext = qmlContext(baseTarget);

        QQmlProperty prop = createQmlProperty(baseTarget, b.propertyName, rawContext);
        if (!prop.isValid()) {
            qmlWarning(pc) << "Cannot assign to non-existent property \"" << b.propertyName << "\"";
            continue;
        }
        if (!prop.isWritable()) {
            qmlWarning(pc) << "Cannot assign to read-only property \"" << b.propertyName << "\"";
            continue;
        }

        const ResolvedProperty resolved = resolveCanonicalProperty(prop.object(), prop.name());
        if (!resolved.target) continue;
        PropertyOverride override;
        override.target = resolved.target;
        override.name = resolved.key.name;
        override.restore = pc->restoreEntryValues();

        if (b.isExpression) {
            evaluateDecodedBinding(pc, b, prop, rawContext, override.value, override.binding,
                                   override.isBinding);
        } else {
            override.value = b.literalValue;
            override.isBinding = false;
        }

        outOverrides.insert(resolved.key, override);
        if (outAppliedKeys) outAppliedKeys->insert(resolved.key);
    }
}

void StateController::collectOverridesForState(
    State* state, QMap<CanonicalProperty, PropertyOverride>& outOverrides) {
    if (!state) return;

    for (PropertyChanges* pc : state->propertyChangesList()) {
        if (!pc) continue;

        if (!pc->properties().isEmpty() && pc->d_func() && pc->d_func()->hasCustomBindings) {
            qmlWarning(pc) << "Cannot mix properties map and direct assignments in PropertyChanges";
            continue;
        }

        if (pc->d_func() && pc->d_func()->hasCustomBindings) {
            m_appliedTargets[pc] = pc->target() ? pc->target() : defaultTarget();
            QSet<CanonicalProperty> appliedKeys;
            collectCustomOverrides(pc, outOverrides, &appliedKeys);
            m_appliedCustomProperties[pc] = appliedKeys;
            continue;
        }

        // Process legacy properties map
        QObject* target = pc->target() ? pc->target() : defaultTarget();
        if (!target) {
            if (!pc->properties().isEmpty()) {
                qmlWarning(pc) << "PropertyChanges inside StateGroup requires explicit target";
            }
            continue;
        }

        const QVariantMap props = pc->properties();
        m_appliedMaps[pc] = props;
        m_appliedTargets[pc] = target;
        for (auto it = props.begin(); it != props.end(); ++it) {
            const QString propName = it.key();
            const QVariant val = it.value();

            QQmlContext* rawContext = qmlContext(pc);
            if (!rawContext && parent()) rawContext = qmlContext(parent());
            if (!rawContext && target) rawContext = qmlContext(target);

            QQmlProperty prop = createQmlProperty(target, propName, rawContext);
            if (!prop.isValid()) {
                qmlWarning(pc) << "Cannot assign to non-existent property \"" << propName << "\"";
                continue;
            }
            if (!prop.isWritable()) {
                qmlWarning(pc) << "Cannot assign to read-only property \"" << propName << "\"";
                continue;
            }
            const ResolvedProperty resolved = resolveCanonicalProperty(prop.object(), prop.name());
            if (!resolved.target) continue;
            PropertyOverride override;
            override.target = resolved.target;
            override.name = resolved.key.name;
            override.value = val;
            override.restore = pc->restoreEntryValues();
            override.isBinding = false;
            outOverrides.insert(resolved.key, override);
        }
    }
}

void StateController::handlePropertiesChanged(PropertyChanges* pc) {
    if (!pc) return;

    // Check if the state containing this PropertyChanges is currently active
    bool isActive = false;
    for (const QPointer<State>& state : m_statesList) {
        if (state && state->name() == m_stateName) {
            if (state->propertyChangesList().contains(pc)) {
                isActive = true;
                break;
            }
        }
    }
    if (!isActive) return;

    if (pc->d_func() && pc->d_func()->isExplicit) {
        // explicit: true freezes advanced map values until reentry
        return;
    }

    QObject* target = pc->target() ? pc->target() : defaultTarget();
    if (!target) return;

    const QVariantMap oldMap = m_appliedMaps.value(pc);
    const QVariantMap newMap = pc->properties();
    m_appliedMaps[pc] = newMap;

    // 1. Removed keys: revert
    for (auto it = oldMap.begin(); it != oldMap.end(); ++it) {
        if (!newMap.contains(it.key())) {
            const ResolvedProperty resolved = resolveCanonicalProperty(target, it.key());
            revertProperty(resolved.key);
            m_baseRecords.remove(resolved.key);
            if (s_propertyOwners.value(resolved.key) == this) s_propertyOwners.remove(resolved.key);
            m_ownedProperties.remove(resolved.key);
        }
    }

    // 2. Newly added or updated keys
    for (auto it = newMap.begin(); it != newMap.end(); ++it) {
        const ResolvedProperty resolved = resolveCanonicalProperty(target, it.key());
        if (!resolved.target) continue;
        if (!oldMap.contains(it.key())) {
            // Newly added key
            watchTarget(resolved.target);
            if (!m_baseRecords.contains(resolved.key)) {
                BasePropertyRecord rec;
                rec.target = resolved.target;
                rec.name = resolved.key.name;
                rec.restore = pc->restoreEntryValues();
                rec.baseBinding = SavedBinding::takeFrom(resolved.target, resolved.key.name);
                rec.hadBinding = static_cast<bool>(rec.baseBinding);
                rec.baseValue = resolved.target->property(resolved.key.name.toUtf8().constData());
                m_baseRecords.insert(resolved.key, rec);
            }
            s_propertyOwners.insert(resolved.key, this);
            m_ownedProperties.insert(resolved.key);
            resolved.target->setProperty(resolved.key.name.toUtf8().constData(), it.value());
        } else if (oldMap.value(it.key()) != it.value()) {
            // Retained key with updated value
            resolved.target->setProperty(resolved.key.name.toUtf8().constData(), it.value());
        }
    }
}

void StateController::handleTargetChanged(PropertyChanges* pc) {
    if (!pc || m_stateName.isEmpty()) return;
    if (!m_appliedTargets.contains(pc)) return;

    bool isActive = false;
    for (const QPointer<State>& state : m_statesList) {
        if (state && state->name() == m_stateName && state->propertyChangesList().contains(pc)) {
            isActive = true;
            break;
        }
    }
    if (!isActive) return;

    QObject* oldTarget = m_appliedTargets.value(pc).data();
    QObject* newTarget = pc->target() ? pc->target() : defaultTarget();
    if (oldTarget == newTarget) return;

    m_appliedTargets[pc] = newTarget;

    if (pc->d_func() && pc->d_func()->hasCustomBindings) {
        const QSet<CanonicalProperty> oldKeys = m_appliedCustomProperties.take(pc);
        for (const CanonicalProperty& oldKey : oldKeys) {
            for (qsizetype i = m_runningTransitionAnimations.size() - 1; i >= 0; --i) {
                Animation* anim = m_runningTransitionAnimations.at(i).data();
                if (!anim) {
                    m_runningTransitionAnimations.removeAt(i);
                    continue;
                }
                auto* pAnim = qobject_cast<PropertyAnimation*>(anim);
                if (pAnim && reinterpret_cast<quintptr>(pAnim->target()) == oldKey.targetId &&
                    pAnim->property() == oldKey.name) {
                    m_runningTransitionAnimations.removeAt(i);
                    anim->stop();
                    delete anim;
                }
            }
            revertProperty(oldKey);
            m_baseRecords.remove(oldKey);
            if (s_propertyOwners.value(oldKey) == this) s_propertyOwners.remove(oldKey);
            m_ownedProperties.remove(oldKey);
        }

        if (newTarget) {
            QMap<CanonicalProperty, PropertyOverride> newOverrides;
            collectCustomOverrides(pc, newOverrides);
            QSet<CanonicalProperty> committedKeys;
            for (auto it = newOverrides.begin(); it != newOverrides.end(); ++it) {
                const CanonicalProperty& canon = it.key();
                const PropertyOverride& override = it.value();
                if (!override.target) continue;
                if (s_propertyOwners.contains(canon) && s_propertyOwners.value(canon) != this) {
                    qmlWarning(pc) << "Cannot retarget PropertyChanges: property " << canon.name
                                   << " is owned by another StateGroup/Item";
                    continue;
                }
                watchTarget(override.target.data());
                if (!m_baseRecords.contains(canon)) {
                    BasePropertyRecord rec;
                    rec.target = override.target;
                    rec.name = canon.name;
                    rec.restore = override.restore;
                    rec.baseBinding = SavedBinding::takeFrom(override.target.data(), canon.name);
                    rec.hadBinding = static_cast<bool>(rec.baseBinding);
                    rec.baseValue = override.target->property(canon.name.toUtf8().constData());
                    m_baseRecords.insert(canon, rec);
                } else if (!override.restore) {
                    m_baseRecords[canon].restore = false;
                }
                s_propertyOwners.insert(canon, this);
                m_ownedProperties.insert(canon);
                committedKeys.insert(canon);
                installOrWriteProperty(override.target.data(), canon.name, override.value,
                                       override.isBinding, override.binding);
            }
            m_appliedCustomProperties[pc] = committedKeys;
        }
        return;
    }

    const QVariantMap props = pc->properties();

    // 1. Revert properties for oldTarget
    if (oldTarget) {
        for (auto it = props.begin(); it != props.end(); ++it) {
            const ResolvedProperty resolved = resolveCanonicalProperty(oldTarget, it.key());
            revertProperty(resolved.key);
            m_baseRecords.remove(resolved.key);
            if (s_propertyOwners.value(resolved.key) == this) s_propertyOwners.remove(resolved.key);
            m_ownedProperties.remove(resolved.key);
        }
    }

    // 2. Apply properties to newTarget
    if (newTarget) {
        for (auto it = props.begin(); it != props.end(); ++it) {
            const ResolvedProperty resolved = resolveCanonicalProperty(newTarget, it.key());
            if (!resolved.target) continue;
            watchTarget(resolved.target);
            if (!m_baseRecords.contains(resolved.key)) {
                BasePropertyRecord rec;
                rec.target = resolved.target;
                rec.name = resolved.key.name;
                rec.restore = pc->restoreEntryValues();
                rec.baseBinding = SavedBinding::takeFrom(resolved.target, resolved.key.name);
                rec.hadBinding = static_cast<bool>(rec.baseBinding);
                rec.baseValue = resolved.target->property(resolved.key.name.toUtf8().constData());
                m_baseRecords.insert(resolved.key, rec);
            }
            s_propertyOwners.insert(resolved.key, this);
            m_ownedProperties.insert(resolved.key);
            resolved.target->setProperty(resolved.key.name.toUtf8().constData(), it.value());
        }
    }
}

void StateController::revertProperty(const CanonicalProperty& key) {
    auto it = m_baseRecords.find(key);
    if (it == m_baseRecords.end()) return;

    const BasePropertyRecord& rec = it.value();
    if (!rec.target || !rec.restore) return;

    restoreOrWriteProperty(rec.target.data(), rec.name, rec.baseValue, rec.hadBinding,
                           rec.baseBinding);
}

void StateController::applyOverride(const CanonicalProperty& key, const PropertyOverride& override,
                                    Transition* transitionObj,
                                    QSet<CanonicalProperty>& animatedProperties) {
    if (!override.target) return;

    QObject* targetObj = override.target.data();
    QString propName = override.name;
    QQmlProperty prop(targetObj, propName);
    const int colorTypeId = QMetaType::fromType<Color>().id();
    const bool isColorProperty = (prop.propertyType() == colorTypeId);

    QVariant fromVal = targetObj->property(propName.toUtf8().constData());
    QVariant toVal = override.value;

    // Check if matching transition animation exists
    PropertyAnimation* matchedAnim = nullptr;
    if (transitionObj) {
        for (const QPointer<Animation>& anim : transitionObj->animationsList()) {
            if (!anim) continue;
            auto* pAnim = qobject_cast<PropertyAnimation*>(anim.data());
            if (!pAnim) continue;
            if (pAnim->target() && pAnim->target() != targetObj) continue;
            if (!pAnim->property().isEmpty() && pAnim->property() != propName) continue;

            bool isColorAnim = (qobject_cast<ColorAnimation*>(pAnim) != nullptr);
            bool isNumberAnim = (qobject_cast<NumberAnimation*>(pAnim) != nullptr);

            if (isColorAnim) {
                bool looksLikeColor =
                    isColorProperty ||
                    propName.contains(QLatin1String("color"), Qt::CaseInsensitive) ||
                    (toVal.userType() == QMetaType::QString &&
                     toVal.toString().startsWith(QLatin1Char('#'))) ||
                    toVal.userType() == colorTypeId;
                if (!looksLikeColor) continue;
            } else if (isNumberAnim) {
                bool isNumeric =
                    (toVal.userType() == QMetaType::Int || toVal.userType() == QMetaType::Double ||
                     toVal.userType() == QMetaType::Float ||
                     toVal.userType() == QMetaType::LongLong ||
                     toVal.userType() == QMetaType::ULongLong);
                if (!isNumeric) continue;
            }
            matchedAnim = pAnim;
            break;
        }
    }

    // Endpoint equality check: do not allocate transition animation for unchanged endpoints
    bool endpointsEqual = (fromVal == toVal);
    if (!endpointsEqual &&
        (isColorProperty || fromVal.userType() == colorTypeId || toVal.userType() == colorTypeId ||
         qobject_cast<ColorAnimation*>(matchedAnim) != nullptr)) {
        auto toColor = [colorTypeId](const QVariant& val) -> Color {
            if (val.userType() == colorTypeId) return val.value<Color>();
            if (val.userType() == QMetaType::QString) return Color::fromString(val.toString());
            return Color::invalid();
        };
        const Color c1 = toColor(fromVal);
        const Color c2 = toColor(toVal);
        if (c1.isValid() && c2.isValid() && c1 == c2) endpointsEqual = true;
    }

    if (matchedAnim && !endpointsEqual && matchedAnim->duration() > 0) {
        animatedProperties.insert(key);

        PropertyAnimation* newAnim = nullptr;
        if (qobject_cast<ColorAnimation*>(matchedAnim)) {
            newAnim = new ColorAnimation(this);
        } else if (qobject_cast<NumberAnimation*>(matchedAnim)) {
            newAnim = new NumberAnimation(this);
        } else {
            newAnim = new PropertyAnimation(this);
        }

        newAnim->setDuration(matchedAnim->duration());
        newAnim->setEasing(matchedAnim->easing());
        newAnim->setLoops(matchedAnim->loops());
        newAnim->setTarget(targetObj);
        newAnim->setProperty(propName);
        newAnim->setFrom(fromVal);
        newAnim->setTo(toVal);

        SavedBinding destBinding = override.isBinding ? override.binding : SavedBinding();
        connect(newAnim, &Animation::finished, this, [this, newAnim, prop, destBinding]() mutable {
            m_runningTransitionAnimations.removeOne(newAnim);
            newAnim->deleteLater();
            if (prop.isValid() && prop.object() != nullptr && destBinding) {
                // Attach and reevaluate destination binding at transition completion
                destBinding.installOn(prop);
            }
        });

        removeBindingFromProperty(prop);
        m_runningTransitionAnimations.append(newAnim);
        newAnim->start();
    } else {
        // Not animated or endpoint equal
        installOrWriteProperty(targetObj, propName, toVal, override.isBinding, override.binding);
    }
}

void StateController::applyState(const QString& newState) {
    const QString oldState = m_stateName;
    State* newStateObj = nullptr;
    for (const QPointer<State>& s : m_statesList) {
        if (!s) continue;
        if (s->name() == newState) newStateObj = s.data();
    }

    // 1. Collect requested overrides for newState
    QMap<CanonicalProperty, PropertyOverride> requestedOverrides;
    if (newStateObj) {
        collectOverridesForState(newStateObj, requestedOverrides);
    }

    // 2. Preflight Ownership Check
    for (const CanonicalProperty& canon : requestedOverrides.keys()) {
        if (s_propertyOwners.contains(canon) && s_propertyOwners.value(canon) != this) {
            qmlWarning(m_defaultTarget ? m_defaultTarget.data() : this)
                << "Cannot apply state " << newState << ": property " << canon.name
                << " is owned by another StateGroup/Item";
            return; // Reject entire activation, leave current state intact!
        }
    }

    // Preflight passed! Update state name and notify
    m_stateName = newState;
    emit stateChanged(m_stateName);

    // 3. Stop running transition animations
    for (const QPointer<Animation>& anim : m_runningTransitionAnimations) {
        if (anim) {
            anim->stop();
            delete anim.data();
        }
    }
    m_runningTransitionAnimations.clear();

    // 4. Find matching transition
    Transition* transitionObj = nullptr;
    for (const QPointer<Transition>& t : m_transitionsList) {
        if (t && t->matches(oldState, newState)) {
            transitionObj = t.data();
            break;
        }
    }

    // 5. Properties to revert (present in m_baseRecords, but NOT requested by newStateObj)
    QList<CanonicalProperty> toRevert;
    for (const CanonicalProperty& canon : m_baseRecords.keys()) {
        if (!requestedOverrides.contains(canon)) {
            toRevert.append(canon);
        }
    }

    for (const CanonicalProperty& canon : toRevert) {
        revertProperty(canon);
        m_baseRecords.remove(canon);
        if (s_propertyOwners.value(canon) == this) s_propertyOwners.remove(canon);
        m_ownedProperties.remove(canon);
    }

    // 6. Properties to apply
    QSet<CanonicalProperty> animatedProperties;
    for (auto it = requestedOverrides.begin(); it != requestedOverrides.end(); ++it) {
        const CanonicalProperty& canon = it.key();
        const PropertyOverride& override = it.value();
        if (!override.target) continue;

        watchTarget(override.target.data());

        // Capture base state if not already captured
        if (!m_baseRecords.contains(canon)) {
            BasePropertyRecord rec;
            rec.target = override.target;
            rec.name = canon.name;
            rec.restore = override.restore;
            rec.baseBinding = SavedBinding::takeFrom(override.target.data(), canon.name);
            rec.hadBinding = static_cast<bool>(rec.baseBinding);
            rec.baseValue = override.target->property(canon.name.toUtf8().constData());
            m_baseRecords.insert(canon, rec);
        } else if (!override.restore) {
            // restoreEntryValues: false retains final value/binding and discards predecessor
            m_baseRecords[canon].restore = false;
        }

        s_propertyOwners.insert(canon, this);
        m_ownedProperties.insert(canon);
        applyOverride(canon, override, transitionObj, animatedProperties);
    }

    for (auto it = m_baseRecords.begin(); it != m_baseRecords.end();) {
        if (it.value().target.isNull()) {
            if (s_propertyOwners.value(it.key()) == this) s_propertyOwners.remove(it.key());
            m_ownedProperties.remove(it.key());
            it = m_baseRecords.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace qmltui
