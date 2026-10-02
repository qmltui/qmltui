#pragma once

#include <Item.h>
#include <QVariantMap>
#include <QVariantList>
#include <QSet>
#include <QString>
#include <QJSValue>
#include <QQmlComponent>
#include <QAbstractItemModel>
#include <QPointer>
#include <QPersistentModelIndex>

namespace qtui {

class TreeView : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QVariant rootNode READ rootNode WRITE setRootNode NOTIFY rootNodeChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QVariantMap currentItemData READ currentItemData NOTIFY currentIndexChanged)

    Q_PROPERTY(QString selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(QString selectionBgColor READ selectionBgColor WRITE setSelectionBgColor NOTIFY selectionBgColorChanged)
    Q_PROPERTY(QString textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(QString branchColor READ branchColor WRITE setBranchColor NOTIFY branchColorChanged)
    Q_PROPERTY(QString expandedIcon READ expandedIcon WRITE setExpandedIcon NOTIFY expandedIconChanged)
    Q_PROPERTY(QString collapsedIcon READ collapsedIcon WRITE setCollapsedIcon NOTIFY collapsedIconChanged)
    Q_PROPERTY(QString borderStyle READ borderStyle WRITE setBorderStyle NOTIFY borderStyleChanged)
    Q_PROPERTY(QString borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)

public:
    explicit TreeView(QObject *parent = nullptr);
    ~TreeView() override;

    QVariant model() const { return m_modelVariant; }
    void setModel(const QVariant &model);

    QVariant rootNode() const { return m_rootNodeVariant; }
    void setRootNode(const QVariant &node);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent *delegate);

    int currentIndex() const { return m_selectedIndex; }
    void setCurrentIndex(int index);

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index) { setCurrentIndex(index); }
    QVariantMap currentItemData() const;

    QString selectionColor() const { return m_selectionColor; }
    void setSelectionColor(const QString &color);

    QString selectionBgColor() const { return m_selectionBgColor; }
    void setSelectionBgColor(const QString &color);

    QString textColor() const { return m_textColor; }
    void setTextColor(const QString &color);

    QString branchColor() const { return m_branchColor; }
    void setBranchColor(const QString &color);

    QString expandedIcon() const { return m_expandedIcon; }
    void setExpandedIcon(const QString &icon);

    QString collapsedIcon() const { return m_collapsedIcon; }
    void setCollapsedIcon(const QString &icon);

    QString borderStyle() const { return m_borderStyle; }
    void setBorderStyle(const QString &style);

    QString borderColor() const { return m_borderColor; }
    void setBorderColor(const QString &color);

    // Q_INVOKABLE Qt Quick TreeView Methods
    Q_INVOKABLE void expand(int index);
    Q_INVOKABLE void collapse(int index);
    Q_INVOKABLE void toggleExpanded(int index);
    Q_INVOKABLE bool isExpanded(int index) const;
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();

    void layoutChildren();
    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;
    void keyPressEvent(KeyEvent *event) override;
    void mousePressEvent(MouseEvent *event) override;

signals:
    void modelChanged();
    void rootNodeChanged();
    void delegateChanged();
    void currentIndexChanged();
    void selectedIndexChanged();
    void selectionColorChanged();
    void selectionBgColorChanged();
    void textColorChanged();
    void branchColorChanged();
    void expandedIconChanged();
    void collapsedIconChanged();
    void borderStyleChanged();
    void borderColorChanged();
    void nodeActivated(const QString &path);
    void itemActivated(int index);

private:
    struct VisibleNode {
        QString text;
        QString path;
        int depth = 0;
        bool hasChildren = false;
        bool expanded = false;
        QVariantMap rawData;
        QPersistentModelIndex modelIndex;
    };

    void flattenTree(const QVariantMap &map, const QString &parentPath, int depth, QList<VisibleNode> &list);
    void flattenCppModel(QAbstractItemModel *model, const QModelIndex &parent, const QString &parentPath, int depth, QList<VisibleNode> &list);
    void updateTree();

    void clearDelegateItems();
    void generateDelegateItems(QList<Item*> oldItems = {},
                               QList<QPersistentModelIndex> oldIndexes = {},
                               QStringList oldPaths = {});
    void updateDelegateItem(Item *item, int index);

    QVariant m_modelVariant;
    QVariant m_rootNodeVariant;
    QPointer<QAbstractItemModel> m_cppModel;
    QVariantMap m_rootNodeMap;

    QPointer<QQmlComponent> m_delegate;
    QList<Item*> m_delegateItems;
    QList<QPersistentModelIndex> m_delegateModelIndexes;
    QStringList m_delegatePaths;
    int m_delegateStartIndex = -1;
    int m_delegateCapacity = 0;

    QSet<QString> m_expandedPaths;
    QList<VisibleNode> m_visibleNodes;
    bool m_treeDirty = true;
    int m_selectedIndex = 0;
    int m_scrollTop = 0;

    QString m_selectionColor = "white";
    uint32_t m_resolvedSelectionColor = 0xFFFFFFFF;
    QString m_selectionBgColor = "blue";
    uint32_t m_resolvedSelectionBgColor = 0xFF0000FF;

    QString m_textColor = "cyan";
    uint32_t m_resolvedTextColor = 0xFF00FFFF;

    QString m_branchColor = "gray";
    uint32_t m_resolvedBranchColor = 0xFF808080;

    QString m_expandedIcon = "[-] ";
    QString m_collapsedIcon = "[+] ";

    QString m_borderStyle = "none";
    QString m_borderColor = "gray";
    uint32_t m_resolvedBorderColor = 0xFF808080;
};

} // namespace qtui
