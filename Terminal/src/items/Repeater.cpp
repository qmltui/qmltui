#include "Repeater.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QAbstractItemModel>
#include <QQmlPropertyMap>

namespace qtui {

static void setPropertyIfExists(QObject* obj, const char* name, const QVariant& value) {
    if (!obj) return;
    if (obj->metaObject()->indexOfProperty(name) != -1) {
        obj->setProperty(name, value);
    }
}

static QObject* createDelegateWithFilteredProps(QQmlComponent* delegate, QQmlContext* context, const QVariantMap& initialProps) {
    if (!delegate || !context) return nullptr;
    QVariantMap filteredProps;
    if (QObject* temp = delegate->beginCreate(context)) {
        const QMetaObject* meta = temp->metaObject();
        for (auto it = initialProps.cbegin(); it != initialProps.cend(); ++it) {
            if (meta->indexOfProperty(it.key().toUtf8().constData()) != -1) {
                filteredProps[it.key()] = it.value();
            }
        }
        delegate->completeCreate();
        delete temp;
    }
    return delegate->createWithInitialProperties(filteredProps, context);
}

Repeater::Repeater(QObject* parent) : Item(parent) {
    setVisible(false); // Repeater itself is not visible, it just injects children
}

Repeater::~Repeater() {
    clearItems();
}

void Repeater::setModel(const QVariant& model) {
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

void Repeater::setDelegate(QQmlComponent* delegate) {
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

void Repeater::clearItems() {
    for (Item* item : m_createdItems) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_createdItems.clear();
    m_contexts.clear();
    m_rowMaps.clear();
}

void Repeater::generateItems() {
    clearItems();
    
    if (!m_delegate) return;
    
    QQmlEngine* engine = qmlEngine(this);
    if (!engine && m_delegate) engine = m_delegate->engine();
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
            QVariant itemVal = list.at(i);

            QVariantMap initialProps;
            initialProps["index"] = i;
            initialProps["modelData"] = itemVal;
            initialProps["model"] = itemVal;

            context->setContextProperty("index", i);
            context->setContextProperty("modelData", itemVal);
            context->setContextProperty("model", itemVal);

            QVariant elem = itemVal;
            if (elem.userType() == QMetaType::fromType<QJSValue>().id()) {
                elem = elem.value<QJSValue>().toVariant();
            }
            if (elem.canConvert<QVariantMap>()) {
                QVariantMap map = elem.toMap();
                for (auto it = map.cbegin(); it != map.cend(); ++it) {
                    initialProps[it.key()] = it.value();
                    context->setContextProperty(it.key(), it.value());
                }
            }

            QObject* obj = createDelegateWithFilteredProps(m_delegate, context, initialProps);
            if (obj) {
                setPropertyIfExists(obj, "index", i);
                setPropertyIfExists(obj, "modelData", itemVal);
                setPropertyIfExists(obj, "model", itemVal);
                if (elem.canConvert<QVariantMap>()) {
                    QVariantMap map = elem.toMap();
                    for (auto it = map.cbegin(); it != map.cend(); ++it) {
                        setPropertyIfExists(obj, it.key().toUtf8().constData(), it.value());
                    }
                }
            }
            if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                item->setParentItem(parentVis);
                context->setParent(item);
                m_createdItems.append(item);
                m_contexts.append(context);
            } else {
                delete obj;
                delete context;
                m_createdItems.append(nullptr);
                m_contexts.append(nullptr);
            }
        }
    } else if (isCount) {
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = new QQmlContext(creationCtx, this);
            QVariantMap initialProps;
            initialProps["index"] = i;

            context->setContextProperty("index", i);

            QObject* obj = createDelegateWithFilteredProps(m_delegate, context, initialProps);
            if (obj) {
                setPropertyIfExists(obj, "index", i);
            }
            if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                item->setParentItem(parentVis);
                context->setParent(item);
                m_createdItems.append(item);
                m_contexts.append(context);
            } else {
                delete obj;
                delete context;
                m_createdItems.append(nullptr);
                m_contexts.append(nullptr);
            }
        }
    } else if (m_model.canConvert<QObject*>()) {
        QObject* modelObj = m_model.value<QObject*>();
        if (modelObj && modelObj->inherits("QAbstractItemModel")) {
            QAbstractItemModel* model = qobject_cast<QAbstractItemModel*>(modelObj);
            if (model) {
                disconnect(model, nullptr, this, nullptr);

                auto buildRoleMap = [](QAbstractItemModel* m) {
                    QHash<int, QString> roleMap;
                    const QHash<int, QByteArray> roles = m->roleNames();
                    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
                        roleMap.insert(it.key(), QString::fromUtf8(it.value()));
                    }
                    return roleMap;
                };

                connect(model, &QAbstractItemModel::dataChanged, this, [this, model, buildRoleMap](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
                    if (topLeft.parent().isValid()) return; 
                    const QHash<int, QString> roleMap = buildRoleMap(model);
                    int first = topLeft.row();
                    int last = bottomRight.row();
                    for (int row = first; row <= last; ++row) {
                        if (row >= 0 && row < m_rowMaps.size()) {
                            QQmlPropertyMap* rowMap = m_rowMaps.at(row);
                            QQmlContext* ctx = (row < m_contexts.size()) ? m_contexts.at(row) : nullptr;
                            Item* item = (row < m_createdItems.size()) ? m_createdItems.at(row) : nullptr;
                            const QModelIndex modelIndex = model->index(row, 0);
                            for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                                QVariant val = model->data(modelIndex, it.key());
                                if (rowMap) rowMap->insert(it.value(), val);
                                if (ctx) ctx->setContextProperty(it.value(), val);
                                setPropertyIfExists(item, it.value().toUtf8().constData(), val);
                            }
                        }
                    }
                    markDirty();
                });

                connect(model, &QAbstractItemModel::rowsInserted, this, [this, model](const QModelIndex &parent, int first, int last) {
                    if (parent.isValid()) return;
                    onRowsInserted(model, first, last);
                });
                connect(model, &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &parent, int first, int last) {
                    if (parent.isValid()) return;
                    onRowsRemoved(first, last);
                });
                connect(model, &QAbstractItemModel::rowsMoved, this,
                        [this](const QModelIndex &sourceParent, int first, int last,
                               const QModelIndex &destinationParent, int destination) {
                    if (sourceParent.isValid() || destinationParent.isValid()) return;
                    onRowsMoved(first, last, destination);
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

                    QQmlContext* context = new QQmlContext(creationCtx, this);
                    context->setContextProperty("index", i);
                    context->setContextProperty("model", rowMap);
                    context->setContextProperty("modelData", rowMap);
                    for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                        context->setContextProperty(it.value(), rowMap->value(it.value()));
                    }

                    QObject* obj = createDelegateWithFilteredProps(m_delegate, context, initialProps);
                    if (obj) {
                        setPropertyIfExists(obj, "index", i);
                        setPropertyIfExists(obj, "modelData", QVariant::fromValue(rowMap));
                        setPropertyIfExists(obj, "model", QVariant::fromValue(rowMap));
                        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                            setPropertyIfExists(obj, it.value().toUtf8().constData(), rowMap->value(it.value()));
                        }
                    }
                    if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                        item->setParentItem(parentVis);
                        context->setParent(item);
                        rowMap->setParent(item);
                        m_createdItems.append(item);
                        m_contexts.append(context);
                        m_rowMaps.append(rowMap);
                    } else {
                        delete obj;
                        delete context;
                        delete rowMap;
                        m_createdItems.append(nullptr);
                        m_contexts.append(nullptr);
                        m_rowMaps.append(nullptr);
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

        QQmlContext* creationCtx = m_delegate->creationContext();
        if (!creationCtx) creationCtx = qmlContext(this);
        QQmlContext* context = new QQmlContext(creationCtx, this);
        context->setContextProperty("index", i);
        context->setContextProperty("model", rowMap);
        context->setContextProperty("modelData", rowMap);
        for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
            context->setContextProperty(it.value(), rowMap->value(it.value()));
        }

        QObject* obj = createDelegateWithFilteredProps(m_delegate, context, initialProps);
        if (obj) {
            setPropertyIfExists(obj, "index", i);
            setPropertyIfExists(obj, "modelData", QVariant::fromValue(rowMap));
            setPropertyIfExists(obj, "model", QVariant::fromValue(rowMap));
            for (auto it = roleMap.cbegin(); it != roleMap.cend(); ++it) {
                setPropertyIfExists(obj, it.value().toUtf8().constData(), rowMap->value(it.value()));
            }
        }
        if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
            item->setParentItem(parentVis);
            context->setParent(item);
            rowMap->setParent(item);
            m_createdItems.insert(i, item);
            m_contexts.insert(i, context);
            m_rowMaps.insert(i, rowMap);
        } else {
            delete obj;
            delete context;
            delete rowMap;
            m_createdItems.insert(i, nullptr);
            m_contexts.insert(i, nullptr);
            m_rowMaps.insert(i, nullptr);
        }
    }

    // Update indices for all contexts after the inserted range
    for (int i = last + 1; i < m_contexts.size(); ++i) {
        if (m_contexts[i]) m_contexts[i]->setContextProperty("index", i);
        if (i < m_createdItems.size() && m_createdItems[i]) setPropertyIfExists(m_createdItems[i], "index", i);
    }
    
    markDirty();
}

void Repeater::onRowsRemoved(int first, int last) {
    if (first < 0 || first > last) return;
    for (int i = last; i >= first; --i) {
        Item* item = i >= 0 && i < m_createdItems.size()
            ? m_createdItems.takeAt(i) : nullptr;
        QQmlContext* context = i >= 0 && i < m_contexts.size()
            ? m_contexts.takeAt(i) : nullptr;
        QQmlPropertyMap* rowMap = i >= 0 && i < m_rowMaps.size()
            ? m_rowMaps.takeAt(i) : nullptr;
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        } else {
            delete context;
            delete rowMap;
        }
    }

    // Update indices for all contexts after the removed range
    for (int i = first; i < m_contexts.size(); ++i) {
        if (m_contexts[i]) m_contexts[i]->setContextProperty("index", i);
        if (i < m_createdItems.size() && m_createdItems[i]) setPropertyIfExists(m_createdItems[i], "index", i);
    }

    markDirty();
}

void Repeater::onRowsMoved(int first, int last, int destination) {
    if (first < 0 || last < first || last >= m_createdItems.size()) return;
    QList<Item*> movedItems;
    QList<QQmlContext*> movedContexts;
    QList<QQmlPropertyMap*> movedMaps;
    for (int row = first; row <= last; ++row) {
        movedItems.append(m_createdItems.takeAt(first));
        movedContexts.append(m_contexts.takeAt(first));
        movedMaps.append(m_rowMaps.takeAt(first));
    }
    int insertion = destination > first ? destination - movedItems.size() : destination;
    insertion = std::clamp(insertion, 0, static_cast<int>(m_createdItems.size()));
    for (int index = 0; index < movedItems.size(); ++index) {
        m_createdItems.insert(insertion + index, movedItems[index]);
        m_contexts.insert(insertion + index, movedContexts[index]);
        m_rowMaps.insert(insertion + index, movedMaps[index]);
    }
    for (int row = 0; row < m_contexts.size(); ++row) {
        if (m_contexts[row]) m_contexts[row]->setContextProperty("index", row);
        if (row < m_createdItems.size() && m_createdItems[row]) setPropertyIfExists(m_createdItems[row], "index", row);
    }
    markDirty();
}

} // namespace qtui
