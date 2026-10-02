#include "Repeater.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>

namespace qtui {

Repeater::Repeater(QObject* parent) : Item(parent) {
    setVisible(false); // Repeater itself is not visible, it just injects children
}

Repeater::~Repeater() {
    clearItems();
}

void Repeater::setModel(const QVariant& model) {
    if (m_model == model) return;
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
}

void Repeater::generateItems() {
    clearItems();
    
    if (!m_delegate) return;
    
    QQmlEngine* engine = qmlEngine(this);
    if (!engine) return;

    Item* parentVis = parentItem();
    if (!parentVis) return;
    
    int count = 0;
    if (m_model.canConvert<int>()) {
        count = m_model.toInt();
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = new QQmlContext(qmlContext(this), this);
            context->setContextProperty("index", i);
            QObject* obj = m_delegate->create(context);
            if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                // Ensure it gets visually parented to Repeater's parent
                item->setParentItem(parentVis);
                // But QObject parent is the Repeater to manage lifetime
                item->setParent(this);
                m_createdItems.append(item);
            } else {
                delete obj;
                delete context;
            }
        }
    } else if (m_model.userType() == qMetaTypeId<QJSValue>()) {
        QJSValue val = m_model.value<QJSValue>();
        if (val.isArray()) {
            count = val.property("length").toInt();
            for (int i = 0; i < count; ++i) {
                QQmlContext* context = new QQmlContext(qmlContext(this), this);
                context->setContextProperty("index", i);
                context->setContextProperty("modelData", val.property(i).toVariant());
                QObject* obj = m_delegate->create(context);
                if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                    item->setParentItem(parentVis);
                    item->setParent(this);
                    m_createdItems.append(item);
                } else {
                    delete obj;
                    delete context;
                }
            }
        }
    } else if (m_model.canConvert<QObject*>()) {
        // Fallback for QAbstractItemModel or ListModel.
        // Handling ListModel properly requires QAbstractItemModel API, let's add basic support
        QObject* modelObj = m_model.value<QObject*>();
        if (modelObj && modelObj->inherits("QAbstractItemModel")) {
            // Very basic support for QAbstractItemModel
            int rowCount = modelObj->metaObject()->invokeMethod(modelObj, "rowCount", Qt::DirectConnection, Q_RETURN_ARG(int, count)) ? count : 0;
            // Actually, ListModel has "count" property usually.
            if (rowCount == 0) {
                 rowCount = modelObj->property("count").toInt();
            }
            for (int i = 0; i < rowCount; ++i) {
                QQmlContext* context = new QQmlContext(qmlContext(this), this);
                context->setContextProperty("index", i);
                // We should expose roles, but for simplicity we rely on model attachment
                QObject* obj = m_delegate->create(context);
                if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                    item->setParentItem(parentVis);
                    item->setParent(this);
                    m_createdItems.append(item);
                } else {
                    delete obj;
                    delete context;
                }
            }
        }
    }
}

} // namespace qtui
