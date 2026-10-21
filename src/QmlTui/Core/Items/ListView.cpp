#include <QmlTui/Core/Items/ListView.h>
#include <QmlTui/Core/Items/DelegateUtils.h>
#include <QmlTui/Core/Items/NumericItemRoles_p.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QAbstractItemModel>
#include <QQmlPropertyMap>
#include <QQmlListReference>
#include <algorithm>
#include <utility>

namespace qmltui {

static QQmlContext* delegateContext(Item* item) {
    return item ? item->findChild<QQmlContext*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
}

using DelegateUtils::setPropertyIfExists;

ListView* ListViewAttached::view() const {
    return qobject_cast<ListView*>(m_view.data());
}

void ListViewAttached::update(ListView* view, bool current) {
    if (m_view != view) {
        m_view = view;
        emit viewChanged();
    }
    if (m_isCurrentItem == current) return;
    m_isCurrentItem = current;
    emit isCurrentItemChanged();
}

ListViewAttached* ListView::qmlAttachedProperties(QObject* object) {
    return new ListViewAttached(object);
}

ListView::ListView(QObject* parent) : ScrollView(parent) {
    setFocusPolicy(Qt::StrongFocus);
    connect(this, &ScrollView::contentYChanged, this,
            [this]() { markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty); });
    connect(this, &Item::heightChanged, this,
            [this]() { markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty); });
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

ListView::~ListView() {
    clearItems();
}

void ListView::setRootIndex(const QModelIndex& rootIndex) {
    if (m_rootIndex == rootIndex) return;
    m_rootIndex = rootIndex;
    m_nestedWarned = false;
    emit rootIndexChanged();
    if (qobject_cast<QAbstractItemModel*>(m_objectModel.data())) generateItems();
}

void ListView::setModel(const QVariant& model) {
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
            clearItems();
            setContentHeight(0);
            emit modelChanged();
        });
    }
    emit modelChanged();
    generateItems();
}

void ListView::setDelegate(QQmlComponent* delegate) {
    if (m_delegate == delegate) return;
    if (m_delegate) disconnect(m_delegate, nullptr, this, nullptr);
    m_delegate = delegate;
    if (m_delegate) {
        connect(m_delegate, &QObject::destroyed, this, [this]() {
            m_delegate.clear();
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

void ListView::setOrientation(Qt::Orientation orientation) {
    if (m_orientation == orientation) return;
    m_orientation = orientation;
    emit orientationChanged();
    generateItems();
}

void ListView::setSpacing(int spacing) {
    const int sanitized = (std::max)(0, spacing);
    if (m_spacing == sanitized) return;
    m_spacing = sanitized;
    emit spacingChanged();
    updateContentExtent();
    rebuildViewport(true);
}

void ListView::setCurrentIndex(int index) {
    const int maxIndex = m_modelCount - 1;
    // `currentIndex: 0` is usually assigned before the `model` binding
    // resolves; keep it until rows exist, as Qt Quick's ListView does.
    m_pendingCurrentIndex = maxIndex < 0 && index >= 0 ? index : -1;
    const int normalized = index < 0 || maxIndex < 0 ? -1 : (std::min)(index, maxIndex);
    if (m_currentIndex == normalized) return;
    const int previousIndex = m_currentIndex;
    m_currentIndex = normalized;
    emit currentIndexChanged();
    const int oldContentY = contentY();
    if (m_orientation == Qt::Vertical && normalized >= 0) {
        const int viewportHeight = (std::max)(0, height() - topPadding() - bottomPadding());
        const int rowTop = normalized * delegateStride();
        const int rowBottom = rowTop + m_delegateExtent;
        if (viewportHeight <= 0 || rowTop < contentY())
            setContentY(rowTop);
        else if (rowBottom > contentY() + viewportHeight)
            setContentY(rowBottom - viewportHeight);
    }
    if (m_createdItems.isEmpty() || contentY() != oldContentY) rebuildViewport();
    if (Item* prevItem = m_createdItems.value(previousIndex, nullptr))
        updateDelegateState(prevItem, previousIndex);
    if (Item* currItem = m_createdItems.value(m_currentIndex, nullptr))
        updateDelegateState(currItem, m_currentIndex);
    markDirty();
}

void ListView::setReuseItems(bool reuse) {
    if (m_reuseItems == reuse) return;
    m_reuseItems = reuse;
    if (!m_reuseItems) {
        while (!m_reusePool.isEmpty()) {
            Item* item = m_reusePool.pop();
            if (item) {
                item->setParentItem(nullptr);
                delete item;
            }
        }
    }
    emit reuseItemsChanged();
}

void ListView::disconnectModelSignals() {
    for (const QMetaObject::Connection& connection : std::as_const(m_modelConnections))
        disconnect(connection);
    m_modelConnections.clear();
}

void ListView::updateDelegateState(Item* item, int index) {
    if (!item) return;
    DelegateUtils::setContextProperty(m_delegate, delegateContext(item), QStringLiteral("index"),
                                      index);
    setPropertyIfExists(item, "index", index);
    auto* attached =
        qobject_cast<ListViewAttached*>(qmlAttachedPropertiesObject<ListView>(item, true));
    if (attached) attached->update(this, index == m_currentIndex);
}

void ListView::refreshDelegateStates() {
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it)
        updateDelegateState(it.value(), it.key());
}

void ListView::keyPressEvent(KeyEvent* event) {
    int total = m_modelCount;
    const int page = (std::max)(1, height() / delegateStride());
    if (event->key() == Qt::Key_Up) {
        if (m_currentIndex > 0) setCurrentIndex(m_currentIndex - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Down) {
        if (m_currentIndex < total - 1) setCurrentIndex(m_currentIndex + 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageUp) {
        setCurrentIndex((std::max)(0, m_currentIndex - page));
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageDown) {
        setCurrentIndex((std::min)(total - 1, m_currentIndex + page));
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Home) {
        if (total > 0) setCurrentIndex(0);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_End) {
        if (total > 0) setCurrentIndex(total - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Space) {
        emit itemActivated(m_currentIndex);
        event->setAccepted(true);
    }
}

void ListView::clearItems() {
    for (Item* item : std::as_const(m_createdItems)) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_createdItems.clear();
    while (!m_reusePool.isEmpty()) {
        Item* item = m_reusePool.pop();
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_materializedFirst = -1;
    m_materializedLast = -1;
}

void ListView::recycleDelegateItem(Item* item) {
    if (!item) return;
    const bool poolable =
        m_reuseItems && !(m_delegate && QQmlListReference(m_delegate, "choices").isValid());
    if (!poolable) {
        item->setParentItem(nullptr);
        delete item;
        return;
    }
    item->setVisible(false);
    if (auto* attached =
            qobject_cast<ListViewAttached*>(qmlAttachedPropertiesObject<ListView>(item, false))) {
        emit attached->pooled();
    }
    m_reusePool.push(item);
}

void ListView::rebindDelegateItem(Item* item, int index) {
    if (!item || index < 0 || index >= m_modelCount) return;

    // Bound delegates have no per-row context; the row map is parented to the
    // item in both cases and required properties are written directly.
    QQmlContext* context = delegateContext(item);
    QQmlPropertyMap* rowMap =
        item->findChild<QQmlPropertyMap*>(QString(), Qt::FindDirectChildrenOnly);

    QVariant rowValue;
    QVariantMap rowValues;

    if (!m_listValues.isEmpty() && index < m_listValues.size()) {
        rowValue = m_listValues.at(index);
        QVariant normalized = rowValue;
        if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
            normalized = normalized.value<QJSValue>().toVariant();
        if (normalized.canConvert<QVariantMap>()) rowValues = normalized.toMap();
    } else if (auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data())) {
        const QModelIndex modelIndex = model->index(index, 0, m_rootIndex);
        for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
            const QVariant value = model->data(modelIndex, it.key());
            if (rowMap) rowMap->insert(it.value(), value);
            rowValues.insert(it.value(), value);
        }
        NumericItemRoles::apply(model, modelIndex, rowValues, true);
        if (rowMap)
            for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
                rowMap->insert(it.key(), it.value());
    }

    QQmlComponent* component = DelegateUtils::resolveDelegate(m_delegate, index, 0, rowValues);

    if (rowMap) {
        DelegateUtils::setContextProperty(component, context, "model", QVariant::fromValue(rowMap));
        DelegateUtils::setContextProperty(component, context, "modelData",
                                          QVariant::fromValue(rowMap));
    } else {
        DelegateUtils::setContextProperty(component, context, "model", rowValue);
        DelegateUtils::setContextProperty(component, context, "modelData", rowValue);
    }

    DelegateUtils::setContextProperty(component, context, "index", index);
    setPropertyIfExists(item, "index", index);

    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
        DelegateUtils::setContextProperty(component, context, it.key(), it.value());
        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
    }

    updateDelegateState(item, index);
    item->setVisible(true);

    if (auto* attached =
            qobject_cast<ListViewAttached*>(qmlAttachedPropertiesObject<ListView>(item, false))) {
        emit attached->reused();
    }
}

int ListView::delegateStride() const {
    return (std::max)(1, (std::max)(1, m_delegateExtent) + m_spacing);
}

void ListView::updateContentExtent() {
    if (m_orientation != Qt::Vertical) {
        setContentHeight(0);
        return;
    }
    const int total = m_modelCount > 0 ? m_modelCount * delegateStride() - m_spacing : 0;
    setContentHeight((std::max)(0, total));
}

Item* ListView::createDelegateItem(int index) {
    if (index < 0 || index >= m_modelCount) return nullptr;

    QVariant rowValue;
    QVariantMap rowValues;
    QQmlPropertyMap* rowMap = nullptr;
    if (!m_listValues.isEmpty() && index < m_listValues.size()) {
        rowValue = m_listValues.at(index);
        QVariant normalized = rowValue;
        if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
            normalized = normalized.value<QJSValue>().toVariant();
        if (normalized.canConvert<QVariantMap>()) rowValues = normalized.toMap();
    } else if (auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data())) {
        rowMap = new QQmlPropertyMap(this);
        const QModelIndex modelIndex = model->index(index, 0, m_rootIndex);
        for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
            const QVariant value = model->data(modelIndex, it.key());
            rowMap->insert(it.value(), value);
            rowValues.insert(it.value(), value);
        }
        NumericItemRoles::apply(model, modelIndex, rowValues);
        for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
            rowMap->insert(it.key(), it.value());
    }

    QQmlComponent* component = DelegateUtils::resolveDelegate(m_delegate, index, 0, rowValues);
    if (!DelegateUtils::isReady(component, this)) {
        delete rowMap;
        return nullptr;
    }

    QQmlContext* context = DelegateUtils::createContext(component, this);
    if (!context) {
        delete rowMap;
        return nullptr;
    }

    QVariantMap initialProps;
    initialProps["index"] = index;
    DelegateUtils::setContextProperty(component, context, "index", index);

    if (rowMap) {
        initialProps["model"] = QVariant::fromValue(rowMap);
        initialProps["modelData"] = QVariant::fromValue(rowMap);
        DelegateUtils::setContextProperty(component, context, "model", rowMap);
        DelegateUtils::setContextProperty(component, context, "modelData", rowMap);
    } else {
        initialProps["model"] = rowValue;
        initialProps["modelData"] = rowValue;
        DelegateUtils::setContextProperty(component, context, "model", rowValue);
        DelegateUtils::setContextProperty(component, context, "modelData", rowValue);
    }

    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
        initialProps[it.key()] = it.value();
        DelegateUtils::setContextProperty(component, context, it.key(), it.value());
    }

    Item* item = DelegateUtils::createItem(component, initialProps, context, this, this);
    if (!item) {
        delete rowMap;
        return nullptr;
    }
    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
    if (rowMap) rowMap->setParent(item);
    updateDelegateState(item, index);
    if (m_orientation == Qt::Vertical && !m_delegateExtentKnown) {
        m_delegateExtent = (std::max)(1, item->height());
        m_delegateExtentKnown = true;
    }
    return item;
}

void ListView::rebuildViewport(bool force) {
    if (!DelegateUtils::isReady(m_delegate, this)) {
        clearItems();
        return;
    }
    if (m_modelCount <= 0) {
        clearItems();
        maybeFetchMore();
        return;
    }

    int first = 0;
    int last = m_modelCount - 1;
    if (m_orientation == Qt::Vertical) {
        const int stride = delegateStride();
        const int viewportHeight = (std::max)(0, height() - topPadding() - bottomPadding());
        const int firstVisible = contentY() / stride;
        const int lastVisible = (contentY() + (std::max)(1, viewportHeight) - 1) / stride;
        constexpr int overscanRows = 2;
        first = (std::max)(0, firstVisible - overscanRows);
        last = (std::min)(m_modelCount - 1, lastVisible + overscanRows);
    }

    if (!force && first == m_materializedFirst && last == m_materializedLast) return;

    for (auto it = m_createdItems.begin(); it != m_createdItems.end();) {
        if (it.key() >= first && it.key() <= last) {
            ++it;
            continue;
        }
        Item* item = it.value();
        it = m_createdItems.erase(it);
        recycleDelegateItem(item);
    }

    const bool poolable =
        m_reuseItems && !(m_delegate && QQmlListReference(m_delegate, "choices").isValid());
    const int previousStride = delegateStride();
    for (int index = first; index <= last; ++index) {
        if (!m_createdItems.contains(index)) {
            Item* item = nullptr;
            if (poolable && !m_reusePool.isEmpty()) {
                item = m_reusePool.pop();
                rebindDelegateItem(item, index);
            } else {
                item = createDelegateItem(index);
            }
            if (item) {
                m_createdItems.insert(index, item);
            }
        }
    }
    m_materializedFirst = first;
    m_materializedLast = last;
    maybeFetchMore();

    if (delegateStride() != previousStride) {
        updateContentExtent();
        rebuildViewport(true);
        return;
    }
    if (force) refreshDelegateStates();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void ListView::maybeFetchMore() {
    auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data());
    if (!model) return;
    constexpr int overscanRows = 2;
    DelegateUtils::fetchMoreIfNeeded(model, m_rootIndex, overscanRows, m_fetching,
                                     [this] { return m_materializedLast; });
}

void ListView::generateItems() {
    clearItems();
    disconnectModelSignals();
    m_listValues.clear();
    m_roleMap.clear();
    m_modelCount = 0;
    m_delegateExtent = 1;
    m_delegateExtentKnown = false;
    if (!DelegateUtils::isReady(m_delegate, this)) {
        setContentHeight(0);
        return;
    }

    if (DelegateUtils::unpackStaticModel(m_model, m_modelCount, m_listValues)) {
        // Model count and list values unpacked
    } else if (auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data())) {
        DelegateUtils::warnIfNested(model, m_rootIndex, this, m_nestedWarned);
        const QHash<int, QByteArray> roles = model->roleNames();
        for (auto it = roles.cbegin(); it != roles.cend(); ++it)
            m_roleMap.insert(it.key(), QString::fromUtf8(it.value()));
        m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));

        const quint64 generation = m_modelGeneration;
        QPointer<QAbstractItemModel> guardedModel(model);
        m_modelConnections.append(connect(
            model, &QAbstractItemModel::dataChanged, this,
            [this, guardedModel, generation](const QModelIndex& topLeft,
                                             const QModelIndex& bottomRight,
                                             const QVector<int>& changedRoles) {
                if (!guardedModel || generation != m_modelGeneration ||
                    guardedModel.data() != m_objectModel.data() || topLeft.parent() != m_rootIndex)
                    return;
                for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
                    Item* item = m_createdItems.value(row, nullptr);
                    if (!item) continue;
                    // Bound delegates have no per-row context; the row map is
                    // parented to the item in both cases.
                    QQmlContext* context = delegateContext(item);
                    auto* rowMap =
                        item->findChild<QQmlPropertyMap*>(QString(), Qt::FindDirectChildrenOnly);
                    const QModelIndex modelIndex = guardedModel->index(row, 0, m_rootIndex);
                    for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
                        if (!changedRoles.isEmpty() && !changedRoles.contains(it.key())) continue;
                        const QVariant value = guardedModel->data(modelIndex, it.key());
                        if (rowMap) rowMap->insert(it.value(), value);
                        DelegateUtils::setContextProperty(m_delegate, context, it.value(), value);
                        setPropertyIfExists(item, it.value().toUtf8().constData(), value);
                    }
                    QVariantMap standard;
                    NumericItemRoles::apply(guardedModel, modelIndex, standard, true);
                    for (auto it = standard.cbegin(); it != standard.cend(); ++it) {
                        if (rowMap) rowMap->insert(it.key(), it.value());
                        DelegateUtils::setContextProperty(m_delegate, context, it.key(),
                                                          it.value());
                        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
                    }
                }
                markDirty();
            }));
        m_modelConnections.append(connect(
            model, &QAbstractItemModel::rowsInserted, this,
            [this, guardedModel, generation](const QModelIndex& parent, int first, int last) {
                if (!guardedModel || generation != m_modelGeneration ||
                    guardedModel.data() != m_objectModel.data() || parent != m_rootIndex)
                    return;
                insertModelRows(guardedModel, first, last, m_roleMap);
            }));
        m_modelConnections.append(connect(
            model, &QAbstractItemModel::rowsRemoved, this,
            [this, guardedModel, generation](const QModelIndex& parent, int first, int last) {
                if (!guardedModel || generation != m_modelGeneration ||
                    guardedModel.data() != m_objectModel.data() || parent != m_rootIndex)
                    return;
                removeModelRows(first, last);
            }));
        m_modelConnections.append(
            connect(model, &QAbstractItemModel::rowsMoved, this,
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
                        moveModelRows(first, last, destination);
                    }));
        m_modelConnections.append(connect(model, &QAbstractItemModel::layoutChanged, this,
                                          [this, guardedModel, generation]() {
                                              if (guardedModel && generation == m_modelGeneration &&
                                                  guardedModel.data() == m_objectModel.data())
                                                  generateItems();
                                          }));
        m_modelConnections.append(connect(model, &QAbstractItemModel::modelReset, this,
                                          [this, guardedModel, generation]() {
                                              if (guardedModel && generation == m_modelGeneration &&
                                                  guardedModel.data() == m_objectModel.data())
                                                  generateItems();
                                          }));
    }

    if (m_pendingCurrentIndex >= 0 && m_modelCount > 0) {
        m_currentIndex = (std::min)(m_pendingCurrentIndex, m_modelCount - 1);
        m_pendingCurrentIndex = -1;
        emit currentIndexChanged();
    } else if (m_currentIndex >= m_modelCount) {
        m_currentIndex = m_modelCount - 1;
        emit currentIndexChanged();
    }
    updateContentExtent();
    rebuildViewport(true);
}

void ListView::insertModelRows(QAbstractItemModel* model, int first, int last,
                               const QHash<int, QString>& roleMap) {
    Q_UNUSED(roleMap)
    if (!model || !m_delegate || first < 0 || first > last) return;
    if (m_currentIndex >= first) {
        m_currentIndex += last - first + 1;
        emit currentIndexChanged();
    }
    m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));

    const int count = last - first + 1;
    QMap<int, Item*> newCreatedItems;
    for (auto it = m_createdItems.begin(); it != m_createdItems.end(); ++it) {
        int idx = it.key();
        if (idx >= first) {
            newCreatedItems.insert(idx + count, it.value());
        } else {
            newCreatedItems.insert(idx, it.value());
        }
    }
    m_createdItems = newCreatedItems;

    updateContentExtent();
    rebuildViewport(true);
}

void ListView::removeModelRows(int first, int last) {
    if (first < 0 || first > last) return;
    const int oldCurrentIndex = m_currentIndex;
    if (auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data()))
        m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));
    else
        m_modelCount = (std::max)(0, m_modelCount - (last - first + 1));
    if (oldCurrentIndex > last)
        m_currentIndex = oldCurrentIndex - (last - first + 1);
    else if (oldCurrentIndex >= first)
        m_currentIndex = (std::min)(first, m_modelCount - 1);
    if (m_currentIndex != oldCurrentIndex) emit currentIndexChanged();

    const int count = last - first + 1;
    for (int idx = first; idx <= last; ++idx) {
        if (m_createdItems.contains(idx)) {
            Item* item = m_createdItems.take(idx);
            recycleDelegateItem(item);
        }
    }

    QMap<int, Item*> newCreatedItems;
    for (auto it = m_createdItems.begin(); it != m_createdItems.end(); ++it) {
        int idx = it.key();
        if (idx > last) {
            newCreatedItems.insert(idx - count, it.value());
        } else {
            newCreatedItems.insert(idx, it.value());
        }
    }
    m_createdItems = newCreatedItems;

    updateContentExtent();
    rebuildViewport(true);
}

void ListView::moveModelRows(int first, int last, int destination) {
    if (first < 0 || last < first || last >= m_modelCount) return;
    const int movedCount = last - first + 1;
    int insertion = destination > first ? destination - movedCount : destination;
    insertion = std::clamp(insertion, 0, m_modelCount - movedCount);

    const int oldCurrentIndex = m_currentIndex;
    if (oldCurrentIndex >= first && oldCurrentIndex <= last) {
        m_currentIndex = insertion + oldCurrentIndex - first;
    } else if (destination > last && oldCurrentIndex > last && oldCurrentIndex < destination) {
        m_currentIndex -= movedCount;
    } else if (destination < first && oldCurrentIndex >= destination && oldCurrentIndex < first) {
        m_currentIndex += movedCount;
    }
    if (m_currentIndex != oldCurrentIndex) emit currentIndexChanged();
    clearItems();
    rebuildViewport(true);
}

void ListView::layoutChildren() {
    int currentPos = 0;
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
        Item* childItem = it.value();
        if (childItem && childItem->visible()) {
            if (m_orientation == Qt::Vertical) {
                childItem->setX(0);
                childItem->setY(it.key() * delegateStride());
            } else {
                childItem->setX(currentPos);
                childItem->setY(0);
                currentPos += childItem->width() + m_spacing;
            }
        }
    }

    updateContentExtent();
}

void ListView::mousePressEvent(MouseEvent* event) {
    ScrollView::mousePressEvent(event);
    if (event->isAccepted()) return;

    setFocus(true);
    int clickY = event->y() + contentY() - topPadding();
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
        Item* item = it.value();
        if (item && item->visible()) {
            if (clickY >= item->y() && clickY < item->y() + item->height()) {
                setCurrentIndex(it.key());
                emit itemActivated(it.key());
                event->setAccepted(true);
                break;
            }
        }
    }
}

int ListView::implicitWidth() const {
    if (m_orientation == Qt::Vertical) {
        int maxW = 0;
        for (Item* childItem : m_createdItems) {
            if (childItem && childItem->visible() && childItem->width() > maxW)
                maxW = childItem->width();
        }
        return maxW + leftPadding() + rightPadding();
    } else {
        int totalW = 0;
        for (Item* childItem : m_createdItems) {
            if (childItem && childItem->visible()) totalW += childItem->width() + m_spacing;
        }
        if (totalW > 0) totalW -= m_spacing;
        return totalW + leftPadding() + rightPadding();
    }
}

int ListView::implicitHeight() const {
    if (m_orientation == Qt::Vertical) {
        const int totalH = m_modelCount > 0 ? m_modelCount * delegateStride() - m_spacing : 0;
        return totalH + topPadding() + bottomPadding();
    } else {
        int maxH = 0;
        for (Item* childItem : m_createdItems) {
            if (childItem && childItem->visible() && childItem->height() > maxH)
                maxH = childItem->height();
        }
        return maxH + topPadding() + bottomPadding();
    }
}

void ListView::updateLayout() {
    ScrollView::updateLayout();
    rebuildViewport();
    layoutChildren();
}

} // namespace qmltui
