#include "TreeView.h"
#include "DelegateUtils.h"
#include <ColorMapper.h>
#include "KeyEvent.h"
#include "MouseEvent.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <algorithm>
#include <functional>
#include <utility>

namespace qtui {

static QQmlContext *delegateContext(Item *item) {
    return item ? item->findChild<QQmlContext*>(QString(), Qt::FindDirectChildrenOnly)
                : nullptr;
}

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
    const bool currentChanged = m_selectedIndex != -1;
    const bool hadRootIndex = m_rootIndex.isValid();

    if (m_cppModel) {
        disconnect(m_cppModel.data(), nullptr, this, nullptr);
        m_cppModel = nullptr;
    }

    m_modelVariant = model;
    m_rootNodeMap.clear();
    m_expandedPaths.clear();
    m_collapsedPaths.clear();
    m_rootIndex = QPersistentModelIndex();
    m_selectedIndex = -1;
    m_scrollTop = 0;

    QObject *obj = qvariant_cast<QObject*>(model);
    if (!obj && model.canConvert<QJSValue>()) {
        obj = model.value<QJSValue>().toQObject();
    }

    if (QAbstractItemModel *cppModel = qobject_cast<QAbstractItemModel*>(obj)) {
        m_cppModel = cppModel;
        connect(m_cppModel.data(), &QObject::destroyed, this, [this]() {
            m_cppModel.clear();
            m_modelVariant.clear();
            m_visibleNodes.clear();
            clearDelegateItems();
            emit modelChanged();
            emit currentIndexChanged();
            emit selectedIndexChanged();
            emit currentItemDataChanged();
            markDirty();
        });
        connect(m_cppModel.data(), &QAbstractItemModel::modelReset, this, [this]() {
            m_treeDirty = true;
            markDirty();
        });
        connect(m_cppModel.data(), &QAbstractItemModel::dataChanged, this, [this]() {
            m_treeDirty = true;
            emit currentItemDataChanged();
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
        connect(m_cppModel.data(), &QAbstractItemModel::rowsMoved, this, [this]() {
            m_treeDirty = true;
            markDirty();
        });
        connect(m_cppModel.data(), &QAbstractItemModel::layoutChanged, this, [this]() {
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
    if (hadRootIndex) emit rootIndexChanged();
    if (currentChanged) {
        emit currentIndexChanged();
        emit selectedIndexChanged();
    }
    emit currentItemDataChanged();
    markDirty();
}

void TreeView::setRootNode(const QVariant &node) {
    if (m_rootNodeVariant == node) return;
    m_rootNodeVariant = node;
    setModel(node);
    emit rootNodeChanged();
}

void TreeView::setRootIndex(const QModelIndex &index)
{
    const QPersistentModelIndex persistent(index);
    if (m_rootIndex == persistent) return;
    m_rootIndex = persistent;
    m_treeDirty = true;
    emit rootIndexChanged();
    markDirty();
}

void TreeView::setDelegate(QQmlComponent *delegate) {
    if (m_delegate == delegate) return;
    clearDelegateItems();
    if (m_delegate)
        disconnect(m_delegate, nullptr, this, nullptr);
    m_delegate = delegate;
    if (m_delegate) {
        connect(m_delegate, &QObject::destroyed, this, [this]() {
            m_delegate.clear();
            clearDelegateItems();
            emit delegateChanged();
            markDirty();
        });
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

void TreeView::adjustScrollTop() {
    const int total = static_cast<int>(m_visibleNodes.size());
    if (total == 0) {
        m_scrollTop = 0;
        m_selectedIndex = -1;
        return;
    }
    if (m_selectedIndex >= 0)
        m_selectedIndex = std::clamp(m_selectedIndex, 0, total - 1);
    int availH = height();
    if (m_borderStyle != "none") availH -= 2;
    if (availH <= 0) availH = 10;

    if (m_selectedIndex < m_scrollTop) {
        m_scrollTop = m_selectedIndex;
    } else if (m_selectedIndex >= m_scrollTop + availH) {
        m_scrollTop = m_selectedIndex - availH + 1;
    }
    m_scrollTop = std::clamp(m_scrollTop, 0, total - 1);
}

void TreeView::setCurrentIndex(int index) {
    if (m_treeDirty)
        updateTree();
    int total = m_visibleNodes.size();
    if (total == 0)
        m_pendingCurrentIndex = index;
    int newIdx = index < 0 || total == 0 ? -1 : std::clamp(index, 0, total - 1);
    if (m_selectedIndex != newIdx) {
        m_selectedIndex = newIdx;
        emit currentIndexChanged();
        emit selectedIndexChanged();
        emit currentItemDataChanged();
    }

    adjustScrollTop();

    // Update delegate current/selected properties.
    for (Item *item : m_delegateItems) {
        if (item) {
            const bool isCurr = item->property("index").toInt() == m_selectedIndex;
            item->setProperty("current", isCurr);
            item->setProperty("selected", isCurr);
            if (QQmlContext *context = delegateContext(item)) {
                DelegateUtils::setContextProperty(m_delegate, context, "current", isCurr);
                DelegateUtils::setContextProperty(m_delegate, context, "selected", isCurr);
            }
        }
    }

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
        if (node.hasChildren) {
            m_expandedPaths.insert(node.key);
            m_collapsedPaths.remove(node.key);
            m_treeDirty = true;
            markDirty();
            updateTree();
            emit expanded(index, 1);
        }
    }
}

void TreeView::collapse(int index) {
    updateTree();
    if (index >= 0 && index < m_visibleNodes.size()) {
        const VisibleNode &node = m_visibleNodes[index];
        if (node.hasChildren) {
            m_collapsedPaths.insert(node.key);
            m_expandedPaths.remove(node.key);
            m_treeDirty = true;
            markDirty();
            updateTree();
            emit collapsed(index, false);
        }
    }
}

void TreeView::toggleExpanded(int index) {
    updateTree();
    if (index >= 0 && index < m_visibleNodes.size()) {
        const VisibleNode &node = m_visibleNodes[index];
        if (node.hasChildren) {
            if (node.expanded) {
                collapse(index);
            } else {
                expand(index);
            }
        }
    }
}

bool TreeView::isExpanded(int index) const {
    const_cast<TreeView*>(this)->updateTree();
    if (index >= 0 && index < m_visibleNodes.size()) {
        return m_visibleNodes[index].expanded;
    }
    return false;
}

int TreeView::depth(int row) const
{
    const_cast<TreeView *>(this)->updateTree();
    return row >= 0 && row < m_visibleNodes.size()
        ? m_visibleNodes[row].depth : -1;
}

void TreeView::expandRecursively(int row, int requestedDepth)
{
    if (row < 0) {
        expandAll();
        emit expanded(row, requestedDepth);
        return;
    }

    updateTree();
    if (row >= m_visibleNodes.size()) return;
    const int baseDepth = m_visibleNodes[row].depth;
    const QString baseKey = m_visibleNodes[row].key;
    bool changed = true;
    while (changed) {
        changed = false;
        updateTree();
        for (const VisibleNode &node : std::as_const(m_visibleNodes)) {
            const bool inSubtree = node.key == baseKey
                || node.key.startsWith(baseKey + '/');
            const int relativeDepth = node.depth - baseDepth;
            if (inSubtree && node.hasChildren
                && (requestedDepth < 0 || relativeDepth < requestedDepth)
                && !m_expandedPaths.contains(node.key)) {
                m_expandedPaths.insert(node.key);
                m_collapsedPaths.remove(node.key);
                changed = true;
            }
        }
        if (changed) m_treeDirty = true;
    }
    markDirty();
    emit expanded(row, requestedDepth);
}

void TreeView::collapseRecursively(int row)
{
    updateTree();
    if (row < 0) {
        collapseAll();
        emit collapsed(row, true);
        return;
    }
    if (row >= m_visibleNodes.size()) return;
    const QString baseKey = m_visibleNodes[row].key;
    for (const VisibleNode &node : std::as_const(m_visibleNodes)) {
        if (node.key == baseKey || node.key.startsWith(baseKey + '/')) {
            m_expandedPaths.remove(node.key);
            m_collapsedPaths.insert(node.key);
        }
    }
    m_treeDirty = true;
    updateTree();
    markDirty();
    emit collapsed(row, true);
}

void TreeView::expandToIndex(const QModelIndex &index)
{
    if (!m_cppModel || !index.isValid() || index.model() != m_cppModel) return;
    QList<QModelIndex> chain;
    for (QModelIndex parent = index.parent(); parent.isValid(); parent = parent.parent())
        chain.prepend(parent);
    QString key;
    for (const QModelIndex &ancestor : std::as_const(chain)) {
        key = key.isEmpty() ? QString::number(ancestor.row())
                            : key + '/' + QString::number(ancestor.row());
        m_expandedPaths.insert(key);
        m_collapsedPaths.remove(key);
    }
    m_treeDirty = true;
    updateTree();
    for (int row = 0; row < m_visibleNodes.size(); ++row) {
        if (m_visibleNodes[row].modelIndex == index) {
            setCurrentIndex(row);
            break;
        }
    }
    markDirty();
}

static void collectAllKeys(const QVariantMap &map, const QString &parentKey, QStringList &keys) {
    QString name = map.value("text", map.value("display", "node")).toString();
    QString currentKey = parentKey.isEmpty() ? name + "#0" : parentKey;
    keys.append(currentKey);

    QVariant childrenVar = map.value("children");
    if (!childrenVar.isValid() || childrenVar.isNull()) childrenVar = map.value("nodes");
    if (childrenVar.userType() == QMetaType::fromType<QJSValue>().id()) {
        childrenVar = childrenVar.value<QJSValue>().toVariant();
    }
    QVariantList children = childrenVar.toList();
    int childIdx = 0;
    for (const QVariant &childVar : children) {
        QVariant cVar = childVar;
        if (cVar.userType() == QMetaType::fromType<QJSValue>().id()) {
            cVar = cVar.value<QJSValue>().toVariant();
        }
        QVariantMap childMap = cVar.toMap();
        QString childName = childMap.value("text", childMap.value("display", "node")).toString();
        QString childKey = currentKey + "/" + childName + "#" + QString::number(childIdx);
        collectAllKeys(childMap, childKey, keys);
        childIdx++;
    }
}

void TreeView::expandAll() {
    m_collapsedPaths.clear();
    QStringList keys;
    if (m_cppModel) {
        std::function<void(const QModelIndex &, const QString &)> collectModelKeys;
        collectModelKeys = [this, &collectModelKeys](const QModelIndex &parent,
                                                     const QString &parentKey) {
            const int rows = m_cppModel->rowCount(parent);
            for (int row = 0; row < rows; ++row) {
                const QModelIndex index = m_cppModel->index(row, 0, parent);
                if (!index.isValid()) continue;
                QString name = m_cppModel->data(index, Qt::DisplayRole).toString();
                if (name.isEmpty())
                    name = QStringLiteral("Item %1").arg(row);
                const QString segment = name + QLatin1Char('#') + QString::number(row);
                const QString key = parentKey.isEmpty()
                    ? segment : parentKey + QLatin1Char('/') + segment;
                if (m_cppModel->rowCount(index) > 0) {
                    m_expandedPaths.insert(key);
                    collectModelKeys(index, key);
                }
            }
        };
        collectModelKeys(m_rootIndex, QString());
    } else if (!m_rootNodeMap.isEmpty()) {
        collectAllKeys(m_rootNodeMap, "", keys);
        for (const QString &k : keys) {
            m_expandedPaths.insert(k);
        }
    }
    m_treeDirty = true;
    markDirty();
}

void TreeView::collapseAll() {
    m_expandedPaths.clear();
    QStringList keys;
    if (!m_rootNodeMap.isEmpty()) {
        collectAllKeys(m_rootNodeMap, "", keys);
        for (const QString &k : keys) {
            m_collapsedPaths.insert(k);
        }
    }
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
    m_delegateModelIndexes.clear();
    m_delegatePaths.clear();
    m_delegateStartIndex = -1;
    m_delegateCapacity = 0;
}

void TreeView::flattenTree(const QVariantMap &map, const QString &parentPath, const QString &parentKey, int depth, QList<VisibleNode> &list) {
    QString name = map.value("text", map.value("display", "node")).toString();
    QString currentPath = parentPath.isEmpty() ? name : parentPath + "/" + name;
    QString currentKey = parentKey;

    QVariant childrenVar = map.value("children");
    if (!childrenVar.isValid() || childrenVar.isNull()) childrenVar = map.value("nodes");
    if (childrenVar.userType() == QMetaType::fromType<QJSValue>().id()) {
        childrenVar = childrenVar.value<QJSValue>().toVariant();
    }
    QVariantList children = childrenVar.toList();
    bool hasChildren = !children.isEmpty();

    bool expanded = false;
    if (m_collapsedPaths.contains(currentKey)) {
        expanded = false;
    } else if (m_expandedPaths.contains(currentKey)) {
        expanded = true;
    } else if (map.contains("expanded")) {
        expanded = map.value("expanded").toBool();
        if (expanded) {
            m_expandedPaths.insert(currentKey);
        } else {
            m_collapsedPaths.insert(currentKey);
        }
    }

    VisibleNode node;
    node.text = name;
    node.path = currentPath;
    node.key = currentKey;
    node.depth = depth;
    node.hasChildren = hasChildren;
    node.expanded = expanded;
    node.rawData = map;

    list.append(node);

    if (hasChildren && expanded) {
        int childIdx = 0;
        for (const QVariant &childVar : children) {
            QVariant cVar = childVar;
            if (cVar.userType() == QMetaType::fromType<QJSValue>().id()) {
                cVar = cVar.value<QJSValue>().toVariant();
            }
            QVariantMap childMap = cVar.toMap();
            QString childName = childMap.value("text", childMap.value("display", "node")).toString();
            QString childKey = currentKey + "/" + childName + "#" + QString::number(childIdx);
            flattenTree(childMap, currentPath, childKey, depth + 1, list);
            childIdx++;
        }
    }
}

void TreeView::flattenCppModel(QAbstractItemModel *model, const QModelIndex &parent, const QString &parentPath, const QString &parentKey, int depth, QList<VisibleNode> &list) {
    if (!model) return;

    int rows = model->rowCount(parent);
    QHash<int, QByteArray> roles = model->roleNames();

    for (int r = 0; r < rows; ++r) {
        QModelIndex idx = model->index(r, 0, parent);
        if (!idx.isValid()) continue;

        QString name = model->data(idx, Qt::DisplayRole).toString();
        if (name.isEmpty()) name = QString("Item %1").arg(r);

        QString currentPath = parentPath.isEmpty() ? name : parentPath + "/" + name;
        QString currentKey = parentKey.isEmpty() ? name + "#" + QString::number(r) : parentKey + "/" + name + "#" + QString::number(r);
        bool hasChildren = model->hasChildren(idx) || (model->rowCount(idx) > 0);

        bool expanded = false;
        if (m_collapsedPaths.contains(currentKey)) {
            expanded = false;
        } else if (m_expandedPaths.contains(currentKey)) {
            expanded = true;
        }

        QVariantMap rawData;
        for (auto it = roles.begin(); it != roles.end(); ++it) {
            rawData[QString::fromUtf8(it.value())] = model->data(idx, it.key());
        }
        rawData["display"] = name;
        rawData["text"] = name;

        VisibleNode node;
        node.text = name;
        node.path = currentPath;
        node.key = currentKey;
        node.depth = depth;
        node.hasChildren = hasChildren;
        node.expanded = expanded;
        node.rawData = rawData;
        node.modelIndex = idx;

        list.append(node);

        if (hasChildren && expanded) {
            flattenCppModel(model, idx, currentPath, currentKey, depth + 1, list);
        }
    }
}

void TreeView::updateTree() {
    if (!m_treeDirty) return;

    const int previousIndex = m_selectedIndex;
    QPersistentModelIndex selectedModelIndex;
    QString selectedKey;
    if (m_selectedIndex >= 0 && m_selectedIndex < m_visibleNodes.size()) {
        selectedModelIndex = m_visibleNodes[m_selectedIndex].modelIndex;
        selectedKey = m_visibleNodes[m_selectedIndex].key;
    }

    QList<Item*> oldItems = m_delegateItems;
    QList<QPersistentModelIndex> oldIndexes = m_delegateModelIndexes;
    QStringList oldPaths = m_delegatePaths;
    m_visibleNodes.clear();
    m_delegateItems.clear();
    m_delegateModelIndexes.clear();
    m_delegatePaths.clear();
    if (m_cppModel) {
        flattenCppModel(m_cppModel.data(), m_rootIndex, "", "", 0, m_visibleNodes);
    } else if (!m_rootNodeMap.isEmpty()) {
        QString rootName = m_rootNodeMap.value("text", m_rootNodeMap.value("display", "node")).toString();
        QString rootKey = rootName + "#0";
        flattenTree(m_rootNodeMap, "", rootKey, 0, m_visibleNodes);
    }

    m_selectedIndex = -1;
    for (int row = 0; row < m_visibleNodes.size(); ++row) {
        const VisibleNode &node = m_visibleNodes[row];
        if ((selectedModelIndex.isValid() && node.modelIndex == selectedModelIndex)
            || (!selectedKey.isEmpty() && node.key == selectedKey)) {
            m_selectedIndex = row;
            break;
        }
    }
    if (m_pendingCurrentIndex >= 0 && !m_visibleNodes.isEmpty()) {
        m_selectedIndex = std::clamp(m_pendingCurrentIndex, 0,
                                     static_cast<int>(m_visibleNodes.size()) - 1);
        m_pendingCurrentIndex = -1;
    } else if (m_selectedIndex < 0 && !m_visibleNodes.isEmpty()) {
        m_selectedIndex = 0;
    }
    adjustScrollTop();
    if (previousIndex != m_selectedIndex) {
        emit currentIndexChanged();
        emit selectedIndexChanged();
    }
    emit currentItemDataChanged();

    generateDelegateItems(oldItems, oldIndexes, oldPaths);
    m_treeDirty = false;
}

static void setPropertyIfExists(QObject* obj, const char* name, const QVariant& value) {
    if (!obj) return;
    if (obj->metaObject()->indexOfProperty(name) != -1) {
        obj->setProperty(name, value);
    }
}

void TreeView::updateDelegateItem(Item *item, int index) {
    if (!item || index < 0 || index >= m_visibleNodes.size()) return;
    const VisibleNode &node = m_visibleNodes[index];
    const bool isCurrent = index == m_selectedIndex;
    item->setProperty("index", index);
    item->setProperty("row", index);
    item->setProperty("column", 0);
    item->setProperty("current", isCurrent);
    item->setProperty("selected", isCurrent);
    item->setProperty("treeView", QVariant::fromValue(this));
    item->setProperty("isTreeNode", true);
    item->setProperty("depth", node.depth);
    item->setProperty("expanded", node.expanded);
    item->setProperty("hasChildren", node.hasChildren);
    item->setProperty("path", node.path);
    item->setProperty("display", node.text);
    item->setProperty("text", node.text);
    item->setProperty("modelData", node.rawData);

    QVariantMap modelMap = node.rawData;
    modelMap["index"] = index;
    modelMap["row"] = index;
    modelMap["column"] = 0;
    modelMap["depth"] = node.depth;
    modelMap["expanded"] = node.expanded;
    modelMap["hasChildren"] = node.hasChildren;
    modelMap["current"] = isCurrent;
    modelMap["selected"] = isCurrent;
    item->setProperty("model", modelMap);

    for (auto it = node.rawData.cbegin(); it != node.rawData.cend(); ++it)
        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());

    if (QQmlContext *context = delegateContext(item)) {
        DelegateUtils::setContextProperty(m_delegate, context, "index", index);
        DelegateUtils::setContextProperty(m_delegate, context, "row", index);
        DelegateUtils::setContextProperty(m_delegate, context, "column", 0);
        DelegateUtils::setContextProperty(m_delegate, context, "current", isCurrent);
        DelegateUtils::setContextProperty(m_delegate, context, "selected", isCurrent);
        DelegateUtils::setContextProperty(m_delegate, context, "treeView", this);
        DelegateUtils::setContextProperty(m_delegate, context, "isTreeNode", true);
        DelegateUtils::setContextProperty(m_delegate, context, "depth", node.depth);
        DelegateUtils::setContextProperty(m_delegate, context, "expanded", node.expanded);
        DelegateUtils::setContextProperty(m_delegate, context, "hasChildren", node.hasChildren);
        DelegateUtils::setContextProperty(m_delegate, context, "path", node.path);
        DelegateUtils::setContextProperty(m_delegate, context, "display", node.text);
        DelegateUtils::setContextProperty(m_delegate, context, "text", node.text);
        DelegateUtils::setContextProperty(m_delegate, context, "modelData", node.rawData);
        DelegateUtils::setContextProperty(m_delegate, context, "model", modelMap);

        for (auto it = node.rawData.cbegin(); it != node.rawData.cend(); ++it) {
            DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());
        }
    }
}

void TreeView::generateDelegateItems(QList<Item*> oldItems,
                                     QList<QPersistentModelIndex> oldIndexes,
                                     QStringList oldPaths) {
    const auto deleteOldItems = [&oldItems]() {
        for (Item *item : oldItems) {
            if (!item) continue;
            item->setParentItem(nullptr);
            delete item;
        }
        oldItems.clear();
    };
    if (!DelegateUtils::isReady(m_delegate, this)) {
        deleteOldItems();
        return;
    }

    QQmlEngine *engine = qmlEngine(this);
    if (!engine && m_delegate) engine = m_delegate->engine();
    if (!engine) {
        deleteOldItems();
        return;
    }

    Item *contentItem = this;

    QHash<QPersistentModelIndex, int> persistentLookup;
    QHash<QString, int> pathLookup;
    persistentLookup.reserve(oldIndexes.size());
    pathLookup.reserve(oldPaths.size());
    for (int oldIndex = 0; oldIndex < oldItems.size(); ++oldIndex) {
        if (oldIndex < oldIndexes.size() && oldIndexes[oldIndex].isValid())
            persistentLookup.insert(oldIndexes[oldIndex], oldIndex);
        else if (oldIndex < oldPaths.size())
            pathLookup.insert(oldPaths[oldIndex], oldIndex);
    }

    const int borderRows = m_borderStyle != "none" ? 2 : 0;
    const int viewportRows = (std::max)(0, height() - borderRows);
    const int visibleCount = static_cast<int>(m_visibleNodes.size());
    const int firstRow = visibleCount == 0 ? 0
        : std::clamp(m_scrollTop, 0, visibleCount - 1);
    const int lastRow = (std::min)(visibleCount,
                                   firstRow + viewportRows + 2);
    m_delegateStartIndex = firstRow;
    m_delegateCapacity = viewportRows;

    for (int i = firstRow; i < lastRow; ++i) {
        const VisibleNode &node = m_visibleNodes[i];
        Item *reused = nullptr;
        const int oldIndex = node.modelIndex.isValid()
            ? persistentLookup.value(node.modelIndex, -1)
            : pathLookup.value(node.key, -1);
        if (oldIndex >= 0 && oldIndex < oldItems.size() && oldItems[oldIndex]) {
            reused = oldItems[oldIndex];
            oldItems[oldIndex] = nullptr;
        }
        if (reused) {
            updateDelegateItem(reused, i);
            m_delegateItems.append(reused);
            m_delegateModelIndexes.append(node.modelIndex);
            m_delegatePaths.append(node.key);
            continue;
        }

        QQmlContext *context = DelegateUtils::createContext(m_delegate, this);
        bool isCurr = (i == m_selectedIndex);

        DelegateUtils::setContextProperty(m_delegate, context, "index", i);
        DelegateUtils::setContextProperty(m_delegate, context, "row", i);
        DelegateUtils::setContextProperty(m_delegate, context, "column", 0);
        DelegateUtils::setContextProperty(m_delegate, context, "current", isCurr);
        DelegateUtils::setContextProperty(m_delegate, context, "selected", isCurr);
        DelegateUtils::setContextProperty(m_delegate, context, "treeView", this);
        DelegateUtils::setContextProperty(m_delegate, context, "isTreeNode", true);
        DelegateUtils::setContextProperty(m_delegate, context, "depth", node.depth);
        DelegateUtils::setContextProperty(m_delegate, context, "expanded", node.expanded);
        DelegateUtils::setContextProperty(m_delegate, context, "hasChildren", node.hasChildren);
        DelegateUtils::setContextProperty(m_delegate, context, "path", node.path);
        DelegateUtils::setContextProperty(m_delegate, context, "display", node.text);
        DelegateUtils::setContextProperty(m_delegate, context, "text", node.text);
        DelegateUtils::setContextProperty(m_delegate, context, "modelData", node.rawData);

        // Combined Qt Quick model map context
        QVariantMap modelMap = node.rawData;
        modelMap["index"] = i;
        modelMap["row"] = i;
        modelMap["column"] = 0;
        modelMap["depth"] = node.depth;
        modelMap["expanded"] = node.expanded;
        modelMap["hasChildren"] = node.hasChildren;
        modelMap["current"] = isCurr;
        modelMap["selected"] = isCurr;
        modelMap["path"] = node.path;
        modelMap["display"] = node.text;
        modelMap["modelData"] = node.rawData;
        DelegateUtils::setContextProperty(m_delegate, context, "model", modelMap);

        for (auto it = node.rawData.cbegin(); it != node.rawData.cend(); ++it) {
            DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());
        }

        QVariantMap initialProps = node.rawData;
        if (!initialProps.contains("icon")) initialProps["icon"] = QString();
        if (!initialProps.contains("status")) initialProps["status"] = QString();
        if (!initialProps.contains("text")) initialProps["text"] = node.text;
        if (!initialProps.contains("path")) initialProps["path"] = node.path;
        initialProps["index"] = i;
        initialProps["row"] = i;
        initialProps["column"] = 0;
        initialProps["depth"] = node.depth;
        initialProps["hasChildren"] = node.hasChildren;
        initialProps["expanded"] = node.expanded;
        initialProps["current"] = isCurr;
        initialProps["selected"] = isCurr;
        initialProps["treeView"] = QVariant::fromValue(this);
        initialProps["isTreeNode"] = true;
        initialProps["display"] = node.text;
        initialProps["modelData"] = node.rawData;
        initialProps["model"] = modelMap;

        if (Item *item = DelegateUtils::createItem(
                m_delegate, initialProps, context, contentItem, this)) {

            item->setProperty("index", i);
            item->setProperty("row", i);
            item->setProperty("column", 0);
            item->setProperty("current", isCurr);
            item->setProperty("selected", isCurr);
            item->setProperty("treeView", QVariant::fromValue(this));
            item->setProperty("isTreeNode", true);
            item->setProperty("depth", node.depth);
            item->setProperty("expanded", node.expanded);
            item->setProperty("hasChildren", node.hasChildren);
            item->setProperty("display", node.text);
            item->setProperty("text", node.text);
            item->setProperty("path", node.path);
            item->setProperty("modelData", node.rawData);

            for (auto it = node.rawData.cbegin(); it != node.rawData.cend(); ++it) {
                setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
            }

            m_delegateItems.append(item);
            m_delegateModelIndexes.append(node.modelIndex);
            m_delegatePaths.append(node.key);
        } else {
            m_delegateItems.append(nullptr);
            m_delegateModelIndexes.append(node.modelIndex);
            m_delegatePaths.append(node.key);
        }
    }
    deleteOldItems();
}

void TreeView::layoutChildren() {
    if (!m_delegate || m_delegateItems.isEmpty()) return;

    int offsetY = (m_borderStyle != "none") ? 1 : 0;
    int offsetX = (m_borderStyle != "none") ? 1 : 0;

    int curY = offsetY;
    for (Item *item : m_delegateItems) {
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
    adjustScrollTop();

    const int expectedCapacity = (std::max)(0, height()
        - (m_borderStyle != "none" ? 2 : 0));
    if (m_delegate && (m_delegateStartIndex != m_scrollTop
                       || m_delegateCapacity != expectedCapacity)) {
        QList<Item*> oldItems = m_delegateItems;
        QList<QPersistentModelIndex> oldIndexes = m_delegateModelIndexes;
        QStringList oldPaths = m_delegatePaths;
        m_delegateItems.clear();
        m_delegateModelIndexes.clear();
        m_delegatePaths.clear();
        generateDelegateItems(oldItems, oldIndexes, oldPaths);
    }

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
        for (Item *item : m_delegateItems) {
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
        if (m_selectedIndex >= 0 && m_selectedIndex < total) {
            const VisibleNode &node = m_visibleNodes[m_selectedIndex];
            if (node.hasChildren) {
                if (!node.expanded) {
                    expand(m_selectedIndex);
                } else if (m_selectedIndex + 1 < total) {
                    setCurrentIndex(m_selectedIndex + 1);
                }
            }
        }
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Left) {
        if (m_selectedIndex >= 0 && m_selectedIndex < total) {
            const VisibleNode &node = m_visibleNodes[m_selectedIndex];
            if (node.hasChildren && node.expanded) {
                collapse(m_selectedIndex);
            } else {
                int currentDepth = node.depth;
                for (int i = m_selectedIndex - 1; i >= 0; --i) {
                    if (m_visibleNodes[i].depth < currentDepth) {
                        setCurrentIndex(i);
                        break;
                    }
                }
            }
        }
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
