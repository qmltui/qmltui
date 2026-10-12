#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Items/Item.h>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlListProperty>
#include <cstdint>

namespace qmltui {

class QMLTUI_CORE_EXPORT Keys : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Keys is only available as an attached property")
    QML_ATTACHED(Keys)

public:
    enum Priority : uint8_t { BeforeItem, AfterItem, };
    Q_ENUM(Priority)

    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)
    Q_PROPERTY(QQmlListProperty<qmltui::Item> forwardTo READ forwardTo)

    explicit Keys(QObject *parent = nullptr) : QObject(parent), m_priority(BeforeItem) {}

    static Keys *qmlAttachedProperties(QObject *object);

    Priority priority() const { return m_priority; }
    void setPriority(Priority priority) {
        if (m_priority != priority) {
            m_priority = priority;
            emit priorityChanged();
        }
    }

    QQmlListProperty<qmltui::Item> forwardTo() const {
        return QQmlListProperty<qmltui::Item>(const_cast<Keys*>(this), const_cast<Keys*>(this),
                                            &Keys::appendForwardTo,
                                            &Keys::forwardToCount,
                                            &Keys::forwardToAt,
                                            &Keys::clearForwardTo);
    }

    const QList<QPointer<qmltui::Item>>& forwardToItems() const { return m_forwardToItems; }

signals:
    void priorityChanged();
    void pressed(qmltui::KeyEvent *event);
    void returnPressed(qmltui::KeyEvent *event);
    void escapePressed(qmltui::KeyEvent *event);
    void spacePressed(qmltui::KeyEvent *event);
    void upPressed(qmltui::KeyEvent *event);
    void downPressed(qmltui::KeyEvent *event);
    void leftPressed(qmltui::KeyEvent *event);
    void rightPressed(qmltui::KeyEvent *event);
    void tabPressed(qmltui::KeyEvent *event);

private:
    static void appendForwardTo(QQmlListProperty<qmltui::Item> *list, qmltui::Item *item) {
        if (item) {
            Keys *self = static_cast<Keys *>(list->object);
            self->m_forwardToItems.append(item);
        }
    }
    static qsizetype forwardToCount(QQmlListProperty<qmltui::Item> *list) {
        return static_cast<Keys *>(list->object)->m_forwardToItems.size();
    }
    static qmltui::Item *forwardToAt(QQmlListProperty<qmltui::Item> *list, qsizetype index) {
        return static_cast<Keys *>(list->object)->m_forwardToItems.at(index).data();
    }
    static void clearForwardTo(QQmlListProperty<qmltui::Item> *list) {
        static_cast<Keys *>(list->object)->m_forwardToItems.clear();
    }

    Priority m_priority;
    QList<QPointer<qmltui::Item>> m_forwardToItems;
};

} // namespace qmltui

QML_DECLARE_TYPEINFO(qmltui::Keys, QML_HAS_ATTACHED_PROPERTIES)
