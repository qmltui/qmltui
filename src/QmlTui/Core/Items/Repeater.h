#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QVariant>
#include <QAbstractItemModel>
#include <QPersistentModelIndex>
#include <QQmlComponent>
#include <QQmlContext>
#include <QPointer>
#include <QMetaObject>
#include <QHash>

class QQmlPropertyMap;

namespace qmltui {

// Instantiates one `delegate` item per model entry and inserts those items as
// visual children of the Repeater's parent Item (for use inside Row, Column, etc.).
class QMLTUI_CORE_EXPORT Repeater : public Item {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Repeater)
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "delegate")
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)

public:
    explicit Repeater(QObject* parent = nullptr);
    ~Repeater() override;

    QVariant model() const { return m_model; }
    void setModel(const QVariant& model);

    // Parent index whose children the repeater instantiates; invalid means the top level.
    QModelIndex rootIndex() const { return m_rootIndex; }
    void setRootIndex(const QModelIndex& rootIndex);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent* delegate);

signals:
    void modelChanged();
    void rootIndexChanged();
    void delegateChanged();

private:
    void disconnectModelSignals();
    void clearItems();
    void generateItems();
    void instantiateModelRow(QAbstractItemModel* model, int row, Item* parentVis, bool append);
    void onRowsInserted(QAbstractItemModel* model, int first, int last);
    void maybeFetchMore();
    void onRowsRemoved(int first, int last);
    void onRowsMoved(int first, int last, int destination);

    QVariant m_model;
    QPointer<QObject> m_objectModel;
    QPointer<QQmlComponent> m_delegate;
    QList<Item*> m_createdItems;
    QList<QQmlContext*> m_contexts;
    QList<QQmlPropertyMap*> m_rowMaps;
    QList<QMetaObject::Connection> m_modelConnections;
    quint64 m_modelGeneration = 0;
    QPersistentModelIndex m_rootIndex;
    QHash<int, QString> m_roleMap;
    quint16 m_claimedNames = 0;
    bool m_nestedWarned = false;
    bool m_fetching = false;
};

} // namespace qmltui
