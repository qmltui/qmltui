#include "GridView.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QAbstractItemModel>
#include <QQmlPropertyMap>

namespace qtui {

static QQmlContext *delegateContext(Item *item) {
    return item ? item->findChild<QQmlContext*>(QString(), Qt::FindDirectChildrenOnly)
                : nullptr;
}

GridView::GridView(QObject* parent) : ScrollView(parent) {
}

GridView::~GridView() {
    clearItems();
}

void GridView::setModel(const QVariant& model) {
    if (m_model == model) return;
    if (m_objectModel) m_objectModel->disconnect(this);
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

void GridView::generateItems() {
    clearItems();
    
    if (!m_delegate) return;
    
    QQmlEngine* engine = qmlEngine(this);
    if (!engine) return;

    Item* contentItem = this;

    int count = 0;
    if (m_model.canConvert<int>() && m_model.userType() != qMetaTypeId<QJSValue>()) {
        count = std::clamp(m_model.toInt(), 0, 10000);
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = new QQmlContext(qmlContext(this), this);
            context->setContextProperty("index", i);
            QObject* obj = m_delegate->create(context);
            if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                item->setParentItem(contentItem);
                item->setParent(this);
                context->setParent(item);
                m_createdItems.append(item);
            } else {
                delete obj;
                delete context;
                m_createdItems.append(nullptr);
            }
        }
    } else if (m_model.userType() == qMetaTypeId<QJSValue>()) {
        QJSValue val = m_model.value<QJSValue>();
        if (val.isArray()) {
            count = std::clamp(val.property("length").toInt(), 0, 10000);
            for (int i = 0; i < count; ++i) {
                QQmlContext* context = new QQmlContext(qmlContext(this), this);
                context->setContextProperty("index", i);
                context->setContextProperty("modelData", val.property(i).toVariant());
                QObject* obj = m_delegate->create(context);
                if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                    item->setParentItem(contentItem);
                    item->setParent(this);
                    context->setParent(item);
                    m_createdItems.append(item);
                } else {
                    delete obj;
                    delete context;
                    m_createdItems.append(nullptr);
                }
            }
        }
    } else if (m_model.canConvert<QObject*>()) {
        QObject* modelObj = m_model.value<QObject*>();
        if (modelObj && modelObj->inherits("QAbstractItemModel")) {
            QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(modelObj);
            if (model) {
                disconnect(model, nullptr, this, nullptr);

                QHash<int, QString> roleMap;
                const QHash<int, QByteArray> roles = model->roleNames();
                for (auto it = roles.cbegin(); it != roles.cend(); ++it)
                    roleMap.insert(it.key(), QString::fromUtf8(it.value()));

                connect(model, &QAbstractItemModel::dataChanged, this,
                        [this, model, roleMap](const QModelIndex &topLeft,
                                              const QModelIndex &bottomRight) {
                    if (topLeft.parent().isValid()) return;
                    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
                        if (row < 0 || row >= m_createdItems.size()) continue;
                        Item *item = m_createdItems[row];
                        QQmlContext *context = delegateContext(item);
                        if (!context) continue;
                        const QVariant value = context->contextProperty("model");
                        if (QQmlPropertyMap *map =
                                qobject_cast<QQmlPropertyMap*>(value.value<QObject*>())) {
                            const QModelIndex index = model->index(row, 0);
                            for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it)
                                map->insert(it.value(), model->data(index, it.key()));
                        }
                    }
                    markDirty();
                });
                connect(model, &QAbstractItemModel::rowsInserted, this,
                        [this, model, roleMap](const QModelIndex &parent, int first, int last) {
                    if (parent.isValid()) return;
                    insertModelRows(model, first, last, roleMap);
                });
                connect(model, &QAbstractItemModel::rowsRemoved, this,
                        [this](const QModelIndex &parent, int first, int last) {
                    if (parent.isValid()) return;
                    removeModelRows(first, last);
                });
                connect(model, &QAbstractItemModel::rowsMoved, this,
                        [this](const QModelIndex &sourceParent, int first, int last,
                               const QModelIndex &destinationParent, int destination) {
                    if (sourceParent.isValid() || destinationParent.isValid()) return;
                    moveModelRows(first, last, destination);
                });
                connect(model, &QAbstractItemModel::layoutChanged, this, [this]() {
                    generateItems();
                });
                connect(model, &QAbstractItemModel::modelReset, this, [this]() {
                    generateItems();
                });

                int rowCount = std::max(0, model->rowCount());
                for (int i = 0; i < rowCount; ++i) {
                    QQmlPropertyMap* rowMap = QQmlPropertyMap::create(this);
                    const QModelIndex modelIndex = model->index(i, 0);
                    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                        rowMap->insert(it.value(), model->data(modelIndex, it.key()));
                    }

                    QQmlContext* context = new QQmlContext(qmlContext(this), this);
                    context->setContextProperty("index", i);
                    context->setContextProperty("model", rowMap);
                    context->setContextProperty("modelData", rowMap);

                    QObject* obj = m_delegate->create(context);
                    if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                        item->setParentItem(contentItem);
                        item->setParent(this);
                        context->setParent(item);
                        rowMap->setParent(item);
                        m_createdItems.append(item);
                    } else {
                        delete obj;
                        delete context;
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
        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it)
            rowMap->insert(it.value(), model->data(modelIndex, it.key()));
        QQmlContext *context = new QQmlContext(qmlContext(this), this);
        context->setContextProperty("index", row);
        context->setContextProperty("model", rowMap);
        context->setContextProperty("modelData", rowMap);
        QObject *object = m_delegate->create(context);
        if (Item *item = qobject_cast<Item*>(object)) {
            item->setParentItem(this);
            item->setParent(this);
            context->setParent(item);
            rowMap->setParent(item);
            m_createdItems.insert(row, item);
        } else {
            delete object;
            delete context;
            delete rowMap;
            m_createdItems.insert(row, nullptr);
        }
    }
    for (int row = last + 1; row < m_createdItems.size(); ++row) {
        if (QQmlContext *context = delegateContext(m_createdItems[row]))
            context->setContextProperty("index", row);
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
            context->setContextProperty("index", row);
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
            context->setContextProperty("index", row);
    }
    markDirty();
}

void GridView::layoutChildren() {
    // ScrollView doesn't have layoutChildren.
    Item* content = this;
    if (!content || m_cellWidth <= 0 || m_cellHeight <= 0) return;

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
    content->setWidth(columns * m_cellWidth);
    content->setHeight(rows * m_cellHeight);
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
