#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/ScrollView.h>
#include <QVariant>
#include <QPersistentModelIndex>
#include <QQmlComponent>
#include <QHash>
#include <QMap>
#include <QList>
#include <QPointer>
#include <QMetaObject>

class QAbstractItemModel;

namespace qtui {

class QTUI_CORE_EXPORT GridView : public ScrollView {
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "delegate")
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(int cellWidth READ cellWidth WRITE setCellWidth NOTIFY cellWidthChanged)
    Q_PROPERTY(int cellHeight READ cellHeight WRITE setCellHeight NOTIFY cellHeightChanged)
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
    Q_PROPERTY(bool reuseItems READ reuseItems WRITE setReuseItems NOTIFY reuseItemsChanged)

public:
    GridView(QObject* parent = nullptr);
    ~GridView() override;

    QVariant model() const { return m_model; }
    void setModel(const QVariant& model);

    // Parent index whose children the view shows; invalid means the top level.
    QModelIndex rootIndex() const { return m_rootIndex; }
    void setRootIndex(const QModelIndex &rootIndex);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent* delegate);

    int cellWidth() const { return m_cellWidth; }
    void setCellWidth(int width);

    int cellHeight() const { return m_cellHeight; }
    void setCellHeight(int height);

    // Pool delegates that scroll out and rebind them to rows that scroll in,
    // as ListView does; off recreates them.
    bool reuseItems() const { return m_reuseItems; }
    void setReuseItems(bool reuse);

    void layoutChildren();
    void paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) const override;
    int implicitWidth() const override;
    int implicitHeight() const override;

signals:
    void modelChanged();
    void rootIndexChanged();
    void delegateChanged();
    void cellWidthChanged();
    void cellHeightChanged();
    void reuseItemsChanged();

protected:
    // Positions delegate items in the layout pass, never from paint().
    void updateLayout() override;

private:
    void disconnectModelSignals();
    void clearItems();
    void generateItems();
    // Only the delegates for cells inside the viewport (plus one row of
    // overscan above and below) exist at any time.
    void rebuildViewport(bool force = false);
    void maybeFetchMore();
    void refreshFromModel();
    Item *createDelegateItem(int index);
    void bindDelegateItem(Item *item, int index, bool created);
    void releaseDelegateItem(Item *item);
    void reindexItem(Item *item, int index);
    void updateContentExtent();
    int columnsPerRow() const;
    void insertModelRows(QAbstractItemModel *model, int first, int last);
    void removeModelRows(int first, int last);

    QVariant m_model;
    QPointer<QObject> m_objectModel;
    QPointer<QQmlComponent> m_delegate;
    int m_cellWidth = 0;
    int m_cellHeight = 0;
    QMap<int, Item*> m_createdItems;
    QList<Item*> m_reusePool;
    bool m_reuseItems = true;
    QVariantList m_listValues;
    QHash<int, QString> m_roleMap;
    int m_modelCount = 0;
    int m_materializedFirst = -1;
    int m_materializedLast = -1;
    QList<QMetaObject::Connection> m_modelConnections;
    quint64 m_modelGeneration = 0;
    QPersistentModelIndex m_rootIndex;
    bool m_nestedWarned = false;
    bool m_fetching = false;
};

} // namespace qtui
