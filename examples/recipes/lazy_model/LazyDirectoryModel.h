#pragma once

#include <QAbstractItemModel>
#include <QQmlEngine>
#include <QList>

// A directory tree whose children are produced only when a branch is expanded
// (canFetchMore/fetchMore), the contract QFileSystemModel follows. Nothing is
// read from disk: entries are generated so the example runs anywhere.
class LazyDirectoryModel : public QAbstractItemModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int fetchCount READ fetchCount NOTIFY fetchCountChanged)
    Q_PROPERTY(int loadedCount READ loadedCount NOTIFY fetchCountChanged)

public:
    enum Roles { NameRole = Qt::DisplayRole, SizeRole = Qt::UserRole + 1, IsDirRole };

    explicit LazyDirectoryModel(QObject* parent = nullptr);
    ~LazyDirectoryModel() override;

    int fetchCount() const { return m_fetchCount; }
    int loadedCount() const { return m_loadedCount; }

    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    bool hasChildren(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

signals:
    void fetchCountChanged();

private:
    struct Node {
        QString name;
        bool isDir = false;
        bool loaded = false;
        qint64 size = 0;
        Node* parent = nullptr;
        QList<Node*> children;
        ~Node() { qDeleteAll(children); }
    };
    Node* nodeFor(const QModelIndex& index) const;
    Node m_root;
    int m_fetchCount = 0;
    int m_loadedCount = 0;
};
