#pragma once

#include "KeyEvent.h"
#include "../items/Item.h"
#include <QObject>
#include <QQmlEngine>
#include <QQmlListProperty>

namespace qtui {

class Keys : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Keys is only available as an attached property")
    QML_ATTACHED(Keys)

public:
    enum Priority { BeforeItem, AfterItem };
    Q_ENUM(Priority)

    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)
    Q_PROPERTY(QQmlListProperty<qtui::Item> forwardTo READ forwardTo)

    explicit Keys(QObject *parent = nullptr) : QObject(parent), m_priority(BeforeItem) {}

    static Keys *qmlAttachedProperties(QObject *object) {
        // Create or find the attached Keys object for this target object
        return new Keys(object);
    }

    Priority priority() const { return m_priority; }
    void setPriority(Priority priority) {
        if (m_priority != priority) {
            m_priority = priority;
            emit priorityChanged();
        }
    }

    QQmlListProperty<qtui::Item> forwardTo() {
        return QQmlListProperty<qtui::Item>(this, this,
                                            &Keys::appendForwardTo,
                                            &Keys::forwardToCount,
                                            &Keys::forwardToAt,
                                            &Keys::clearForwardTo);
    }

    const QList<qtui::Item*>& forwardToItems() const { return m_forwardToItems; }

signals:
    void priorityChanged();
    void pressed(qtui::KeyEvent *event);
    void released(qtui::KeyEvent *event);
    void returnPressed(qtui::KeyEvent *event);
    void escapePressed(qtui::KeyEvent *event);
    void spacePressed(qtui::KeyEvent *event);
    void upPressed(qtui::KeyEvent *event);
    void downPressed(qtui::KeyEvent *event);
    void leftPressed(qtui::KeyEvent *event);
    void rightPressed(qtui::KeyEvent *event);
    void tabPressed(qtui::KeyEvent *event);

private:
    static void appendForwardTo(QQmlListProperty<qtui::Item> *list, qtui::Item *item) {
        if (item) {
            Keys *self = static_cast<Keys *>(list->object);
            self->m_forwardToItems.append(item);
        }
    }
    static qsizetype forwardToCount(QQmlListProperty<qtui::Item> *list) {
        return static_cast<Keys *>(list->object)->m_forwardToItems.count();
    }
    static qtui::Item *forwardToAt(QQmlListProperty<qtui::Item> *list, qsizetype index) {
        return static_cast<Keys *>(list->object)->m_forwardToItems.at(index);
    }
    static void clearForwardTo(QQmlListProperty<qtui::Item> *list) {
        static_cast<Keys *>(list->object)->m_forwardToItems.clear();
    }

    Priority m_priority;
    QList<qtui::Item*> m_forwardToItems;
};

} // namespace qtui

QML_DECLARE_TYPEINFO(qtui::Keys, QML_HAS_ATTACHED_PROPERTIES)
