#pragma once

#include "Item.h"
#include <QVariant>
#include <QAbstractItemModel>
#include <QQmlComponent>
#include <QQmlContext>

class QAbstractItemModel;
class QQmlPropertyMap;

namespace qtui {

class Repeater : public Item {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Repeater)
    QML_ELEMENT
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)

public:
    Repeater(QObject* parent = nullptr);
    ~Repeater() override;

    QVariant model() const { return m_model; }
    void setModel(const QVariant& model);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent* delegate);

signals:
    void modelChanged();
    void delegateChanged();

private:
    void clearItems();
    void generateItems();
    void onRowsInserted(QAbstractItemModel* model, int first, int last);
    void onRowsRemoved(int first, int last);

    QVariant m_model;
    QQmlComponent* m_delegate = nullptr;
    QList<Item*> m_createdItems;
    QList<QQmlContext*> m_contexts;
    QList<QQmlPropertyMap*> m_rowMaps;
};

} // namespace qtui
