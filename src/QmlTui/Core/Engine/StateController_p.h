#pragma once

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
#include <utility>

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
#include <private/qqmlanybinding_p.h>
#endif

namespace qmltui {

class State;
class Transition;
class PropertyChanges;
class Animation;

struct CanonicalProperty {
    QPointer<QObject> target;
    QString name;

    bool operator<(const CanonicalProperty& other) const {
        if (target != other.target)
            return target < other.target;
        return name < other.name;
    }
    bool operator==(const CanonicalProperty& other) const {
        return target == other.target && name == other.name;
    }
    friend size_t qHash(const CanonicalProperty& key, size_t seed = 0) noexcept {
        return qHashMulti(seed, key.target.data(), key.name);
    }
};

class QMLTUI_CORE_EXPORT StateController : public QObject {
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

    // Release all property ownership tracked by this controller
    void releaseAllOwnership();

signals:
    void stateChanged(const QString& state);

private slots:
    void handlePropertiesChanged(PropertyChanges* pc);
    void handleTargetChanged(PropertyChanges* pc);

private:
    struct BasePropertyRecord {
        QPointer<QObject> target;
        QString name;
        QVariant baseValue;
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
        QQmlAnyBinding baseBinding;
#endif
        bool hadBinding = false;
        bool restore = true;
    };

    struct PropertyOverride {
        QPointer<QObject> target;
        QString name;
        QVariant value;
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
        QQmlAnyBinding binding;
        bool isBinding = false;
#endif
        bool restore = true;
    };

    void applyState(const QString& newState);
    void collectOverridesForState(State* state, QMap<CanonicalProperty, PropertyOverride>& outOverrides);
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

    // Automatic 'when' state tracking
    bool m_isAutomatic = false;
    QString m_automaticState;
    bool m_evaluatingWhenConditions = false;

    QList<QPointer<State>> m_statesList;
    QList<QPointer<Transition>> m_transitionsList;

    // Base state records (survives across A -> B -> base transitions)
    QMap<CanonicalProperty, BasePropertyRecord> m_baseRecords;

    // Properties currently owned by this controller
    QSet<CanonicalProperty> m_ownedProperties;

    // Running animations for active transitions
    QList<QPointer<Animation>> m_runningTransitionAnimations;

    // Previous maps for active map updates
    QMap<PropertyChanges*, QVariantMap> m_appliedMaps;
    QMap<PropertyChanges*, QPointer<QObject>> m_appliedTargets;

    // Global registry of property ownership across all controllers
    static QMap<CanonicalProperty, StateController*> s_propertyOwners;
};

} // namespace qmltui
