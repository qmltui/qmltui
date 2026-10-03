#include "FleetListModel.h"
#include <QRandomGenerator>
#include <algorithm>

FleetListModel::FleetListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_allNodes = {
        { "node-01", "us-east-web-01", "RUNNING",  6, 4, "10.0.1.12", "<Nginx> <SSL> <Edge>",      {3, 4, 5, 6, 5, 4, 6, 7, 6, 5, 6, 7, 8, 6, 5, 6} },
        { "node-02", "us-east-db-01",  "CRITICAL", 9, 9, "10.0.1.45", "<PostgreSQL> <Primary>",  {8, 9, 9, 10, 9, 8, 9, 9, 10, 9, 9, 10, 9, 9, 9, 9} },
        { "node-03", "eu-cent-cache",  "RUNNING",  3, 5, "10.0.2.88", "<Redis> <Cluster>",       {2, 3, 2, 4, 3, 2, 3, 4, 3, 2, 3, 2, 3, 4, 3, 3} },
        { "node-04", "ap-south-worker","STANDBY",  1, 2, "10.0.3.19", "<Kafka> <Consumer>",      {1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1} },
        { "node-05", "us-west-gpu-01", "RUNNING",  8, 7, "10.0.4.77", "<PyTorch> <v100>",        {7, 8, 8, 9, 8, 7, 8, 9, 8, 8, 9, 8, 7, 8, 8, 8} },
        { "node-06", "sa-east-auth",   "CRITICAL", 10,8, "10.0.5.31", "<OAuth2> <Vault>",        {9, 10, 10, 10, 9, 10, 10, 9, 10, 10, 9, 10, 10, 10, 10, 10} },
        { "node-07", "eu-west-proxy",  "RUNNING",  4, 3, "10.0.6.14", "<Envoy> <Mesh>",          {3, 4, 3, 5, 4, 3, 4, 5, 4, 3, 4, 3, 4, 5, 4, 4} }
    };

    rebuildVisibleNodes();

    connect(&m_timer, &QTimer::timeout, this, &FleetListModel::onTelemetryTimer);
    m_timer.start(2000);
}

int FleetListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_visibleNodes.size();
}

QVariant FleetListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleNodes.size())
        return QVariant();

    const FleetNode &node = m_visibleNodes.at(index.row());
    switch (role) {
        case IdRole: return node.id;
        case NameRole: return node.name;
        case StatusRole: return node.status;
        case CpuRole: return node.cpu;
        case MemRole: return node.mem;
        case IpRole: return node.ip;
        case TagsRole: return node.tags;
        default: return QVariant();
    }
}

QHash<int, QByteArray> FleetListModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "nodeId";
    roles[NameRole] = "name";
    roles[StatusRole] = "status";
    roles[CpuRole] = "cpu";
    roles[MemRole] = "mem";
    roles[IpRole] = "ip";
    roles[TagsRole] = "tags";
    return roles;
}

void FleetListModel::setActiveFilter(int filter) {
    if (m_activeFilter == filter) return;
    m_activeFilter = filter;
    emit activeFilterChanged();
    
    beginResetModel();
    rebuildVisibleNodes();
    endResetModel();
}

void FleetListModel::setSearchQuery(const QString &query) {
    if (m_searchQuery == query) return;
    m_searchQuery = query;
    emit searchQueryChanged();

    beginResetModel();
    rebuildVisibleNodes();
    endResetModel();
}

void FleetListModel::setSortMode(int mode) {
    if (m_sortMode == mode) return;
    m_sortMode = mode;
    emit sortModeChanged();

    beginResetModel();
    rebuildVisibleNodes();
    endResetModel();
}

void FleetListModel::cycleSortMode() {
    setSortMode((m_sortMode + 1) % 4);
}

int FleetListModel::runningNodes() const {
    int count = 0;
    for (const auto &node : m_allNodes) {
        if (node.status == "RUNNING") count++;
    }
    return count;
}

int FleetListModel::alertNodes() const {
    int count = 0;
    for (const auto &node : m_allNodes) {
        if (node.status == "CRITICAL") count++;
    }
    return count;
}

void FleetListModel::rebuildVisibleNodes() {
    m_visibleNodes.clear();
    for (const auto &node : m_allNodes) {
        if (m_activeFilter == 1 && node.status != "RUNNING") continue;
        if (m_activeFilter == 2 && node.status != "CRITICAL") continue;
        if (!m_searchQuery.trimmed().isEmpty()) {
            QString q = m_searchQuery.trimmed();
            bool matches = node.name.contains(q, Qt::CaseInsensitive) ||
                           node.ip.contains(q, Qt::CaseInsensitive) ||
                           node.tags.contains(q, Qt::CaseInsensitive) ||
                           node.id.contains(q, Qt::CaseInsensitive);
            if (!matches) continue;
        }
        m_visibleNodes.append(node);
    }
    applySort();
    emit modelChanged();
}

void FleetListModel::applySort() {
    if (m_sortMode == 1) { // CPU High -> Low
        std::stable_sort(m_visibleNodes.begin(), m_visibleNodes.end(),
            [](const FleetNode &a, const FleetNode &b) { return a.cpu > b.cpu; });
    } else if (m_sortMode == 2) { // Mem High -> Low
        std::stable_sort(m_visibleNodes.begin(), m_visibleNodes.end(),
            [](const FleetNode &a, const FleetNode &b) { return a.mem > b.mem; });
    } else if (m_sortMode == 3) { // Status (CRITICAL first)
        std::stable_sort(m_visibleNodes.begin(), m_visibleNodes.end(),
            [](const FleetNode &a, const FleetNode &b) { return a.status > b.status; });
    } else { // Default by Name
        std::stable_sort(m_visibleNodes.begin(), m_visibleNodes.end(),
            [](const FleetNode &a, const FleetNode &b) { return a.name < b.name; });
    }
}

void FleetListModel::toggleNodeStatus(int visibleIndex) {
    if (visibleIndex < 0 || visibleIndex >= m_visibleNodes.size()) return;

    QString targetId = m_visibleNodes.at(visibleIndex).id;
    for (int i = 0; i < m_allNodes.size(); ++i) {
        if (m_allNodes[i].id == targetId) {
            if (m_allNodes[i].status == "CRITICAL" || m_allNodes[i].status == "STANDBY") {
                m_allNodes[i].status = "RUNNING";
                m_allNodes[i].cpu = 4;
            } else {
                m_allNodes[i].status = "CRITICAL";
                m_allNodes[i].cpu = 9;
            }
            break;
        }
    }

    beginResetModel();
    rebuildVisibleNodes();
    endResetModel();
    emit statsChanged();
}

void FleetListModel::restartAllCritical() {
    beginResetModel();
    for (auto &node : m_allNodes) {
        if (node.status == "CRITICAL") {
            node.status = "RUNNING";
            node.cpu = 3;
            node.mem = 4;
        }
    }
    rebuildVisibleNodes();
    endResetModel();
    emit statsChanged();
}

QVariantMap FleetListModel::getNodeAt(int visibleIndex) const {
    QVariantMap res;
    if (visibleIndex >= 0 && visibleIndex < m_visibleNodes.size()) {
        const auto &n = m_visibleNodes.at(visibleIndex);
        res["id"] = n.id;
        res["name"] = n.name;
        res["status"] = n.status;
        res["cpu"] = n.cpu;
        res["mem"] = n.mem;
        res["ip"] = n.ip;
        res["tags"] = n.tags;
    }
    return res;
}

QString FleetListModel::getNodeName(int visibleIndex) const {
    if (visibleIndex >= 0 && visibleIndex < m_visibleNodes.size())
        return m_visibleNodes.at(visibleIndex).name;
    return QString();
}

QString FleetListModel::getNodeId(int visibleIndex) const {
    if (visibleIndex >= 0 && visibleIndex < m_visibleNodes.size())
        return m_visibleNodes.at(visibleIndex).id;
    return QString();
}

QString FleetListModel::getNodeStatus(int visibleIndex) const {
    if (visibleIndex >= 0 && visibleIndex < m_visibleNodes.size())
        return m_visibleNodes.at(visibleIndex).status;
    return QString();
}

QString FleetListModel::getNodeIp(int visibleIndex) const {
    if (visibleIndex >= 0 && visibleIndex < m_visibleNodes.size())
        return m_visibleNodes.at(visibleIndex).ip;
    return QString();
}

int FleetListModel::getNodeCpu(int visibleIndex) const {
    if (visibleIndex >= 0 && visibleIndex < m_visibleNodes.size())
        return m_visibleNodes.at(visibleIndex).cpu;
    return 0;
}

int FleetListModel::getNodeMem(int visibleIndex) const {
    if (visibleIndex >= 0 && visibleIndex < m_visibleNodes.size())
        return m_visibleNodes.at(visibleIndex).mem;
    return 0;
}

QString FleetListModel::getSparkline(int visibleIndex) const {
    if (visibleIndex < 0 || visibleIndex >= m_visibleNodes.size())
        return QString();
    const auto &n = m_visibleNodes.at(visibleIndex);
    static const char* glyphs[] = {" ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    QString res;
    for (int val : n.cpuHistory) {
        int gIdx = std::clamp((val - 1) * 7 / 9, 0, 7);
        res += QString::fromUtf8(glyphs[gIdx]);
    }
    return res;
}

QStringList FleetListModel::getLogs(int visibleIndex) const {
    QStringList logs;
    if (visibleIndex >= 0 && visibleIndex < m_visibleNodes.size()) {
        const auto &n = m_visibleNodes.at(visibleIndex);
        logs << QString("[INFO] Node %1 (%2) heartbeat sync active").arg(n.name, n.id);
        logs << QString("[METRICS] %1 -> CPU=%20% RAM=%30%").arg(n.ip).arg(n.cpu * 10).arg(n.mem * 10);
        if (n.status == "CRITICAL") {
            logs << QString("[ALERT] Readiness probe failed for %1!").arg(n.id);
            logs << QString("[WARN] OOM Warning: process memory limit near capacity on %1").arg(n.tags);
        } else {
            logs << QString("[OK] Ingress proxy healthy on %1").arg(n.ip);
            logs << QString("[OK] TLS certificate valid for %1").arg(n.name);
        }
    } else {
        logs << "[SYSTEM] Fleet daemon watching 7 nodes across 4 regions";
    }
    return logs;
}

void FleetListModel::onTelemetryTimer() {
    if (m_allNodes.isEmpty()) return;

    int idx = QRandomGenerator::global()->bounded(m_allNodes.size());
    if (m_allNodes[idx].status == "RUNNING") {
        m_allNodes[idx].cpu = QRandomGenerator::global()->bounded(1, 9);
        m_allNodes[idx].mem = QRandomGenerator::global()->bounded(1, 9);
    }
    m_allNodes[idx].cpuHistory.append(m_allNodes[idx].cpu);
    if (m_allNodes[idx].cpuHistory.size() > 16)
        m_allNodes[idx].cpuHistory.removeFirst();

    int visibleRow = -1;
    QString targetId = m_allNodes[idx].id;
    for (int i = 0; i < m_visibleNodes.size(); ++i) {
        if (m_visibleNodes[i].id == targetId) {
            m_visibleNodes[i].cpu = m_allNodes[idx].cpu;
            m_visibleNodes[i].mem = m_allNodes[idx].mem;
            m_visibleNodes[i].cpuHistory = m_allNodes[idx].cpuHistory;
            visibleRow = i;
            break;
        }
    }

    if (visibleRow >= 0) {
        QModelIndex modelIdx = createIndex(visibleRow, 0);
        emit dataChanged(modelIdx, modelIdx, { CpuRole, MemRole });
    }
    emit statsChanged();
    emit modelChanged();
}

QVariantList FleetListModel::tableData() const {
    QVariantList list;
    for (const auto &n : m_visibleNodes) {
        QVariantList row;
        row << n.name
            << n.ip
            << n.status
            << QString("[%1] %2%").arg(QString("█").repeated(n.cpu) + QString("░").repeated(10 - n.cpu)).arg(n.cpu * 10)
            << QString("[%1] %2%").arg(QString("█").repeated(n.mem) + QString("░").repeated(10 - n.mem)).arg(n.mem * 10)
            << n.tags;
        list << QVariant(row);
    }
    return list;
}
