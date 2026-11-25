#include <QmlTui/Core/Items/ListView.h>
#include <QmlTui/Core/Items/DelegateUtils.h>
#include <QmlTui/Core/Items/ModelSignalConnections_p.h>
#include <QmlTui/Core/Items/NumericItemRoles_p.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QAbstractItemModel>
#include <QQmlListReference>
#include <algorithm>
#include <utility>

namespace qmltui {

using DelegateUtils::delegateContext;
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
    connect(this, &ScrollView::contentXChanged, this,
            [this]() { markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty); });
    connect(this, &ScrollView::contentYChanged, this,
            [this]() { markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty); });
    connect(this, &Item::widthChanged, this,
            [this]() { markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty); });
    connect(this, &Item::heightChanged, this,
            [this]() { markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty); });
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

ListView::~ListView() {
    disconnectModelSignals();
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
            const int previousCount = m_modelCount;
            m_objectModel.clear();
            m_model.clear();
            m_modelCount = 0;
            m_listValues.clear();
            m_roleMap.clear();
            m_claimedNames = 0;
            clearItems();
            setContentWidth(0);
            setContentHeight(0);
            if (previousCount != 0) emit countChanged();
            emit modelDataChanged();
            emit modelChanged();
        });
    }
    emit modelChanged();
    generateItems();
}

QVariant ListView::itemData(int index, const QString& role) const {
    if (index < 0 || index >= m_modelCount) return {};

    if (!m_listValues.isEmpty() && index < m_listValues.size()) {
        QVariant value = m_listValues.at(index);
        if (value.userType() == QMetaType::fromType<QJSValue>().id())
            value = value.value<QJSValue>().toVariant();
        if (!role.isEmpty()) {
            if (value.canConvert<QVariantMap>()) return value.toMap().value(role);
            return {};
        }
        return value;
    }

    if (auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data())) {
        const QModelIndex modelIndex = model->index(index, 0, m_rootIndex);
        if (!modelIndex.isValid()) return {};
        const QHash<int, QByteArray> roles = model->roleNames();
        if (!role.isEmpty()) {
            const QByteArray roleBytes = role.toUtf8();
            for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
                if (it.value() == roleBytes) return model->data(modelIndex, it.key());
            }
            if (role == QLatin1String("display") &&
                NumericItemRoles::hasStandardNumericRole(roles, Qt::DisplayRole, "display")) {
                return model->data(modelIndex, Qt::DisplayRole);
            }
            if (role == QLatin1String("edit") &&
                NumericItemRoles::hasStandardNumericRole(roles, Qt::EditRole, "edit")) {
                return model->data(modelIndex, Qt::EditRole);
            }
            return {};
        }
        QVariantMap rowValues;
        for (auto it = roles.cbegin(); it != roles.cend(); ++it)
            rowValues.insert(QString::fromUtf8(it.value()), model->data(modelIndex, it.key()));
        if (!rowValues.contains(QStringLiteral("display")) &&
            NumericItemRoles::hasStandardNumericRole(roles, Qt::DisplayRole, "display")) {
            const QVariant displayValue = model->data(modelIndex, Qt::DisplayRole);
            if (displayValue.isValid()) rowValues.insert(QStringLiteral("display"), displayValue);
        }
        NumericItemRoles::apply(model, modelIndex, rowValues, false,
                                NumericItemRoles::claimedNames(model));
        return rowValues;
    }

    return role.isEmpty() ? QVariant(index) : QVariant();
}

void ListView::setDelegate(QQmlComponent* delegate) {
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
            setContentWidth(0);
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
    QPointer<ListView> self(this);
    emit currentIndexChanged();
    if (!self) return;
    const int oldContentX = contentX();
    const int oldContentY = contentY();
    if (normalized >= 0) {
        // Ensure content extent is up to date before clamping scroll position.
        updateContentExtent();
        if (m_orientation == Qt::Vertical) {
            const int viewportHeight = (std::max)(0, height() - topPadding() - bottomPadding());
            const int rowTop = normalized * delegateStride();
            const int rowBottom = rowTop + m_delegateExtent;
            if (viewportHeight <= 0 || rowTop < contentY())
                setContentY(rowTop);
            else if (rowBottom > contentY() + viewportHeight)
                setContentY(rowBottom - viewportHeight);
        } else {
            const int viewportWidth = (std::max)(0, width() - leftPadding() - rightPadding());
            const int colLeft = normalized * delegateStride();
            const int colRight = colLeft + m_delegateExtent;
            if (viewportWidth <= 0 || colLeft < contentX())
                setContentX(colLeft);
            else if (colRight > contentX() + viewportWidth)
                setContentX(colRight - viewportWidth);
        }
    }
    if (m_createdItems.isEmpty() || contentY() != oldContentY || contentX() != oldContentX)
        rebuildViewport();
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
        const auto pooled = std::exchange(m_reusePool, {});
        for (Item* item : pooled) {
            if (item) {
                item->setParentItem(nullptr);
                delete item;
            }
        }
    }
    emit reuseItemsChanged();
}

void ListView::disconnectModelSignals() {
    ModelSignalConnections::disconnectAll(m_modelConnections);
}

void ListView::updateDelegateState(Item* item, int index) {
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
    auto* attached =
        qobject_cast<ListViewAttached*>(qmlAttachedPropertiesObject<ListView>(item, true));
    if (attached) attached->update(this, index == m_currentIndex);
}

void ListView::refreshDelegateStates() {
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it)
        updateDelegateState(it.value(), it.key());
}

void ListView::keyPressEvent(KeyEvent* event) {
    const int total = m_modelCount;
    const bool horizontal = (m_orientation == Qt::Horizontal);
    const int viewportSpan = horizontal ? (std::max)(0, width() - leftPadding() - rightPadding())
                                        : (std::max)(0, height() - topPadding() - bottomPadding());
    const int page = (std::max)(1, viewportSpan / delegateStride());
    const int prevKey = horizontal ? Qt::Key_Left : Qt::Key_Up;
    const int nextKey = horizontal ? Qt::Key_Right : Qt::Key_Down;

    if (event->key() == prevKey) {
        if (m_currentIndex > 0) setCurrentIndex(m_currentIndex - 1);
        event->setAccepted(true);
    } else if (event->key() == nextKey) {
        if (m_currentIndex < total - 1) setCurrentIndex(m_currentIndex + 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageUp) {
        if (total > 0) setCurrentIndex((std::max)(0, m_currentIndex - page));
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageDown) {
        if (total > 0) setCurrentIndex((std::min)(total - 1, m_currentIndex + page));
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Home) {
        if (total > 0) setCurrentIndex(0);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_End) {
        if (total > 0) setCurrentIndex(total - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
               event->key() == Qt::Key_Space) {
        if (m_currentIndex >= 0 && m_currentIndex < total) emit itemActivated(m_currentIndex);
        event->setAccepted(true);
    }
}

void ListView::clearItems() {
    // Detach tracking containers before deleting delegates so re-entrant
    // implicit-size queries triggered by ChildRemoved never see freed items.
    const auto created = std::exchange(m_createdItems, {});
    const auto pooled = std::exchange(m_reusePool, {});
    m_materializedFirst = -1;
    m_materializedLast = -1;
    for (Item* item : created) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    for (Item* item : pooled) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
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
    if (!item || index < 0 || index >= m_modelCount || !m_delegate) return;

    auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data());
    const bool countModel = m_listValues.isEmpty() && !model;

    if (m_delegate->isBound() && m_delegateSchemaResolved) {
        if (!m_listValues.isEmpty() && index < m_listValues.size()) {
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
        } else if (m_delegateNeedsModelMap) {
            DelegateUtils::writeDeclaredProperty(item, m_delegateProperties,
                                                 QStringLiteral("model"), index);
            DelegateUtils::writeDeclaredProperty(item, m_delegateProperties,
                                                 QStringLiteral("modelData"), index);
        }
        updateDelegateState(item, index);
        item->setVisible(true);
        if (auto* attached = qobject_cast<ListViewAttached*>(
                qmlAttachedPropertiesObject<ListView>(item, false))) {
            emit attached->reused();
        }
        return;
    }

    QQmlContext* context = delegateContext(item);
    QVariant rowValue;
    QVariantMap rowValues;

    if (!m_listValues.isEmpty() && index < m_listValues.size()) {
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

    QQmlComponent* component = DelegateUtils::resolveDelegate(m_delegate, index, 0, rowValues);

    if (!countModel) {
        DelegateUtils::setContextProperty(component, context, QStringLiteral("model"), rowValue);
        DelegateUtils::setContextProperty(component, context, QStringLiteral("modelData"),
                                          rowValue);
        setPropertyIfExists(item, "model", rowValue);
        setPropertyIfExists(item, "modelData", rowValue);
    }

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
    const int total =
        (std::max)(0, m_modelCount > 0 ? m_modelCount * delegateStride() - m_spacing : 0);
    if (m_orientation == Qt::Vertical) {
        setContentWidth(0);
        setContentHeight(total);
    } else {
        setContentHeight(0);
        setContentWidth(total);
    }
}

Item* ListView::createDelegateItem(int index) {
    if (index < 0 || index >= m_modelCount || !m_delegate) return nullptr;

    const bool isChooser = QQmlListReference(m_delegate, "choices").isValid();
    const bool fastBound = m_delegate->isBound() && !isChooser;
    auto* model = qobject_cast<QAbstractItemModel*>(m_objectModel.data());
    const bool countModel = m_listValues.isEmpty() && !model;

    if (fastBound) {
        if (!DelegateUtils::isReady(m_delegate, this)) return nullptr;
        QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
        if (!context) return nullptr;

        Item* item = DelegateUtils::createItemWithSchema(
            m_delegate, context, this, this, m_delegateSchemaResolved, m_delegateProperties,
            m_delegateStandardMask, m_delegateNeedsModelMap, [&](QVariantMap& initialProps) {
                if (m_delegateProperties.contains(QStringLiteral("index")))
                    initialProps.insert(QStringLiteral("index"), index);
                if (!m_listValues.isEmpty() && index < m_listValues.size()) {
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
                        NumericItemRoles::apply(model, modelIndex, rowValues, true, m_claimedNames);
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
                            NumericItemRoles::apply(model, modelIndex, initialProps, true,
                                                    m_claimedNames, m_delegateStandardMask);
                        }
                    }
                } else if (m_delegateNeedsModelMap) {
                    initialProps.insert(QStringLiteral("model"), index);
                    initialProps.insert(QStringLiteral("modelData"), index);
                }
            });
        if (!item) return nullptr;
        updateDelegateState(item, index);
        if (!m_delegateExtentKnown) {
            m_delegateExtent =
                (std::max)(1, m_orientation == Qt::Vertical ? item->height() : item->width());
            m_delegateExtentKnown = true;
        }
        return item;
    }

    QVariant rowValue;
    QVariantMap rowValues;
    if (!m_listValues.isEmpty() && index < m_listValues.size()) {
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

    QQmlComponent* component = DelegateUtils::resolveDelegate(m_delegate, index, 0, rowValues);
    if (!DelegateUtils::isReady(component, this)) return nullptr;

    QQmlContext* context = DelegateUtils::createContext(component, this);
    if (!context) return nullptr;

    QVariantMap initialProps;
    initialProps.insert(QStringLiteral("index"), index);
    DelegateUtils::setContextProperty(component, context, QStringLiteral("index"), index);

    const QVariant modelVal = countModel ? QVariant(index) : rowValue;
    initialProps.insert(QStringLiteral("model"), modelVal);
    initialProps.insert(QStringLiteral("modelData"), modelVal);
    DelegateUtils::setContextProperty(component, context, QStringLiteral("model"), modelVal);
    DelegateUtils::setContextProperty(component, context, QStringLiteral("modelData"), modelVal);

    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
        initialProps.insert(it.key(), it.value());
        DelegateUtils::setContextProperty(component, context, it.key(), it.value());
    }

    Item* item = DelegateUtils::createItem(component, initialProps, context, this, this);
    if (!item) return nullptr;
    updateDelegateState(item, index);
    if (!m_delegateExtentKnown) {
        m_delegateExtent =
            (std::max)(1, m_orientation == Qt::Vertical ? item->height() : item->width());
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

    const int stride = delegateStride();
    const int viewportExtent = m_orientation == Qt::Vertical
                                   ? (std::max)(0, height() - topPadding() - bottomPadding())
                                   : (std::max)(0, width() - leftPadding() - rightPadding());
    const int scrollOffset = m_orientation == Qt::Vertical ? contentY() : contentX();
    const int firstVisible = scrollOffset / stride;
    const int lastVisible = (scrollOffset + (std::max)(1, viewportExtent) - 1) / stride;
    constexpr int overscanRows = 2;
    const int first = (std::max)(0, firstVisible - overscanRows);
    const int last = (std::min)(m_modelCount - 1, lastVisible + overscanRows);

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
                if (delegateStride() != previousStride) break;
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
    layoutChildren();
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
    const int previousCount = m_modelCount;
    clearItems();
    disconnectModelSignals();
    m_listValues.clear();
    m_roleMap.clear();
    m_claimedNames = 0;
    m_modelCount = 0;
    m_delegateExtent = 1;
    m_delegateExtentKnown = false;

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

        ModelSignalConnections::connectGuardedFlatRowSignals(
            m_modelConnections, model, this, m_objectModel, m_modelGeneration, m_rootIndex,
            [this](QAbstractItemModel* liveModel, int firstRow, int lastRow,
                   const QList<int>& changedRoles) {
                for (int row = firstRow; row <= lastRow; ++row) {
                    Item* item = m_createdItems.value(row, nullptr);
                    if (!item) continue;
                    QQmlContext* context =
                        m_delegate && !m_delegate->isBound() ? delegateContext(item) : nullptr;
                    ModelSignalConnections::updateDelegateRowData(
                        item, m_delegate, context, liveModel, liveModel->index(row, 0, m_rootIndex),
                        m_roleMap, changedRoles, m_delegateSchemaResolved, m_delegateNeedsModelMap,
                        m_delegateProperties, m_claimedNames, m_delegateStandardMask);
                }
                emit modelDataChanged();
                markDirty();
            },
            [this](QAbstractItemModel* liveModel, int first, int last) {
                insertModelRows(liveModel, first, last, m_roleMap);
            },
            [this](int first, int last) { removeModelRows(first, last); },
            [this](int first, int last, int destination) {
                moveModelRows(first, last, destination);
            },
            [this]() { generateItems(); }, [this]() { generateItems(); });
    }

    if (m_pendingCurrentIndex >= 0 && m_modelCount > 0) {
        m_currentIndex = (std::min)(m_pendingCurrentIndex, m_modelCount - 1);
        m_pendingCurrentIndex = -1;
        emit currentIndexChanged();
    } else if (m_currentIndex >= m_modelCount) {
        m_currentIndex = m_modelCount - 1;
        emit currentIndexChanged();
    }
    if (m_modelCount != previousCount) emit countChanged();
    emit modelDataChanged();

    if (!DelegateUtils::isReady(m_delegate, this)) {
        setContentWidth(0);
        setContentHeight(0);
        return;
    }
    updateContentExtent();
    rebuildViewport(true);
}

void ListView::insertModelRows(QAbstractItemModel* model, int first, int last,
                               const QHash<int, QString>& roleMap) {
    Q_UNUSED(roleMap)
    if (!model || first < 0 || first > last) return;
    const QHash<int, QByteArray> roles = model->roleNames();
    if (m_roleMap.size() != roles.size()) {
        m_roleMap.clear();
        for (auto it = roles.cbegin(); it != roles.cend(); ++it)
            m_roleMap.insert(it.key(), QString::fromUtf8(it.value()));
        m_claimedNames = NumericItemRoles::claimedNames(model);
    }
    if (m_currentIndex >= first) {
        m_currentIndex += last - first + 1;
        emit currentIndexChanged();
    }
    const int previousCount = m_modelCount;
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

    if (m_modelCount != previousCount) emit countChanged();
    emit modelDataChanged();
    if (!m_delegate) return;
    updateContentExtent();
    rebuildViewport(true);
}

void ListView::removeModelRows(int first, int last) {
    if (first < 0 || first > last) return;
    const int previousCount = m_modelCount;
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

    if (m_modelCount != previousCount) emit countChanged();
    emit modelDataChanged();
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
    emit modelDataChanged();
    clearItems();
    rebuildViewport(true);
}

void ListView::layoutChildren() {
    const int stride = delegateStride();
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
        Item* childItem = it.value();
        if (childItem && childItem->visible()) {
            if (m_orientation == Qt::Vertical) {
                childItem->setX(0);
                childItem->setY(it.key() * stride);
            } else {
                childItem->setX(it.key() * stride);
                childItem->setY(0);
            }
        }
    }

    updateContentExtent();
}

void ListView::mousePressEvent(MouseEvent* event) {
    ScrollView::mousePressEvent(event);
    if (event->isAccepted()) return;

    QPointer<ListView> self(this);
    setFocus(true);
    if (!self) return;

    const int clickX = event->x() + contentX() - leftPadding();
    const int clickY = event->y() + contentY() - topPadding();
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
        Item* item = it.value();
        if (item && item->visible()) {
            const bool hit = m_orientation == Qt::Vertical
                                 ? (clickY >= item->y() && clickY < item->y() + item->height())
                                 : (clickX >= item->x() && clickX < item->x() + item->width());
            if (hit) {
                const int clickedIndex = it.key();
                setCurrentIndex(clickedIndex);
                if (!self) return;
                emit itemActivated(clickedIndex);
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
        const int totalW = m_modelCount > 0 ? m_modelCount * delegateStride() - m_spacing : 0;
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
