#include <QmlTui/Core/Items/TreeView.h>
#include <QmlTui/Core/Items/DelegateUtils.h>
#include <QmlTui/Core/Items/NumericItemRoles_p.h>
#include <QMetaProperty>
#include <QmlTui/Core/Primitives/UnicodeText.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlInfo>
#include <algorithm>
#include <functional>
#include <utility>

namespace qmltui {

using DelegateUtils::delegateContext;
using DelegateUtils::setDelegateProperty;
using DelegateUtils::setPropertyIfExists;

TreeView::TreeView(QObject* parent) : Item(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setFocus(true);
    setImplicitWidth(30);
    setImplicitHeight(10);
}

TreeView::~TreeView() {
    clearDelegateItems();
}

void TreeView::setModel(const QVariant& model) {
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
    m_expandedIndexes.clear();
    m_expandedSnapshot.clear();
    m_visibleLookup.clear();
    m_currentModelIndex = QPersistentModelIndex();
    m_rootIndex = QPersistentModelIndex();
    m_selectedIndex = -1;
    m_scrollTop = 0;

    QObject* obj = qvariant_cast<QObject*>(model);
    if (!obj && model.canConvert<QJSValue>()) {
        obj = model.value<QJSValue>().toQObject();
    }

    if (QAbstractItemModel* cppModel = qobject_cast<QAbstractItemModel*>(obj)) {
        m_cppModel = cppModel;
        m_roleNameStrings.clear();
        const QHash<int, QByteArray> roles = m_cppModel->roleNames();
        for (auto it = roles.cbegin(); it != roles.cend(); ++it)
            m_roleNameStrings.insert(it.key(), QString::fromUtf8(it.value()));
        m_claimedNames = NumericItemRoles::claimedNames(m_cppModel.data());
        connect(m_cppModel.data(), &QObject::destroyed, this, [this]() {
            m_cppModel.clear();
            m_modelVariant.clear();
            m_visibleNodes.clear();
            m_roleNameStrings.clear();
            m_claimedNames = 0;
            clearDelegateItems();
            emit modelChanged();
            emit currentIndexChanged();
            emit selectedIndexChanged();
            emit currentItemDataChanged();
            markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
        });
        const auto invalidate = [this]() {
            m_treeDirty = true;
            markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
        };
        connect(m_cppModel.data(), &QAbstractItemModel::modelReset, this, [this, invalidate]() {
            // Persistent indexes are invalid after a reset; nothing is expanded.
            m_expandedIndexes.clear();
            m_roleNameStrings.clear();
            if (m_cppModel) {
                const QHash<int, QByteArray> roles = m_cppModel->roleNames();
                for (auto it = roles.cbegin(); it != roles.cend(); ++it)
                    m_roleNameStrings.insert(it.key(), QString::fromUtf8(it.value()));
                m_claimedNames = NumericItemRoles::claimedNames(m_cppModel.data());
            }
            invalidate();
        });
        connect(m_cppModel.data(), &QAbstractItemModel::dataChanged, this,
                [this](const QModelIndex& topLeft, const QModelIndex& bottomRight) {
                    handleModelDataChanged(topLeft, bottomRight);
                });
        connect(m_cppModel.data(), &QAbstractItemModel::rowsInserted, this,
                [this, invalidate](const QModelIndex& parent) {
                    if (isLevelShown(parent)) invalidate();
                });
        connect(m_cppModel.data(), &QAbstractItemModel::rowsRemoved, this,
                [this, invalidate](const QModelIndex& parent) {
                    if (isLevelShown(parent)) invalidate();
                });
        connect(m_cppModel.data(), &QAbstractItemModel::rowsMoved, this,
                [this, invalidate](const QModelIndex& sourceParent, int, int,
                                   const QModelIndex& destinationParent, int) {
                    if (isLevelShown(sourceParent) || isLevelShown(destinationParent)) invalidate();
                });
        connect(m_cppModel.data(), &QAbstractItemModel::layoutChanged, this, invalidate);
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
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::setRootNode(const QVariant& node) {
    if (m_rootNodeVariant == node) return;
    m_rootNodeVariant = node;
    setModel(node);
    emit rootNodeChanged();
}

void TreeView::setRootIndex(const QModelIndex& index) {
    const QPersistentModelIndex persistent(index);
    if (m_rootIndex == persistent) return;
    m_rootIndex = persistent;
    m_treeDirty = true;
    emit rootIndexChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::setDelegate(QQmlComponent* delegate) {
    if (m_delegate == delegate) return;
    clearDelegateItems();
    if (m_delegate) disconnect(m_delegate, nullptr, this, nullptr);
    m_delegate = delegate;
    if (m_delegate) {
        connect(m_delegate, &QObject::destroyed, this, [this]() {
            m_delegate.clear();
            clearDelegateItems();
            emit delegateChanged();
            markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
        });
        connect(m_delegate, &QQmlComponent::statusChanged, this,
                [this](QQmlComponent::Status status) {
                    if (status == QQmlComponent::Ready) {
                        m_treeDirty = true;
                        markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
                    }
                });
    }
    m_treeDirty = true;
    emit delegateChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::setNodeFormat(const QJSValue& format) {
    m_nodeFormat = format;
    m_formatErrors.clear();
    for (VisibleNode& node : m_visibleNodes)
        node.formattedAtRow = -1;
    emit nodeFormatChanged();
    markDirty(DirtyFlag::SelfDirty);
}

void TreeView::callbackWarning(const QString& message, QSet<QString>& errors) const {
    if (!errors.contains(message) && errors.size() < 16) {
        errors.insert(message);
        qmlWarning(this) << message;
    }
}

Color TreeView::resolveCachedColor(const QString& name) const {
    if (name.isEmpty()) return {};
    const auto cached = m_colorCache.constFind(name);
    if (cached != m_colorCache.cend()) return Color::fromArgb32(cached.value());
    const Color parsed = Color::fromString(name);
    if (!parsed.isValid()) return {};
    const uint32_t resolved = parsed.argb32();
    if (m_colorCache.size() >= 256) m_colorCache.clear();
    m_colorCache.insert(name, resolved);
    return parsed;
}

void TreeView::adjustScrollTop() {
    const int total = static_cast<int>(m_visibleNodes.size());
    if (total == 0) {
        m_scrollTop = 0;
        m_selectedIndex = -1;
        return;
    }
    if (m_selectedIndex >= 0) m_selectedIndex = std::clamp(m_selectedIndex, 0, total - 1);
    int availH = height();
    if (availH <= 0) availH = 10;

    if (m_selectedIndex < m_scrollTop) {
        m_scrollTop = m_selectedIndex;
    } else if (m_selectedIndex >= m_scrollTop + availH) {
        m_scrollTop = m_selectedIndex - availH + 1;
    }
    m_scrollTop = std::clamp(m_scrollTop, 0, total - 1);
}

void TreeView::setCurrentIndex(int index) {
    if (m_treeDirty) updateTree();
    int total = m_visibleNodes.size();
    if (total == 0) m_pendingCurrentIndex = index;
    int newIdx = index < 0 || total == 0 ? -1 : std::clamp(index, 0, total - 1);
    m_currentModelIndex = newIdx >= 0 && m_cppModel
                              ? QPersistentModelIndex(m_visibleNodes[newIdx].modelIndex)
                              : QPersistentModelIndex();
    if (m_selectedIndex != newIdx) {
        m_selectedIndex = newIdx;
        emit currentIndexChanged();
        emit selectedIndexChanged();
        emit currentItemDataChanged();
    }

    adjustScrollTop();

    // Update delegate current/selected properties.
    for (Item* item : m_delegateItems) {
        if (item) {
            const bool isCurr = item->property("index").toInt() == m_selectedIndex;
            const PositionProperties& props = positionProperties(item);
            const QMetaObject* meta = item->metaObject();
            if (props.current >= 0)
                meta->property(props.current).write(item, isCurr);
            else
                setDelegateProperty(item, "current", isCurr);
            if (props.selected >= 0)
                meta->property(props.selected).write(item, isCurr);
            else
                setDelegateProperty(item, "selected", isCurr);
            if (QQmlContext* context = delegateContext(item)) {
                DelegateUtils::setContextProperty(m_delegate, context, "current", isCurr);
                DelegateUtils::setContextProperty(m_delegate, context, "selected", isCurr);
            }
        }
    }

    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

QVariantMap TreeView::currentItemData() const {
    if (m_selectedIndex < 0 || m_selectedIndex >= m_visibleNodes.size()) return {};
    const_cast<TreeView*>(this)->ensureLoaded(
        const_cast<TreeView*>(this)->m_visibleNodes[m_selectedIndex]);
    return m_visibleNodes[m_selectedIndex].rawData;
}

void TreeView::setSelectionColor(Color color) {
    if (m_selectionColor == color) return;
    m_selectionColor = color;
    m_resolvedSelectionColor = Color::resolve(m_selectionColor, 0);
    emit selectionColorChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::setSelectionBackgroundColor(Color color) {
    if (m_selectionBackgroundColor == color) return;
    m_selectionBackgroundColor = color;
    m_resolvedSelectionBackgroundColor = Color::resolve(m_selectionBackgroundColor, 0);
    emit selectionBackgroundColorChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::setTextColor(Color color) {
    if (m_textColor == color) return;
    m_textColor = color;
    m_resolvedTextColor = Color::resolve(m_textColor, 0);
    emit textColorChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::setBranchColor(Color color) {
    if (m_branchColor == color) return;
    m_branchColor = color;
    m_resolvedBranchColor = Color::resolve(m_branchColor, 0);
    emit branchColorChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::setExpandedIcon(const QString& icon) {
    if (m_expandedIcon == icon) return;
    m_expandedIcon = icon;
    emit expandedIconChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::setCollapsedIcon(const QString& icon) {
    if (m_collapsedIcon == icon) return;
    m_collapsedIcon = icon;
    emit collapsedIconChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::expand(int index) {
    updateTree();
    if (index >= 0 && index < m_visibleNodes.size()) {
        const VisibleNode& node = m_visibleNodes[index];
        if (node.hasChildren) {
            setNodeExpanded(node, true);
            const QPersistentModelIndex modelIndex = node.modelIndex;
            m_treeDirty = true;
            markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
            updateTree();
            // Lazily populated branches (QFileSystemModel style) load their
            // children on first expansion; rowsInserted then rebuilds the tree.
            if (m_cppModel && modelIndex.isValid() && m_cppModel->canFetchMore(modelIndex))
                m_cppModel->fetchMore(modelIndex);
            emit expanded(index, 1);
        }
    }
}

void TreeView::collapse(int index) {
    updateTree();
    if (index >= 0 && index < m_visibleNodes.size()) {
        const VisibleNode& node = m_visibleNodes[index];
        if (node.hasChildren) {
            setNodeExpanded(node, false);
            m_treeDirty = true;
            markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
            updateTree();
            emit collapsed(index, false);
        }
    }
}

void TreeView::toggleExpanded(int index) {
    updateTree();
    if (index >= 0 && index < m_visibleNodes.size()) {
        const VisibleNode& node = m_visibleNodes[index];
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

int TreeView::depth(int row) const {
    const_cast<TreeView*>(this)->updateTree();
    return row >= 0 && row < m_visibleNodes.size() ? m_visibleNodes[row].depth : -1;
}

void TreeView::expandRecursively(int row, int requestedDepth) {
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
        for (const VisibleNode& node : std::as_const(m_visibleNodes)) {
            const bool inSubtree = node.key == baseKey || node.key.startsWith(baseKey + '/');
            const int relativeDepth = node.depth - baseDepth;
            if (inSubtree && node.hasChildren &&
                (requestedDepth < 0 || relativeDepth < requestedDepth) && !node.expanded) {
                setNodeExpanded(node, true);
                changed = true;
            }
        }
        if (changed) m_treeDirty = true;
    }
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    emit expanded(row, requestedDepth);
}

void TreeView::collapseRecursively(int row) {
    updateTree();
    if (row < 0) {
        collapseAll();
        emit collapsed(row, true);
        return;
    }
    if (row >= m_visibleNodes.size()) return;
    const QString baseKey = m_visibleNodes[row].key;
    for (const VisibleNode& node : std::as_const(m_visibleNodes)) {
        if (node.key == baseKey || node.key.startsWith(baseKey + '/')) setNodeExpanded(node, false);
    }
    m_treeDirty = true;
    updateTree();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    emit collapsed(row, true);
}

void TreeView::expandToIndex(const QModelIndex& index) {
    if (!m_cppModel || !index.isValid() || index.model() != m_cppModel) return;
    for (QModelIndex parent = index.parent(); parent.isValid(); parent = parent.parent())
        m_expandedIndexes.insert(QPersistentModelIndex(parent));
    m_treeDirty = true;
    updateTree();
    for (int row = 0; row < m_visibleNodes.size(); ++row) {
        if (m_visibleNodes[row].modelIndex == index) {
            setCurrentIndex(row);
            break;
        }
    }
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

static void extractMapLabel(const QVariantMap& map, QString& text, QList<Span>& spans) {
    spans.clear();
    if (map.contains(QStringLiteral("spans"))) {
        Span::tryExtractList(map.value(QStringLiteral("spans")), /*allowVariantMap=*/true, spans);
    }
    const QVariant labelVar =
        map.value(QStringLiteral("text"), map.value(QStringLiteral("display")));
    if (labelVar.isValid() && !labelVar.isNull()) {
        if (labelVar.metaType() == QMetaType::fromType<QString>()) {
            text = labelVar.toString();
            return;
        }
        if (spans.isEmpty() && Span::tryExtractList(labelVar, /*allowVariantMap=*/true, spans)) {
            text = Span::plainText(spans);
            return;
        }
        text = labelVar.toString();
        return;
    }
    if (!spans.isEmpty()) {
        text = Span::plainText(spans);
        return;
    }
    text = QStringLiteral("node");
}

static QString extractMapName(const QVariantMap& map) {
    QString text;
    QList<Span> spans;
    extractMapLabel(map, text, spans);
    return text;
}

static void collectAllKeys(const QVariantMap& map, const QString& parentKey, QStringList& keys) {
    QString name = extractMapName(map);
    QString currentKey = parentKey.isEmpty() ? name + "#0" : parentKey;
    keys.append(currentKey);

    QVariant childrenVar = map.value("children");
    if (!childrenVar.isValid() || childrenVar.isNull()) childrenVar = map.value("nodes");
    if (childrenVar.userType() == QMetaType::fromType<QJSValue>().id()) {
        childrenVar = childrenVar.value<QJSValue>().toVariant();
    }
    QVariantList children = childrenVar.toList();
    int childIdx = 0;
    for (const QVariant& childVar : children) {
        QVariant cVar = childVar;
        if (cVar.userType() == QMetaType::fromType<QJSValue>().id()) {
            cVar = cVar.value<QJSValue>().toVariant();
        }
        QVariantMap childMap = cVar.toMap();
        QString childName = extractMapName(childMap);
        QString childKey = currentKey + "/" + childName + "#" + QString::number(childIdx);
        collectAllKeys(childMap, childKey, keys);
        childIdx++;
    }
}

void TreeView::expandAll() {
    m_collapsedPaths.clear();
    QStringList keys;
    if (m_cppModel) {
        std::function<void(const QModelIndex&)> collectModelKeys;
        collectModelKeys = [this, &collectModelKeys](const QModelIndex& parent) {
            const int rows = m_cppModel->rowCount(parent);
            for (int row = 0; row < rows; ++row) {
                const QModelIndex index = m_cppModel->index(row, 0, parent);
                if (!index.isValid()) continue;
                // Only rows that are already loaded; expandAll() never fetches.
                if (m_cppModel->rowCount(index) > 0) {
                    m_expandedIndexes.insert(QPersistentModelIndex(index));
                    collectModelKeys(index);
                }
            }
        };
        collectModelKeys(m_rootIndex);
    } else if (!m_rootNodeMap.isEmpty()) {
        collectAllKeys(m_rootNodeMap, "", keys);
        for (const QString& k : keys) {
            m_expandedPaths.insert(k);
        }
    }
    m_treeDirty = true;
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::collapseAll() {
    m_expandedPaths.clear();
    m_expandedIndexes.clear();
    QStringList keys;
    if (!m_rootNodeMap.isEmpty()) {
        collectAllKeys(m_rootNodeMap, "", keys);
        for (const QString& k : keys) {
            m_collapsedPaths.insert(k);
        }
    }
    m_treeDirty = true;
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::clearDelegateItems() {
    for (Item* item : m_delegateItems) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_delegateItems.clear();
    m_delegateModelIndexes.clear();
    m_delegatePaths.clear();
    m_delegateNodes.clear();
    m_delegateStartIndex = -1;
    m_delegateCapacity = 0;
}

void TreeView::flattenTree(const QVariantMap& map, const QString& parentPath,
                           const QString& parentKey, int depth, QList<VisibleNode>& list) {
    QString name;
    QList<Span> spans;
    extractMapLabel(map, name, spans);
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
    node.spans = std::move(spans);
    node.path = currentPath;
    node.key = currentKey;
    node.depth = depth;
    node.hasChildren = hasChildren;
    node.expanded = expanded;
    node.rawData = map;
    node.roleFg = Color::fromString(map.value(QStringLiteral("foreground")).toString());
    node.roleBg = Color::fromString(map.value(QStringLiteral("background")).toString());
    node.roleEnabled = NumericItemRoles::enabled(map);
    node.rolePrefix = NumericItemRoles::prefix(map);

    list.append(node);

    if (hasChildren && expanded) {
        int childIdx = 0;
        for (const QVariant& childVar : children) {
            QVariant cVar = childVar;
            if (cVar.userType() == QMetaType::fromType<QJSValue>().id()) {
                cVar = cVar.value<QJSValue>().toVariant();
            }
            QVariantMap childMap = cVar.toMap();
            QString childName = extractMapName(childMap);
            QString childKey = currentKey + "/" + childName + "#" + QString::number(childIdx);
            flattenTree(childMap, currentPath, childKey, depth + 1, list);
            childIdx++;
        }
    }
}

void TreeView::fillNodeFromModel(QAbstractItemModel* model, const QModelIndex& index,
                                 VisibleNode& node, bool includeUnsetRoles) {
    QList<Span> spans;
    const QVariant displayVar = model->data(index, Qt::DisplayRole);
    QString name;
    if (displayVar.metaType() == QMetaType::fromType<QString>()) {
        name = displayVar.toString();
    } else if (Span::tryExtractList(displayVar, /*allowVariantMap=*/false, spans)) {
        name = Span::plainText(spans);
    } else {
        name = displayVar.toString();
    }
    if (name.isEmpty() && spans.isEmpty()) name = QStringLiteral("Item %1").arg(index.row());
    QVariantMap rawData;
    if (m_roleNameStrings.isEmpty()) {
        const QHash<int, QByteArray> roles = model->roleNames();
        for (auto it = roles.cbegin(); it != roles.cend(); ++it)
            m_roleNameStrings.insert(it.key(), QString::fromUtf8(it.value()));
        m_claimedNames = NumericItemRoles::claimedNames(model);
    }
    for (auto it = m_roleNameStrings.cbegin(); it != m_roleNameStrings.cend(); ++it) {
        const QVariant roleVal = model->data(index, it.key());
        rawData[it.value()] = roleVal;
        if (spans.isEmpty() && it.value() == QStringLiteral("spans")) {
            Span::tryExtractList(roleVal, /*allowVariantMap=*/true, spans);
        }
    }
    if (name.isEmpty() && !spans.isEmpty()) name = Span::plainText(spans);
    rawData[QStringLiteral("display")] = name;
    rawData[QStringLiteral("text")] = name;
    NumericItemRoles::apply(model, index, rawData, includeUnsetRoles, m_claimedNames);
    node.text = name;
    node.spans = std::move(spans);
    node.rawData = rawData;
    node.roleFg = Color::fromString(rawData.value(QStringLiteral("foreground")).toString());
    node.roleBg = Color::fromString(rawData.value(QStringLiteral("background")).toString());
    node.roleEnabled = NumericItemRoles::enabled(rawData);
    node.rolePrefix = NumericItemRoles::prefix(rawData);
    node.formattedAtRow = -1;
    node.hasChildren = model->hasChildren(index) || model->rowCount(index) > 0;
    node.modelIndex = index;
    node.loaded = true;
}

QString TreeView::modelPath(const QAbstractItemModel* model, const QModelIndex& index) {
    QStringList names;
    for (QModelIndex at = index; at.isValid(); at = at.parent()) {
        const QVariant displayVar = model->data(at, Qt::DisplayRole);
        QString name;
        QList<Span> spans;
        if (displayVar.metaType() == QMetaType::fromType<QString>()) {
            name = displayVar.toString();
        } else if (Span::tryExtractList(displayVar, /*allowVariantMap=*/false, spans)) {
            name = Span::plainText(spans);
        } else {
            name = displayVar.toString();
        }
        if (name.isEmpty()) name = QStringLiteral("Item %1").arg(at.row());
        names.prepend(name);
    }
    return names.join(QLatin1Char('/'));
}

void TreeView::ensureLoaded(VisibleNode& node) {
    if (node.loaded || !m_cppModel || !node.modelIndex.isValid()) return;
    fillNodeFromModel(m_cppModel.data(), node.modelIndex, node);
    node.path = modelPath(m_cppModel.data(), node.modelIndex);
}

void TreeView::ensureFormatted(VisibleNode& node, int row) {
    if (node.formattedAtRow == row || !m_nodeFormat.isCallable()) return;
    node.formattedAtRow = row;
    node.formattedText = node.text;
    node.formattedSpans = node.spans;
    node.formatFg = {};
    node.formatBg = {};
    node.formatStyle = {};
    node.spansOverrideSelection = false;

    const QJSValue result = m_nodeFormat.call({row, node.text});
    if (result.isError()) {
        callbackWarning(QStringLiteral("nodeFormat: ") + result.toString() + QLatin1Char('\n') +
                            result.property("stack").toString(),
                        m_formatErrors);
        return;
    }
    m_formatErrors.clear();
    if (result.isArray()) {
        QList<Span> formattedSpans;
        if (Span::tryExtractList(result.toVariant(), /*allowVariantMap=*/true, formattedSpans)) {
            node.formattedSpans = std::move(formattedSpans);
            node.spansOverrideSelection = true;
        }
    } else if (result.isObject()) {
        const QVariant retained = result.toVariant(QJSValue::RetainJSObjects);
        if (retained.metaType() == QMetaType::fromType<Span>()) {
            const Span s = retained.value<Span>();
            node.formattedText = s.text();
            if (s.color().isValid() && !s.color().isTransparent()) node.formatFg = s.color();
            if (s.backgroundColor().isValid() && !s.backgroundColor().isTransparent())
                node.formatBg = s.backgroundColor();
            node.formatStyle = s.style();
            node.formattedSpans.clear();
        } else {
            const QJSValue textProp = result.property(QStringLiteral("text"));
            if (!textProp.isUndefined()) {
                node.formattedText = textProp.toString();
                node.formattedSpans.clear();
            }
            const QJSValue spansProp = result.property(QStringLiteral("spans"));
            if (!spansProp.isUndefined()) {
                QList<Span> formattedSpans;
                if (Span::tryExtractList(spansProp.toVariant(), /*allowVariantMap=*/true,
                                         formattedSpans)) {
                    node.formattedSpans = std::move(formattedSpans);
                    node.spansOverrideSelection = true;
                }
            }
            bool formatGaveFg = false;
            bool formatGaveBg = false;
            const QJSValue cProp = result.property(QStringLiteral("color"));
            if (!cProp.isUndefined()) {
                if (cProp.isString()) {
                    node.formatFg = resolveCachedColor(cProp.toString());
                } else {
                    const QVariant cv = cProp.toVariant(QJSValue::RetainJSObjects);
                    if (cv.metaType() == QMetaType::fromType<Color>())
                        node.formatFg = cv.value<Color>();
                    else
                        node.formatFg = resolveCachedColor(cProp.toString());
                }
                formatGaveFg = node.formatFg.isValid();
            }
            const QJSValue bgProp = result.property(QStringLiteral("backgroundColor"));
            if (!bgProp.isUndefined()) {
                if (bgProp.isString()) {
                    node.formatBg = resolveCachedColor(bgProp.toString());
                } else {
                    const QVariant bgv = bgProp.toVariant(QJSValue::RetainJSObjects);
                    if (bgv.metaType() == QMetaType::fromType<Color>())
                        node.formatBg = bgv.value<Color>();
                    else
                        node.formatBg = resolveCachedColor(bgProp.toString());
                }
                formatGaveBg = node.formatBg.isValid();
            }
            const auto applyFlag = [&](const QString& key, void (TextStyle::*setter)(bool)) {
                const QJSValue p = result.property(key);
                if (!p.isUndefined()) (node.formatStyle.*setter)(p.toBool());
            };
            applyFlag(QStringLiteral("bold"), &TextStyle::setBold);
            applyFlag(QStringLiteral("dim"), &TextStyle::setDim);
            applyFlag(QStringLiteral("italic"), &TextStyle::setItalic);
            applyFlag(QStringLiteral("underline"), &TextStyle::setUnderline);
            applyFlag(QStringLiteral("reversed"), &TextStyle::setReversed);
            if (!node.spansOverrideSelection && !node.formattedSpans.isEmpty()) {
                for (Span& s : node.formattedSpans) {
                    if (formatGaveFg) s.resetColor();
                    if (formatGaveBg) s.resetBackgroundColor();
                    if (!node.formatStyle.isEmpty()) {
                        TextStyle st = s.style();
                        st.applyOverlay(node.formatStyle);
                        s.setStyle(st);
                    }
                }
            }
        }
    }
}

bool TreeView::sameNode(const VisibleNode& a, const VisibleNode& b) {
    return a.loaded && b.loaded && a.depth == b.depth && a.expanded == b.expanded &&
           a.hasChildren == b.hasChildren && a.text == b.text && a.spans == b.spans &&
           a.path == b.path && a.rawData == b.rawData;
}

void TreeView::flattenCppModel(QAbstractItemModel* model, const QModelIndex& parent,
                               const QString& parentKey, int depth, QList<VisibleNode>& list) {
    // Shape only. Keys are row paths ("#2/#0"), unique within one pass and
    // hierarchical, which is all expandRecursively() and delegate reuse need;
    // no role is read here (see ensureLoaded()).
    const int rows = model->rowCount(parent);
    list.reserve(list.size() + rows);
    for (int r = 0; r < rows; ++r) {
        const QModelIndex idx = model->index(r, 0, parent);
        if (!idx.isValid()) continue;

        VisibleNode node;
        node.loaded = false;
        node.modelIndex = idx;
        node.key = parentKey.isEmpty() ? QLatin1Char('#') + QString::number(r)
                                       : parentKey + QLatin1String("/#") + QString::number(r);
        node.depth = depth;
        node.hasChildren = model->hasChildren(idx);
        node.expanded = node.hasChildren && m_expandedSnapshot.contains(idx);
        list.append(node);

        if (node.expanded) flattenCppModel(model, idx, node.key, depth + 1, list);
    }
}

bool TreeView::isLevelShown(const QModelIndex& parent) const {
    if (parent == m_rootIndex) return true;
    return parent.isValid() && m_visibleLookup.contains(parent);
}

void TreeView::setNodeExpanded(const VisibleNode& node, bool expanded) {
    if (node.modelIndex.isValid()) {
        if (expanded)
            m_expandedIndexes.insert(node.modelIndex);
        else
            m_expandedIndexes.remove(node.modelIndex);
        return;
    }
    if (expanded) {
        m_expandedPaths.insert(node.key);
        m_collapsedPaths.remove(node.key);
    } else {
        m_collapsedPaths.insert(node.key);
        m_expandedPaths.remove(node.key);
    }
}

Item* TreeView::delegateItemForRow(int row) const {
    const int slot = row - m_delegateStartIndex;
    return m_delegateStartIndex >= 0 && slot >= 0 && slot < m_delegateItems.size()
               ? m_delegateItems[slot]
               : nullptr;
}

void TreeView::handleModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) {
    if (!m_cppModel) return;
    const auto invalidate = [this]() {
        m_treeDirty = true;
        markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    };
    if (m_treeDirty) {
        invalidate();
        return;
    }
    // Refresh the affected visible nodes in place. Only a change that alters
    // the shape of the tree (a leaf gaining children, or an expanded branch
    // being renamed, which changes every descendant path) needs a re-flatten.
    const QModelIndex parent = topLeft.parent();
    bool touched = false;
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        const QModelIndex index = m_cppModel->index(row, 0, parent);
        const int visibleRow = m_visibleLookup.value(index, -1);
        if (visibleRow < 0) continue;
        VisibleNode& node = m_visibleNodes[visibleRow];
        const bool hadChildren = node.hasChildren;
        const bool wasLoaded = node.loaded;
        const QString oldText = node.text;
        fillNodeFromModel(m_cppModel.data(), index, node, true);
        node.path = modelPath(m_cppModel.data(), index);
        node.expanded = node.hasChildren && m_expandedSnapshot.contains(index);
        const bool renamed = wasLoaded && node.text != oldText;
        if (hadChildren != node.hasChildren || (renamed && node.expanded)) {
            invalidate();
            return;
        }
        if (Item* item = delegateItemForRow(visibleRow)) {
            updateDelegateItem(item, visibleRow);
            const int slot = visibleRow - m_delegateStartIndex;
            if (slot >= 0 && slot < m_delegateNodes.size()) m_delegateNodes[slot] = node;
        }
        if (visibleRow == m_selectedIndex) emit currentItemDataChanged();
        touched = true;
    }
    if (touched) markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void TreeView::updateTree() {
    if (!m_treeDirty) return;

    const int previousIndex = m_selectedIndex;
    // The persistent index follows the current node through inserts, moves
    // and proxy reordering; the string key is for JS map trees.
    const QPersistentModelIndex selectedModelIndex = m_currentModelIndex;
    QString selectedKey;
    if (!m_cppModel && m_selectedIndex >= 0 && m_selectedIndex < m_visibleNodes.size())
        selectedKey = m_visibleNodes[m_selectedIndex].key;

    QList<Item*> oldItems = std::move(m_delegateItems);
    QList<QPersistentModelIndex> oldIndexes = std::move(m_delegateModelIndexes);
    QStringList oldPaths = std::move(m_delegatePaths);
    QList<VisibleNode> oldNodes = std::move(m_delegateNodes);
    m_visibleNodes.clear();
    m_delegateItems.clear();
    m_delegateModelIndexes.clear();
    m_delegatePaths.clear();
    m_delegateNodes.clear();
    if (m_cppModel) {
        m_expandedSnapshot.clear();
        for (const QPersistentModelIndex& expanded : std::as_const(m_expandedIndexes))
            if (expanded.isValid()) m_expandedSnapshot.insert(QModelIndex(expanded));
        flattenCppModel(m_cppModel.data(), m_rootIndex, "", 0, m_visibleNodes);
    } else if (!m_rootNodeMap.isEmpty()) {
        QString rootName = extractMapName(m_rootNodeMap);
        QString rootKey = rootName + "#0";
        flattenTree(m_rootNodeMap, "", rootKey, 0, m_visibleNodes);
    }

    m_visibleLookup.clear();
    for (int row = 0; row < m_visibleNodes.size(); ++row)
        if (m_visibleNodes[row].modelIndex.isValid())
            m_visibleLookup.insert(m_visibleNodes[row].modelIndex, row);
    for (auto it = m_expandedIndexes.begin(); it != m_expandedIndexes.end();)
        it = it->isValid() ? std::next(it) : m_expandedIndexes.erase(it);

    m_selectedIndex = -1;
    for (int row = 0; row < m_visibleNodes.size(); ++row) {
        const VisibleNode& node = m_visibleNodes[row];
        if ((selectedModelIndex.isValid() && node.modelIndex == selectedModelIndex) ||
            (!selectedKey.isEmpty() && node.key == selectedKey)) {
            m_selectedIndex = row;
            break;
        }
    }
    if (m_pendingCurrentIndex >= 0 && !m_visibleNodes.isEmpty()) {
        m_selectedIndex =
            std::clamp(m_pendingCurrentIndex, 0, static_cast<int>(m_visibleNodes.size()) - 1);
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
    m_currentModelIndex = m_selectedIndex >= 0 && m_cppModel
                              ? QPersistentModelIndex(m_visibleNodes[m_selectedIndex].modelIndex)
                              : QPersistentModelIndex();

    generateDelegateItems(std::move(oldItems), std::move(oldIndexes), std::move(oldPaths),
                          std::move(oldNodes));
    m_treeDirty = false;
}

const TreeView::PositionProperties& TreeView::positionProperties(const QObject* item) {
    const QMetaObject* meta = item->metaObject();
    auto it = m_positionProperties.find(meta);
    if (it == m_positionProperties.end()) {
        PositionProperties props;
        props.index = meta->indexOfProperty("index");
        props.row = meta->indexOfProperty("row");
        props.current = meta->indexOfProperty("current");
        props.selected = meta->indexOfProperty("selected");
        it = m_positionProperties.insert(meta, props);
    }
    return it.value();
}

void TreeView::updateDelegatePosition(Item* item, int index) {
    if (!item || index < 0 || index >= m_visibleNodes.size()) return;
    const bool isCurrent = index == m_selectedIndex;
    // Declared properties are written through their cached index; a delegate
    // without them keeps receiving dynamic properties, as updateDelegateItem
    // has always done.
    const PositionProperties& props = positionProperties(item);
    const QMetaObject* meta = item->metaObject();
    const auto write = [&](int propertyIndex, const char* name, const QVariant& value) {
        if (propertyIndex >= 0)
            meta->property(propertyIndex).write(item, value);
        else
            setDelegateProperty(item, name, value);
    };
    write(props.index, "index", index);
    write(props.row, "row", index);
    write(props.current, "current", isCurrent);
    write(props.selected, "selected", isCurrent);
    if (QQmlContext* context = delegateContext(item)) {
        DelegateUtils::setContextProperty(m_delegate, context, "index", index);
        DelegateUtils::setContextProperty(m_delegate, context, "row", index);
        DelegateUtils::setContextProperty(m_delegate, context, "current", isCurrent);
        DelegateUtils::setContextProperty(m_delegate, context, "selected", isCurrent);
    }
}

void TreeView::updateDelegateItem(Item* item, int index) {
    if (!item || index < 0 || index >= m_visibleNodes.size()) return;
    ensureLoaded(m_visibleNodes[index]);
    const VisibleNode& node = m_visibleNodes[index];
    const bool isCurrent = index == m_selectedIndex;

    QVariantMap roleData = node.rawData;
    if (m_cppModel && node.modelIndex.isValid()) {
        NumericItemRoles::apply(m_cppModel.data(), node.modelIndex, roleData, true, m_claimedNames);
    }

    setDelegateProperty(item, "index", index);
    setDelegateProperty(item, "row", index);
    setDelegateProperty(item, "column", 0);
    setDelegateProperty(item, "current", isCurrent);
    setDelegateProperty(item, "selected", isCurrent);
    setDelegateProperty(item, "treeView", QVariant::fromValue(this));
    setDelegateProperty(item, "isTreeNode", true);
    setDelegateProperty(item, "depth", node.depth);
    setDelegateProperty(item, "expanded", node.expanded);
    setDelegateProperty(item, "hasChildren", node.hasChildren);
    setDelegateProperty(item, "path", node.path);
    setDelegateProperty(item, "display", node.text);
    setDelegateProperty(item, "text", node.text);
    setDelegateProperty(item, "modelData", node.rawData);

    QVariantMap modelMap = node.rawData;
    modelMap["index"] = index;
    modelMap["row"] = index;
    modelMap["column"] = 0;
    modelMap["depth"] = node.depth;
    modelMap["expanded"] = node.expanded;
    modelMap["hasChildren"] = node.hasChildren;
    modelMap["current"] = isCurrent;
    modelMap["selected"] = isCurrent;
    modelMap["path"] = node.path;
    modelMap["display"] = node.text;
    modelMap["modelData"] = node.rawData;
    setDelegateProperty(item, "model", modelMap);

    if (!roleData.contains(QStringLiteral("icon"))) setPropertyIfExists(item, "icon", QString());
    if (!roleData.contains(QStringLiteral("status")))
        setPropertyIfExists(item, "status", QString());

    for (auto it = roleData.cbegin(); it != roleData.cend(); ++it) {
        if (it.key() == QStringLiteral("children") || it.key() == QStringLiteral("parent"))
            continue;
        setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
    }

    if (QQmlContext* context = delegateContext(item)) {
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

        for (auto it = roleData.cbegin(); it != roleData.cend(); ++it) {
            DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());
        }
    }
}

void TreeView::generateDelegateItems(QList<Item*> oldItems, QList<QPersistentModelIndex> oldIndexes,
                                     QStringList oldPaths, QList<VisibleNode> oldNodes) {
    const auto deleteOldItems = [&oldItems]() {
        for (Item* item : oldItems) {
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

    QQmlEngine* engine = qmlEngine(this);
    if (!engine && m_delegate) engine = m_delegate->engine();
    if (!engine) {
        deleteOldItems();
        return;
    }

    Item* contentItem = this;

    QHash<QModelIndex, int> indexLookup;
    QHash<QString, int> pathLookup;
    indexLookup.reserve(oldIndexes.size());
    pathLookup.reserve(oldPaths.size());
    for (int oldIndex = 0; oldIndex < oldItems.size(); ++oldIndex) {
        if (oldIndex < oldIndexes.size() && oldIndexes[oldIndex].isValid())
            indexLookup.insert(QModelIndex(oldIndexes[oldIndex]), oldIndex);
        else if (oldIndex < oldPaths.size())
            pathLookup.insert(oldPaths[oldIndex], oldIndex);
    }

    const int viewportRows = (std::max)(0, height());
    const int visibleCount = static_cast<int>(m_visibleNodes.size());
    const int firstRow = visibleCount == 0 ? 0 : std::clamp(m_scrollTop, 0, visibleCount - 1);
    const int lastRow = (std::min)(visibleCount, firstRow + viewportRows + 2);
    m_delegateStartIndex = firstRow;
    m_delegateCapacity = viewportRows;

    const int rowCount = (std::max)(0, lastRow - firstRow);
    QList<int> matchedOldIndex(rowCount, -1);
    QList<bool> claimedOld(oldItems.size(), false);
    for (int i = firstRow; i < lastRow; ++i) {
        const VisibleNode& node = m_visibleNodes[i];
        const int oldIndex = node.modelIndex.isValid() ? indexLookup.value(node.modelIndex, -1)
                                                       : pathLookup.value(node.key, -1);
        if (oldIndex >= 0 && oldIndex < oldItems.size() && oldItems[oldIndex] &&
            !claimedOld[oldIndex]) {
            matchedOldIndex[i - firstRow] = oldIndex;
            claimedOld[oldIndex] = true;
        }
    }

    const bool canRecycleUnmatched =
        m_delegate && !QQmlListReference(m_delegate, "choices").isValid();
    int nextUnclaimedOld = 0;

    for (int i = firstRow; i < lastRow; ++i) {
        ensureLoaded(m_visibleNodes[i]);
        const VisibleNode& node = m_visibleNodes[i];
        Item* reused = nullptr;
        const int oldIndex = matchedOldIndex[i - firstRow];
        if (oldIndex >= 0) {
            reused = oldItems[oldIndex];
            oldItems[oldIndex] = nullptr;
            // Same node, unchanged: only its position moved (scrolling).
            if (oldIndex < oldNodes.size() && sameNode(oldNodes[oldIndex], node))
                updateDelegatePosition(reused, i);
            else
                updateDelegateItem(reused, i);
            m_delegateItems.append(reused);
            m_delegateModelIndexes.append(node.modelIndex);
            m_delegatePaths.append(node.key);
            m_delegateNodes.append(node);
            continue;
        }

        if (canRecycleUnmatched) {
            while (nextUnclaimedOld < oldItems.size() &&
                   (claimedOld[nextUnclaimedOld] || !oldItems[nextUnclaimedOld])) {
                ++nextUnclaimedOld;
            }
            if (nextUnclaimedOld < oldItems.size()) {
                reused = oldItems[nextUnclaimedOld];
                oldItems[nextUnclaimedOld] = nullptr;
                claimedOld[nextUnclaimedOld] = true;
                ++nextUnclaimedOld;
                reused->setParentItem(contentItem);
                updateDelegateItem(reused, i);
                m_delegateItems.append(reused);
                m_delegateModelIndexes.append(node.modelIndex);
                m_delegatePaths.append(node.key);
                m_delegateNodes.append(node);
                continue;
            }
        }

        QQmlContext* context = DelegateUtils::createContext(m_delegate, this);
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
        initialProps.remove(QStringLiteral("children"));
        initialProps.remove(QStringLiteral("parent"));
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

        if (Item* item =
                DelegateUtils::createItem(m_delegate, initialProps, context, contentItem, this)) {

            setDelegateProperty(item, "index", i);
            setDelegateProperty(item, "row", i);
            setDelegateProperty(item, "column", 0);
            setDelegateProperty(item, "current", isCurr);
            setDelegateProperty(item, "selected", isCurr);
            setDelegateProperty(item, "treeView", QVariant::fromValue(this));
            setDelegateProperty(item, "isTreeNode", true);
            setDelegateProperty(item, "depth", node.depth);
            setDelegateProperty(item, "expanded", node.expanded);
            setDelegateProperty(item, "hasChildren", node.hasChildren);
            setDelegateProperty(item, "display", node.text);
            setDelegateProperty(item, "text", node.text);
            setDelegateProperty(item, "path", node.path);
            setDelegateProperty(item, "modelData", node.rawData);

            for (auto it = node.rawData.cbegin(); it != node.rawData.cend(); ++it) {
                if (it.key() == QStringLiteral("children") || it.key() == QStringLiteral("parent"))
                    continue;
                setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
            }

            m_delegateItems.append(item);
            m_delegateModelIndexes.append(node.modelIndex);
            m_delegatePaths.append(node.key);
            m_delegateNodes.append(node);
        } else {
            m_delegateItems.append(nullptr);
            m_delegateModelIndexes.append(node.modelIndex);
            m_delegatePaths.append(node.key);
            m_delegateNodes.append(node);
        }
    }
    deleteOldItems();
}

void TreeView::layoutChildren() {
    if (!m_delegate || m_delegateItems.isEmpty()) return;

    int curY = 0;
    for (Item* item : m_delegateItems) {
        if (!item) continue;

        int itemH = item->height();
        if (itemH <= 0) itemH = 1;

        if (curY + itemH > height()) {
            item->setVisible(false);
        } else {
            item->setX(0);
            item->setY(curY);
            item->setWidth(width());
            item->setVisible(true);
            curY += itemH;
        }
    }
}

void TreeView::updateLayout() {
    Item::updateLayout();
    updateTree();
    adjustScrollTop();
    if (m_cppModel) {
        // Like QTreeView, load the shown level of a lazily populated model
        // until the viewport is filled. Branches fetch on expand() instead.
        DelegateUtils::fetchMoreIfNeeded(m_cppModel.data(), m_rootIndex, 1, m_fetching, [this] {
            updateTree();
            return (std::min)(int(m_visibleNodes.size()) - 1,
                              m_scrollTop + (std::max)(1, height()) - 1);
        });
        updateTree();
        adjustScrollTop();
    }

    const int expectedCapacity = (std::max)(0, height());
    if (m_delegate &&
        (m_delegateStartIndex != m_scrollTop || m_delegateCapacity != expectedCapacity)) {
        QList<Item*> oldItems = std::move(m_delegateItems);
        QList<QPersistentModelIndex> oldIndexes = std::move(m_delegateModelIndexes);
        QStringList oldPaths = std::move(m_delegatePaths);
        QList<VisibleNode> oldNodes = std::move(m_delegateNodes);
        m_delegateItems.clear();
        m_delegateModelIndexes.clear();
        m_delegatePaths.clear();
        m_delegateNodes.clear();
        generateDelegateItems(std::move(oldItems), std::move(oldIndexes), std::move(oldPaths),
                              std::move(oldNodes));
    }

    if (m_delegate) layoutChildren();
}

void TreeView::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!m_visible) return;

    // Delegates were created and positioned by updateLayout() and paint
    // themselves as children. The flattened node list was refreshed there too.
    if (m_delegate) return;

    const int availH = height();
    const int availW = width();
    const int startX = absX;
    const int startY = absY;

    if (availW <= 0 || availH <= 0) return;

    // Default Rendering (No Delegate)
    auto* self = const_cast<TreeView*>(this);
    const bool hasNodeFormat = m_nodeFormat.isCallable();
    for (int idx = m_scrollTop; idx < m_visibleNodes.size() && (idx - m_scrollTop) < availH;
         ++idx) {
        self->ensureLoaded(self->m_visibleNodes[idx]);
        if (hasNodeFormat) self->ensureFormatted(self->m_visibleNodes[idx], idx);
        const VisibleNode& node = m_visibleNodes[idx];
        int rowY = startY + (idx - m_scrollTop);
        bool isSelected = (idx == m_selectedIndex);

        uint32_t bg = isSelected ? m_resolvedSelectionBackgroundColor : 0x00000000;
        uint32_t fg = isSelected && m_selectionColor.isValid() ? m_resolvedSelectionColor
                                                               : m_resolvedTextColor;
        TextAttribute attr =
            (isSelected ? TextAttribute::Bold : TextAttribute::None) |
            (isSelected && !m_selectionColor.isValid() && !m_selectionBackgroundColor.isValid()
                 ? TextAttribute::Reversed
                 : TextAttribute::None);

        if (isSelected) {
            buffer.fillRect(startX, rowY, availW, 1, " ", fg, bg);
            for (int x = 0; x < availW; ++x)
                if (Cell* cell = buffer.tryCellAt(startX + x, rowY)) cell->attributes = attr;
        }

        if (!isSelected) {
            if (node.roleFg.isValid()) fg = node.roleFg.argb32();
            if (node.roleBg.isValid()) bg = node.roleBg.argb32();
        }
        if (!node.roleEnabled) attr = attr | TextAttribute::Dim;

        const QString prefix = node.hasChildren ? (node.expanded ? m_expandedIcon : m_collapsedIcon)
                                                : QStringLiteral("    ");
        const uint32_t branchFg = (!isSelected && m_resolvedBranchColor != 0 &&
                                   !Color::isTransparent(m_resolvedBranchColor))
                                      ? m_resolvedBranchColor
                                      : fg;

        const QString& labelText = hasNodeFormat ? node.formattedText : node.text;
        const QList<Span>& labelSpans = hasNodeFormat ? node.formattedSpans : node.spans;
        uint32_t labelFg = fg;
        uint32_t labelBg = bg;
        TextAttribute labelAttr = attr;
        bool spansOverrideSelection = false;

        if (hasNodeFormat) {
            if (node.formatFg.isValid()) labelFg = node.formatFg.argb32();
            if (node.formatBg.isValid()) labelBg = node.formatBg.argb32();
            labelAttr = node.formatStyle.resolveAttributes(labelAttr);
            spansOverrideSelection = node.spansOverrideSelection;
        }

        int curX = 0;
        for (int i = 0; i < node.depth * 2 && curX < availW; ++i) {
            buffer.setGrapheme(startX + curX++, rowY, QByteArrayView(" ", 1), 1, fg, bg, attr);
        }
        const auto writeSegment = [&](QStringView segText, uint32_t segFg, uint32_t segBg,
                                      TextAttribute segAttr) {
            if (curX >= availW || segText.isEmpty()) return;
            GraphemeIterator it(segText, /*sanitizeControls=*/true);
            GraphemeRef cluster;
            while (it.next(cluster)) {
                if (cluster.width > availW - curX) break;
                const int advance = buffer.setGrapheme(startX + curX, rowY, cluster.utf16,
                                                       cluster.width, segFg, segBg, segAttr);
                if (advance < 0) break;
                curX += advance;
            }
        };

        writeSegment(prefix, branchFg, bg, attr);
        writeSegment(node.rolePrefix, fg, bg, attr);
        if (labelSpans.isEmpty()) {
            writeSegment(labelText, labelFg, labelBg, labelAttr);
        } else {
            const bool useSpanColors = !isSelected || spansOverrideSelection;
            for (const Span& span : labelSpans) {
                if (curX >= availW) break;
                const uint32_t spanFg = useSpanColors ? span.resolveForeground(labelFg) : labelFg;
                const uint32_t spanBg = useSpanColors ? span.resolveBackground(labelBg) : labelBg;
                const TextAttribute spanAttr = span.resolveAttributes(labelAttr);
                writeSegment(span.text(), spanFg, spanBg, spanAttr);
            }
        }
    }
}

ClipRect TreeView::clipRectForChildren(int absX, int absY, const ClipRect& clipRect) const {
    return clipRect.intersected(ClipRect{
        absX,
        absY,
        (std::max)(0, width()),
        (std::max)(0, height()),
    });
}

void TreeView::keyPressEvent(KeyEvent* event) {
    updateTree();
    int total = m_visibleNodes.size();
    const int page = height() > 0 ? height() : 10;

    if (event->key() == Qt::Key_Up) {
        if (m_selectedIndex > 0) setCurrentIndex(m_selectedIndex - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Down) {
        if (m_selectedIndex < total - 1) setCurrentIndex(m_selectedIndex + 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageUp) {
        setCurrentIndex((std::max)(0, m_selectedIndex - page));
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageDown) {
        setCurrentIndex((std::min)(total - 1, m_selectedIndex + page));
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Home) {
        if (total > 0) setCurrentIndex(0);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_End) {
        if (total > 0) setCurrentIndex(total - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Right) {
        if (m_selectedIndex >= 0 && m_selectedIndex < total) {
            const VisibleNode& node = m_visibleNodes[m_selectedIndex];
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
            const VisibleNode& node = m_visibleNodes[m_selectedIndex];
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
            ensureLoaded(m_visibleNodes[m_selectedIndex]);
            emit nodeActivated(m_visibleNodes[m_selectedIndex].path);
            emit itemActivated(m_selectedIndex);
        }
        event->setAccepted(true);
    }
}

void TreeView::mousePressEvent(MouseEvent* event) {
    setFocus(true);
    int row = event->y() + m_scrollTop;
    updateTree();

    if (row >= 0 && row < m_visibleNodes.size()) {
        setCurrentIndex(row);
        toggleExpanded(row);
        ensureLoaded(m_visibleNodes[row]);
        emit nodeActivated(m_visibleNodes[row].path);
        emit itemActivated(row);
        event->setAccepted(true);
    }
}

void TreeView::wheelEvent(MouseEvent* event) {
    if (event->wheelDeltaY() == 0) return;
    if ((focusPolicy() & (Qt::ClickFocus | Qt::WheelFocus)) && !focus()) setFocus(true);
    updateTree();
    const int total = static_cast<int>(m_visibleNodes.size());
    const int availH = height() > 0 ? height() : 10;
    const int maxScrollTop = (std::max)(0, total - availH);
    const int nextScrollTop = std::clamp(m_scrollTop - event->wheelDeltaY(), 0, maxScrollTop);
    if (nextScrollTop != m_scrollTop) {
        m_scrollTop = nextScrollTop;
        if (m_selectedIndex >= 0 && total > 0) {
            const int clampedSelected = std::clamp(m_selectedIndex, m_scrollTop,
                                                   (std::min)(total - 1, m_scrollTop + availH - 1));
            if (clampedSelected != m_selectedIndex) setCurrentIndex(clampedSelected);
        }
        markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    }
    event->setAccepted(true);
}

} // namespace qmltui
