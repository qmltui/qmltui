#include "ListView.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>

namespace qtui {

ListView::ListView(QObject* parent) : ScrollView(parent) {
}

ListView::~ListView() {
    clearItems();
}

void ListView::setModel(const QVariant& model) {
    if (m_model == model) return;
    m_model = model;
    emit modelChanged();
    generateItems();
}

void ListView::setDelegate(QQmlComponent* delegate) {
    if (m_delegate == delegate) return;
    m_delegate = delegate;
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
    if (m_model.canConvert<int>()) {
        count = m_model.toInt();
        for (int i = 0; i < count; ++i) {
            QQmlContext* context = new QQmlContext(qmlContext(this), this);
            context->setContextProperty("index", i);
            QObject* obj = m_delegate->create(context);
            if (qtui::Item* item = qobject_cast<qtui::Item*>(obj)) {
                item->setParentItem(contentItem);
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
                    item->setParentItem(contentItem);
                    item->setParent(this);
                    m_createdItems.append(item);
                } else {
                    delete obj;
                    delete context;
                }
            }
        }
    }
    markDirty();
}

void ListView::layoutChildren() {
    // wait, I can just not call ScrollView::layoutChildren()
    Item* content = this;
    if (!content) return;

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
    
    // update content size
    if (m_orientation == Qt::Vertical) {
        content->setHeight(currentPos > 0 ? currentPos - m_spacing : 0);
        int maxW = 0;
        for (Item* childItem : m_createdItems) {
             if (childItem && childItem->visible() && childItem->width() > maxW)
                 maxW = childItem->width();
        }
        content->setWidth(maxW);
    } else {
        content->setWidth(currentPos > 0 ? currentPos - m_spacing : 0);
        int maxH = 0;
        for (Item* childItem : m_createdItems) {
             if (childItem && childItem->visible() && childItem->height() > maxH)
                 maxH = childItem->height();
        }
        content->setHeight(maxH);
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
