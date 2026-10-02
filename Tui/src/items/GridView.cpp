#include "GridView.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QJSValue>

namespace qtui {

GridView::GridView(QObject* parent) : ScrollView(parent) {
}

GridView::~GridView() {
    clearItems();
}

void GridView::setModel(const QVariant& model) {
    if (m_model == model) return;
    m_model = model;
    emit modelChanged();
    generateItems();
}

void GridView::setDelegate(QQmlComponent* delegate) {
    if (m_delegate == delegate) return;
    m_delegate = delegate;
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
