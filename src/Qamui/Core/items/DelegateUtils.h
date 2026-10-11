#pragma once

#include <Qamui/Core/items/Item.h>

#include <QAbstractItemModel>
#include <QByteArrayView>
#include <QMetaProperty>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QQmlListReference>

namespace qamui::DelegateUtils {

// True when the property at `propertyIndex` was declared in QML (a plain or
// required `property` of the delegate file or one of its QML base files),
// as opposed to a C++ property of the root type such as Text.text,
// Rectangle.color or Item.enabled. Views publish model data only to the
// former: the latter are the type's own API, usually carry a binding, and a
// role that happens to share the name must not overwrite it.
inline bool isDeclaredInQml(const QObject *object, int propertyIndex)
{
    const QMetaObject *meta = object->metaObject();
    while (meta && propertyIndex < meta->propertyOffset()) meta = meta->superClass();
    // Inline objects with declared members are named "<Type>_QML_<n>", file
    // components "<Name>_QMLTYPE_<n>".
    return meta && QByteArrayView(meta->className()).contains("_QML");
}

inline void setPropertyIfExists(QObject *object, const char *name, const QVariant &value)
{
    if (!object) return;
    const int index = object->metaObject()->indexOfProperty(name);
    if (index >= 0 && isDeclaredInQml(object, index))
        object->metaObject()->property(index).write(object, value);
}


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
            // Only QML-declared properties (required or plain) receive model
            // data: an initial property replaces the binding, so writing a
            // C++ property such as Text.text would discard the delegate's own
            // `text:` expression. Read-only ones (Item's "children") would make
            // setInitialProperties() warn for every delegate it creates.
            if (idx >= 0 && metaObject->property(idx).isWritable() && isDeclaredInQml(object, idx))
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
            qmlWarning(owner) << "Delegate root must derive from Qamui.Item";
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

// Flat views show one level of a model. Warn once when the level shown has
// nested rows that the view will silently skip.
inline void warnIfNested(QAbstractItemModel *model, const QModelIndex &root, QObject *owner, bool &warned)
{
    if (warned || !model) return;
    const QModelIndex first = model->index(0, 0, root);
    if (!first.isValid() || !model->hasChildren(first)) return;
    warned = true;
    qmlWarning(owner) << owner->metaObject()->className()
                      << ": model has nested rows; this view shows one level. "
                         "Use TreeView, or set rootIndex to show a subtree.";
}

// Drives lazily populated models (canFetchMore/fetchMore). Keeps fetching
// while the shown window still reaches the end of the loaded rows and the
// model grows synchronously; an asynchronous model continues through its own
// rowsInserted, which brings the caller back here. `fetching` is the caller's
// re-entrancy guard, `lastShown()` the highest row index it currently shows
// (-1 for none) and `threshold` how close to the end counts as "at the end".
template <typename LastShownFn>
inline void fetchMoreIfNeeded(QAbstractItemModel *model, const QModelIndex &parent,
                              int threshold, bool &fetching, LastShownFn lastShown)
{
    if (!model || fetching) return;
    fetching = true;
    for (int guard = 0; guard < 4096; ++guard) {
        const int count = model->rowCount(parent);
        if (!model->canFetchMore(parent) || lastShown() + 1 + threshold < count) break;
        model->fetchMore(parent);
        if (model->rowCount(parent) == count) break;
    }
    fetching = false;
}

// TreeView's historical contract: a name the delegate does not declare is
// published as a dynamic property; a declared one is written; a C++ property
// of the root type is left alone.
inline void setDelegateProperty(QObject *object, const char *name, const QVariant &value)
{
    if (!object) return;
    const int index = object->metaObject()->indexOfProperty(name);
    if (index < 0) object->setProperty(name, value);
    else if (isDeclaredInQml(object, index)) object->metaObject()->property(index).write(object, value);
}

} // namespace qamui::DelegateUtils
