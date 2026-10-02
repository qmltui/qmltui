#include "ListView.h"
#include <ColorMapper.h>
#include <KeyEvent.h>
#include <MouseEvent.h>
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

ListView::ListView(QObject* parent) : ScrollView(parent) {
    setFocusable(true);
}

ListView::~ListView() {
    clearItems();
}

void ListView::setModel(const QVariant& model) {
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
    if (m_currentIndex == index) return;
    int maxIdx = static_cast<int>(m_createdItems.size()) - 1;
    m_currentIndex = (std::max)(0, (std::min)(index, maxIdx));
    emit currentIndexChanged();
    
    for (int i = 0; i < m_createdItems.size(); ++i) {
        if (Item* item = m_createdItems[i]) {
            item->setProperty("isCurrentItem", i == m_currentIndex);
        }
    }
    markDirty();
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
            context->setContextProperty("isCurrentItem", i == m_currentIndex);
            QObject* obj = m_delegate->create(context);
            if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                item->setParentItem(contentItem);
                item->setParent(this);
                context->setParent(item);
                item->setProperty("isCurrentItem", i == m_currentIndex);
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
                context->setContextProperty("isCurrentItem", i == m_currentIndex);
                QObject* obj = m_delegate->create(context);
                if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                    item->setParentItem(contentItem);
                    item->setParent(this);
                    context->setParent(item);
                    item->setProperty("isCurrentItem", i == m_currentIndex);
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
                for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
                    roleMap.insert(it.key(), QString::fromUtf8(it.value()));
                }

                connect(model, &QAbstractItemModel::dataChanged, this, [this, model, roleMap](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> & /*roles*/) {
                    if (topLeft.parent().isValid()) return;
                    for (int r = topLeft.row(); r <= bottomRight.row(); ++r) {
                        if (r >= 0 && r < m_createdItems.size()) {
                            Item* item = m_createdItems[r];
                            if (!item) continue;
                            QQmlContext* context = delegateContext(item);
                            if (!context) continue;
                            QVariant modelVal = context->contextProperty("model");
                            if (QQmlPropertyMap* rowMap = qobject_cast<QQmlPropertyMap*>(modelVal.value<QObject*>())) {
                                const QModelIndex modelIndex = model->index(r, 0);
                                for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                                    rowMap->insert(it.value(), model->data(modelIndex, it.key()));
                                }
                            }
                        }
                    }
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



                int rowCount = (std::max)(0, model->rowCount());
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
                    context->setContextProperty("isCurrentItem", i == m_currentIndex);

                    QObject* obj = m_delegate->create(context);
                    if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                        item->setParentItem(contentItem);
                        item->setParent(this);
                        context->setParent(item);
                        rowMap->setParent(item);
                        item->setProperty("isCurrentItem", i == m_currentIndex);
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

void ListView::insertModelRows(QAbstractItemModel *model, int first, int last,
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
        context->setContextProperty("isCurrentItem", row == m_currentIndex);
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

void ListView::removeModelRows(int first, int last) {
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
    setCurrentIndex(std::min(m_currentIndex, static_cast<int>(m_createdItems.size()) - 1));
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
    for (int row = 0; row < m_createdItems.size(); ++row) {
        if (QQmlContext *context = delegateContext(m_createdItems[row]))
            context->setContextProperty("index", row);
    }
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
