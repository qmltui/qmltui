#include <QmlTui/Core/Engine/StateGroup.h>
#include <QmlTui/Core/Engine/StateController_p.h>
#include <QmlTui/Core/Engine/State.h>
#include <QmlTui/Core/Engine/Transition.h>

namespace qmltui {

StateGroup::StateGroup(QObject* parent)
    : QObject(parent), m_stateController(std::make_unique<StateController>(this)) {
    m_stateController->setDefaultTarget(nullptr);
    connect(m_stateController.get(), &StateController::stateChanged, this, &StateGroup::stateChanged);
}

StateGroup::~StateGroup() = default;

QString StateGroup::state() const {
    return m_stateController->state();
}

void StateGroup::setState(const QString& state) {
    m_stateController->setState(state);
}

QQmlListProperty<State> StateGroup::statesProperty() {
    return m_stateController->statesProperty();
}

QQmlListProperty<Transition> StateGroup::transitionsProperty() {
    return m_stateController->transitionsProperty();
}

void StateGroup::classBegin() {
    m_stateController->setComponentComplete(false);
}

void StateGroup::componentComplete() {
    m_stateController->setComponentComplete(true);
    m_stateController->evaluateStateConditions();
}

} // namespace qmltui
