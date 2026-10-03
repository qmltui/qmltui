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
            clearItems();
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
    markDirty();
}

void ListView::setSpacing(int spacing) {
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
    markDirty();
}

void ListView::setCurrentIndex(int index) {
    const int maxIndex = static_cast<int>(m_createdItems.size()) - 1;
    const int normalized = index < 0 || maxIndex < 0
        ? -1 : (std::min)(index, maxIndex);
    if (m_currentIndex == normalized) return;
    m_currentIndex = normalized;
    emit currentIndexChanged();
    refreshDelegateStates();
    markDirty();
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
    for (int index = 0; index < m_createdItems.size(); ++index)
        updateDelegateState(m_createdItems[index], index);
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
    int total = m_createdItems.size();
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
    for (Item* item : m_createdItems) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_createdItems.clear();
}

void ListView::generateItems() {
    clearItems();
    
    disconnectModelSignals();
    if (!DelegateUtils::isReady(m_delegate, this)) return;
    
    Item* contentItem = this;

    int count = 0;
    if (m_model.canConvert<int>() && m_model.userType() != qMetaTypeId<QJSValue>()) {
        count = std::clamp(m_model.toInt(), 0, 10000);
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
            if (!context) {
                m_createdItems.append(nullptr);
                continue;
            }
            QVariantMap initialProps;
            initialProps["index"] = i;

            DelegateUtils::setContextProperty(m_delegate, context, "index", i);
            if (Item *item = DelegateUtils::createItem(
                    m_delegate, initialProps, context, contentItem, this)) {
                m_createdItems.append(item);
            } else {
                m_createdItems.append(nullptr);
            }
        }
    } else if (m_model.canConvert<QVariantList>() || m_model.userType() == qMetaTypeId<QJSValue>()) {
        QVariantList list;
        if (m_model.userType() == qMetaTypeId<QJSValue>()) {
            QJSValue val = m_model.value<QJSValue>();
            if (val.isArray()) {
                int len = val.property("length").toInt();
                for (int i = 0; i < len; ++i) {
                    list.append(val.property(i).toVariant());
                }
            }
        } else if (m_model.canConvert<QVariantList>()) {
            list = m_model.toList();
        }

        count = std::clamp(static_cast<int>(list.size()), 0, 10000);
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
            if (!context) {
                m_createdItems.append(nullptr);
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

            Item *item = DelegateUtils::createItem(
                m_delegate, initialProps, context, contentItem, this);
            if (item) {
                if (elem.canConvert<QVariantMap>()) {
                    QVariantMap map = elem.toMap();
                    for (auto it = map.cbegin(); it != map.cend(); ++it) {
                        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
                    }
                }
                m_createdItems.append(item);
            } else {
                m_createdItems.append(nullptr);
            }
        }
    } else if (m_model.canConvert<QObject*>()) {
        QObject* modelObj = m_model.value<QObject*>();
        if (modelObj && modelObj->inherits("QAbstractItemModel")) {
            QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(modelObj);
            if (model) {
                QHash<int, QString> roleMap;
                const QHash<int, QByteArray> roles = model->roleNames();
                for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
                    roleMap.insert(it.key(), QString::fromUtf8(it.value()));
                }

                const quint64 generation = m_modelGeneration;
                QPointer<QAbstractItemModel> guardedModel(model);
                m_modelConnections.append(connect(model, &QAbstractItemModel::dataChanged, this, [this, guardedModel, generation, roleMap](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &changedRoles) {
                    if (!guardedModel || generation != m_modelGeneration
                        || guardedModel.data() != m_objectModel.data()) return;
                    if (topLeft.parent().isValid()) return;
                    for (int r = topLeft.row(); r <= bottomRight.row(); ++r) {
                        if (r >= 0 && r < m_createdItems.size()) {
                            Item* item = m_createdItems[r];
                            if (!item) continue;
                            QQmlContext* context = delegateContext(item);
                            if (!context) continue;
                            QVariant modelVal = context->contextProperty("model");
                            QQmlPropertyMap* rowMap = qobject_cast<QQmlPropertyMap*>(modelVal.value<QObject*>());
                            const QModelIndex modelIndex = guardedModel->index(r, 0);
                            for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                                if (!changedRoles.isEmpty() && !changedRoles.contains(it.key())) continue;
                                QVariant val = guardedModel->data(modelIndex, it.key());
                                if (rowMap) rowMap->insert(it.value(), val);
                                DelegateUtils::setContextProperty(m_delegate, context, it.value(), val);
                                setPropertyIfExists(item, it.value().toUtf8().constData(), val);
                            }
                        }
                    }
                }));
                m_modelConnections.append(connect(model, &QAbstractItemModel::rowsInserted, this,
                        [this, guardedModel, generation, roleMap](const QModelIndex &parent, int first, int last) {
                    if (!guardedModel || generation != m_modelGeneration
                        || guardedModel.data() != m_objectModel.data()) return;
                    if (parent.isValid()) return;
                    insertModelRows(guardedModel, first, last, roleMap);
                }));
                m_modelConnections.append(connect(model, &QAbstractItemModel::rowsRemoved, this,
                        [this, guardedModel, generation](const QModelIndex &parent, int first, int last) {
                    if (!guardedModel || generation != m_modelGeneration
                        || guardedModel.data() != m_objectModel.data()) return;
                    if (parent.isValid()) return;
                    removeModelRows(first, last);
                }));
                m_modelConnections.append(connect(model, &QAbstractItemModel::rowsMoved, this,
                        [this, guardedModel, generation](const QModelIndex &sourceParent, int first, int last,
                               const QModelIndex &destinationParent, int destination) {
                    if (!guardedModel || generation != m_modelGeneration
                        || guardedModel.data() != m_objectModel.data()) return;
                    if (sourceParent.isValid() || destinationParent.isValid()) return;
                    moveModelRows(first, last, destination);
                }));
                m_modelConnections.append(connect(model, &QAbstractItemModel::layoutChanged, this,
                        [this, guardedModel, generation]() {
                    if (!guardedModel || generation != m_modelGeneration
                        || guardedModel.data() != m_objectModel.data()) return;
                    generateItems();
                }));
                m_modelConnections.append(connect(model, &QAbstractItemModel::modelReset, this,
                        [this, guardedModel, generation]() {
                    if (!guardedModel || generation != m_modelGeneration
                        || guardedModel.data() != m_objectModel.data()) return;
                    generateItems();
                }));

                int rowCount = (std::max)(0, model->rowCount());
                for (int i = 0; i < rowCount; ++i) {
                    QQmlPropertyMap* rowMap = QQmlPropertyMap::create(this);
                    const QModelIndex modelIndex = model->index(i, 0);
                    QVariantMap initialProps;
                    initialProps["index"] = i;
                    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                        QVariant val = model->data(modelIndex, it.key());
                        rowMap->insert(it.value(), val);
                        initialProps[it.value()] = val;
                    }
                    initialProps["model"] = QVariant::fromValue(rowMap);
                    initialProps["modelData"] = QVariant::fromValue(rowMap);

                    QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
                    if (!context) {
                        delete rowMap;
                        m_createdItems.append(nullptr);
                        continue;
                    }
                    DelegateUtils::setContextProperty(m_delegate, context, "index", i);
                    DelegateUtils::setContextProperty(m_delegate, context, "model", rowMap);
                    DelegateUtils::setContextProperty(m_delegate, context, "modelData", rowMap);
                    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                        DelegateUtils::setContextProperty(
                            m_delegate, context, it.value(), rowMap->value(it.value()));
                    }

                    Item *item = DelegateUtils::createItem(
                        m_delegate, initialProps, context, contentItem, this);
                    if (item) {
                        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                            setPropertyIfExists(item, it.value().toUtf8().constData(), rowMap->value(it.value()));
                        }
                        rowMap->setParent(item);
                        m_createdItems.append(item);
                    } else {
                        delete rowMap;
                        m_createdItems.append(nullptr);
                    }
                }
            }
        }
    }
    refreshDelegateStates();
    markDirty();
}

void ListView::insertModelRows(QAbstractItemModel *model, int first, int last,
                               const QHash<int, QString> &roleMap) {
    if (!model || !m_delegate || first < 0 || first > last) return;
    if (m_currentIndex >= first) {
        m_currentIndex += last - first + 1;
        emit currentIndexChanged();
    }
    for (int row = first; row <= last; ++row) {
        QQmlPropertyMap *rowMap = QQmlPropertyMap::create(this);
        const QModelIndex modelIndex = model->index(row, 0);
        QVariantMap initialProps;
        initialProps["index"] = row;
        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
            QVariant val = model->data(modelIndex, it.key());
            rowMap->insert(it.value(), val);
            initialProps[it.value()] = val;
        }
        initialProps["model"] = QVariant::fromValue(rowMap);
        initialProps["modelData"] = QVariant::fromValue(rowMap);

        QQmlContext *context = DelegateUtils::createContext(m_delegate, this);
        if (!context) {
            delete rowMap;
            m_createdItems.insert(row, nullptr);
            continue;
        }
        DelegateUtils::setContextProperty(m_delegate, context, "index", row);
        DelegateUtils::setContextProperty(m_delegate, context, "model", rowMap);
        DelegateUtils::setContextProperty(m_delegate, context, "modelData", rowMap);
        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
            DelegateUtils::setContextProperty(
                m_delegate, context, it.value(), rowMap->value(it.value()));
        }

        Item *item = DelegateUtils::createItem(
            m_delegate, initialProps, context, this, this);
        if (item) {
            for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                setPropertyIfExists(item, it.value().toUtf8().constData(), rowMap->value(it.value()));
            }
        }
        if (item) {
            rowMap->setParent(item);
            m_createdItems.insert(row, item);
        } else {
            delete rowMap;
            m_createdItems.insert(row, nullptr);
        }
    }
    for (int row = last + 1; row < m_createdItems.size(); ++row) {
        if (QQmlContext *context = delegateContext(m_createdItems[row]))
            DelegateUtils::setContextProperty(m_delegate, context, "index", row);
        if (m_createdItems[row])
            setPropertyIfExists(m_createdItems[row], "index", row);
    }
    refreshDelegateStates();
    markDirty();
}

void ListView::removeModelRows(int first, int last) {
    const int oldCurrentIndex = m_currentIndex;
    for (int row = last; row >= first; --row) {
        if (row < 0 || row >= m_createdItems.size()) continue;
        Item *item = m_createdItems.takeAt(row);
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    for (int row = std::max(0, first); row < m_createdItems.size(); ++row) {
        if (QQmlContext *context = delegateContext(m_createdItems[row]))
            DelegateUtils::setContextProperty(m_delegate, context, "index", row);
        updateDelegateState(m_createdItems[row], row);
    }
    if (oldCurrentIndex > last)
        m_currentIndex = oldCurrentIndex - (last - first + 1);
    else if (oldCurrentIndex >= first)
        m_currentIndex = std::min(first, static_cast<int>(m_createdItems.size()) - 1);
    if (m_currentIndex != oldCurrentIndex)
        emit currentIndexChanged();
    refreshDelegateStates();
    markDirty();
}

void ListView::moveModelRows(int first, int last, int destination) {
    if (first < 0 || last < first || last >= m_createdItems.size()) return;
    QList<Item*> moved;
    for (int row = first; row <= last; ++row)
        moved.append(m_createdItems.takeAt(first));
    int insertion = destination > first ? destination - moved.size() : destination;
    insertion = std::clamp(insertion, 0, static_cast<int>(m_createdItems.size()));
    for (int index = 0; index < moved.size(); ++index)
        m_createdItems.insert(insertion + index, moved[index]);

    const int oldCurrentIndex = m_currentIndex;
    if (oldCurrentIndex >= first && oldCurrentIndex <= last) {
        m_currentIndex = insertion + oldCurrentIndex - first;
    } else if (destination > last && oldCurrentIndex > last
               && oldCurrentIndex < destination) {
        m_currentIndex -= moved.size();
    } else if (destination < first && oldCurrentIndex >= destination
               && oldCurrentIndex < first) {
        m_currentIndex += moved.size();
    }
    if (m_currentIndex != oldCurrentIndex)
        emit currentIndexChanged();
    for (int row = 0; row < m_createdItems.size(); ++row) {
        if (QQmlContext *context = delegateContext(m_createdItems[row]))
            DelegateUtils::setContextProperty(m_delegate, context, "index", row);
        updateDelegateState(m_createdItems[row], row);
    }
    refreshDelegateStates();
    markDirty();
}

void ListView::layoutChildren() {
    int currentPos = 0;
    for (Item* childItem : m_createdItems) {
        if (childItem && childItem->visible()) {
            if (m_orientation == Qt::Vertical) {
                childItem->setX(0);
                childItem->setY(currentPos);
                currentPos += childItem->height() + m_spacing;
            } else {
                childItem->setX(currentPos);
                childItem->setY(0);
                currentPos += childItem->width() + m_spacing;
            }
        }
    }
    
    // update ScrollView content size
    if (m_orientation == Qt::Vertical) {
        int totalH = currentPos > 0 ? currentPos - m_spacing : 0;
        setContentHeight(totalH);
    }
}

void ListView::mousePressEvent(MouseEvent* event) {
    ScrollView::mousePressEvent(event);
    if (event->isAccepted()) return;

    setFocus(true);
    int clickY = event->y() + contentY() - topPadding();
    for (int i = 0; i < m_createdItems.size(); ++i) {
        Item* item = m_createdItems[i];
        if (item && item->visible()) {
            if (clickY >= item->y() && clickY < item->y() + item->height()) {
                setCurrentIndex(i);
                emit itemActivated(i);
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
        int totalH = 0;
        for (Item* childItem : m_createdItems) {
             if (childItem && childItem->visible())
                 totalH += childItem->height() + m_spacing;
        }
        if (totalH > 0) totalH -= m_spacing;
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
    layoutChildren();
    ScrollView::render(buffer, absX, absY, clipRect);
}

} // namespace qtui
