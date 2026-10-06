#pragma once

#include "Item.h"

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QQmlListReference>

namespace qtui::DelegateUtils {

inline QQmlComponent *resolveDelegate(QQmlComponent *delegate,
                                      int row,
                                      int column,
                                      const QVariantMap &modelMap)
{
    if (!delegate)
        return nullptr;

    QQmlListReference choices(delegate, "choices");
    if (!choices.isValid())
        return delegate;

    const QString roleName = delegate->property("role").toString();
    const QVariant roleValue = !roleName.isEmpty() ? modelMap.value(roleName) : QVariant();

    for (qsizetype choiceIndex = 0; choiceIndex < choices.count(); ++choiceIndex) {
        QObject *choice = choices.at(choiceIndex);
        if (!choice)
            continue;

        const QVariant choiceRowVar = choice->property("row");
        const QVariant choiceColVar = choice->property("column");
        const int choiceRow = choiceRowVar.isValid() ? choiceRowVar.toInt() : -1;
        const int choiceColumn = choiceColVar.isValid() ? choiceColVar.toInt() : -1;
        const QVariant choiceValue = choice->property("roleValue");

        const bool rowMatches = choiceRow < 0 || choiceRow == row;
        const bool columnMatches = choiceColumn < 0 || choiceColumn == column;
        const bool roleMatches = !choiceValue.isValid() || choiceValue == roleValue;
        if (!rowMatches || !columnMatches || !roleMatches)
            continue;

        const QVariant prop = choice->property("delegate");
        if (auto *comp = prop.value<QQmlComponent *>())
            return comp;
        if (auto *obj = prop.value<QObject *>()) {
            if (auto *comp = qobject_cast<QQmlComponent *>(obj))
                return comp;
        }
    }
    return nullptr;
}

inline QQmlContext *createContext(QQmlComponent *component, QObject *owner)
{
    if (!component || !owner)
        return nullptr;

    QQmlContext *parentContext = component->creationContext();
    if (component->isBound())
        return parentContext;
    if (!parentContext)
        parentContext = qmlContext(owner);
    if (!parentContext && component->engine())
        parentContext = component->engine()->rootContext();
    return parentContext ? new QQmlContext(parentContext, owner) : nullptr;
}

inline void setContextProperty(QQmlComponent *component, QQmlContext *context,
                               const QString &name, const QVariant &value)
{
    if (component && context && !component->isBound())
        context->setContextProperty(name, value);
}

inline void setContextProperty(QQmlComponent *component, QQmlContext *context,
                               const QString &name, QObject *value)
{
    if (component && context && !component->isBound())
        context->setContextProperty(name, value);
}

inline bool isReady(QQmlComponent *component, QObject *owner)
{
    if (!component)
        return false;
    QQmlListReference choices(component, "choices");
    if (choices.isValid())
        return true;
    if (component->status() == QQmlComponent::Ready)
        return true;
    if (component->status() == QQmlComponent::Error)
        qmlWarning(owner) << component->errorString();
    return false;
}

inline Item *createItem(QQmlComponent *component,
                        const QVariantMap &initialProperties,
                        QQmlContext *context,
                        Item *parentItem,
                        QObject *owner)
{
    if (!component || !context || !parentItem || !owner)
        return nullptr;

    const bool ownsContext = context != component->creationContext();
    QObject *object = component->beginCreate(context);
    if (object) {
        QVariantMap filteredProperties;
        const QMetaObject *metaObject = object->metaObject();
        for (auto it = initialProperties.cbegin(); it != initialProperties.cend(); ++it) {
            const int idx = metaObject->indexOfProperty(it.key().toUtf8().constData());
            // Read-only properties (notably Item's "children" list) would make
            // setInitialProperties() warn for every delegate it creates.
            if (idx >= 0 && metaObject->property(idx).isWritable())
                filteredProperties.insert(it.key(), it.value());
        }
        component->setInitialProperties(object, filteredProperties);
        component->completeCreate();
    }
    if (object && !component->errors().isEmpty()) {
        qmlWarning(owner) << component->errorString();
        delete object;
        if (ownsContext)
            delete context;
        return nullptr;
    }
    Item *item = qobject_cast<Item *>(object);
    if (!item) {
        if (!component->errors().isEmpty())
            qmlWarning(owner) << component->errorString();
        else if (object)
            qmlWarning(owner) << "Delegate root must derive from Qtui.Item";
        delete object;
        if (ownsContext)
            delete context;
        return nullptr;
    }

    item->setParentItem(parentItem);
    item->setParent(parentItem);
    if (ownsContext)
        context->setParent(item);
    return item;
}

inline void setPropertyIfExists(QObject *object, const char *name, const QVariant &value)
{
    if (object && object->metaObject()->indexOfProperty(name) >= 0)
        object->setProperty(name, value);
}

} // namespace qtui::DelegateUtils
