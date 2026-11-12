#pragma once

// Internal: shared QAbstractItemModel signal connection management for views.

#include <QmlTui/Core/Items/DelegateUtils.h>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/NumericItemRoles_p.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QMetaProperty>
#include <QModelIndex>
#include <QObject>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QString>
#include <QVariantMap>
#include <utility>

namespace qmltui::ModelSignalConnections {

// Disconnects every recorded connection and clears the list.
inline void disconnectAll(QList<QMetaObject::Connection>& connections) {
    for (const QMetaObject::Connection& connection : std::as_const(connections))
        QObject::disconnect(connection);
    connections.clear();
}

// Appends a QObject::connect result to `connections` for uniform teardown.
template <typename Sender, typename Signal, typename Context, typename Slot>
inline void record(QList<QMetaObject::Connection>& connections, const Sender* sender, Signal signal,
                   const Context* context, Slot&& slot) {
    connections.append(QObject::connect(sender, signal, context, std::forward<Slot>(slot)));
}

// Re-evaluates and writes role properties for a single materialized row
// delegate in response to QAbstractItemModel::dataChanged.
inline void updateDelegateRowData(Item* item, QQmlComponent* delegate, QQmlContext* context,
                                  QAbstractItemModel* model, const QModelIndex& modelIndex,
                                  const QHash<int, QString>& roleMap,
                                  const QList<int>& changedRoles, bool schemaResolved,
                                  bool needsModelMap, const QHash<QString, int>& delegateProperties,
                                  quint16 claimedNames, quint16 delegateStandardMask) {
    if (!item || !model) return;
    if (delegate && delegate->isBound() && schemaResolved && !needsModelMap) {
        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
            if (!changedRoles.isEmpty() && !changedRoles.contains(it.key())) continue;
            const auto propIt = delegateProperties.constFind(it.value());
            if (propIt != delegateProperties.cend()) {
                item->metaObject()
                    ->property(propIt.value())
                    .write(item, model->data(modelIndex, it.key()));
            }
        }
        if (delegateStandardMask != 0) {
            QVariantMap standard;
            NumericItemRoles::apply(model, modelIndex, standard, true, claimedNames,
                                    delegateStandardMask);
            for (auto it = standard.cbegin(); it != standard.cend(); ++it)
                DelegateUtils::writeDeclaredProperty(item, delegateProperties, it.key(),
                                                     it.value());
        }
        return;
    }

    QVariantMap rowValues;
    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
        const QVariant value = model->data(modelIndex, it.key());
        rowValues.insert(it.value(), value);
        if (changedRoles.isEmpty() || changedRoles.contains(it.key())) {
            DelegateUtils::setContextProperty(delegate, context, it.value(), value);
            DelegateUtils::setPropertyIfExists(item, it.value().toUtf8().constData(), value);
        }
    }
    NumericItemRoles::apply(model, modelIndex, rowValues, true, claimedNames);
    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
        DelegateUtils::setContextProperty(delegate, context, it.key(), it.value());
        DelegateUtils::setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
    }
    DelegateUtils::setContextProperty(delegate, context, QStringLiteral("model"), rowValues);
    DelegateUtils::setContextProperty(delegate, context, QStringLiteral("modelData"), rowValues);
    DelegateUtils::setPropertyIfExists(item, "model", rowValues);
    DelegateUtils::setPropertyIfExists(item, "modelData", rowValues);
}

// Connects the six row-level QAbstractItemModel signals (`dataChanged`,
// `rowsInserted`, `rowsRemoved`, `rowsMoved`, `layoutChanged`, `modelReset`)
// with generation + active-model liveness guards and `rootIndex` filtering for
// flat row views (`ListView`, `GridView`, `Repeater`).
template <typename Context, typename OnDataChanged, typename OnRowsInserted, typename OnRowsRemoved,
          typename OnRowsMovedWithinRoot, typename OnStructureReset, typename OnModelReset>
inline void
connectGuardedFlatRowSignals(QList<QMetaObject::Connection>& connections, QAbstractItemModel* model,
                             Context* context, const QPointer<QObject>& activeModel,
                             const quint64& activeGeneration,
                             const QPersistentModelIndex& rootIndex, OnDataChanged&& onDataChanged,
                             OnRowsInserted&& onRowsInserted, OnRowsRemoved&& onRowsRemoved,
                             OnRowsMovedWithinRoot&& onRowsMovedWithinRoot,
                             OnStructureReset&& onStructureReset, OnModelReset&& onModelReset) {
    const quint64 generation = activeGeneration;
    const QPointer<QAbstractItemModel> guardedModel(model);
    const auto isLive = [guardedModel, generation, &activeModel, &activeGeneration]() {
        return guardedModel && generation == activeGeneration &&
               guardedModel.data() == activeModel.data();
    };
    auto structureReset = std::forward<OnStructureReset>(onStructureReset);

    record(connections, model, &QAbstractItemModel::dataChanged, context,
           [isLive, guardedModel, &rootIndex, fn = std::forward<OnDataChanged>(onDataChanged)](
               const QModelIndex& topLeft, const QModelIndex& bottomRight,
               const QList<int>& changedRoles) {
               if (!isLive() || topLeft.parent() != rootIndex) return;
               fn(guardedModel.data(), topLeft.row(), bottomRight.row(), changedRoles);
           });
    record(connections, model, &QAbstractItemModel::rowsInserted, context,
           [isLive, guardedModel, &rootIndex, fn = std::forward<OnRowsInserted>(onRowsInserted)](
               const QModelIndex& parent, int first, int last) {
               if (!isLive() || parent != rootIndex) return;
               fn(guardedModel.data(), first, last);
           });
    record(connections, model, &QAbstractItemModel::rowsRemoved, context,
           [isLive, &rootIndex, fn = std::forward<OnRowsRemoved>(onRowsRemoved)](
               const QModelIndex& parent, int first, int last) {
               if (!isLive() || parent != rootIndex) return;
               fn(first, last);
           });
    record(connections, model, &QAbstractItemModel::rowsMoved, context,
           [isLive, &rootIndex,
            movedFn = std::forward<OnRowsMovedWithinRoot>(onRowsMovedWithinRoot),
            structureReset](const QModelIndex& sourceParent, int first, int last,
                            const QModelIndex& destinationParent, int destination) {
               if (!isLive()) return;
               if (sourceParent != rootIndex || destinationParent != rootIndex) {
                   if (sourceParent == rootIndex || destinationParent == rootIndex)
                       structureReset();
                   return;
               }
               movedFn(first, last, destination);
           });
    record(connections, model, &QAbstractItemModel::layoutChanged, context,
           [isLive, structureReset = std::move(structureReset)]() {
               if (isLive()) structureReset();
           });
    record(connections, model, &QAbstractItemModel::modelReset, context,
           [isLive, modelResetFn = std::forward<OnModelReset>(onModelReset)]() {
               if (isLive()) modelResetFn();
           });
}

// Connects row/column structural signals (`rowsInserted`, `rowsRemoved`,
// `rowsMoved`, `columnsInserted`, `columnsRemoved`, `columnsMoved`,
// `layoutChanged`) to a shared callback for two-dimensional views (`TableView`).
template <typename Context, typename OnStructureChanged>
inline void connectTableStructureSignals(QList<QMetaObject::Connection>& connections,
                                         QAbstractItemModel* model, Context* context,
                                         OnStructureChanged&& onStructureChanged) {
    auto callback = std::forward<OnStructureChanged>(onStructureChanged);
    record(connections, model, &QAbstractItemModel::rowsInserted, context, callback);
    record(connections, model, &QAbstractItemModel::rowsRemoved, context, callback);
    record(connections, model, &QAbstractItemModel::rowsMoved, context, callback);
    record(connections, model, &QAbstractItemModel::columnsInserted, context, callback);
    record(connections, model, &QAbstractItemModel::columnsRemoved, context, callback);
    record(connections, model, &QAbstractItemModel::columnsMoved, context, callback);
    record(connections, model, &QAbstractItemModel::layoutChanged, context, std::move(callback));
}

} // namespace qmltui::ModelSignalConnections
