#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>

namespace qtui {

class Terminal;

class StreamAppender : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)

public:
    explicit StreamAppender(QObject* parent = nullptr);

    QAbstractItemModel* model() const { return m_model; }
    void setModel(QAbstractItemModel* model);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent* delegate);

signals:
    void modelChanged();
    void delegateChanged();

private:
    int appendRows(int first, int last);
    void appendExistingRows();
    QQmlComponent* delegateForRow(int row) const;
    Terminal* terminal() const;

    QPointer<QAbstractItemModel> m_model;
    QPointer<QQmlComponent> m_delegate;
    int m_consumedRowCount = 0;
};

} // namespace qtui
