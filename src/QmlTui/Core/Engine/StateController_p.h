#pragma once

// Internal: state machine engine backing Item::states and StateGroup.

#include <QmlTui/Core/Engine/QmlPrivateBridge_p.h>
#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>
#include <QHash>
#include <QPointer>
#include <QVariant>
#include <QQmlListProperty>
#include <optional>

namespace qmltui {

class State;
class Transition;
class PropertyChanges;
class Animation;

// Identifies a (target QObject address, property name) pair managed by StateController.
// Keyed by immutable quintptr rather than QPointer so QObject destruction never mutates
// live QMap/QSet keys; StateController purges entries synchronously via QObject::destroyed.
struct CanonicalProperty {
    quintptr targetId = 0;
    QString name;

    bool operator<(const CanonicalProperty& other) const noexcept {
        if (targetId != other.targetId) return targetId < other.targetId;
        return name < other.name;
    }
    bool operator==(const CanonicalProperty& other) const noexcept {
        return targetId == other.targetId && name == other.name;
    }
    friend size_t qHash(const CanonicalProperty& key, size_t seed = 0) noexcept {
        return qHashMulti(seed, key.targetId, key.name);
    }
};

// Evaluates State::when conditions, applies and reverts PropertyChanges
// overrides (preserving base values and bindings across state transitions),
// and triggers matching Transition animations.
class StateController : public QObject {
    Q_OBJECT

public:
    explicit StateController(QObject* parent = nullptr);
    ~StateController() override;

    QString state() const { return m_stateName; }
    void setState(const QString& newState);

    QQmlListProperty<State> statesProperty();
    const QList<QPointer<State>>& statesList() const { return m_statesList; }

    QQmlListProperty<Transition> transitionsProperty();
    const QList<QPointer<Transition>>& transitionsList() const { return m_transitionsList; }

    void evaluateStateConditions();

    QObject* defaultTarget() const { return m_defaultTarget.data(); }
    void setDefaultTarget(QObject* target);

    bool isComponentComplete() const { return m_componentComplete; }
    void setComponentComplete(bool complete);

    void stateRemoved(State* state);
    void transitionRemoved(Transition* transition);

    void releaseAllOwnership();

signals:
    void stateChanged(const QString& state);

protected:
    void childEvent(QChildEvent* event) override;

private slots:
    void handlePropertiesChanged(PropertyChanges* pc);
    void handleTargetChanged(PropertyChanges* pc);
    void handleTargetDestroyed(QObject* obj);

private:
    struct BasePropertyRecord {
        QPointer<QObject> target;
        QString name;
        QVariant baseValue;
        SavedBinding baseBinding;
        bool hadBinding = false;
        bool restore = true;
    };

    struct PropertyOverride {
        QPointer<QObject> target;
        QString name;
        QVariant value;
        SavedBinding binding;
        bool isBinding = false;
        bool restore = true;
    };

    void watchTarget(QObject* target);
    void applyState(const QString& newState);
    void collectCustomOverrides(PropertyChanges* pc,
                                QMap<CanonicalProperty, PropertyOverride>& outOverrides,
                                QSet<CanonicalProperty>* outAppliedKeys = nullptr);
    void collectOverridesForState(State* state,
                                  QMap<CanonicalProperty, PropertyOverride>& outOverrides);
    void revertProperty(const CanonicalProperty& key);
    void applyOverride(const CanonicalProperty& key, const PropertyOverride& override,
                       Transition* transitionObj, QSet<CanonicalProperty>& animatedProperties);

    static void appendState(QQmlListProperty<State>* list, State* state);
    static qsizetype stateCount(QQmlListProperty<State>* list);
    static State* stateAt(QQmlListProperty<State>* list, qsizetype index);
    static void clearStates(QQmlListProperty<State>* list);

    static void appendTransition(QQmlListProperty<Transition>* list, Transition* transition);
    static qsizetype transitionCount(QQmlListProperty<Transition>* list);
    static Transition* transitionAt(QQmlListProperty<Transition>* list, qsizetype index);
    static void clearTransitions(QQmlListProperty<Transition>* list);

    QPointer<QObject> m_defaultTarget;
    QString m_stateName;
    bool m_updatingState = false;
    std::optional<QString> m_pendingState;
    bool m_componentComplete = true;

    bool m_isAutomatic = false;
    QString m_automaticState;
    bool m_evaluatingWhenConditions = false;

    QList<QPointer<State>> m_statesList;
    QList<QPointer<Transition>> m_transitionsList;

    // Preserves pre-state values and bindings across A -> B -> base transitions.
    QMap<CanonicalProperty, BasePropertyRecord> m_baseRecords;
    QSet<CanonicalProperty> m_ownedProperties;
    QList<QPointer<Animation>> m_runningTransitionAnimations;
    QMap<PropertyChanges*, QVariantMap> m_appliedMaps;
    QMap<PropertyChanges*, QPointer<QObject>> m_appliedTargets;
    QMap<PropertyChanges*, QSet<CanonicalProperty>> m_appliedCustomProperties;
    QSet<QObject*> m_watchedTargets;
};

} // namespace qmltui
