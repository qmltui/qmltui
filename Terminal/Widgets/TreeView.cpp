#include "TreeView.h"
#include <ColorMapper.h>
#include <KeyEvent.h>
#include <MouseEvent.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <algorithm>

namespace qtui {

TreeView::TreeView(QObject *parent) : Item(parent) {
    setFocusable(true);
    setFocus(true);
    setImplicitWidth(30);
    setImplicitHeight(10);
}

TreeView::~TreeView() {
    clearDelegateItems();
}

void TreeView::setModel(const QVariant &model) {
    if (m_modelVariant == model) return;

    if (m_cppModel) {
        disconnect(m_cppModel.data(), nullptr, this, nullptr);
        m_cppModel = nullptr;
    }

    m_modelVariant = model;
    m_rootNodeMap.clear();

    QObject *obj = qvariant_cast<QObject*>(model);
    if (!obj && model.canConvert<QJSValue>()) {
        obj = model.value<QJSValue>().toQObject();
    }

    if (QAbstractItemModel *cppModel = qobject_cast<QAbstractItemModel*>(obj)) {
        m_cppModel = cppModel;
        connect(m_cppModel.data(), &QAbstractItemModel::modelReset, this, [this]() {
            m_treeDirty = true;
            markDirty();
        });
        connect(m_cppModel.data(), &QAbstractItemModel::dataChanged, this, [this]() {
            m_treeDirty = true;
            markDirty();
        });
        connect(m_cppModel.data(), &QAbstractItemModel::rowsInserted, this, [this]() {
            m_treeDirty = true;
            markDirty();
        });
        connect(m_cppModel.data(), &QAbstractItemModel::rowsRemoved, this, [this]() {
            m_treeDirty = true;
            markDirty();
        });
    } else if (model.userType() == QMetaType::fromType<QJSValue>().id()) {
        m_rootNodeMap = model.value<QJSValue>().toVariant().toMap();
    } else if (model.canConvert<QVariantMap>()) {
        m_rootNodeMap = model.toMap();
    }

    m_treeDirty = true;
    emit modelChanged();
    markDirty();
}

void TreeView::setRootNode(const QVariant &node) {
    m_rootNodeVariant = node;
    setModel(node);
    emit rootNodeChanged();
}

void TreeView::setDelegate(QQmlComponent *delegate) {
    if (m_delegate == delegate) return;
    if (m_delegate)
        disconnect(m_delegate, nullptr, this, nullptr);
    m_delegate = delegate;
    if (m_delegate) {
        connect(m_delegate, &QQmlComponent::statusChanged, this,
                [this](QQmlComponent::Status status) {
                    if (status == QQmlComponent::Ready) {
                        m_treeDirty = true;
                        markDirty();
                    }
                });
    }
    m_treeDirty = true;
    emit delegateChanged();
    markDirty();
}

void TreeView::setCurrentIndex(int index) {
    int total = m_visibleNodes.size();
    int newIdx = total > 0 ? std::clamp(index, 0, total - 1) : 0;
    if (m_selectedIndex == newIdx) return;

    m_selectedIndex = newIdx;
    
    // Adjust scrollTop to keep current index visible
    int availH = height();
    if (m_borderStyle != "none") availH -= 2;
    if (availH > 0) {
        if (m_selectedIndex < m_scrollTop) {
            m_scrollTop = m_selectedIndex;
        } else if (m_selectedIndex >= m_scrollTop + availH) {
            m_scrollTop = m_selectedIndex - availH + 1;
        }
    }

    // Update delegate isCurrent properties
    for (int i = 0; i < m_delegateItems.size(); ++i) {
        if (Item *item = m_delegateItems[i]) {
            bool isCurr = (i == m_selectedIndex);
            item->setProperty("isCurrentItem", isCurr);
            item->setProperty("isCurrent", isCurr);
        }
    }

    emit currentIndexChanged();
    emit selectedIndexChanged();
    markDirty();
}

QVariantMap TreeView::currentItemData() const {
    if (m_selectedIndex < 0 || m_selectedIndex >= m_visibleNodes.size())
        return {};
    return m_visibleNodes[m_selectedIndex].rawData;
}

void TreeView::setSelectionColor(const QString &color) {
    if (m_selectionColor == color) return;
    m_selectionColor = color;
    m_resolvedSelectionColor = ColorMapper::fromString(m_selectionColor, 0xFFFFFFFF);
    emit selectionColorChanged();
    markDirty();
}

void TreeView::setSelectionBgColor(const QString &color) {
    if (m_selectionBgColor == color) return;
    m_selectionBgColor = color;
    m_resolvedSelectionBgColor = ColorMapper::fromString(m_selectionBgColor, 0xFF0000FF);
    emit selectionBgColorChanged();
    markDirty();
}

void TreeView::setTextColor(const QString &color) {
    if (m_textColor == color) return;
    m_textColor = color;
    m_resolvedTextColor = ColorMapper::fromString(m_textColor, 0xFF00FFFF);
    emit textColorChanged();
    markDirty();
}

void TreeView::setBranchColor(const QString &color) {
    if (m_branchColor == color) return;
    m_branchColor = color;
    m_resolvedBranchColor = ColorMapper::fromString(m_branchColor, 0xFF808080);
    emit branchColorChanged();
    markDirty();
}

void TreeView::setExpandedIcon(const QString &icon) {
    if (m_expandedIcon == icon) return;
    m_expandedIcon = icon;
    emit expandedIconChanged();
    markDirty();
}

void TreeView::setCollapsedIcon(const QString &icon) {
    if (m_collapsedIcon == icon) return;
    m_collapsedIcon = icon;
    emit collapsedIconChanged();
    markDirty();
}

void TreeView::setBorderStyle(const QString &style) {
    if (m_borderStyle == style) return;
    m_borderStyle = style;
    emit borderStyleChanged();
    markDirty();
}

void TreeView::setBorderColor(const QString &color) {
    if (m_borderColor == color) return;
    m_borderColor = color;
    m_resolvedBorderColor = ColorMapper::fromString(m_borderColor, 0xFF808080);
    emit borderColorChanged();
    markDirty();
}

void TreeView::expand(int index) {
    updateTree();
    if (index >= 0 && index < m_visibleNodes.size()) {
        const VisibleNode &node = m_visibleNodes[index];
        if (node.hasChildren && !m_expandedPaths.contains(node.path)) {
            m_expandedPaths.insert(node.path);
            m_treeDirty = true;
            markDirty();
        }
    }
}

void TreeView::collapse(int index) {
    updateTree();
    if (index >= 0 && index < m_visibleNodes.size()) {
        const VisibleNode &node = m_visibleNodes[index];
        if (node.hasChildren && m_expandedPaths.contains(node.path)) {
            m_expandedPaths.remove(node.path);
            m_treeDirty = true;
            markDirty();
        }
    }
}

void TreeView::toggleExpanded(int index) {
    updateTree();
    if (index >= 0 && index < m_visibleNodes.size()) {
        const VisibleNode &node = m_visibleNodes[index];
        if (node.hasChildren) {
            if (m_expandedPaths.contains(node.path)) {
                m_expandedPaths.remove(node.path);
            } else {
                m_expandedPaths.insert(node.path);
            }
            m_treeDirty = true;
            markDirty();
        }
    }
}

bool TreeView::isExpanded(int index) const {
    if (index >= 0 && index < m_visibleNodes.size()) {
        return m_visibleNodes[index].expanded;
    }
    return false;
}

void TreeView::expandAll() {
    // Expand top level and visible sub-nodes
    m_treeDirty = true;
    markDirty();
}

void TreeView::collapseAll() {
    m_expandedPaths.clear();
    m_treeDirty = true;
    markDirty();
}

void TreeView::clearDelegateItems() {
    for (Item *item : m_delegateItems) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_delegateItems.clear();
}

void TreeView::flattenTree(const QVariantMap &map, const QString &parentPath, int depth, QList<VisibleNode> &list) {
    QString name = map.value("text", map.value("display", "node")).toString();
    QString currentPath = parentPath.isEmpty() ? name : parentPath + "/" + name;

    QVariant childrenVar = map.value("children");
    if (!childrenVar.isValid() || childrenVar.isNull()) childrenVar = map.value("nodes");
    if (childrenVar.userType() == QMetaType::fromType<QJSValue>().id()) {
        childrenVar = childrenVar.value<QJSValue>().toVariant();
    }
    QVariantList children = childrenVar.toList();
    bool hasChildren = !children.isEmpty();

    bool expanded = false;
    if (m_expandedPaths.contains(currentPath)) {
        expanded = true;
    } else if (map.contains("expanded")) {
        expanded = map.value("expanded").toBool();
        if (expanded) {
            m_expandedPaths.insert(currentPath);
        }
    }

    VisibleNode node;
    node.text = name;
    node.path = currentPath;
    node.depth = depth;
    node.hasChildren = hasChildren;
    node.expanded = expanded;
    node.rawData = map;

    list.append(node);

    if (hasChildren && expanded) {
        for (const QVariant &childVar : children) {
            QVariant cVar = childVar;
            if (cVar.userType() == QMetaType::fromType<QJSValue>().id()) {
                cVar = cVar.value<QJSValue>().toVariant();
            }
            QVariantMap childMap = cVar.toMap();
            flattenTree(childMap, currentPath, depth + 1, list);
        }
    }
}

void TreeView::flattenCppModel(QAbstractItemModel *model, const QModelIndex &parent, const QString &parentPath, int depth, QList<VisibleNode> &list) {
    if (!model) return;

    int rows = model->rowCount(parent);
    QHash<int, QByteArray> roles = model->roleNames();

    for (int r = 0; r < rows; ++r) {
        QModelIndex idx = model->index(r, 0, parent);
        if (!idx.isValid()) continue;

        QString name = model->data(idx, Qt::DisplayRole).toString();
        if (name.isEmpty()) name = QString("Item %1").arg(r);

        QString currentPath = parentPath.isEmpty() ? name : parentPath + "/" + name;
        bool hasChildren = model->hasChildren(idx) || (model->rowCount(idx) > 0);

        bool expanded = m_expandedPaths.contains(currentPath);

        QVariantMap rawData;
        for (auto it = roles.begin(); it != roles.end(); ++it) {
            rawData[QString::fromUtf8(it.value())] = model->data(idx, it.key());
        }
        rawData["display"] = name;
        rawData["text"] = name;

        VisibleNode node;
        node.text = name;
        node.path = currentPath;
        node.depth = depth;
        node.hasChildren = hasChildren;
        node.expanded = expanded;
        node.rawData = rawData;
        node.modelIndex = idx;

        list.append(node);

        if (hasChildren && expanded) {
            flattenCppModel(model, idx, currentPath, depth + 1, list);
        }
    }
}

void TreeView::updateTree() {
    if (!m_treeDirty) return;

    m_visibleNodes.clear();
    if (m_cppModel) {
        flattenCppModel(m_cppModel.data(), QModelIndex(), "", 0, m_visibleNodes);
    } else if (!m_rootNodeMap.isEmpty()) {
        flattenTree(m_rootNodeMap, "", 0, m_visibleNodes);
    }

    generateDelegateItems();
    m_treeDirty = false;
}

void TreeView::generateDelegateItems() {
    clearDelegateItems();
    if (!m_delegate || m_delegate->status() != QQmlComponent::Ready) return;

    QQmlEngine *engine = qmlEngine(this);
    if (!engine) return;

    Item *contentItem = this;

    for (int i = 0; i < m_visibleNodes.size(); ++i) {
        const VisibleNode &node = m_visibleNodes[i];

        QQmlContext *context = new QQmlContext(qmlContext(this), this);
        bool isCurr = (i == m_selectedIndex);

        context->setContextProperty("index", i);
        context->setContextProperty("isCurrentItem", isCurr);
        context->setContextProperty("isCurrent", isCurr);
        context->setContextProperty("depth", node.depth);
        context->setContextProperty("expanded", node.expanded);
        context->setContextProperty("hasChildren", node.hasChildren);
        context->setContextProperty("path", node.path);
        context->setContextProperty("display", node.text);
        context->setContextProperty("text", node.text);
        context->setContextProperty("modelData", node.rawData);

        // Combined Qt Quick model map context
        QVariantMap modelMap = node.rawData;
        modelMap["index"] = i;
        modelMap["depth"] = node.depth;
        modelMap["expanded"] = node.expanded;
        modelMap["hasChildren"] = node.hasChildren;
        modelMap["isCurrent"] = isCurr;
        modelMap["isCurrentItem"] = isCurr;
        modelMap["path"] = node.path;
        modelMap["display"] = node.text;
        modelMap["modelData"] = node.rawData;
        context->setContextProperty("model", modelMap);

        QVariantMap initialProps;
        initialProps["depth"] = node.depth;
        initialProps["hasChildren"] = node.hasChildren;
        initialProps["expanded"] = node.expanded;
        initialProps["isCurrent"] = isCurr;
        initialProps["display"] = node.text;
        initialProps["modelData"] = node.rawData;

        QObject *obj = m_delegate->createWithInitialProperties(initialProps, context);
        if (Item *item = qobject_cast<Item*>(obj)) {
            item->setParentItem(contentItem);
            item->setParent(this);
            context->setParent(item);

            item->setProperty("isCurrentItem", isCurr);
            item->setProperty("isCurrent", isCurr);
            item->setProperty("depth", node.depth);
            item->setProperty("expanded", node.expanded);
            item->setProperty("hasChildren", node.hasChildren);

            m_delegateItems.append(item);
        } else {
            delete obj;
            delete context;
            m_delegateItems.append(nullptr);
        }
    }
}

void TreeView::layoutChildren() {
    if (!m_delegate || m_delegateItems.isEmpty()) return;

    int offsetY = (m_borderStyle != "none") ? 1 : 0;
    int offsetX = (m_borderStyle != "none") ? 1 : 0;

    for (int i = 0; i < m_scrollTop && i < m_delegateItems.size(); ++i) {
        if (Item *item = m_delegateItems[i]) {
            item->setVisible(false);
        }
    }

    int curY = offsetY;
    for (int i = m_scrollTop; i < m_delegateItems.size(); ++i) {
        Item *item = m_delegateItems[i];
        if (!item) continue;

        int itemH = item->height();
        if (itemH <= 0) itemH = 1;

        if (curY + itemH > height() - offsetY) {
            item->setVisible(false);
        } else {
            item->setX(offsetX);
            item->setY(curY);
            item->setWidth(width() - (offsetX * 2));
            item->setVisible(true);
            curY += itemH;
        }
    }
}

void TreeView::render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) {
    if (!m_visible) return;

    updateTree();

    int availH = height();
    int availW = width();
    int startX = absX;
    int startY = absY;

    if (m_borderStyle != "none") {
        buffer.drawBox(absX, absY, availW, availH, m_resolvedBorderColor, 0x00000000);
        startX += 1;
        startY += 1;
        availW -= 2;
        availH -= 2;
    }

    if (availW <= 0 || availH <= 0) return;

    if (m_delegate) {
        layoutChildren();
        for (int i = m_scrollTop; i < m_delegateItems.size(); ++i) {
            Item *item = m_delegateItems[i];
            if (item && item->visible()) {
                item->render(buffer, absX + item->x(), absY + item->y(), clipRect);
            }
        }
        return;
    }

    // Default Rendering (No Delegate)
    for (int idx = m_scrollTop; idx < m_visibleNodes.size() && (idx - m_scrollTop) < availH; ++idx) {
        const VisibleNode &node = m_visibleNodes[idx];
        int rowY = startY + (idx - m_scrollTop);
        bool isSelected = (idx == m_selectedIndex);

        uint32_t bg = isSelected ? m_resolvedSelectionBgColor : 0x00000000;
        uint32_t fg = isSelected ? m_resolvedSelectionColor : m_resolvedTextColor;
        TextAttribute attr = isSelected ? TextAttribute::Bold : TextAttribute::None;

        if (isSelected) {
            buffer.fillRect(startX, rowY, availW, 1, " ", fg, bg);
        }

        QString indent = QString(node.depth * 2, ' ');
        QString prefix = node.hasChildren ? (node.expanded ? m_expandedIcon : m_collapsedIcon) : "    ";
        QString label = indent + prefix + node.text;

        buffer.writeText(startX, rowY, label.left(availW), fg, bg, attr);
    }
}

void TreeView::keyPressEvent(KeyEvent *event) {
    updateTree();
    int total = m_visibleNodes.size();

    if (event->key() == Qt::Key_Up) {
        if (m_selectedIndex > 0) setCurrentIndex(m_selectedIndex - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Down) {
        if (m_selectedIndex < total - 1) setCurrentIndex(m_selectedIndex + 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageUp) {
        setCurrentIndex((std::max)(0, m_selectedIndex - 10));
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageDown) {
        setCurrentIndex((std::min)(total - 1, m_selectedIndex + 10));
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Right) {
        expand(m_selectedIndex);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Left) {
        collapse(m_selectedIndex);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Space) {
        toggleExpanded(m_selectedIndex);
        if (m_selectedIndex >= 0 && m_selectedIndex < total) {
            emit nodeActivated(m_visibleNodes[m_selectedIndex].path);
            emit itemActivated(m_selectedIndex);
        }
        event->setAccepted(true);
    }
}

void TreeView::mousePressEvent(MouseEvent *event) {
    setFocus(true);
    int offsetY = (m_borderStyle != "none") ? 1 : 0;
    int row = event->y() - offsetY + m_scrollTop;
    updateTree();

    if (row >= 0 && row < m_visibleNodes.size()) {
        setCurrentIndex(row);
        toggleExpanded(row);
        emit nodeActivated(m_visibleNodes[row].path);
        emit itemActivated(row);
        event->setAccepted(true);
    }
}

} // namespace qtui
