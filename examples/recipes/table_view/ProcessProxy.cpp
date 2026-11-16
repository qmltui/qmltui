#include "ProcessProxy.h"

int ProcessTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : int(m_processes.size());
}

int ProcessTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : 4;
}

QVariant ProcessTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount() || index.column() < 0 ||
        index.column() >= columnCount() || (role != Qt::DisplayRole && role != Qt::UserRole))
        return {};
    const auto& process = m_processes[size_t(index.row())];
    switch (index.column()) {
    case 0:
        return process.pid;
    case 1:
        return QString::fromLatin1(process.name);
    case 2:
        return role == Qt::UserRole
                   ? QVariant(process.cpu)
                   : QVariant(QString::number(process.cpu, 'f', 1) + QLatin1Char('%'));
    case 3:
        return QString::fromLatin1(process.status);
    }
    return {};
}

ProcessProxy::ProcessProxy(QObject* parent) : QSortFilterProxyModel(parent), m_source(this) {
    setSourceModel(&m_source);
    setSortRole(Qt::UserRole);
    setFilterKeyColumn(1);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    sort(2, Qt::DescendingOrder);
}

void ProcessProxy::setFilterText(const QString& text) {
    if (m_filterText == text) return;
    m_filterText = text;
    setFilterFixedString(text);
    emit filterTextChanged();
}

QString ProcessProxy::sortLabel() const {
    const std::array<QString, 4> names{tr("PID"), tr("PROCESS NAME"), tr("CPU%"), tr("STATUS")};
    if (sortColumn() < 0 || sortColumn() >= int(names.size())) return {};
    return tr("Sort: %1 %2")
        .arg(names[size_t(sortColumn())],
             sortOrder() == Qt::AscendingOrder ? tr("ascending") : tr("descending"));
}

void ProcessProxy::headerClicked(int column) {
    if (column < 0 || column >= columnCount()) return;
    sort(column, sortColumn() == column && sortOrder() == Qt::AscendingOrder ? Qt::DescendingOrder
                                                                             : Qt::AscendingOrder);
    emit sortLabelChanged();
}

void ProcessProxy::cycleSort() {
    if (columnCount() <= 0) return;
    if (sortColumn() >= 0 && sortOrder() == Qt::AscendingOrder) {
        sort(sortColumn(), Qt::DescendingOrder);
    } else {
        const int nextCol = (sortColumn() + 1) % columnCount();
        sort(nextCol, Qt::AscendingOrder);
    }
    emit sortLabelChanged();
}

void ProcessProxy::activate(int proxyRow) {
    const QModelIndex source = mapToSource(index(proxyRow, 0));
    if (source.isValid()) emit activatedPid(m_source.data(source, Qt::UserRole).toInt());
}
