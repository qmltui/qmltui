#pragma once

#include "Item.h"
#include <QVariant>
#include <QQmlComponent>
#include <QQmlContext>

namespace qtui {

class Repeater : public Item {
    Q_OBJECT
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

    QVariant m_model;
    QQmlComponent* m_delegate = nullptr;
    QList<Item*> m_createdItems;
    QList<QQmlContext*> m_contexts;
};

} // namespace qtui
