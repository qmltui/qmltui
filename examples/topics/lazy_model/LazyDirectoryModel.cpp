#include "LazyDirectoryModel.h"

LazyDirectoryModel::LazyDirectoryModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_root.isDir = true;
    m_root.name = QStringLiteral("/");
}

LazyDirectoryModel::~LazyDirectoryModel() = default;

LazyDirectoryModel::Node *LazyDirectoryModel::nodeFor(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<Node *>(index.internalPointer())
                           : const_cast<Node *>(&m_root);
}

QModelIndex LazyDirectoryModel::index(int row, int column, const QModelIndex &parent) const
{
    Node *node = nodeFor(parent);
    if (column != 0 || row < 0 || row >= node->children.size()) return {};
    return createIndex(row, 0, node->children.at(row));
}

QModelIndex LazyDirectoryModel::parent(const QModelIndex &child) const
{
    Node *node = nodeFor(child);
    if (!child.isValid() || !node->parent || node->parent == &m_root) return {};
    Node *grand = node->parent->parent;
    return createIndex(int(grand->children.indexOf(node->parent)), 0, node->parent);
}

int LazyDirectoryModel::rowCount(const QModelIndex &parent) const
{
    return int(nodeFor(parent)->children.size());
}

int LazyDirectoryModel::columnCount(const QModelIndex &) const { return 1; }

// A directory reports children before they are loaded so the view shows an
// expander; the rows arrive through fetchMore().
bool LazyDirectoryModel::hasChildren(const QModelIndex &parent) const
{
    Node *node = nodeFor(parent);
    return node->isDir && (!node->loaded || !node->children.isEmpty());
}

QVariant LazyDirectoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return {};
    Node *node = nodeFor(index);
    switch (role) {
    case NameRole: return node->name;
    case SizeRole: return node->isDir ? QStringLiteral("<dir>") : QStringLiteral("%1 KiB").arg(node->size);
    case IsDirRole: return node->isDir;
    default: return {};
    }
}

QHash<int, QByteArray> LazyDirectoryModel::roleNames() const
{
    return {{NameRole, "display"}, {SizeRole, "size"}, {IsDirRole, "isDir"}};
}

bool LazyDirectoryModel::canFetchMore(const QModelIndex &parent) const
{
    Node *node = nodeFor(parent);
    return node->isDir && !node->loaded;
}

void LazyDirectoryModel::fetchMore(const QModelIndex &parent)
{
    Node *node = nodeFor(parent);
    if (!canFetchMore(parent)) return;
    node->loaded = true;
    ++m_fetchCount;

    // Three sub-directories and four files per directory, four levels deep.
    int depth = 0;
    for (Node *up = node; up != &m_root; up = up->parent) ++depth;
    QList<Node *> fresh;
    if (depth < 4) {
        for (int d = 0; d < 3; ++d) {
            auto *dir = new Node;
            dir->name = QStringLiteral("dir-%1").arg(d);
            dir->isDir = true;
            dir->parent = node;
            fresh.append(dir);
        }
    }
    for (int f = 0; f < 4; ++f) {
        auto *file = new Node;
        file->name = QStringLiteral("file-%1.txt").arg(f);
        file->size = 4 + 13 * (f + depth);
        file->parent = node;
        fresh.append(file);
    }
    if (fresh.isEmpty()) return;

    beginInsertRows(parent, 0, int(fresh.size()) - 1);
    node->children = fresh;
    m_loadedCount += int(fresh.size());
    endInsertRows();
    emit fetchCountChanged();
}
