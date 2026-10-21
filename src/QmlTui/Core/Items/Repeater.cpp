#include <QmlTui/Core/Items/Repeater.h>
#include <QmlTui/Core/Items/DelegateUtils.h>
#include <QmlTui/Core/Items/NumericItemRoles_p.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QAbstractItemModel>
#include <QQmlPropertyMap>
#include <utility>

namespace qmltui {

using DelegateUtils::setPropertyIfExists;

Repeater::Repeater(QObject* parent) : Item(parent) {
    setVisible(false); // Repeater itself is not visible, it just injects children
}

Repeater::~Repeater() {
    clearItems();
}

void Repeater::setRootIndex(const QModelIndex& rootIndex) {
    if (m_rootIndex == rootIndex) return;
    m_rootIndex = rootIndex;
    m_nestedWarned = false;
    emit rootIndexChanged();
    if (qobject_cast<QAbstractItemModel*>(m_objectModel.data())) generateItems();
}

void Repeater::setModel(const QVariant& model) {
    if (m_model == model) return;
    m_nestedWarned = false;
    disconnectModelSignals();
    if (m_objectModel) disconnect(m_objectModel, nullptr, this, nullptr);
    ++m_modelGeneration;
    m_model = model;
    m_objectModel = model.canConvert<QObject*>() ? model.value<QObject*>() : nullptr;
    if (m_objectModel) {
        connect(m_objectModel, &QObject::destroyed, this, [this]() {
            m_objectModel.clear();
            m_model.clear();
            clearItems();
            emit modelChanged();
        });
    }
    emit modelChanged();
    generateItems();
}

void Repeater::setDelegate(QQmlComponent* delegate) {
    if (m_delegate == delegate) return;
    if (m_delegate) disconnect(m_delegate, nullptr, this, nullptr);
    m_delegate = delegate;
    if (m_delegate) {
        connect(m_delegate, &QObject::destroyed, this, [this]() {
            m_delegate.clear();
            clearItems();
            emit delegateChanged();
        });
        connect(m_delegate, &QQmlComponent::statusChanged, this,
                [this](QQmlComponent::Status status) {
                    if (status == QQmlComponent::Ready)
                        generateItems();
                    else if (status == QQmlComponent::Error)
                        DelegateUtils::isReady(m_delegate, this);
                });
    }
    emit delegateChanged();
    generateItems();
}

void Repeater::clearItems() {
    for (Item* item : m_createdItems) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_createdItems.clear();
    m_contexts.clear();
    m_rowMaps.clear();
    m_roleMap.clear();
    m_claimedNames = 0;
}

void Repeater::disconnectModelSignals() {
    for (const QMetaObject::Connection& connection : std::as_const(m_modelConnections))
        disconnect(connection);
    m_modelConnections.clear();
}

void Repeater::generateItems() {
    clearItems();
    disconnectModelSignals();
    if (!DelegateUtils::isReady(m_delegate, this)) return;

    Item* parentVis = parentItem();
    if (!parentVis) return;

    QVariantList list;
    int count = 0;
    const bool isStatic = DelegateUtils::unpackStaticModel(m_model, count, list);

    if (isStatic && !list.isEmpty()) {
        for (int i = 0; i < list.size(); ++i) {
            QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
            if (!context) {
                m_createdItems.append(nullptr);
                m_contexts.append(nullptr);
                continue;
            }
            QVariant itemVal = list.at(i);

            QVariantMap initialProps;
            initialProps["index"] = i;
            initialProps["modelData"] = itemVal;
            initialProps["model"] = itemVal;

            DelegateUtils::setContextProperty(m_delegate, context, "index", i);
            DelegateUtils::setContextProperty(m_delegate, context, "modelData", itemVal);
            DelegateUtils::setContextProperty(m_delegate, context, "model", itemVal);

            QVariant elem = itemVal;
            if (elem.userType() == QMetaType::fromType<QJSValue>().id()) {
                elem = elem.value<QJSValue>().toVariant();
            }
            if (elem.canConvert<QVariantMap>()) {
                QVariantMap map = elem.toMap();
                for (auto it = map.cbegin(); it != map.cend(); ++it) {
                    initialProps[it.key()] = it.value();
                    DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());
                }
            }

            Item* item =
                DelegateUtils::createItem(m_delegate, initialProps, context, parentVis, this);
            if (item) {
                if (elem.canConvert<QVariantMap>()) {
                    QVariantMap map = elem.toMap();
                    for (auto it = map.cbegin(); it != map.cend(); ++it) {
                        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
                    }
                }
                m_createdItems.append(item);
                m_contexts.append(m_delegate->isBound() ? nullptr : context);
            } else {
                m_createdItems.append(nullptr);
                m_contexts.append(nullptr);
            }
        }
    } else if (isStatic) {
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
            if (!context) {
                m_createdItems.append(nullptr);
                m_contexts.append(nullptr);
                continue;
            }
            QVariantMap initialProps;
            initialProps["index"] = i;

            DelegateUtils::setContextProperty(m_delegate, context, "index", i);

            Item* item =
                DelegateUtils::createItem(m_delegate, initialProps, context, parentVis, this);
            if (item) {
                m_createdItems.append(item);
                m_contexts.append(m_delegate->isBound() ? nullptr : context);
            } else {
                m_createdItems.append(nullptr);
                m_contexts.append(nullptr);
            }
        }
    } else if (m_model.canConvert<QObject*>()) {
        QObject* modelObj = m_model.value<QObject*>();
        if (modelObj && modelObj->inherits("QAbstractItemModel")) {
            QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(modelObj);
            if (model) {
                DelegateUtils::warnIfNested(model, m_rootIndex, this, m_nestedWarned);
                m_roleMap.clear();
                const QHash<int, QByteArray> roles = model->roleNames();
                for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
                    m_roleMap.insert(it.key(), QString::fromUtf8(it.value()));
                }
                m_claimedNames = NumericItemRoles::claimedNames(model);

                const quint64 generation = m_modelGeneration;
                QPointer<QAbstractItemModel> guardedModel(model);
                m_modelConnections.append(connect(
                    model, &QAbstractItemModel::dataChanged, this,
                    [this, guardedModel, generation](const QModelIndex& topLeft,
                                                     const QModelIndex& bottomRight,
                                                     const QList<int>& changedRoles) {
                        if (!guardedModel || generation != m_modelGeneration ||
                            guardedModel.data() != m_objectModel.data())
                            return;
                        if (topLeft.parent() != m_rootIndex) return;
                        const QHash<int, QString>& roleMap = m_roleMap;
                        int first = topLeft.row();
                        int last = bottomRight.row();
                        for (int row = first; row <= last; ++row) {
                            if (row >= 0 && row < m_rowMaps.size()) {
                                QQmlPropertyMap* rowMap = m_rowMaps.at(row);
                                QQmlContext* ctx =
                                    (row < m_contexts.size()) ? m_contexts.at(row) : nullptr;
                                Item* item = (row < m_createdItems.size()) ? m_createdItems.at(row)
                                                                           : nullptr;
                                const QModelIndex modelIndex =
                                    guardedModel->index(row, 0, m_rootIndex);
                                for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                                    if (!changedRoles.isEmpty() && !changedRoles.contains(it.key()))
                                        continue;
                                    QVariant val = guardedModel->data(modelIndex, it.key());
                                    if (rowMap) rowMap->insert(it.value(), val);
                                    DelegateUtils::setContextProperty(m_delegate, ctx, it.value(),
                                                                      val);
                                    setPropertyIfExists(item, it.value().toUtf8().constData(), val);
                                }
                                QVariantMap standard;
                                NumericItemRoles::apply(guardedModel, modelIndex, standard, true,
                                                        m_claimedNames);
                                for (auto it = standard.cbegin(); it != standard.cend(); ++it) {
                                    if (rowMap) rowMap->insert(it.key(), it.value());
                                    DelegateUtils::setContextProperty(m_delegate, ctx, it.key(),
                                                                      it.value());
                                    setPropertyIfExists(item, it.key().toUtf8().constData(),
                                                        it.value());
                                }
                            }
                        }
                        markDirty();
                    }));

                m_modelConnections.append(
                    connect(model, &QAbstractItemModel::rowsInserted, this,
                            [this, guardedModel, generation](const QModelIndex& parent, int first,
                                                             int last) {
                                if (!guardedModel || generation != m_modelGeneration ||
                                    guardedModel.data() != m_objectModel.data())
                                    return;
                                if (parent != m_rootIndex) return;
                                onRowsInserted(guardedModel, first, last);
                            }));
                m_modelConnections.append(
                    connect(model, &QAbstractItemModel::rowsRemoved, this,
                            [this, guardedModel, generation](const QModelIndex& parent, int first,
                                                             int last) {
                                if (!guardedModel || generation != m_modelGeneration ||
                                    guardedModel.data() != m_objectModel.data())
                                    return;
                                if (parent != m_rootIndex) return;
                                onRowsRemoved(first, last);
                            }));
                m_modelConnections.append(connect(
                    model, &QAbstractItemModel::rowsMoved, this,
                    [this, guardedModel, generation](const QModelIndex& sourceParent, int first,
                                                     int last, const QModelIndex& destinationParent,
                                                     int destination) {
                        if (!guardedModel || generation != m_modelGeneration ||
                            guardedModel.data() != m_objectModel.data())
                            return;
                        if (sourceParent != m_rootIndex || destinationParent != m_rootIndex) {
                            // A move into or out of the shown level changes the row set.
                            if (sourceParent == m_rootIndex || destinationParent == m_rootIndex)
                                generateItems();
                            return;
                        }
                        onRowsMoved(first, last, destination);
                    }));
                m_modelConnections.append(
                    connect(model, &QAbstractItemModel::layoutChanged, this,
                            [this, guardedModel, generation]() {
                                if (!guardedModel || generation != m_modelGeneration ||
                                    guardedModel.data() != m_objectModel.data())
                                    return;
                                generateItems();
                            }));
                m_modelConnections.append(
                    connect(model, &QAbstractItemModel::modelReset, this,
                            [this, guardedModel, generation]() {
                                if (!guardedModel || generation != m_modelGeneration ||
                                    guardedModel.data() != m_objectModel.data())
                                    return;
                                generateItems();
                            }));

                int rowCount = (std::max)(0, model->rowCount(m_rootIndex));
                for (int i = 0; i < rowCount; ++i)
                    instantiateModelRow(model, i, parentVis, true);
            }
        }
    }
    parentVis->reorderChildItemsAfter(this, m_createdItems);
    maybeFetchMore();
}

void Repeater::instantiateModelRow(QAbstractItemModel* model, int row, Item* parentVis,
                                   bool append) {
    const QHash<int, QString>& roleMap = m_roleMap;
    QQmlPropertyMap* rowMap = new QQmlPropertyMap(this);
    const QModelIndex modelIndex = model->index(row, 0, m_rootIndex);
    QVariantMap initialProps;
    initialProps["index"] = row;
    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
        QVariant val = model->data(modelIndex, it.key());
        rowMap->insert(it.value(), val);
        initialProps[it.value()] = val;
    }
    QVariantMap standard;
    NumericItemRoles::apply(model, modelIndex, standard, false, m_claimedNames);
    for (auto it = standard.cbegin(); it != standard.cend(); ++it) {
        rowMap->insert(it.key(), it.value());
        initialProps[it.key()] = it.value();
    }
    initialProps["model"] = QVariant::fromValue(rowMap);
    initialProps["modelData"] = QVariant::fromValue(rowMap);

    const auto recordRow = [this, row, append](Item* item, QQmlContext* context,
                                               QQmlPropertyMap* map) {
        if (append) {
            m_createdItems.append(item);
            m_contexts.append(context);
            m_rowMaps.append(map);
        } else {
            m_createdItems.insert(row, item);
            m_contexts.insert(row, context);
            m_rowMaps.insert(row, map);
        }
    };

    QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
    if (!context) {
        delete rowMap;
        recordRow(nullptr, nullptr, nullptr);
        return;
    }
    DelegateUtils::setContextProperty(m_delegate, context, "index", row);
    DelegateUtils::setContextProperty(m_delegate, context, "model", rowMap);
    DelegateUtils::setContextProperty(m_delegate, context, "modelData", rowMap);
    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
        DelegateUtils::setContextProperty(m_delegate, context, it.value(),
                                          rowMap->value(it.value()));
    }

    Item* item = DelegateUtils::createItem(m_delegate, initialProps, context, parentVis, this);
    if (item) {
        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
            setPropertyIfExists(item, it.value().toUtf8().constData(), rowMap->value(it.value()));
        }
        for (auto it = standard.cbegin(); it != standard.cend(); ++it) {
            DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());
            setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
        }
        rowMap->setParent(item);
        recordRow(item, m_delegate->isBound() ? nullptr : context, rowMap);
    } else {
        delete rowMap;
        recordRow(nullptr, nullptr, nullptr);
    }
}

void Repeater::maybeFetchMore() {
    auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data());
    if (!model) return;
    // A Repeater instantiates every row, so it also loads every row.
    DelegateUtils::fetchMoreIfNeeded(model, m_rootIndex, 0, m_fetching,
                                     [this] { return int(m_createdItems.size()) - 1; });
}

void Repeater::onRowsInserted(QAbstractItemModel* model, int first, int last) {
    if (!model || first < 0 || first > last) return;
    Item* parentVis = parentItem();
    if (!parentVis || !m_delegate) return;

    if (m_roleMap.isEmpty()) {
        const QHash<int, QByteArray> roles = model->roleNames();
        for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
            m_roleMap.insert(it.key(), QString::fromUtf8(it.value()));
        }
        m_claimedNames = NumericItemRoles::claimedNames(model);
    }

    for (int i = first; i <= last; ++i)
        instantiateModelRow(model, i, parentVis, false);

    // Update indices for all contexts after the inserted range
    for (int i = last + 1; i < m_contexts.size(); ++i) {
        DelegateUtils::setContextProperty(m_delegate, m_contexts[i], "index", i);
        if (i < m_createdItems.size() && m_createdItems[i])
            setPropertyIfExists(m_createdItems[i], "index", i);
    }

    parentVis->reorderChildItemsAfter(this, m_createdItems);
    markDirty();
    maybeFetchMore();
}

void Repeater::onRowsRemoved(int first, int last) {
    if (first < 0 || first > last) return;
    for (int i = last; i >= first; --i) {
        Item* item = i >= 0 && i < m_createdItems.size() ? m_createdItems.takeAt(i) : nullptr;
        QQmlContext* context = i >= 0 && i < m_contexts.size() ? m_contexts.takeAt(i) : nullptr;
        QQmlPropertyMap* rowMap = i >= 0 && i < m_rowMaps.size() ? m_rowMaps.takeAt(i) : nullptr;
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        } else {
            delete context;
            delete rowMap;
        }
    }

    // Update indices for all contexts after the removed range
    for (int i = first; i < m_contexts.size(); ++i) {
        DelegateUtils::setContextProperty(m_delegate, m_contexts[i], "index", i);
        if (i < m_createdItems.size() && m_createdItems[i])
            setPropertyIfExists(m_createdItems[i], "index", i);
    }

    markDirty();
}

void Repeater::onRowsMoved(int first, int last, int destination) {
    if (first < 0 || last < first || last >= m_createdItems.size()) return;
    QList<Item*> movedItems;
    QList<QQmlContext*> movedContexts;
    QList<QQmlPropertyMap*> movedMaps;
    for (int row = first; row <= last; ++row) {
        movedItems.append(m_createdItems.takeAt(first));
        movedContexts.append(m_contexts.takeAt(first));
        movedMaps.append(m_rowMaps.takeAt(first));
    }
    int insertion = destination > first ? destination - movedItems.size() : destination;
    insertion = std::clamp(insertion, 0, static_cast<int>(m_createdItems.size()));
    for (int index = 0; index < movedItems.size(); ++index) {
        m_createdItems.insert(insertion + index, movedItems[index]);
        m_contexts.insert(insertion + index, movedContexts[index]);
        m_rowMaps.insert(insertion + index, movedMaps[index]);
    }
    for (int row = 0; row < m_contexts.size(); ++row) {
        DelegateUtils::setContextProperty(m_delegate, m_contexts[row], "index", row);
        if (row < m_createdItems.size() && m_createdItems[row])
            setPropertyIfExists(m_createdItems[row], "index", row);
    }
    if (Item* parentVis = parentItem()) parentVis->reorderChildItemsAfter(this, m_createdItems);
    markDirty();
}

} // namespace qmltui
