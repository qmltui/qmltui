#pragma once

#include <QtuiExport.h>

#include "Item.h"
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
#include "../primitives/ColorMapper.h"

namespace qtui {

class QTUI_CORE_EXPORT TreeView : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "delegate")

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QVariant rootNode READ rootNode WRITE setRootNode NOTIFY rootNodeChanged)
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(int currentRow READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int currentColumn READ currentColumn NOTIFY currentIndexChanged)
    Q_PROPERTY(QVariantMap currentItemData READ currentItemData NOTIFY currentItemDataChanged)

    Q_PROPERTY(qtui::Color selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(qtui::Color selectionBackgroundColor READ selectionBackgroundColor WRITE setSelectionBackgroundColor NOTIFY selectionBackgroundColorChanged)
    Q_PROPERTY(qtui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(qtui::Color branchColor READ branchColor WRITE setBranchColor NOTIFY branchColorChanged)
    Q_PROPERTY(QString expandedIcon READ expandedIcon WRITE setExpandedIcon NOTIFY expandedIconChanged)
    Q_PROPERTY(QString collapsedIcon READ collapsedIcon WRITE setCollapsedIcon NOTIFY collapsedIconChanged)

public:
    explicit TreeView(QObject *parent = nullptr);
    ~TreeView() override;

    QVariant model() const { return m_modelVariant; }
    void setModel(const QVariant &model);

    QVariant rootNode() const { return m_rootNodeVariant; }
    void setRootNode(const QVariant &node);

    QModelIndex rootIndex() const { return m_rootIndex; }
    void setRootIndex(const QModelIndex &index);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent *delegate);

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
    void setExpandedIcon(const QString &icon);

    QString collapsedIcon() const { return m_collapsedIcon; }
    void setCollapsedIcon(const QString &icon);


    // Q_INVOKABLE Qt Quick TreeView Methods
    Q_INVOKABLE void expand(int index);
    Q_INVOKABLE void collapse(int index);
    Q_INVOKABLE void toggleExpanded(int index);
    Q_INVOKABLE bool isExpanded(int index) const;
    Q_INVOKABLE int depth(int row) const;
    Q_INVOKABLE void expandRecursively(int row = -1, int depth = -1);
    Q_INVOKABLE void collapseRecursively(int row = -1);
    Q_INVOKABLE void expandToIndex(const QModelIndex &index);
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();

    void layoutChildren();
    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;
    void keyPressEvent(KeyEvent *event) override;
    void mousePressEvent(MouseEvent *event) override;

signals:
    void modelChanged();
    void rootNodeChanged();
    void rootIndexChanged();
    void delegateChanged();
    void currentIndexChanged();
    void selectedIndexChanged();
    void currentItemDataChanged();
    void selectionColorChanged();
    void selectionBackgroundColorChanged();
    void textColorChanged();
    void branchColorChanged();
    void expandedIconChanged();
    void collapsedIconChanged();
    void nodeActivated(const QString &path);
    void itemActivated(int index);
    void expanded(int row, int depth);
    void collapsed(int row, bool recursively);

protected:
    // Flattens the tree, follows the selection, and creates and positions
    // delegate items. Runs in the layout pass, never from paint().
    void updateLayout() override;

private:
    ClipRect clipRectForChildren(int absX, int absY,
                                const ClipRect &clipRect) const override;
    struct VisibleNode {
        QString text;
        QString path;
        QString key;
        int depth = 0;
        bool hasChildren = false;
        bool expanded = false;
        QVariantMap rawData;
        QPersistentModelIndex modelIndex;
    };

    void flattenTree(const QVariantMap &map, const QString &parentPath, const QString &parentKey, int depth, QList<VisibleNode> &list);
    void flattenCppModel(QAbstractItemModel *model, const QModelIndex &parent, const QString &parentPath, const QString &parentKey, int depth, QList<VisibleNode> &list);
    static void fillNodeFromModel(QAbstractItemModel *model, const QModelIndex &index, VisibleNode &node);
    // True when rows under `parent` are part of the flattened list (the root
    // level, or a visible node), so a structural change there is observable.
    bool isLevelShown(const QModelIndex &parent) const;
    void setNodeExpanded(const VisibleNode &node, bool expanded);
    void handleModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    Item *delegateItemForRow(int row) const;
    void adjustScrollTop();
    void updateTree();

    void clearDelegateItems();
    void generateDelegateItems(QList<Item*> oldItems = {},
                               QList<QPersistentModelIndex> oldIndexes = {},
                               QStringList oldPaths = {});
    void updateDelegateItem(Item *item, int index);

    QVariant m_modelVariant;
    QVariant m_rootNodeVariant;
    QPointer<QAbstractItemModel> m_cppModel;
    QPersistentModelIndex m_rootIndex;
    QVariantMap m_rootNodeMap;

    QPointer<QQmlComponent> m_delegate;
    QList<Item*> m_delegateItems;
    QList<QPersistentModelIndex> m_delegateModelIndexes;
    QStringList m_delegatePaths;
    int m_delegateStartIndex = -1;
    int m_delegateCapacity = 0;

    // JS map models are keyed by path; C++ models by persistent index, which
    // survives renames, inserts above, sorting, and proxies.
    QSet<QString> m_expandedPaths;
    QSet<QString> m_collapsedPaths;
    QSet<QPersistentModelIndex> m_expandedIndexes;
    QList<VisibleNode> m_visibleNodes;
    QHash<QPersistentModelIndex, int> m_visibleLookup;
    bool m_treeDirty = true;
    bool m_fetching = false;
    int m_selectedIndex = -1;
    int m_pendingCurrentIndex = -1;
    int m_scrollTop = 0;

    Color m_selectionColor;
    // Resolved caches mirror the Color members: an ANSI-named default must not be
    // written as literal RGB, or the terminal theme stops applying to it (setters
    // early-return when the color is unchanged, so the initializer is the only chance).
    uint32_t m_resolvedSelectionColor = ColorMapper::fromString(m_selectionColor, 0);
    Color m_selectionBackgroundColor;
    uint32_t m_resolvedSelectionBackgroundColor = ColorMapper::fromString(m_selectionBackgroundColor, 0);

    Color m_textColor = "default";
    uint32_t m_resolvedTextColor = ColorMapper::fromString(m_textColor, 0);

    Color m_branchColor = "default";
    uint32_t m_resolvedBranchColor = ColorMapper::fromString(m_branchColor, 0);

    QString m_expandedIcon = "[-] ";
    QString m_collapsedIcon = "[+] ";


};

} // namespace qtui
