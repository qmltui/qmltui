#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/Span.h>
#include <QVariantMap>
#include <QVariantList>
#include <QSet>
#include <QHash>
#include <QString>
#include <QJSValue>
#include <QQmlComponent>
#include <QAbstractItemModel>
#include <QPointer>
#include <QPersistentModelIndex>
#include <memory>

namespace qmltui {

// Hierarchical tree view with virtualized row delegates.
//
// Supports QAbstractItemModel (from rootIndex) and nested QVariantMap / JS
// object trees (`rootNode`), tracking expanded state by persistent model index
// or "/" path and drawing Unicode branch connectors when no custom delegate is
// set.
class QMLTUI_CORE_EXPORT TreeView : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "delegate")

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    // Legacy map-tree root (`{ text, children: [...] }`); prefer `model`.
    Q_PROPERTY(QVariant rootNode READ rootNode WRITE setRootNode NOTIFY rootNodeChanged)
    // Parent QModelIndex whose children form the top level of the tree.
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    // Optional JS callback `(row, value) => Span | list<Span> | { text, spans, color, ... }`.
    Q_PROPERTY(QJSValue nodeFormat READ nodeFormat WRITE setNodeFormat NOTIFY nodeFormatChanged)

    // Index in the flattened visible-row list (-1 for no selection);
    // selectedIndex and currentRow are aliases.
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(
        int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(int currentRow READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int currentColumn READ currentColumn NOTIFY currentIndexChanged)
    Q_PROPERTY(QVariantMap currentItemData READ currentItemData NOTIFY currentItemDataChanged)

    Q_PROPERTY(qmltui::Color selectionColor READ selectionColor WRITE setSelectionColor NOTIFY
                   selectionColorChanged)
    Q_PROPERTY(qmltui::Color selectionBackgroundColor READ selectionBackgroundColor WRITE
                   setSelectionBackgroundColor NOTIFY selectionBackgroundColorChanged)
    Q_PROPERTY(qmltui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(
        qmltui::Color branchColor READ branchColor WRITE setBranchColor NOTIFY branchColorChanged)
    Q_PROPERTY(
        QString expandedIcon READ expandedIcon WRITE setExpandedIcon NOTIFY expandedIconChanged)
    Q_PROPERTY(
        QString collapsedIcon READ collapsedIcon WRITE setCollapsedIcon NOTIFY collapsedIconChanged)

public:
    explicit TreeView(QObject* parent = nullptr);
    ~TreeView() override;

    QVariant model() const { return m_modelVariant; }
    void setModel(const QVariant& model);

    QVariant rootNode() const { return m_rootNodeVariant; }
    void setRootNode(const QVariant& node);

    QModelIndex rootIndex() const { return m_rootIndex; }
    void setRootIndex(const QModelIndex& index);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent* delegate);

    QJSValue nodeFormat() const { return m_nodeFormat; }
    void setNodeFormat(const QJSValue& format);

    int currentIndex() const { return m_selectedIndex; }
    void setCurrentIndex(int index);

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index) { setCurrentIndex(index); }
    int currentColumn() const { return m_selectedIndex >= 0 ? 0 : -1; }
    QVariantMap currentItemData() const;

    Color selectionColor() const { return m_selectionColor; }
    void setSelectionColor(Color color);

    Color selectionBackgroundColor() const { return m_selectionBackgroundColor; }
    void setSelectionBackgroundColor(Color color);

    Color textColor() const { return m_textColor; }
    void setTextColor(Color color);

    Color branchColor() const { return m_branchColor; }
    void setBranchColor(Color color);

    QString expandedIcon() const { return m_expandedIcon; }
    void setExpandedIcon(const QString& icon);

    QString collapsedIcon() const { return m_collapsedIcon; }
    void setCollapsedIcon(const QString& icon);

    // Q_INVOKABLE Qt Quick TreeView Methods
    Q_INVOKABLE void expand(int index);
    Q_INVOKABLE void collapse(int index);
    Q_INVOKABLE void toggleExpanded(int index);
    Q_INVOKABLE bool isExpanded(int index) const;
    Q_INVOKABLE int depth(int row) const;
    Q_INVOKABLE void expandRecursively(int row = -1, int depth = -1);
    Q_INVOKABLE void collapseRecursively(int row = -1);
    Q_INVOKABLE void expandToIndex(const QModelIndex& index);
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();

    void layoutChildren();
    void paint(TextGridBuffer& buffer, int absX, int absY,
               const ClipRect& clipRect = ClipRect()) const override;
    void keyPressEvent(KeyEvent* event) override;
    void mousePressEvent(MouseEvent* event) override;
    void wheelEvent(MouseEvent* event) override;

signals:
    void modelChanged();
    void rootNodeChanged();
    void rootIndexChanged();
    void delegateChanged();
    void nodeFormatChanged();
    void currentIndexChanged();
    void selectedIndexChanged();
    void currentItemDataChanged();
    void selectionColorChanged();
    void selectionBackgroundColorChanged();
    void textColorChanged();
    void branchColorChanged();
    void expandedIconChanged();
    void collapsedIconChanged();
    void nodeActivated(const QString& path);
    void itemActivated(int index);
    void expanded(int row, int depth);
    void collapsed(int row, bool recursively);

protected:
    // Flattens the tree, follows the selection, and creates and positions
    // delegate items. Runs in the layout pass, never from paint().
    void updateLayout() override;

private:
    ClipRect clipRectForChildren(int absX, int absY, const ClipRect& clipRect) const override;
    struct NodeFormatCache {
        int formattedAtRow = -1;
        QString formattedText;
        QList<Span> formattedSpans;
        Color formatFg = Color::Invalid;
        Color formatBg = Color::Invalid;
        TextStyle formatStyle;
        bool spansOverrideSelection = false;
    };

    struct NodePayload {
        QString text;
        QList<Span> spans;
        QString path;
        QString key;
        QVariantMap rawData;
        Color roleFg = Color::Invalid;
        Color roleBg = Color::Invalid;
        QString rolePrefix;
        std::shared_ptr<NodeFormatCache> format;
    };

    // One row of the flattened tree. For C++ models only the 48-byte shape
    // (modelIndex, depth, flags) is filled while flattening; hasChildren and
    // the NodePayload (text, path, roles) are allocated on first use by
    // ensureHasChildren() / ensureLoaded(), so a 100k-row model costs no role
    // or payload allocations until rows become visible.
    struct VisibleNode {
        QModelIndex modelIndex;
        std::shared_ptr<NodePayload> d;
        int depth = 0;
        bool hasChildren = false;
        bool hasChildrenChecked = true;
        bool expanded = false;
        bool loaded = true;
        bool roleEnabled = true;

        NodePayload& ensurePayload() {
            if (!d) d = std::make_shared<NodePayload>();
            return *d;
        }
        const NodePayload& payload() const {
            static const NodePayload empty;
            return d ? *d : empty;
        }
    };

    void flattenTree(const QVariantMap& map, const QString& parentPath, const QString& parentKey,
                     int depth, QList<VisibleNode>& list);
    void flattenCppModel(QAbstractItemModel* model, const QModelIndex& parent, int depth,
                         QList<VisibleNode>& list);
    void fillNodeFromModel(QAbstractItemModel* model, const QModelIndex& index, VisibleNode& node,
                           bool includeUnsetRoles = false);
    static QString modelPath(const QAbstractItemModel* model, const QModelIndex& index,
                             const QString& leafName);
    void ensureHasChildren(VisibleNode& node);
    void ensureLoaded(VisibleNode& node);
    void ensureFormatted(VisibleNode& node, int row);
    int visibleRowForIndex(const QModelIndex& index) const;
    static bool sameNode(const VisibleNode& a, const VisibleNode& b);
    Color resolveCachedColor(const QString& name) const;
    void callbackWarning(const QString& message, QSet<QString>& errors) const;
    // Writes only index, row, current and selected to a delegate that still
    // shows the same, unchanged node after a re-flatten (the scroll path).
    void updateDelegatePosition(Item* item, int index);
    // Property indexes of index/row/current/selected per delegate meta-object,
    // so the scroll path does not resolve property names on every write.
    struct PositionProperties {
        int index = -1, row = -1, current = -1, selected = -1;
    };
    const PositionProperties& positionProperties(const QObject* item);
    QHash<const QMetaObject*, PositionProperties> m_positionProperties;
    // True when rows under `parent` are part of the flattened list (the root
    // level, or a visible node), so a structural change there is observable.
    bool isLevelShown(const QModelIndex& parent) const;
    void setNodeExpanded(const VisibleNode& node, bool expanded);
    void handleModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    Item* delegateItemForRow(int row) const;
    void adjustScrollTop();
    void updateTree();

    void clearDelegateItems();
    void generateDelegateItems(QList<Item*> oldItems = {},
                               QList<QPersistentModelIndex> oldIndexes = {},
                               QStringList oldPaths = {}, QList<VisibleNode> oldNodes = {},
                               int oldStartIndex = -1, bool treeRebuilt = true);
    void updateDelegateItem(Item* item, int index);

    QVariant m_modelVariant;
    QVariant m_rootNodeVariant;
    QPointer<QAbstractItemModel> m_cppModel;
    QPersistentModelIndex m_rootIndex;
    QVariantMap m_rootNodeMap;

    QPointer<QQmlComponent> m_delegate;
    QJSValue m_nodeFormat;
    mutable QHash<QString, uint32_t> m_colorCache;
    mutable QSet<QString> m_formatErrors;
    QList<Item*> m_delegateItems;
    QList<QPersistentModelIndex> m_delegateModelIndexes;
    QStringList m_delegatePaths;
    QList<VisibleNode> m_delegateNodes;
    int m_delegateStartIndex = -1;
    int m_delegateCapacity = 0;

    // JS map models are keyed by path; C++ models by persistent index, which
    // survives renames, inserts above, sorting, and proxies.
    QSet<QString> m_expandedPaths;
    QSet<QString> m_collapsedPaths;
    QSet<QPersistentModelIndex> m_expandedIndexes;
    // Plain-index snapshot of m_expandedIndexes for the current flatten pass.
    QSet<QModelIndex> m_expandedSnapshot;
    QList<VisibleNode> m_visibleNodes;
    mutable QHash<QModelIndex, int> m_visibleLookup;
    // The current node, tracked persistently so selection follows inserts
    // and moves across a re-flatten.
    QPersistentModelIndex m_currentModelIndex;
    bool m_treeDirty = true;
    bool m_fetching = false;
    int m_selectedIndex = -1;
    int m_pendingCurrentIndex = -1;
    int m_scrollTop = 0;

    Color m_selectionColor = Color::Invalid;
    uint32_t m_resolvedSelectionColor = Color::resolve(m_selectionColor, 0);
    Color m_selectionBackgroundColor = Color::Invalid;
    uint32_t m_resolvedSelectionBackgroundColor = Color::resolve(m_selectionBackgroundColor, 0);

    Color m_textColor;
    uint32_t m_resolvedTextColor = Color::resolve(m_textColor, 0);

    Color m_branchColor;
    uint32_t m_resolvedBranchColor = Color::resolve(m_branchColor, 0);

    QString m_expandedIcon = "[-] ";
    QString m_collapsedIcon = "[+] ";

    QHash<int, QString> m_roleNameStrings;
    quint16 m_claimedNames = 0;
    QHash<QString, int> m_delegateProperties;
    quint16 m_delegateStandardMask = 0xFFu;
    bool m_delegateSchemaResolved = false;
    bool m_delegateNeedsModelMap = true;
};

} // namespace qmltui
