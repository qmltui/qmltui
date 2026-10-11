#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>

#include <Qamui/Core/engine/KeyEvent.h>
#include <Qamui/Core/items/Item.h>
#include <QObject>
#include <QQmlEngine>
#include <QQmlListProperty>

namespace qamui {

class QAMUI_CORE_EXPORT Keys : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Keys is only available as an attached property")
    QML_ATTACHED(Keys)

public:
    enum Priority { BeforeItem, AfterItem };
    Q_ENUM(Priority)

    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)
    Q_PROPERTY(QQmlListProperty<qamui::Item> forwardTo READ forwardTo)

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

    QQmlListProperty<qamui::Item> forwardTo() {
        return QQmlListProperty<qamui::Item>(this, this,
                                            &Keys::appendForwardTo,
                                            &Keys::forwardToCount,
                                            &Keys::forwardToAt,
                                            &Keys::clearForwardTo);
    }

    const QList<qamui::Item*>& forwardToItems() const { return m_forwardToItems; }

signals:
    void priorityChanged();
    void pressed(qamui::KeyEvent *event);
    void returnPressed(qamui::KeyEvent *event);
    void escapePressed(qamui::KeyEvent *event);
    void spacePressed(qamui::KeyEvent *event);
    void upPressed(qamui::KeyEvent *event);
    void downPressed(qamui::KeyEvent *event);
    void leftPressed(qamui::KeyEvent *event);
    void rightPressed(qamui::KeyEvent *event);
    void tabPressed(qamui::KeyEvent *event);

private:
    static void appendForwardTo(QQmlListProperty<qamui::Item> *list, qamui::Item *item) {
        if (item) {
            Keys *self = static_cast<Keys *>(list->object);
            self->m_forwardToItems.append(item);
        }
    }
    static qsizetype forwardToCount(QQmlListProperty<qamui::Item> *list) {
        return static_cast<Keys *>(list->object)->m_forwardToItems.count();
    }
    static qamui::Item *forwardToAt(QQmlListProperty<qamui::Item> *list, qsizetype index) {
        return static_cast<Keys *>(list->object)->m_forwardToItems.at(index);
    }
    static void clearForwardTo(QQmlListProperty<qamui::Item> *list) {
        static_cast<Keys *>(list->object)->m_forwardToItems.clear();
    }

    Priority m_priority;
    QList<qamui::Item*> m_forwardToItems;
};

} // namespace qamui

QML_DECLARE_TYPEINFO(qamui::Keys, QML_HAS_ATTACHED_PROPERTIES)
