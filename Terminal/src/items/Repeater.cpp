#include "Repeater.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QAbstractItemModel>
#include <QQmlPropertyMap>

namespace qtui {

Repeater::Repeater(QObject* parent) : Item(parent) {
    setVisible(false); // Repeater itself is not visible, it just injects children
}

Repeater::~Repeater() {
    clearItems();
}

void Repeater::setModel(const QVariant& model) {
    if (m_model.canConvert<QObject*>()) {
        QObject* oldModelObj = m_model.value<QObject*>();
        if (oldModelObj) {
            oldModelObj->disconnect(this);
        }
    }
    m_model = model;
    emit modelChanged();
    generateItems();
}

void Repeater::setDelegate(QQmlComponent* delegate) {
    if (m_delegate == delegate) return;
    m_delegate = delegate;
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
    qDeleteAll(m_contexts);
    m_contexts.clear();
    qDeleteAll(m_rowMaps);
    m_rowMaps.clear();
}

static void setPropertyIfExists(QObject* obj, const char* name, const QVariant& value) {
    if (!obj) return;
    if (obj->metaObject()->indexOfProperty(name) != -1) {
        obj->setProperty(name, value);
    }
}

void Repeater::generateItems() {
    clearItems();
    
    if (!m_delegate) return;
    
    QQmlEngine* engine = qmlEngine(this);
    if (!engine) return;

    Item* parentVis = parentItem();
    if (!parentVis) return;
    
    QVariantList list;
    bool isList = false;
    int count = 0;
    bool isCount = false;

    if (!m_model.isValid() || m_model.isNull()) {
        // Empty model
    } else if (m_model.userType() == qMetaTypeId<QJSValue>()) {
        QJSValue val = m_model.value<QJSValue>();
        if (val.isArray()) {
            quint32 len = val.property("length").toUInt();
            for (quint32 i = 0; i < len; ++i) {
                list.append(val.property(i).toVariant());
            }
            isList = true;
        } else if (val.isNumber()) {
            count = std::clamp(val.toInt(), 0, 10000);
            isCount = true;
        } else {
            QVariant v = val.toVariant();
            if (v.typeId() == QMetaType::QVariantList || v.canConvert<QVariantList>()) {
                list = v.toList();
                isList = true;
            }
        }
    } else if (m_model.typeId() == QMetaType::QVariantList || m_model.typeId() == QMetaType::QStringList || m_model.canConvert<QVariantList>()) {
        list = m_model.toList();
        isList = true;
    } else if (m_model.canConvert<QStringList>()) {
        QStringList strList = m_model.toStringList();
        for (const QString& s : strList) {
            list.append(s);
        }
        isList = true;
    } else if (m_model.typeId() == QMetaType::Int || m_model.typeId() == QMetaType::UInt || m_model.typeId() == QMetaType::Double || m_model.canConvert<int>()) {
        count = std::clamp(m_model.toInt(), 0, 10000);
        isCount = true;
    }

    QQmlContext* creationCtx = m_delegate->creationContext();
    if (!creationCtx) creationCtx = qmlContext(this);

    if (isList) {
        for (int i = 0; i < list.size(); ++i) {
            QQmlContext* context = new QQmlContext(creationCtx, this);
            context->setContextProperty("index", i);
            context->setContextProperty("modelData", list.at(i));
            context->setContextProperty("model", list.at(i));

            QObject* obj = m_delegate->create(context);
            if (obj) {
                setPropertyIfExists(obj, "index", i);
                setPropertyIfExists(obj, "modelData", list.at(i));
                setPropertyIfExists(obj, "model", list.at(i));
            }
            if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                item->setParentItem(parentVis);
                m_createdItems.append(item);
                m_contexts.append(context);
            } else {
                delete obj;
                delete context;
            }
        }
    } else if (isCount) {
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = new QQmlContext(creationCtx, this);
            context->setContextProperty("index", i);

            QObject* obj = m_delegate->create(context);
            if (obj) {
                setPropertyIfExists(obj, "index", i);
            }
            if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                item->setParentItem(parentVis);
                m_createdItems.append(item);
                m_contexts.append(context);
            } else {
                delete obj;
                delete context;
            }
        }
    } else if (m_model.canConvert<QObject*>()) {
        QObject* modelObj = m_model.value<QObject*>();
        if (modelObj && modelObj->inherits("QAbstractItemModel")) {
            QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(modelObj);
            if (model) {
                // Disconnect any existing connections from this model to this Repeater to prevent duplicates
                disconnect(model, nullptr, this, nullptr);

                // Helper to build role names map (cached per model operation)
                auto buildRoleMap = [](QAbstractItemModel* m) {
                    QHash<int, QString> roleMap;
                    const QHash<int, QByteArray> roles = m->roleNames();
                    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
                        roleMap.insert(it.key(), QString::fromUtf8(it.value()));
                    }
                    return roleMap;
                };

                // Connect to dataChanged to update QQmlPropertyMap properties dynamically
                connect(model, &QAbstractItemModel::dataChanged, this, [this, model, buildRoleMap](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
                    if (topLeft.parent().isValid()) return; // Ignore child items in tree models
                    const QHash<int, QString> roleMap = buildRoleMap(model);
                    int first = topLeft.row();
                    int last = bottomRight.row();
                    for (int row = first; row <= last; ++row) {
                        if (row >= 0 && row < m_rowMaps.size()) {
                            QQmlPropertyMap* rowMap = m_rowMaps.at(row);
                            if (rowMap) {
                                const QModelIndex modelIndex = model->index(row, 0);
                                for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                                    rowMap->insert(it.value(), model->data(modelIndex, it.key()));
                                }
                            }
                        }
                    }
                    markDirty();
                });

                // Connect to dynamic insert/remove/move/reset signals
                connect(model, &QAbstractItemModel::rowsInserted, this, [this, model](const QModelIndex &parent, int first, int last) {
                    if (parent.isValid()) return;
                    onRowsInserted(model, first, last);
                });
                connect(model, &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &parent, int first, int last) {
                    if (parent.isValid()) return;
                    onRowsRemoved(first, last);
                });
                connect(model, &QAbstractItemModel::rowsMoved, this, [this](const QModelIndex &parent, int, int, const QModelIndex &, int) {
                    if (parent.isValid()) return;
                    generateItems();
                });
                connect(model, &QAbstractItemModel::layoutChanged, this, [this]() {
                    generateItems();
                });
                connect(model, &QAbstractItemModel::modelReset, this, [this](){
                    generateItems();
                });

                const QHash<int, QString> roleMap = buildRoleMap(model);
                int rowCount = std::max(0, model->rowCount());
                for (int i = 0; i < rowCount; ++i) {
                    QQmlPropertyMap* rowMap = new QQmlPropertyMap(this);
                    const QModelIndex modelIndex = model->index(i, 0);
                    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                        rowMap->insert(it.value(), model->data(modelIndex, it.key()));
                    }

                    QQmlContext* context = new QQmlContext(creationCtx, this);
                    context->setContextProperty("index", i);
                    context->setContextProperty("model", rowMap);
                    context->setContextProperty("modelData", rowMap);

                    QObject* obj = m_delegate->create(context);
                    if (obj) {
                        setPropertyIfExists(obj, "index", i);
                        setPropertyIfExists(obj, "modelData", QVariant::fromValue(rowMap));
                        setPropertyIfExists(obj, "model", QVariant::fromValue(rowMap));
                    }
                    if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                        item->setParentItem(parentVis);
                        m_createdItems.append(item);
                        m_contexts.append(context);
                        m_rowMaps.append(rowMap);
                    } else {
                        delete obj;
                        delete context;
                        delete rowMap;
                    }
                }
            }
        }
    }
}

void Repeater::onRowsInserted(QAbstractItemModel* model, int first, int last) {
    if (!model || first < 0 || first > last) return;
    Item* parentVis = parentItem();
    if (!parentVis || !m_delegate) return;

    QHash<int, QString> roleMap;
    const QHash<int, QByteArray> roles = model->roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        roleMap.insert(it.key(), QString::fromUtf8(it.value()));
    }

    for (int i = first; i <= last; ++i) {
        QQmlPropertyMap* rowMap = new QQmlPropertyMap(this);
        const QModelIndex modelIndex = model->index(i, 0);
        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
            rowMap->insert(it.value(), model->data(modelIndex, it.key()));
        }

        QQmlContext* creationCtx = m_delegate->creationContext();
        if (!creationCtx) creationCtx = qmlContext(this);
        QQmlContext* context = new QQmlContext(creationCtx, this);
        context->setContextProperty("index", i);
        context->setContextProperty("model", rowMap);
        context->setContextProperty("modelData", rowMap);

        QObject* obj = m_delegate->create(context);
        if (obj) {
            setPropertyIfExists(obj, "index", i);
            setPropertyIfExists(obj, "modelData", QVariant::fromValue(rowMap));
            setPropertyIfExists(obj, "model", QVariant::fromValue(rowMap));
        }
        if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
            item->setParentItem(parentVis);
            m_createdItems.insert(i, item);
            m_contexts.insert(i, context);
            m_rowMaps.insert(i, rowMap);
        } else {
            delete obj;
            delete context;
            delete rowMap;
        }
    }

    // Update indices for all contexts after the inserted range
    for (int i = last + 1; i < m_contexts.size(); ++i) {
        m_contexts[i]->setContextProperty("index", i);
    }
    
    markDirty();
}

void Repeater::onRowsRemoved(int first, int last) {
    if (first < 0 || first > last) return;
    for (int i = last; i >= first; --i) {
        if (i >= 0 && i < m_createdItems.size()) {
            Item* item = m_createdItems.takeAt(i);
            if (item) {
                item->setParentItem(nullptr);
                delete item;
            }
        }
        if (i >= 0 && i < m_contexts.size()) {
            QQmlContext* context = m_contexts.takeAt(i);
            delete context;
        }
        if (i >= 0 && i < m_rowMaps.size()) {
            QQmlPropertyMap* rowMap = m_rowMaps.takeAt(i);
            delete rowMap;
        }
    }

    // Update indices for all contexts after the removed range
    for (int i = first; i < m_contexts.size(); ++i) {
        m_contexts[i]->setContextProperty("index", i);
    }

    markDirty();
}

} // namespace qtui
