#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QQmlEngine>
#include <QQmlParserStatus>
#include <QQmlListProperty>
#include <memory>

namespace qmltui {

class State;
class Transition;
class StateController;

// Standalone state and transition container for non-Item objects or secondary
// state machines.
class QMLTUI_CORE_EXPORT StateGroup : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QQmlListProperty<qmltui::State> states READ statesProperty)
    Q_PROPERTY(QQmlListProperty<qmltui::Transition> transitions READ transitionsProperty)
    Q_CLASSINFO("DefaultProperty", "states")

public:
    explicit StateGroup(QObject* parent = nullptr);
    ~StateGroup() override;

    QString state() const;
    void setState(const QString& state);

    QQmlListProperty<State> statesProperty();
    QQmlListProperty<Transition> transitionsProperty();

    QQmlListProperty<State> states() { return statesProperty(); }
    QQmlListProperty<Transition> transitions() { return transitionsProperty(); }

    void classBegin() override;
    void componentComplete() override;

signals:
    void stateChanged();

private:
    std::unique_ptr<StateController> m_stateController;
};

} // namespace qmltui
