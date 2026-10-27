#include <QmlTui/Core/Items/GridView.h>
#include <QmlTui/Core/Items/DelegateUtils.h>
#include <QmlTui/Core/Items/NumericItemRoles_p.h>
#include <QAbstractItemModel>
#include <QJSValue>
#include <QQmlContext>
#include <QQmlListReference>
#include <QQmlEngine>
#include <algorithm>
#include <utility>

namespace qmltui {

using DelegateUtils::delegateContext;
using DelegateUtils::setPropertyIfExists;

GridView::GridView(QObject* parent) : ScrollView(parent) {
    // Scrolling and resizing change the visible window; ask for a layout pass
    // so rebuildViewport() runs before the next paint.
    const auto requestViewport = [this]() {
        markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    };
    connect(this, &ScrollView::contentYChanged, this, requestViewport);
    connect(this, &Item::heightChanged, this, requestViewport);
    connect(this, &Item::widthChanged, this, requestViewport);
}

GridView::~GridView() {
    clearItems();
}

void GridView::setRootIndex(const QModelIndex& rootIndex) {
    if (m_rootIndex == rootIndex) return;
    m_rootIndex = rootIndex;
    m_nestedWarned = false;
    emit rootIndexChanged();
    if (qobject_cast<QAbstractItemModel*>(m_objectModel.data())) generateItems();
}

void GridView::setModel(const QVariant& model) {
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
            m_modelCount = 0;
            m_listValues.clear();
            m_roleMap.clear();
            m_claimedNames = 0;
            clearItems();
            setContentHeight(0);
            emit modelChanged();
        });
    }
    emit modelChanged();
    generateItems();
}

void GridView::setDelegate(QQmlComponent* delegate) {
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
            setContentHeight(0);
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

void GridView::setCellWidth(int width) {
    if (m_cellWidth == width) return;
    m_cellWidth = width;
    emit cellWidthChanged();
    updateContentExtent();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void GridView::setReuseItems(bool reuse) {
    if (m_reuseItems == reuse) return;
    m_reuseItems = reuse;
    emit reuseItemsChanged();
    if (!reuse) {
        for (Item* item : std::as_const(m_reusePool)) {
            item->setParentItem(nullptr);
            delete item;
        }
        m_reusePool.clear();
    }
}

// A delegate that scrolled out: pool it (hidden) or destroy it. Delegates
// picked through `choices` are not pooled because the component may differ
// per row.
void GridView::releaseDelegateItem(Item* item) {
    if (!item) return;
    const bool poolable = m_reuseItems && !QQmlListReference(m_delegate, "choices").isValid();
    if (poolable) {
        item->setVisible(false);
        m_reusePool.append(item);
        return;
    }
    item->setParentItem(nullptr);
    delete item;
}

void GridView::setCellHeight(int height) {
    if (m_cellHeight == height) return;
    m_cellHeight = height;
    emit cellHeightChanged();
    updateContentExtent();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void GridView::clearItems() {
    for (Item* item : std::as_const(m_createdItems)) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_createdItems.clear();
    for (Item* item : std::as_const(m_reusePool)) {
        item->setParentItem(nullptr);
        delete item;
    }
    m_reusePool.clear();
    m_materializedFirst = -1;
    m_materializedLast = -1;
}

void GridView::disconnectModelSignals() {
    for (const QMetaObject::Connection& connection : std::as_const(m_modelConnections))
        disconnect(connection);
    m_modelConnections.clear();
}

int GridView::columnsPerRow() const {
    return m_cellWidth > 0 ? (std::max)(1, width() / m_cellWidth) : 1;
}

void GridView::updateContentExtent() {
    if (m_cellHeight <= 0 || m_modelCount <= 0) {
        setContentHeight(0);
        return;
    }
    const int columns = columnsPerRow();
    const int rows = (m_modelCount + columns - 1) / columns;
    setContentHeight(rows * m_cellHeight);
}

void GridView::reindexItem(Item* item, int index) {
    if (!item) return;
    if (m_delegate && !m_delegate->isBound()) {
        DelegateUtils::setContextProperty(m_delegate, delegateContext(item),
                                          QStringLiteral("index"), index);
    }
    if (m_delegateSchemaResolved)
        DelegateUtils::writeDeclaredProperty(item, m_delegateProperties, QStringLiteral("index"),
                                             index);
    else
        setPropertyIfExists(item, "index", index);
}

// Publishes row `index` to a delegate: on creation through initial
// properties and a fresh context, on rebind by writing the context and
// declared properties in place.
void GridView::bindDelegateItem(Item* item, int index, bool created) {
    Q_UNUSED(created);
    if (!item || !m_delegate) return;
    auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data());
    const bool countModel = m_listValues.isEmpty() && !model;

    if (m_delegate->isBound() && m_delegateSchemaResolved) {
        DelegateUtils::writeDeclaredProperty(item, m_delegateProperties, QStringLiteral("index"),
                                             index);
        if (!countModel && index < m_listValues.size()) {
            const QVariant& rowValue = m_listValues.at(index);
            if (m_delegateNeedsModelMap) {
                DelegateUtils::writeDeclaredProperty(item, m_delegateProperties,
                                                     QStringLiteral("model"), rowValue);
                DelegateUtils::writeDeclaredProperty(item, m_delegateProperties,
                                                     QStringLiteral("modelData"), rowValue);
            }
            QVariant normalized = rowValue;
            if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
                normalized = normalized.value<QJSValue>().toVariant();
            if (normalized.canConvert<QVariantMap>()) {
                const QVariantMap rowValues = normalized.toMap();
                for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
                    DelegateUtils::writeDeclaredProperty(item, m_delegateProperties, it.key(),
                                                         it.value());
            }
        } else if (model) {
            const QModelIndex modelIndex = model->index(index, 0, m_rootIndex);
            if (m_delegateNeedsModelMap) {
                QVariantMap rowValues;
                for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it)
                    rowValues.insert(it.value(), model->data(modelIndex, it.key()));
                NumericItemRoles::apply(model, modelIndex, rowValues, true, m_claimedNames);
                DelegateUtils::writeDeclaredProperty(item, m_delegateProperties,
                                                     QStringLiteral("model"), rowValues);
                DelegateUtils::writeDeclaredProperty(item, m_delegateProperties,
                                                     QStringLiteral("modelData"), rowValues);
                for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
                    DelegateUtils::writeDeclaredProperty(item, m_delegateProperties, it.key(),
                                                         it.value());
            } else {
                for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
                    const auto propIt = m_delegateProperties.constFind(it.value());
                    if (propIt != m_delegateProperties.cend()) {
                        item->metaObject()
                            ->property(propIt.value())
                            .write(item, model->data(modelIndex, it.key()));
                    }
                }
                if (m_delegateStandardMask != 0) {
                    QVariantMap standard;
                    NumericItemRoles::apply(model, modelIndex, standard, true, m_claimedNames,
                                            m_delegateStandardMask);
                    for (auto it = standard.cbegin(); it != standard.cend(); ++it)
                        DelegateUtils::writeDeclaredProperty(item, m_delegateProperties, it.key(),
                                                             it.value());
                }
            }
        }
        return;
    }

    QVariant rowValue;
    QVariantMap rowValues;
    if (!countModel && index < m_listValues.size()) {
        rowValue = m_listValues.at(index);
        QVariant normalized = rowValue;
        if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
            normalized = normalized.value<QJSValue>().toVariant();
        if (normalized.canConvert<QVariantMap>()) rowValues = normalized.toMap();
    } else if (model) {
        const QModelIndex modelIndex = model->index(index, 0, m_rootIndex);
        for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it)
            rowValues.insert(it.value(), model->data(modelIndex, it.key()));
        NumericItemRoles::apply(model, modelIndex, rowValues, true, m_claimedNames);
        rowValue = rowValues;
    }
    QQmlContext* context = delegateContext(item);
    DelegateUtils::setContextProperty(m_delegate, context, QStringLiteral("index"), index);
    setPropertyIfExists(item, "index", index);
    if (!countModel) {
        DelegateUtils::setContextProperty(m_delegate, context, QStringLiteral("model"), rowValue);
        DelegateUtils::setContextProperty(m_delegate, context, QStringLiteral("modelData"),
                                          rowValue);
        setPropertyIfExists(item, "model", rowValue);
        setPropertyIfExists(item, "modelData", rowValue);
    }
    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
        DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());
        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
    }
}

Item* GridView::createDelegateItem(int index) {
    if (index < 0 || index >= m_modelCount || !m_delegate) return nullptr;

    const bool isChooser = QQmlListReference(m_delegate, "choices").isValid();
    const bool fastBound = m_delegate->isBound() && !isChooser;
    auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data());
    const bool countModel = m_listValues.isEmpty() && !model;

    if (fastBound) {
        if (!DelegateUtils::isReady(m_delegate, this)) return nullptr;
        QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
        if (!context) return nullptr;

        return DelegateUtils::createItemWithSchema(
            m_delegate, context, this, this, m_delegateSchemaResolved, m_delegateProperties,
            m_delegateStandardMask, m_delegateNeedsModelMap, [&](QVariantMap& initialProps) {
                if (m_delegateProperties.contains(QStringLiteral("index")))
                    initialProps.insert(QStringLiteral("index"), index);
                if (!countModel && index < m_listValues.size()) {
                    const QVariant& rowValue = m_listValues.at(index);
                    if (m_delegateNeedsModelMap) {
                        initialProps.insert(QStringLiteral("model"), rowValue);
                        initialProps.insert(QStringLiteral("modelData"), rowValue);
                    }
                    QVariant normalized = rowValue;
                    if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
                        normalized = normalized.value<QJSValue>().toVariant();
                    if (normalized.canConvert<QVariantMap>()) {
                        const QVariantMap map = normalized.toMap();
                        for (auto it = map.cbegin(); it != map.cend(); ++it) {
                            if (m_delegateProperties.contains(it.key()))
                                initialProps.insert(it.key(), it.value());
                        }
                    }
                } else if (model) {
                    const QModelIndex modelIndex = model->index(index, 0, m_rootIndex);
                    if (m_delegateNeedsModelMap) {
                        QVariantMap rowValues;
                        for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it)
                            rowValues.insert(it.value(), model->data(modelIndex, it.key()));
                        NumericItemRoles::apply(model, modelIndex, rowValues, false,
                                                m_claimedNames);
                        initialProps.insert(QStringLiteral("model"), rowValues);
                        initialProps.insert(QStringLiteral("modelData"), rowValues);
                        for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
                            if (m_delegateProperties.contains(it.key()))
                                initialProps.insert(it.key(), it.value());
                        }
                    } else {
                        for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
                            if (m_delegateProperties.contains(it.value()))
                                initialProps.insert(it.value(), model->data(modelIndex, it.key()));
                        }
                        if (m_delegateStandardMask != 0) {
                            NumericItemRoles::apply(model, modelIndex, initialProps, false,
                                                    m_claimedNames, m_delegateStandardMask);
                        }
                    }
                }
            });
    }

    QVariant rowValue;
    QVariantMap rowValues;
    if (!countModel && index < m_listValues.size()) {
        rowValue = m_listValues.at(index);
        QVariant normalized = rowValue;
        if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
            normalized = normalized.value<QJSValue>().toVariant();
        if (normalized.canConvert<QVariantMap>()) rowValues = normalized.toMap();
    } else if (model) {
        const QModelIndex modelIndex = model->index(index, 0, m_rootIndex);
        for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it)
            rowValues.insert(it.value(), model->data(modelIndex, it.key()));
        NumericItemRoles::apply(model, modelIndex, rowValues, false, m_claimedNames);
        rowValue = rowValues;
    }

    QQmlComponent* component = DelegateUtils::resolveDelegate(m_delegate, index, 0, rowValues);
    if (!DelegateUtils::isReady(component, this)) return nullptr;
    QQmlContext* context = DelegateUtils::createContext(component, this);
    if (!context) return nullptr;

    QVariantMap initialProps;
    initialProps.insert(QStringLiteral("index"), index);
    DelegateUtils::setContextProperty(component, context, QStringLiteral("index"), index);
    if (!countModel) {
        initialProps.insert(QStringLiteral("model"), rowValue);
        initialProps.insert(QStringLiteral("modelData"), rowValue);
        DelegateUtils::setContextProperty(component, context, QStringLiteral("model"), rowValue);
        DelegateUtils::setContextProperty(component, context, QStringLiteral("modelData"),
                                          rowValue);
    }
    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
        initialProps.insert(it.key(), it.value());
        DelegateUtils::setContextProperty(component, context, it.key(), it.value());
    }

    return DelegateUtils::createItem(component, initialProps, context, this, this);
}

void GridView::rebuildViewport(bool force) {
    if (!DelegateUtils::isReady(m_delegate, this)) {
        clearItems();
        return;
    }
    if (m_modelCount <= 0) {
        clearItems();
        maybeFetchMore();
        return;
    }

    // Without cell geometry there is no viewport to compute; fall back to
    // instantiating every row, as before virtualization.
    int first = 0;
    int last = m_modelCount - 1;
    if (m_cellWidth > 0 && m_cellHeight > 0) {
        const int columns = columnsPerRow();
        const int viewportHeight = (std::max)(1, height() - topPadding() - bottomPadding());
        const int firstRow = (std::max)(0, contentY() / m_cellHeight);
        const int lastRow = (contentY() + viewportHeight - 1) / m_cellHeight;
        constexpr int overscanRows = 1;
        first = (std::max)(0, (firstRow - overscanRows) * columns);
        last = (std::min)(m_modelCount - 1, (lastRow + overscanRows + 1) * columns - 1);
    }

    if (!force && first == m_materializedFirst && last == m_materializedLast) return;

    for (auto it = m_createdItems.begin(); it != m_createdItems.end();) {
        if (it.key() >= first && it.key() <= last) {
            ++it;
            continue;
        }
        Item* item = it.value();
        it = m_createdItems.erase(it);
        releaseDelegateItem(item);
    }
    for (int index = first; index <= last; ++index) {
        if (m_createdItems.contains(index)) continue;
        if (!m_reusePool.isEmpty()) {
            Item* item = m_reusePool.takeLast();
            bindDelegateItem(item, index, false);
            item->setVisible(true);
            m_createdItems.insert(index, item);
        } else if (Item* item = createDelegateItem(index)) {
            m_createdItems.insert(index, item);
        }
    }
    m_materializedFirst = first;
    m_materializedLast = last;
    maybeFetchMore();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void GridView::maybeFetchMore() {
    auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data());
    if (!model) return;
    const int threshold = columnsPerRow(); // one overscan row
    DelegateUtils::fetchMoreIfNeeded(model, m_rootIndex, threshold, m_fetching,
                                     [this] { return m_materializedLast; });
}

void GridView::generateItems() {
    clearItems();
    disconnectModelSignals();
    m_listValues.clear();
    m_roleMap.clear();
    m_claimedNames = 0;
    m_modelCount = 0;
    if (!DelegateUtils::isReady(m_delegate, this)) {
        setContentHeight(0);
        return;
    }

    auto* model = DelegateUtils::unpackStaticModel(m_model, m_modelCount, m_listValues)
                      ? nullptr
                      : qobject_cast<QAbstractItemModel*>(m_objectModel.data());
    if (model) {
        DelegateUtils::warnIfNested(model, m_rootIndex, this, m_nestedWarned);
        const QHash<int, QByteArray> roles = model->roleNames();
        for (auto it = roles.cbegin(); it != roles.cend(); ++it)
            m_roleMap.insert(it.key(), QString::fromUtf8(it.value()));
        m_claimedNames = NumericItemRoles::claimedNames(model);
        m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));

        const quint64 generation = m_modelGeneration;
        QPointer<QAbstractItemModel> guardedModel(model);
        const auto live = [this, guardedModel, generation] {
            return guardedModel && generation == m_modelGeneration &&
                   guardedModel.data() == m_objectModel.data();
        };
        m_modelConnections.append(connect(
            model, &QAbstractItemModel::dataChanged, this,
            [this, guardedModel, live](const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                       const QList<int>& changedRoles) {
                if (!live() || topLeft.parent() != m_rootIndex) return;
                for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
                    Item* item = m_createdItems.value(row, nullptr);
                    if (!item) continue;
                    const QModelIndex index = guardedModel->index(row, 0, m_rootIndex);
                    if (m_delegate && m_delegate->isBound() && m_delegateSchemaResolved &&
                        !m_delegateNeedsModelMap) {
                        for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
                            if (!changedRoles.isEmpty() && !changedRoles.contains(it.key()))
                                continue;
                            const auto propIt = m_delegateProperties.constFind(it.value());
                            if (propIt != m_delegateProperties.cend()) {
                                item->metaObject()
                                    ->property(propIt.value())
                                    .write(item, guardedModel->data(index, it.key()));
                            }
                        }
                        if (m_delegateStandardMask != 0) {
                            QVariantMap standard;
                            NumericItemRoles::apply(guardedModel, index, standard, true,
                                                    m_claimedNames, m_delegateStandardMask);
                            for (auto it = standard.cbegin(); it != standard.cend(); ++it)
                                DelegateUtils::writeDeclaredProperty(item, m_delegateProperties,
                                                                     it.key(), it.value());
                        }
                        continue;
                    }
                    QQmlContext* context =
                        m_delegate && !m_delegate->isBound() ? delegateContext(item) : nullptr;
                    QVariantMap rowValues;
                    for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
                        const QVariant value = guardedModel->data(index, it.key());
                        rowValues.insert(it.value(), value);
                        if (changedRoles.isEmpty() || changedRoles.contains(it.key())) {
                            DelegateUtils::setContextProperty(m_delegate, context, it.value(),
                                                              value);
                            setPropertyIfExists(item, it.value().toUtf8().constData(), value);
                        }
                    }
                    NumericItemRoles::apply(guardedModel, index, rowValues, true, m_claimedNames);
                    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
                        DelegateUtils::setContextProperty(m_delegate, context, it.key(),
                                                          it.value());
                        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
                    }
                    DelegateUtils::setContextProperty(m_delegate, context, QStringLiteral("model"),
                                                      rowValues);
                    DelegateUtils::setContextProperty(m_delegate, context,
                                                      QStringLiteral("modelData"), rowValues);
                    setPropertyIfExists(item, "model", rowValues);
                    setPropertyIfExists(item, "modelData", rowValues);
                }
                markDirty();
            }));
        m_modelConnections.append(
            connect(model, &QAbstractItemModel::rowsInserted, this,
                    [this, guardedModel, live](const QModelIndex& parent, int first, int last) {
                        if (!live() || parent != m_rootIndex) return;
                        insertModelRows(guardedModel, first, last);
                    }));
        m_modelConnections.append(
            connect(model, &QAbstractItemModel::rowsRemoved, this,
                    [this, guardedModel, live](const QModelIndex& parent, int first, int last) {
                        if (!live() || parent != m_rootIndex) return;
                        removeModelRows(first, last);
                    }));
        m_modelConnections.append(connect(model, &QAbstractItemModel::rowsMoved, this,
                                          [this, live](const QModelIndex& sourceParent, int, int,
                                                       const QModelIndex& destinationParent, int) {
                                              if (!live()) return;
                                              // Any move touching the shown level changes the row
                                              // order or set; rebuilding the window is cheap
                                              // because it is bounded by the viewport.
                                              if (sourceParent == m_rootIndex ||
                                                  destinationParent == m_rootIndex)
                                                  refreshFromModel();
                                          }));
        m_modelConnections.append(
            connect(model, &QAbstractItemModel::layoutChanged, this, [this, live]() {
                if (live()) refreshFromModel();
            }));
        m_modelConnections.append(
            connect(model, &QAbstractItemModel::modelReset, this, [this, live]() {
                if (live()) generateItems();
            }));
    }

    updateContentExtent();
    rebuildViewport(true);
    markDirty();
}

void GridView::refreshFromModel() {
    if (auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data()))
        m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));
    clearItems();
    updateContentExtent();
    rebuildViewport(true);
}

void GridView::insertModelRows(QAbstractItemModel* model, int first, int last) {
    if (!model || !m_delegate || first < 0 || first > last) return;
    m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));
    const int count = last - first + 1;
    QMap<int, Item*> shifted;
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
        const int index = it.key() >= first ? it.key() + count : it.key();
        if (index != it.key()) reindexItem(it.value(), index);
        shifted.insert(index, it.value());
    }
    m_createdItems = std::move(shifted);
    updateContentExtent();
    rebuildViewport(true);
}

void GridView::removeModelRows(int first, int last) {
    if (first < 0 || first > last) return;
    if (auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data()))
        m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));
    else
        m_modelCount = (std::max)(0, m_modelCount - (last - first + 1));
    const int count = last - first + 1;
    QMap<int, Item*> shifted;
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
        Item* item = it.value();
        if (it.key() >= first && it.key() <= last) {
            releaseDelegateItem(item);
            continue;
        }
        const int index = it.key() > last ? it.key() - count : it.key();
        if (index != it.key()) reindexItem(item, index);
        shifted.insert(index, item);
    }
    m_createdItems = std::move(shifted);
    updateContentExtent();
    rebuildViewport(true);
}

void GridView::layoutChildren() {
    if (m_cellWidth <= 0 || m_cellHeight <= 0) return;
    const int columns = columnsPerRow();
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
        Item* childItem = it.value();
        if (!childItem) continue;
        // Cells are addressed by model index so a hidden delegate keeps its slot,
        // as in Qt Quick's GridView.
        childItem->setX((it.key() % columns) * m_cellWidth);
        childItem->setY((it.key() / columns) * m_cellHeight);
    }
    updateContentExtent();
}

int GridView::implicitWidth() const {
    if (m_cellWidth > 0 && m_modelCount > 0)
        return m_cellWidth + leftPadding() + rightPadding(); // At least 1 column
    return leftPadding() + rightPadding();
}

int GridView::implicitHeight() const {
    if (m_cellHeight > 0 && m_modelCount > 0)
        return m_cellHeight + topPadding() + bottomPadding(); // At least 1 row
    return topPadding() + bottomPadding();
}

void GridView::updateLayout() {
    ScrollView::updateLayout();
    rebuildViewport();
    layoutChildren();
}

} // namespace qmltui
