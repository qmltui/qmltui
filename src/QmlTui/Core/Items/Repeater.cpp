#include <QmlTui/Core/Items/Repeater.h>
#include <QmlTui/Core/Items/DelegateUtils.h>
#include <QmlTui/Core/Items/NumericItemRoles_p.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QAbstractItemModel>
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
    m_delegateProperties.clear();
    m_delegateStandardMask = 0;
    m_delegateSchemaResolved = false;
    m_delegateNeedsModelMap = true;
    if (m_delegate) {
        connect(m_delegate, &QObject::destroyed, this, [this]() {
            m_delegate.clear();
            m_delegateProperties.clear();
            m_delegateStandardMask = 0;
            m_delegateSchemaResolved = false;
            m_delegateNeedsModelMap = true;
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
                            if (row >= 0 && row < m_createdItems.size()) {
                                QQmlContext* ctx =
                                    (row < m_contexts.size()) ? m_contexts.at(row) : nullptr;
                                Item* item = m_createdItems.at(row);
                                if (!item) continue;
                                const QModelIndex modelIndex =
                                    guardedModel->index(row, 0, m_rootIndex);
                                if (m_delegate && m_delegate->isBound() &&
                                    m_delegateSchemaResolved && !m_delegateNeedsModelMap) {
                                    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                                        if (!changedRoles.isEmpty() &&
                                            !changedRoles.contains(it.key()))
                                            continue;
                                        const auto propIt =
                                            m_delegateProperties.constFind(it.value());
                                        if (propIt != m_delegateProperties.cend()) {
                                            item->metaObject()
                                                ->property(propIt.value())
                                                .write(item,
                                                       guardedModel->data(modelIndex, it.key()));
                                        }
                                    }
                                    if (m_delegateStandardMask != 0) {
                                        QVariantMap standard;
                                        NumericItemRoles::apply(guardedModel, modelIndex, standard,
                                                                true, m_claimedNames,
                                                                m_delegateStandardMask);
                                        for (auto it = standard.cbegin(); it != standard.cend();
                                             ++it)
                                            DelegateUtils::writeDeclaredProperty(
                                                item, m_delegateProperties, it.key(), it.value());
                                    }
                                    continue;
                                }
                                QVariantMap rowValues;
                                for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                                    const QVariant val = guardedModel->data(modelIndex, it.key());
                                    rowValues.insert(it.value(), val);
                                    if (changedRoles.isEmpty() || changedRoles.contains(it.key())) {
                                        DelegateUtils::setContextProperty(m_delegate, ctx,
                                                                          it.value(), val);
                                        setPropertyIfExists(item, it.value().toUtf8().constData(),
                                                            val);
                                    }
                                }
                                NumericItemRoles::apply(guardedModel, modelIndex, rowValues, true,
                                                        m_claimedNames);
                                for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
                                    DelegateUtils::setContextProperty(m_delegate, ctx, it.key(),
                                                                      it.value());
                                    setPropertyIfExists(item, it.key().toUtf8().constData(),
                                                        it.value());
                                }
                                DelegateUtils::setContextProperty(
                                    m_delegate, ctx, QStringLiteral("model"), rowValues);
                                DelegateUtils::setContextProperty(
                                    m_delegate, ctx, QStringLiteral("modelData"), rowValues);
                                setPropertyIfExists(item, "model", rowValues);
                                setPropertyIfExists(item, "modelData", rowValues);
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
    const auto recordRow = [this, row, append](Item* item, QQmlContext* context) {
        if (append) {
            m_createdItems.append(item);
            m_contexts.append(context);
        } else {
            m_createdItems.insert(row, item);
            m_contexts.insert(row, context);
        }
    };

    if (!m_delegate) {
        recordRow(nullptr, nullptr);
        return;
    }

    const QHash<int, QString>& roleMap = m_roleMap;
    const QModelIndex modelIndex = model->index(row, 0, m_rootIndex);
    const bool isChooser = QQmlListReference(m_delegate, "choices").isValid();

    if (m_delegate->isBound() && !isChooser) {
        QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
        if (!context) {
            recordRow(nullptr, nullptr);
            return;
        }
        Item* item = DelegateUtils::createItemWithSchema(
            m_delegate, context, parentVis, this, m_delegateSchemaResolved, m_delegateProperties,
            m_delegateStandardMask, m_delegateNeedsModelMap, [&](QVariantMap& initialProps) {
                if (m_delegateProperties.contains(QStringLiteral("index")))
                    initialProps.insert(QStringLiteral("index"), row);
                if (m_delegateNeedsModelMap) {
                    QVariantMap rowValues;
                    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it)
                        rowValues.insert(it.value(), model->data(modelIndex, it.key()));
                    NumericItemRoles::apply(model, modelIndex, rowValues, false, m_claimedNames);
                    initialProps.insert(QStringLiteral("model"), rowValues);
                    initialProps.insert(QStringLiteral("modelData"), rowValues);
                    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
                        if (m_delegateProperties.contains(it.key()))
                            initialProps.insert(it.key(), it.value());
                    }
                } else {
                    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                        if (m_delegateProperties.contains(it.value()))
                            initialProps.insert(it.value(), model->data(modelIndex, it.key()));
                    }
                    if (m_delegateStandardMask != 0) {
                        NumericItemRoles::apply(model, modelIndex, initialProps, false,
                                                m_claimedNames, m_delegateStandardMask);
                    }
                }
            });
        recordRow(item, nullptr);
        return;
    }

    QVariantMap rowValues;
    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it)
        rowValues.insert(it.value(), model->data(modelIndex, it.key()));
    NumericItemRoles::apply(model, modelIndex, rowValues, false, m_claimedNames);

    QVariantMap initialProps = rowValues;
    initialProps.insert(QStringLiteral("index"), row);
    initialProps.insert(QStringLiteral("model"), rowValues);
    initialProps.insert(QStringLiteral("modelData"), rowValues);

    QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
    if (!context) {
        recordRow(nullptr, nullptr);
        return;
    }
    DelegateUtils::setContextProperty(m_delegate, context, QStringLiteral("index"), row);
    DelegateUtils::setContextProperty(m_delegate, context, QStringLiteral("model"), rowValues);
    DelegateUtils::setContextProperty(m_delegate, context, QStringLiteral("modelData"), rowValues);
    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
        DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());

    Item* item = DelegateUtils::createItem(m_delegate, initialProps, context, parentVis, this);
    recordRow(item, item && !m_delegate->isBound() ? context : nullptr);
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
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        } else {
            delete context;
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
    for (int row = first; row <= last; ++row) {
        movedItems.append(m_createdItems.takeAt(first));
        movedContexts.append(m_contexts.takeAt(first));
    }
    int insertion = destination > first ? destination - movedItems.size() : destination;
    insertion = std::clamp(insertion, 0, static_cast<int>(m_createdItems.size()));
    for (int index = 0; index < movedItems.size(); ++index) {
        m_createdItems.insert(insertion + index, movedItems[index]);
        m_contexts.insert(insertion + index, movedContexts[index]);
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
