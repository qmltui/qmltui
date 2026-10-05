#pragma once

#include "ScrollView.h"
#include <QVariant>
#include <QQmlComponent>
#include <QHash>
#include <QPointer>
#include <QMetaObject>

class QAbstractItemModel;

namespace qtui {

class GridView : public ScrollView {
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "delegate")
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(int cellWidth READ cellWidth WRITE setCellWidth NOTIFY cellWidthChanged)
    Q_PROPERTY(int cellHeight READ cellHeight WRITE setCellHeight NOTIFY cellHeightChanged)

public:
    GridView(QObject* parent = nullptr);
    ~GridView() override;

    QVariant model() const { return m_model; }
    void setModel(const QVariant& model);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent* delegate);

    int cellWidth() const { return m_cellWidth; }
    void setCellWidth(int width);

    int cellHeight() const { return m_cellHeight; }
    void setCellHeight(int height);

    void layoutChildren();
    void paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;
    int implicitWidth() const override;
    int implicitHeight() const override;

signals:
    void modelChanged();
    void delegateChanged();
    void cellWidthChanged();
    void cellHeightChanged();

private:
    void disconnectModelSignals();
    void clearItems();
    void generateItems();
    void insertModelRows(QAbstractItemModel *model, int first, int last,
                         const QHash<int, QString> &roleMap);
    void removeModelRows(int first, int last);
    void moveModelRows(int first, int last, int destination);

    QVariant m_model;
    QPointer<QObject> m_objectModel;
    QPointer<QQmlComponent> m_delegate;
    int m_cellWidth = 0;
    int m_cellHeight = 0;
    QList<Item*> m_createdItems;
    QList<QMetaObject::Connection> m_modelConnections;
    quint64 m_modelGeneration = 0;
};

} // namespace qtui
