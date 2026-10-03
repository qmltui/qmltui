#include "ListView.h"
#include "DelegateUtils.h"
#include <ColorMapper.h>
#include <KeyEvent.h>
#include <MouseEvent.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QAbstractItemModel>
#include <QQmlPropertyMap>
#include <QQmlProperty>
#include <algorithm>
#include <utility>

namespace qtui {

static QQmlContext *delegateContext(Item *item) {
    return item ? item->findChild<QQmlContext*>(QString(), Qt::FindDirectChildrenOnly)
                : nullptr;
}

using DelegateUtils::setPropertyIfExists;

ListView *ListViewAttached::view() const
{
    return qobject_cast<ListView *>(m_view.data());
}

void ListViewAttached::update(ListView *view, bool current)
{
    if (m_view != view) {
        m_view = view;
        emit viewChanged();
    }
    if (m_isCurrentItem == current)
        return;
    m_isCurrentItem = current;
    emit isCurrentItemChanged();
}

ListViewAttached *ListView::qmlAttachedProperties(QObject *object)
{
    return new ListViewAttached(object);
}

ListView::ListView(QObject* parent) : ScrollView(parent) {
    setFocusable(true);
    connect(this, &ScrollView::contentYChanged, this,
            [this]() { rebuildViewport(); });
    connect(this, &Item::heightChanged, this,
            [this]() { rebuildViewport(); });
}

ListView::~ListView() {
    clearItems();
}

void ListView::setModel(const QVariant& model) {
    if (m_model == model) return;
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
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
    updateContentExtent();
    rebuildViewport(true);
}

void ListView::setCurrentIndex(int index) {
    const int maxIndex = m_modelCount - 1;
    const int normalized = index < 0 || maxIndex < 0
        ? -1 : (std::min)(index, maxIndex);
    if (m_currentIndex == normalized) return;
    const int previousIndex = m_currentIndex;
    m_currentIndex = normalized;
    emit currentIndexChanged();
    if (m_orientation == Qt::Vertical && normalized >= 0) {
        const int viewportHeight = (std::max)(0,
            height() - topPadding() - bottomPadding());
        const int rowTop = normalized * delegateStride();
        const int rowBottom = rowTop + m_delegateExtent;
        if (viewportHeight <= 0 || rowTop < contentY())
            setContentY(rowTop);
        else if (rowBottom > contentY() + viewportHeight)
            setContentY(rowBottom - viewportHeight);
    }
    rebuildViewport();
    if (Item *prevItem = m_createdItems.value(previousIndex, nullptr))
        updateDelegateState(prevItem, previousIndex);
    if (Item *currItem = m_createdItems.value(m_currentIndex, nullptr))
        updateDelegateState(currItem, m_currentIndex);
    markDirty();
}

void ListView::setReuseItems(bool reuse) {
    if (m_reuseItems == reuse) return;
    m_reuseItems = reuse;
    if (!m_reuseItems) {
        while (!m_reusePool.isEmpty()) {
            Item *item = m_reusePool.pop();
            if (item) {
                item->setParentItem(nullptr);
                delete item;
            }
        }
    }
    emit reuseItemsChanged();
}

void ListView::disconnectModelSignals()
{
    for (const QMetaObject::Connection &connection : std::as_const(m_modelConnections))
        disconnect(connection);
    m_modelConnections.clear();
}

void ListView::updateDelegateState(Item *item, int index)
{
    if (!item)
        return;
    setPropertyIfExists(item, "index", index);
    auto *attached = qobject_cast<ListViewAttached *>(
        qmlAttachedPropertiesObject<ListView>(item, true));
    if (attached)
        attached->update(this, index == m_currentIndex);
}

void ListView::refreshDelegateStates()
{
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it)
        updateDelegateState(it.value(), it.key());
}

void ListView::setHighlightColor(const QString& color) {
    if (m_highlightColor == color) return;
    m_highlightColor = color;
    m_resolvedHighlightColor = ColorMapper::fromString(m_highlightColor, 0xFF000000);
    emit highlightColorChanged();
    markDirty();
}

void ListView::setHighlightBgColor(const QString& color) {
    if (m_highlightBgColor == color) return;
    m_highlightBgColor = color;
    m_resolvedHighlightBgColor = ColorMapper::fromString(m_highlightBgColor, 0xFFFFFFFF);
    emit highlightBgColorChanged();
    markDirty();
}

void ListView::keyPressEvent(KeyEvent* event) {
    int total = m_modelCount;
    if (event->key() == Qt::Key_Up) {
        if (m_currentIndex > 0) setCurrentIndex(m_currentIndex - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Down) {
        if (m_currentIndex < total - 1) setCurrentIndex(m_currentIndex + 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageUp) {
        setCurrentIndex((std::max)(0, m_currentIndex - 10));
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageDown) {
        setCurrentIndex((std::min)(total - 1, m_currentIndex + 10));
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
        Item *item = m_reusePool.pop();
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_materializedFirst = -1;
    m_materializedLast = -1;
}

void ListView::recycleDelegateItem(Item *item) {
    if (!item) return;
    item->setVisible(false);
    if (auto *attached = qobject_cast<ListViewAttached *>(
            qmlAttachedPropertiesObject<ListViewAttached>(item, false))) {
        emit attached->pooled();
    }
    m_reusePool.push(item);
}

void ListView::rebindDelegateItem(Item *item, int index) {
    if (!item || index < 0 || index >= m_modelCount) return;

    QQmlContext *context = delegateContext(item);
    if (!context) return;

    QQmlPropertyMap *rowMap = qobject_cast<QQmlPropertyMap *>(
        context->contextProperty("model").value<QObject *>());

    QVariant rowValue;
    QVariantMap rowValues;

    if (!m_listValues.isEmpty() && index < m_listValues.size()) {
        rowValue = m_listValues.at(index);
        QVariant normalized = rowValue;
        if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
            normalized = normalized.value<QJSValue>().toVariant();
        if (normalized.canConvert<QVariantMap>())
            rowValues = normalized.toMap();
        DelegateUtils::setContextProperty(m_delegate, context, "model", rowValue);
        DelegateUtils::setContextProperty(m_delegate, context, "modelData", rowValue);
    } else if (auto *model = qobject_cast<QAbstractItemModel *>(m_objectModel.data())) {
        const QModelIndex modelIndex = model->index(index, 0);
        for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
            const QVariant value = model->data(modelIndex, it.key());
            if (rowMap) rowMap->insert(it.value(), value);
            rowValues.insert(it.value(), value);
        }
        if (rowMap) {
            DelegateUtils::setContextProperty(m_delegate, context, "model", QVariant::fromValue(rowMap));
            DelegateUtils::setContextProperty(m_delegate, context, "modelData", QVariant::fromValue(rowMap));
        }
    }

    DelegateUtils::setContextProperty(m_delegate, context, "index", index);
    setPropertyIfExists(item, "index", index);

    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
        DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());
        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
    }

    updateDelegateState(item, index);
    item->setVisible(true);

    if (auto *attached = qobject_cast<ListViewAttached *>(
            qmlAttachedPropertiesObject<ListViewAttached>(item, false))) {
        emit attached->reused();
    }
}

int ListView::delegateStride() const
{
    return (std::max)(1, (std::max)(1, m_delegateExtent) + m_spacing);
}

void ListView::updateContentExtent()
{
    if (m_orientation != Qt::Vertical) {
        setContentHeight(0);
        return;
    }
    const int total = m_modelCount > 0
        ? m_modelCount * delegateStride() - m_spacing : 0;
    setContentHeight((std::max)(0, total));
}

Item *ListView::createDelegateItem(int index)
{
    if (index < 0 || index >= m_modelCount)
        return nullptr;
    QQmlContext *context = DelegateUtils::createContext(m_delegate, this);
    if (!context)
        return nullptr;

    QVariantMap initialProps;
    initialProps["index"] = index;
    DelegateUtils::setContextProperty(m_delegate, context, "index", index);

    QVariant rowValue;
    QVariantMap rowValues;
    QQmlPropertyMap *rowMap = nullptr;
    if (!m_listValues.isEmpty() && index < m_listValues.size()) {
        rowValue = m_listValues.at(index);
        QVariant normalized = rowValue;
        if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
            normalized = normalized.value<QJSValue>().toVariant();
        if (normalized.canConvert<QVariantMap>())
            rowValues = normalized.toMap();
        initialProps["model"] = rowValue;
        initialProps["modelData"] = rowValue;
        DelegateUtils::setContextProperty(m_delegate, context, "model", rowValue);
        DelegateUtils::setContextProperty(m_delegate, context, "modelData", rowValue);
    } else if (auto *model = qobject_cast<QAbstractItemModel *>(m_objectModel.data())) {
        rowMap = QQmlPropertyMap::create(this);
        const QModelIndex modelIndex = model->index(index, 0);
        for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
            const QVariant value = model->data(modelIndex, it.key());
            rowMap->insert(it.value(), value);
            rowValues.insert(it.value(), value);
        }
        initialProps["model"] = QVariant::fromValue(rowMap);
        initialProps["modelData"] = QVariant::fromValue(rowMap);
        DelegateUtils::setContextProperty(m_delegate, context, "model", rowMap);
        DelegateUtils::setContextProperty(m_delegate, context, "modelData", rowMap);
    }

    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
        initialProps[it.key()] = it.value();
        DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());
    }

    Item *item = DelegateUtils::createItem(
        m_delegate, initialProps, context, this, this);
    if (!item) {
        delete rowMap;
        return nullptr;
    }
    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
    if (rowMap)
        rowMap->setParent(item);
    updateDelegateState(item, index);
    if (m_orientation == Qt::Vertical && !m_delegateExtentKnown) {
        m_delegateExtent = (std::max)(1, item->height());
        m_delegateExtentKnown = true;
    }
    return item;
}

void ListView::rebuildViewport(bool force)
{
    if (!DelegateUtils::isReady(m_delegate, this) || m_modelCount <= 0) {
        clearItems();
        return;
    }

    int first = 0;
    int last = m_modelCount - 1;
    if (m_orientation == Qt::Vertical) {
        const int stride = delegateStride();
        const int viewportHeight = (std::max)(0,
            height() - topPadding() - bottomPadding());
        const int firstVisible = contentY() / stride;
        const int lastVisible = (contentY() + (std::max)(1, viewportHeight) - 1) / stride;
        constexpr int overscanRows = 2;
        first = (std::max)(0, firstVisible - overscanRows);
        last = (std::min)(m_modelCount - 1, lastVisible + overscanRows);
    }

    if (!force && first == m_materializedFirst && last == m_materializedLast)
        return;

    for (auto it = m_createdItems.begin(); it != m_createdItems.end();) {
        if (it.key() >= first && it.key() <= last) {
            ++it;
            continue;
        }
        Item *item = it.value();
        it = m_createdItems.erase(it);
        if (item) {
            if (m_reuseItems) {
                recycleDelegateItem(item);
            } else {
                item->setParentItem(nullptr);
                delete item;
            }
        }
    }

    const int previousStride = delegateStride();
    for (int index = first; index <= last; ++index) {
        if (!m_createdItems.contains(index)) {
            Item *item = nullptr;
            if (m_reuseItems && !m_reusePool.isEmpty()) {
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

    if (delegateStride() != previousStride) {
        updateContentExtent();
        rebuildViewport(true);
        return;
    }
    refreshDelegateStates();
    markDirty();
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

    if (m_model.canConvert<int>() && m_model.userType() != qMetaTypeId<QJSValue>()) {
        m_modelCount = std::clamp(m_model.toInt(), 0, 10000);
    } else if (m_model.canConvert<QVariantList>()
               || m_model.userType() == qMetaTypeId<QJSValue>()) {
        if (m_model.userType() == qMetaTypeId<QJSValue>()) {
            const QJSValue value = m_model.value<QJSValue>();
            if (value.isArray()) {
                const int length = std::clamp(value.property("length").toInt(), 0, 10000);
                m_listValues.reserve(length);
                for (int index = 0; index < length; ++index)
                    m_listValues.append(value.property(index).toVariant());
            }
        } else {
            m_listValues = m_model.toList().mid(0, 10000);
        }
        m_modelCount = m_listValues.size();
    } else if (auto *model = qobject_cast<QAbstractItemModel *>(m_objectModel.data())) {
        const QHash<int, QByteArray> roles = model->roleNames();
        for (auto it = roles.cbegin(); it != roles.cend(); ++it)
            m_roleMap.insert(it.key(), QString::fromUtf8(it.value()));
        m_modelCount = (std::max)(0, model->rowCount());

        const quint64 generation = m_modelGeneration;
        QPointer<QAbstractItemModel> guardedModel(model);
        m_modelConnections.append(connect(model, &QAbstractItemModel::dataChanged, this,
            [this, guardedModel, generation](const QModelIndex &topLeft,
                                             const QModelIndex &bottomRight,
                                             const QVector<int> &changedRoles) {
                if (!guardedModel || generation != m_modelGeneration
                    || guardedModel.data() != m_objectModel.data()
                    || topLeft.parent().isValid()) return;
                for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
                    Item *item = m_createdItems.value(row, nullptr);
                    QQmlContext *context = delegateContext(item);
                    if (!item || !context) continue;
                    auto *rowMap = qobject_cast<QQmlPropertyMap *>(
                        context->contextProperty("model").value<QObject *>());
                    const QModelIndex modelIndex = guardedModel->index(row, 0);
                    for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
                        if (!changedRoles.isEmpty() && !changedRoles.contains(it.key()))
                            continue;
                        const QVariant value = guardedModel->data(modelIndex, it.key());
                        if (rowMap) rowMap->insert(it.value(), value);
                        DelegateUtils::setContextProperty(m_delegate, context, it.value(), value);
                        setPropertyIfExists(item, it.value().toUtf8().constData(), value);
                    }
                }
                markDirty();
            }));
        m_modelConnections.append(connect(model, &QAbstractItemModel::rowsInserted, this,
            [this, guardedModel, generation](const QModelIndex &parent, int first, int last) {
                if (!guardedModel || generation != m_modelGeneration
                    || guardedModel.data() != m_objectModel.data() || parent.isValid()) return;
                insertModelRows(guardedModel, first, last, m_roleMap);
            }));
        m_modelConnections.append(connect(model, &QAbstractItemModel::rowsRemoved, this,
            [this, guardedModel, generation](const QModelIndex &parent, int first, int last) {
                if (!guardedModel || generation != m_modelGeneration
                    || guardedModel.data() != m_objectModel.data() || parent.isValid()) return;
                removeModelRows(first, last);
            }));
        m_modelConnections.append(connect(model, &QAbstractItemModel::rowsMoved, this,
            [this, guardedModel, generation](const QModelIndex &sourceParent, int first, int last,
                                             const QModelIndex &destinationParent, int destination) {
                if (!guardedModel || generation != m_modelGeneration
                    || guardedModel.data() != m_objectModel.data()
                    || sourceParent.isValid() || destinationParent.isValid()) return;
                moveModelRows(first, last, destination);
            }));
        m_modelConnections.append(connect(model, &QAbstractItemModel::layoutChanged, this,
            [this, guardedModel, generation]() {
                if (guardedModel && generation == m_modelGeneration
                    && guardedModel.data() == m_objectModel.data()) generateItems();
            }));
        m_modelConnections.append(connect(model, &QAbstractItemModel::modelReset, this,
            [this, guardedModel, generation]() {
                if (guardedModel && generation == m_modelGeneration
                    && guardedModel.data() == m_objectModel.data()) generateItems();
            }));
    }

    if (m_currentIndex >= m_modelCount) {
        m_currentIndex = m_modelCount - 1;
        emit currentIndexChanged();
    }
    updateContentExtent();
    rebuildViewport(true);
}

void ListView::insertModelRows(QAbstractItemModel *model, int first, int last,
                               const QHash<int, QString> &roleMap) {
    Q_UNUSED(roleMap)
    if (!model || !m_delegate || first < 0 || first > last) return;
    if (m_currentIndex >= first) {
        m_currentIndex += last - first + 1;
        emit currentIndexChanged();
    }
    m_modelCount = (std::max)(0, model->rowCount());

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
    const int oldCurrentIndex = m_currentIndex;
    if (auto *model = qobject_cast<QAbstractItemModel *>(m_objectModel.data()))
        m_modelCount = (std::max)(0, model->rowCount());
    else
        m_modelCount = (std::max)(0, m_modelCount - (last - first + 1));
    if (oldCurrentIndex > last)
        m_currentIndex = oldCurrentIndex - (last - first + 1);
    else if (oldCurrentIndex >= first)
        m_currentIndex = std::min(first, m_modelCount - 1);
    if (m_currentIndex != oldCurrentIndex)
        emit currentIndexChanged();

    const int count = last - first + 1;
    for (int idx = first; idx <= last; ++idx) {
        if (m_createdItems.contains(idx)) {
            Item *item = m_createdItems.take(idx);
            if (item) {
                if (m_reuseItems) recycleDelegateItem(item);
                else {
                    item->setParentItem(nullptr);
                    delete item;
                }
            }
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
    } else if (destination > last && oldCurrentIndex > last
               && oldCurrentIndex < destination) {
        m_currentIndex -= movedCount;
    } else if (destination < first && oldCurrentIndex >= destination
               && oldCurrentIndex < first) {
        m_currentIndex += movedCount;
    }
    if (m_currentIndex != oldCurrentIndex)
        emit currentIndexChanged();
    clearItems();
    rebuildViewport(true);
}

void ListView::layoutChildren() {
    int currentPos = 0;
    for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
        Item *childItem = it.value();
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
        Item *item = it.value();
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
             if (childItem && childItem->visible())
                 totalW += childItem->width() + m_spacing;
        }
        if (totalW > 0) totalW -= m_spacing;
        return totalW + leftPadding() + rightPadding();
    }
}

int ListView::implicitHeight() const {
    if (m_orientation == Qt::Vertical) {
        const int totalH = m_modelCount > 0
            ? m_modelCount * delegateStride() - m_spacing : 0;
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

void ListView::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!visible()) return;
    rebuildViewport();
    layoutChildren();
    ScrollView::render(buffer, absX, absY, clipRect);
}

} // namespace qtui
