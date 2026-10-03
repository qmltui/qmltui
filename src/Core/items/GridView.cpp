#include "GridView.h"
#include "DelegateUtils.h"
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

GridView::GridView(QObject* parent) : ScrollView(parent) {
}

GridView::~GridView() {
    clearItems();
}

void GridView::setModel(const QVariant& model) {
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

void GridView::setDelegate(QQmlComponent* delegate) {
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

void GridView::setCellWidth(int width) {
    if (m_cellWidth == width) return;
    m_cellWidth = width;
    emit cellWidthChanged();
    markDirty();
}

void GridView::setCellHeight(int height) {
    if (m_cellHeight == height) return;
    m_cellHeight = height;
    emit cellHeightChanged();
    markDirty();
}

void GridView::clearItems() {
    for (Item* item : m_createdItems) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_createdItems.clear();
}

void GridView::disconnectModelSignals()
{
    for (const QMetaObject::Connection &connection : std::as_const(m_modelConnections))
        disconnect(connection);
    m_modelConnections.clear();
}

void GridView::generateItems() {
    clearItems();
    disconnectModelSignals();
    if (!DelegateUtils::isReady(m_delegate, this)) return;
    
    QQmlEngine* engine = qmlEngine(this);
    if (!engine && m_delegate) engine = m_delegate->engine();
    if (!engine) return;

    Item* contentItem = this;

    int count = 0;
    if (m_model.canConvert<int>() && m_model.userType() != qMetaTypeId<QJSValue>()) {
        count = std::clamp(m_model.toInt(), 0, 10000);
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
            QVariantMap initialProps;
            initialProps["index"] = i;

            DelegateUtils::setContextProperty(m_delegate, context, "index", i);
            Item *item = DelegateUtils::createItem(
                m_delegate, initialProps, context, contentItem, this);
            if (item) {
                setPropertyIfExists(item, "index", i);
            }
            if (item) {
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
                setPropertyIfExists(item, "index", i);
                setPropertyIfExists(item, "modelData", itemVal);
                setPropertyIfExists(item, "model", itemVal);
                if (elem.canConvert<QVariantMap>()) {
                    QVariantMap map = elem.toMap();
                    for (auto it = map.cbegin(); it != map.cend(); ++it) {
                        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
                    }
                }
            }
            if (item) {
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
                for (auto it = roles.cbegin(); it != roles.cend(); ++it)
                    roleMap.insert(it.key(), QString::fromUtf8(it.value()));

                const quint64 generation = m_modelGeneration;
                QPointer<QAbstractItemModel> guardedModel(model);
                m_modelConnections.append(connect(model, &QAbstractItemModel::dataChanged, this,
                        [this, guardedModel, generation, roleMap](const QModelIndex &topLeft,
                                              const QModelIndex &bottomRight,
                                              const QList<int> &changedRoles) {
                    if (!guardedModel || generation != m_modelGeneration
                        || guardedModel.data() != m_objectModel.data()) return;
                    if (topLeft.parent().isValid()) return;
                    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
                        if (row < 0 || row >= m_createdItems.size()) continue;
                        Item *item = m_createdItems[row];
                        QQmlContext *context = delegateContext(item);
                        if (!context) continue;
                        const QVariant value = context->contextProperty("model");
                        QQmlPropertyMap *map = qobject_cast<QQmlPropertyMap*>(value.value<QObject*>());
                        const QModelIndex index = guardedModel->index(row, 0);
                        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                            if (!changedRoles.isEmpty() && !changedRoles.contains(it.key())) continue;
                            QVariant val = guardedModel->data(index, it.key());
                            if (map) map->insert(it.value(), val);
                                DelegateUtils::setContextProperty(m_delegate, context, it.value(), val);
                            setPropertyIfExists(item, it.value().toUtf8().constData(), val);
                        }
                    }
                    markDirty();
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

                int rowCount = std::max(0, model->rowCount());
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
                        setPropertyIfExists(item, "index", i);
                        setPropertyIfExists(item, "modelData", QVariant::fromValue(rowMap));
                        setPropertyIfExists(item, "model", QVariant::fromValue(rowMap));
                        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                            setPropertyIfExists(item, it.value().toUtf8().constData(), rowMap->value(it.value()));
                        }
                    }
                    if (item) {
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
    markDirty();
}

void GridView::insertModelRows(QAbstractItemModel *model, int first, int last,
                               const QHash<int, QString> &roleMap) {
    if (!model || !m_delegate || first < 0 || first > last) return;
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
            setPropertyIfExists(item, "index", row);
            setPropertyIfExists(item, "modelData", QVariant::fromValue(rowMap));
            setPropertyIfExists(item, "model", QVariant::fromValue(rowMap));
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
    markDirty();
}

void GridView::removeModelRows(int first, int last) {
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
        if (m_createdItems[row])
            setPropertyIfExists(m_createdItems[row], "index", row);
    }
    markDirty();
}

void GridView::moveModelRows(int first, int last, int destination) {
    if (first < 0 || last < first || last >= m_createdItems.size()) return;
    QList<Item*> moved;
    for (int row = first; row <= last; ++row)
        moved.append(m_createdItems.takeAt(first));
    int insertion = destination > first ? destination - moved.size() : destination;
    insertion = std::clamp(insertion, 0, static_cast<int>(m_createdItems.size()));
    for (int index = 0; index < moved.size(); ++index)
        m_createdItems.insert(insertion + index, moved[index]);
    for (int row = 0; row < m_createdItems.size(); ++row) {
        if (QQmlContext *context = delegateContext(m_createdItems[row]))
            DelegateUtils::setContextProperty(m_delegate, context, "index", row);
        if (m_createdItems[row])
            setPropertyIfExists(m_createdItems[row], "index", row);
    }
    markDirty();
}

void GridView::layoutChildren() {
    // ScrollView doesn't have layoutChildren.
    if (m_cellWidth <= 0 || m_cellHeight <= 0) return;

    int columns = std::max(1, width() / m_cellWidth);
    
    int index = 0;
    for (Item* childItem : m_createdItems) {
        if (childItem && childItem->visible()) {
            int row = index / columns;
            int col = index % columns;
            childItem->setX(col * m_cellWidth);
            childItem->setY(row * m_cellHeight);
            index++;
        }
    }
    
    int rows = (index + columns - 1) / columns;
    setContentHeight(rows * m_cellHeight);
}

int GridView::implicitWidth() const {
    if (m_cellWidth > 0 && m_createdItems.count() > 0) {
        return m_cellWidth + leftPadding() + rightPadding(); // At least 1 column
    }
    return leftPadding() + rightPadding();
}

int GridView::implicitHeight() const {
    if (m_cellHeight > 0 && m_createdItems.count() > 0) {
        return m_cellHeight + topPadding() + bottomPadding(); // At least 1 row
    }
    return topPadding() + bottomPadding();
}

void GridView::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!visible()) return;
    layoutChildren();
    ScrollView::render(buffer, absX, absY, clipRect);
}

} // namespace qtui
