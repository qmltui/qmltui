#include "Repeater.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>
#include <QAbstractItemModel>

namespace qtui {

Repeater::Repeater(QObject* parent) : Item(parent) {
    setVisible(false); // Repeater itself is not visible, it just injects children
}

Repeater::~Repeater() {
    clearItems();
}

void Repeater::setModel(const QVariant& model) {
    if (m_model == model) return;
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
}

void Repeater::generateItems() {
    clearItems();
    
    if (!m_delegate) return;
    
    QQmlEngine* engine = qmlEngine(this);
    if (!engine) return;

    Item* parentVis = parentItem();
    if (!parentVis) return;
    
    int count = 0;
    bool isCount = false;
    if (m_model.canConvert<int>() && m_model.userType() != qMetaTypeId<QJSValue>()) {
        count = m_model.toInt();
        isCount = true;
    } else if (m_model.userType() == qMetaTypeId<QJSValue>()) {
        QJSValue val = m_model.value<QJSValue>();
        if (val.isNumber()) {
            count = val.toInt();
            isCount = true;
        } else if (val.isArray()) {
            count = val.property("length").toInt();
            for (int i = 0; i < count; ++i) {
                QQmlContext* context = new QQmlContext(qmlContext(this), this);
                context->setContextProperty("index", i);
                context->setContextProperty("modelData", val.property(i).toVariant());
                QObject* obj = m_delegate->create(context);
                if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                    item->setParentItem(parentVis);
                    m_createdItems.append(item);
                    m_contexts.append(context);
                } else {
                    delete obj;
                    delete context;
                }
            }
        }
    }

    if (isCount) {
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = new QQmlContext(qmlContext(this), this);
            context->setContextProperty("index", i);
            QObject* obj = m_delegate->create(context);
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
                // Connect to dataChanged to update context properties dynamically
                connect(model, &QAbstractItemModel::dataChanged, this, [this, model](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
                    const QHash<int, QByteArray> roles = model->roleNames();
                    int first = topLeft.row();
                    int last = bottomRight.row();
                    for (int row = first; row <= last; ++row) {
                        if (row >= 0 && row < m_contexts.count()) {
                            QQmlContext* context = m_contexts.at(row);
                            if (context) {
                                const QModelIndex modelIndex = model->index(row, 0);
                                for (auto role = roles.cbegin(); role != roles.cend(); ++role) {
                                    context->setContextProperty(QString::fromUtf8(role.value()), model->data(modelIndex, role.key()));
                                }
                            }
                        }
                    }
                    markDirty();
                }, Qt::UniqueConnection);

                // Connect to dynamic insert/remove/reset to regenerate
                connect(model, &QAbstractItemModel::rowsInserted, this, [this](){ generateItems(); }, Qt::UniqueConnection);
                connect(model, &QAbstractItemModel::rowsRemoved, this, [this](){ generateItems(); }, Qt::UniqueConnection);
                connect(model, &QAbstractItemModel::modelReset, this, [this](){ generateItems(); }, Qt::UniqueConnection);

                const QHash<int, QByteArray> roles = model->roleNames();
                int rowCount = 0;
                if (!model->metaObject()->invokeMethod(model, "rowCount", Qt::DirectConnection, Q_RETURN_ARG(int, rowCount))) {
                    rowCount = modelObj->property("count").toInt();
                }
                for (int i = 0; i < rowCount; ++i) {
                    QQmlContext* context = new QQmlContext(qmlContext(this), this);
                    context->setContextProperty("index", i);
                    
                    const QModelIndex modelIndex = model->index(i, 0);
                    for (auto role = roles.cbegin(); role != roles.cend(); ++role) {
                        context->setContextProperty(QString::fromUtf8(role.value()), model->data(modelIndex, role.key()));
                    }

                    QObject* obj = m_delegate->create(context);
                    if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                        item->setParentItem(parentVis);
                        m_createdItems.append(item);
                        m_contexts.append(context);
                    } else {
                        delete obj;
                        delete context;
                    }
                }
            }
        }
    }
}

} // namespace qtui
