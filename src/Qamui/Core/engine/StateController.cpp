#include <Qamui/Core/engine/StateController_p.h>
#include <Qamui/Core/engine/State.h>
#include <Qamui/Core/engine/PropertyChanges_p.h>
#include <Qamui/Core/engine/Transition.h>
#include <Qamui/Core/engine/Animation.h>
#include <Qamui/Core/primitives/Color.h>
#include <QDebug>
#include <qqmlinfo.h>
#include <QQmlProperty>
#include <QQmlContext>
#include <QMetaProperty>

#if defined(QAMUI_ENABLE_PRIVATE_QT)
#include <private/qqmlproperty_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qqmlbinding_p.h>
#endif

namespace qamui {

QMap<CanonicalProperty, StateController*> StateController::s_propertyOwners;

static CanonicalProperty resolveCanonicalProperty(QObject* target, const QString& name) {
    CanonicalProperty result;
    result.target = target;
    result.name = name;

#if defined(QAMUI_ENABLE_PRIVATE_QT)
    if (target && !name.isEmpty()) {
        QQmlProperty prop(target, name);
        if (prop.isValid()) {
            if (QQmlPropertyPrivate::get(prop)->core.isAlias()) {
                auto resolved = QQmlPropertyPrivate::findAliasTarget(
                    prop.object(), QQmlPropertyPrivate::propertyIndex(prop));
                if (resolved.targetObject) {
                    result.target = resolved.targetObject;
                    const QMetaObject* mo = resolved.targetObject->metaObject();
                    int coreIdx = resolved.targetIndex.coreIndex();
                    if (coreIdx >= 0 && coreIdx < mo->propertyCount()) {
                        result.name = QString::fromUtf8(mo->property(coreIdx).name());
                    }
                }
            } else {
                result.target = prop.object();
                result.name = prop.name();
            }
        }
    }
#endif

    return result;
}

StateController::StateController(QObject* parent) : QObject(parent) {
}

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

void StateController::setDefaultTarget(QObject* target) {
    m_defaultTarget = target;
}

void StateController::setComponentComplete(bool complete) {
    m_componentComplete = complete;
}

QQmlListProperty<State> StateController::statesProperty() {
    return QQmlListProperty<State>(
        this, nullptr,
        &StateController::appendState,
        &StateController::stateCount,
        &StateController::stateAt,
        &StateController::clearStates
    );
}

void StateController::appendState(QQmlListProperty<State>* list, State* state) {
    auto* self = qobject_cast<StateController*>(list->object);
    if (self && state) {
        state->setParent(self);
        self->m_statesList.append(state);
        connect(state, &State::whenChanged, self, &StateController::evaluateStateConditions);

        for (PropertyChanges* pc : state->propertyChangesList()) {
            connect(pc, &PropertyChanges::propertiesChanged, self, [self, pc]() {
                self->handlePropertiesChanged(pc);
            });
            connect(pc, &PropertyChanges::targetChanged, self, [self, pc]() {
                self->handleTargetChanged(pc);
            });
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
        ? self->m_statesList.at(index).data() : nullptr;
}

void StateController::clearStates(QQmlListProperty<State>* list) {
    auto* self = qobject_cast<StateController*>(list->object);
    if (self) {
        const QList<QPointer<State>> states = self->m_statesList;
        self->m_statesList.clear();
        for (const QPointer<State>& state : states) {
            if (state) {
                disconnect(state, nullptr, self, nullptr);
                if (state->parent() == self)
                    state->setParent(nullptr);
            }
        }
        self->evaluateStateConditions();
    }
}

QQmlListProperty<Transition> StateController::transitionsProperty() {
    return QQmlListProperty<Transition>(
        this, nullptr,
        &StateController::appendTransition,
        &StateController::transitionCount,
        &StateController::transitionAt,
        &StateController::clearTransitions
    );
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
        ? self->m_transitionsList.at(index).data() : nullptr;
}

void StateController::clearTransitions(QQmlListProperty<Transition>* list) {
    auto* self = qobject_cast<StateController*>(list->object);
    if (self) {
        const QList<QPointer<Transition>> transitions = self->m_transitionsList;
        self->m_transitionsList.clear();
        for (const QPointer<Transition>& transition : transitions) {
            if (transition && transition->parent() == self)
                transition->setParent(nullptr);
        }
    }
}

void StateController::stateRemoved(State* state) {
    if (!state) return;
    disconnect(state, nullptr, this, nullptr);
    m_statesList.removeIf([state](const QPointer<State>& s) {
        return s.isNull() || s.data() == state;
    });
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
        bool &flag;
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

void StateController::collectOverridesForState(
    State* state, QMap<CanonicalProperty, PropertyOverride>& outOverrides)
{
    if (!state) return;

    for (PropertyChanges* pc : state->propertyChangesList()) {
        if (!pc) continue;

#if defined(QAMUI_ENABLE_PRIVATE_QT)
        if (!pc->properties().isEmpty() && pc->d_func() && pc->d_func()->hasCustomBindings) {
            qmlWarning(pc) << "Cannot mix properties map and direct assignments in PropertyChanges";
            continue;
        }

        if (pc->d_func() && pc->d_func()->hasCustomBindings) {
            pc->d_func()->decode();
            for (const DecodedBinding& b : pc->d_func()->decodedBindings) {
                QObject* baseTarget = pc->target() ? pc->target() : defaultTarget();
                if (!pc->target() && b.propertyName.contains(QLatin1Char('.'))) {
                    QString firstPart = b.propertyName.section(QLatin1Char('.'), 0, 0);
                    if (baseTarget && baseTarget->metaObject()->indexOfProperty(firstPart.toUtf8().constData()) < 0) {
                        baseTarget = nullptr;
                    }
                }
                if (!baseTarget && !b.propertyName.contains(QLatin1Char('.'))) {
                    qmlWarning(pc) << "PropertyChanges inside StateGroup requires explicit target or ID-qualified property";
                    continue;
                }

                QQmlContext* rawContext = qmlContext(pc);
                if (!rawContext && parent())
                    rawContext = qmlContext(parent());
                if (!rawContext && baseTarget)
                    rawContext = qmlContext(baseTarget);

                QQmlRefPointer<QQmlContextData> context = rawContext ? QQmlContextData::get(rawContext) : QQmlRefPointer<QQmlContextData>();
                QQmlProperty prop;
                if (!context.isNull()) {
                    prop = QQmlPropertyPrivate::create(
                        baseTarget, b.propertyName, context,
                        QQmlPropertyPrivate::InitFlag::AllowId);
                } else if (baseTarget) {
                    prop = QQmlProperty(baseTarget, b.propertyName);
                }

                if (!prop.isValid()) {
                    qmlWarning(pc) << "Cannot assign to non-existent property \"" << b.propertyName << "\"";
                    continue;
                }
                if (!prop.isWritable()) {
                    qmlWarning(pc) << "Cannot assign to read-only property \"" << b.propertyName << "\"";
                    continue;
                }

                CanonicalProperty canon = resolveCanonicalProperty(prop.object(), prop.name());
                PropertyOverride override;
                override.target = prop.object();
                override.name = prop.name();
                override.restore = pc->restoreEntryValues();

                if (b.isExpression) {
                    QV4::Scope scope(qmlEngine(pc)->handle());
                    QV4::Scoped<QV4::QmlContext> qmlCtxt(
                        scope, QV4::QmlContext::create(scope.engine->rootContext(), context, prop.object()));

                    std::unique_ptr<QQmlBinding> evalBinding;
                    if (b.rawBinding && b.rawBinding->isTranslationBinding()) {
                        evalBinding.reset(QQmlBinding::createTranslationBinding(
                            pc->d_func()->compilationUnit, b.rawBinding, prop.object(), context));
                    } else if (b.compiledScriptIndex != ~0u &&
                               b.compiledScriptIndex < pc->d_func()->compilationUnit->runtimeFunctions.size()) {
                        evalBinding.reset(QQmlBinding::create(
                            &QQmlPropertyPrivate::get(prop)->core,
                            pc->d_func()->compilationUnit->runtimeFunctions.at(b.compiledScriptIndex),
                            prop.object(), context, qmlCtxt));
                    } else if (!b.expressionCode.isEmpty()) {
                        evalBinding.reset(QQmlBinding::create(
                            &QQmlPropertyPrivate::get(prop)->core,
                            b.expressionCode, prop.object(), context,
                            b.url.toString(), b.line));
                    }
                    override.value = evalBinding ? evalBinding->evaluate() : QVariant();

                    if (pc->isExplicit()) {
                        override.isBinding = false;
                    } else {
                        QQmlAnyBinding newBinding;
                        if (b.rawBinding && b.rawBinding->isTranslationBinding()) {
                            newBinding = QQmlAnyBinding::createTranslationBinding(
                                prop, pc->d_func()->compilationUnit, b.rawBinding, prop.object(), context);
                        } else if (b.compiledScriptIndex != ~0u &&
                                   b.compiledScriptIndex < pc->d_func()->compilationUnit->runtimeFunctions.size()) {
                            newBinding = QQmlAnyBinding::createFromFunction(
                                prop,
                                pc->d_func()->compilationUnit->runtimeFunctions.at(b.compiledScriptIndex),
                                prop.object(), context, qmlCtxt);
                        } else if (!b.expressionCode.isEmpty()) {
                            newBinding = QQmlAnyBinding::createFromCodeString(
                                prop, b.expressionCode, prop.object(), context,
                                b.url.toString(), b.line);
                        }
                        override.binding = newBinding;
                        override.isBinding = true;
                    }
                } else {
                    override.value = b.literalValue;
                    override.isBinding = false;
                }

                outOverrides.insert(canon, override);
            }
            continue;
        }
#endif

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

#if defined(QAMUI_ENABLE_PRIVATE_QT)
            QQmlContext* rawContext = qmlContext(pc);
            if (!rawContext && parent())
                rawContext = qmlContext(parent());
            if (!rawContext && target)
                rawContext = qmlContext(target);

            QQmlRefPointer<QQmlContextData> context = rawContext ? QQmlContextData::get(rawContext) : QQmlRefPointer<QQmlContextData>();
            QQmlProperty prop;
            if (!context.isNull()) {
                prop = QQmlPropertyPrivate::create(
                    target, propName, context, QQmlPropertyPrivate::InitFlag::AllowId);
            } else if (target) {
                prop = QQmlProperty(target, propName);
            }
            if (!prop.isValid()) {
                qmlWarning(pc) << "Cannot assign to non-existent property \"" << propName << "\"";
                continue;
            }
            if (!prop.isWritable()) {
                qmlWarning(pc) << "Cannot assign to read-only property \"" << propName << "\"";
                continue;
            }
            CanonicalProperty canon = resolveCanonicalProperty(prop.object(), prop.name());
            PropertyOverride override;
            override.target = prop.object();
            override.name = prop.name();
            override.value = val;
            override.restore = pc->restoreEntryValues();
#if defined(QAMUI_ENABLE_PRIVATE_QT)
            override.isBinding = false;
#endif
            outOverrides.insert(canon, override);
#else
            const QMetaObject* mo = target->metaObject();
            int idx = mo->indexOfProperty(propName.toUtf8().constData());
            if (idx < 0) {
                qmlWarning(pc) << "Cannot assign to non-existent property \"" << propName << "\"";
                continue;
            }
            QMetaProperty mp = mo->property(idx);
            if (!mp.isWritable()) {
                qmlWarning(pc) << "Cannot assign to read-only property \"" << propName << "\"";
                continue;
            }
            CanonicalProperty canon;
            canon.target = target;
            canon.name = propName;
            PropertyOverride override;
            override.target = target;
            override.name = propName;
            override.value = val;
            override.restore = pc->restoreEntryValues();
            outOverrides.insert(canon, override);
#endif
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

#if defined(QAMUI_ENABLE_PRIVATE_QT)
    if (pc->isExplicit()) {
        // explicit: true freezes advanced map values until reentry
        return;
    }
#endif

    QObject* target = pc->target() ? pc->target() : defaultTarget();
    if (!target) return;

    const QVariantMap oldMap = m_appliedMaps.value(pc);
    const QVariantMap newMap = pc->properties();
    m_appliedMaps[pc] = newMap;

    // 1. Removed keys: revert
    for (auto it = oldMap.begin(); it != oldMap.end(); ++it) {
        if (!newMap.contains(it.key())) {
            CanonicalProperty canon = resolveCanonicalProperty(target, it.key());
            revertProperty(canon);
            m_baseRecords.remove(canon);
            s_propertyOwners.remove(canon);
            m_ownedProperties.remove(canon);
        }
    }

    // 2. Newly added or updated keys
    for (auto it = newMap.begin(); it != newMap.end(); ++it) {
        CanonicalProperty canon = resolveCanonicalProperty(target, it.key());
        if (!oldMap.contains(it.key())) {
            // Newly added key
            if (!m_baseRecords.contains(canon)) {
                BasePropertyRecord rec;
                rec.target = canon.target;
                rec.name = canon.name;
                rec.restore = pc->restoreEntryValues();
#if defined(QAMUI_ENABLE_PRIVATE_QT)
                QQmlProperty prop(canon.target.data(), canon.name);
                if (prop.isValid()) {
                    rec.baseBinding = QQmlAnyBinding::takeFrom(prop);
                    rec.hadBinding = bool(rec.baseBinding);
                }
#endif
                rec.baseValue = canon.target->property(canon.name.toUtf8().constData());
                m_baseRecords.insert(canon, rec);
            }
            s_propertyOwners.insert(canon, this);
            m_ownedProperties.insert(canon);
            canon.target->setProperty(canon.name.toUtf8().constData(), it.value());
        } else if (oldMap.value(it.key()) != it.value()) {
            // Retained key with updated value
            canon.target->setProperty(canon.name.toUtf8().constData(), it.value());
        }
    }
}

void StateController::handleTargetChanged(PropertyChanges* pc) {
    if (!pc) return;
    if (!m_appliedTargets.contains(pc)) return;

    QObject* oldTarget = m_appliedTargets.value(pc).data();
    QObject* newTarget = pc->target() ? pc->target() : defaultTarget();
    if (oldTarget == newTarget) return;

    m_appliedTargets[pc] = newTarget;

    const QVariantMap props = pc->properties();

    // 1. Revert properties for oldTarget
    if (oldTarget) {
        for (auto it = props.begin(); it != props.end(); ++it) {
            CanonicalProperty canon = resolveCanonicalProperty(oldTarget, it.key());
            revertProperty(canon);
            m_baseRecords.remove(canon);
            s_propertyOwners.remove(canon);
            m_ownedProperties.remove(canon);
        }
    }

    // 2. Apply properties to newTarget
    if (newTarget) {
        for (auto it = props.begin(); it != props.end(); ++it) {
            CanonicalProperty canon = resolveCanonicalProperty(newTarget, it.key());
            if (!m_baseRecords.contains(canon)) {
                BasePropertyRecord rec;
                rec.target = canon.target;
                rec.name = canon.name;
                rec.restore = pc->restoreEntryValues();
#if defined(QAMUI_ENABLE_PRIVATE_QT)
                QQmlProperty prop(canon.target.data(), canon.name);
                if (prop.isValid()) {
                    rec.baseBinding = QQmlAnyBinding::takeFrom(prop);
                    rec.hadBinding = bool(rec.baseBinding);
                }
#endif
                rec.baseValue = canon.target->property(canon.name.toUtf8().constData());
                m_baseRecords.insert(canon, rec);
            }
            s_propertyOwners.insert(canon, this);
            m_ownedProperties.insert(canon);
            canon.target->setProperty(canon.name.toUtf8().constData(), it.value());
        }
    }
}

void StateController::revertProperty(const CanonicalProperty& key) {
    if (!key.target || !m_baseRecords.contains(key)) return;

    BasePropertyRecord rec = m_baseRecords.value(key);
    if (!rec.restore) return;

#if defined(QAMUI_ENABLE_PRIVATE_QT)
    QQmlProperty prop(rec.target.data(), rec.name);
    if (prop.isValid()) {
        QQmlAnyBinding::removeBindingFrom(prop);
        if (rec.hadBinding && rec.baseBinding) {
            QQmlAnyBinding baseB = rec.baseBinding;
            baseB.installOn(prop);
        } else {
            prop.write(rec.baseValue);
        }
    } else {
        rec.target->setProperty(rec.name.toUtf8().constData(), rec.baseValue);
    }
#else
    rec.target->setProperty(rec.name.toUtf8().constData(), rec.baseValue);
#endif
}

void StateController::applyOverride(
    const CanonicalProperty& key, const PropertyOverride& override,
    Transition* transitionObj, QSet<CanonicalProperty>& animatedProperties)
{
    if (!override.target) return;

    QObject* targetObj = override.target.data();
    QString propName = override.name;

    QVariant fromVal = targetObj->property(propName.toUtf8().constData());
    QVariant toVal = override.value;

#if defined(QAMUI_ENABLE_PRIVATE_QT)
    QQmlProperty prop(targetObj, propName);
#endif

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
                bool looksLikeColor = propName.contains("color", Qt::CaseInsensitive) ||
                    (toVal.userType() == QMetaType::QString && toVal.toString().startsWith('#')) ||
                    toVal.userType() == QMetaType::fromType<Color>().id();
                if (!looksLikeColor) continue;
            } else if (isNumberAnim) {
                bool isNumeric = (toVal.userType() == QMetaType::Int ||
                                  toVal.userType() == QMetaType::Double ||
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
    if (!endpointsEqual && toVal.userType() == QMetaType::fromType<Color>().id()) {
        Color c1 = (fromVal.userType() == QMetaType::fromType<Color>().id())
            ? fromVal.value<Color>() : Color::fromString(fromVal.toString());
        Color c2 = toVal.value<Color>();
        if (c1.isValid() && c2.isValid() && c1 == c2)
            endpointsEqual = true;
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

#if defined(QAMUI_ENABLE_PRIVATE_QT)
        QQmlAnyBinding destBinding = override.isBinding ? override.binding : QQmlAnyBinding();
        connect(newAnim, &Animation::finished, this, [this, newAnim, prop, destBinding]() mutable {
            m_runningTransitionAnimations.removeOne(newAnim);
            newAnim->deleteLater();
            if (prop.isValid() && destBinding) {
                // Attach and reevaluate destination binding at transition completion
                destBinding.installOn(prop);
            }
        });
#else
        connect(newAnim, &Animation::finished, this, [this, newAnim]() {
            m_runningTransitionAnimations.removeOne(newAnim);
            newAnim->deleteLater();
        });
#endif

        m_runningTransitionAnimations.append(newAnim);
        newAnim->start();
    } else {
        // Not animated or endpoint equal
#if defined(QAMUI_ENABLE_PRIVATE_QT)
        if (prop.isValid()) {
            if (override.isBinding && override.binding) {
                QQmlAnyBinding b = override.binding;
                b.installOn(prop);
            } else {
                prop.write(toVal);
            }
        } else {
            targetObj->setProperty(propName.toUtf8().constData(), toVal);
        }
#else
        targetObj->setProperty(propName.toUtf8().constData(), toVal);
#endif
    }
}

void StateController::applyState(const QString& newState) {
    QString oldState = m_stateName;

    // Find State objects
    State* oldStateObj = nullptr;
    State* newStateObj = nullptr;
    for (const QPointer<State>& s : m_statesList) {
        if (!s) continue;
        if (s->name() == oldState) oldStateObj = s.data();
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
            qWarning() << "Cannot apply state" << newState << ": property"
                       << canon.name << "is owned by another StateGroup/Item";
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
        s_propertyOwners.remove(canon);
        m_ownedProperties.remove(canon);
    }

    // 6. Properties to apply
    QSet<CanonicalProperty> animatedProperties;
    for (auto it = requestedOverrides.begin(); it != requestedOverrides.end(); ++it) {
        const CanonicalProperty& canon = it.key();
        const PropertyOverride& override = it.value();

        // Capture base state if not already captured
        if (!m_baseRecords.contains(canon)) {
            BasePropertyRecord rec;
            rec.target = canon.target;
            rec.name = canon.name;
            rec.restore = override.restore;
#if defined(QAMUI_ENABLE_PRIVATE_QT)
            QQmlProperty prop(canon.target.data(), canon.name);
            if (prop.isValid()) {
                rec.baseBinding = QQmlAnyBinding::takeFrom(prop);
                rec.hadBinding = bool(rec.baseBinding);
            }
#endif
            rec.baseValue = canon.target->property(canon.name.toUtf8().constData());
            m_baseRecords.insert(canon, rec);
        } else if (!override.restore) {
            // restoreEntryValues: false retains final value/binding and discards predecessor
            m_baseRecords[canon].restore = false;
        }

        // Claim ownership
        s_propertyOwners.insert(canon, this);
        m_ownedProperties.insert(canon);

        // Apply override
        applyOverride(canon, override, transitionObj, animatedProperties);
    }

    // Prune dead records
    for (auto it = m_baseRecords.begin(); it != m_baseRecords.end();) {
        if (it.key().target.isNull()) {
            it = m_baseRecords.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace qamui
