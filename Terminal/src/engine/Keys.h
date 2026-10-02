#pragma once

#include "KeyEvent.h"
#include <QObject>
#include <QQmlEngine>

namespace qtui {

class Keys : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Keys is only available as an attached property")
    QML_ATTACHED(Keys)

signals:
    void pressed(qtui::KeyEvent *event);
    void released(qtui::KeyEvent *event);

public:
    explicit Keys(QObject *parent = nullptr) : QObject(parent) {}

    static Keys *qmlAttachedProperties(QObject *object) {
        // Create or find the attached Keys object for this target object
        return new Keys(object);
    }
};

} // namespace qtui

QML_DECLARE_TYPEINFO(qtui::Keys, QML_HAS_ATTACHED_PROPERTIES)
