#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QQmlEngine>

struct FleetNode {
    QString id;
    QString name;
    QString status; // "RUNNING", "CRITICAL", "STANDBY"
    int cpu;        // 1 to 10
    int mem;        // 1 to 10
    QString ip;
    QString tags;
    QList<int> cpuHistory;
};

class FleetListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int activeFilter READ activeFilter WRITE setActiveFilter NOTIFY activeFilterChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(int totalNodes READ totalNodes NOTIFY statsChanged)
    Q_PROPERTY(int runningNodes READ runningNodes NOTIFY statsChanged)
    Q_PROPERTY(int alertNodes READ alertNodes NOTIFY statsChanged)
    Q_PROPERTY(QVariantList tableData READ tableData NOTIFY modelChanged)

public:
    enum NodeRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        StatusRole,
        CpuRole,
        MemRole,
        IpRole,
        TagsRole
    };

    explicit FleetListModel(QObject *parent = nullptr);
    ~FleetListModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int activeFilter() const { return m_activeFilter; }
    void setActiveFilter(int filter);

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &query);

    int sortMode() const { return m_sortMode; }
    void setSortMode(int mode);

    int totalNodes() const { return m_allNodes.size(); }
    int runningNodes() const;
    int alertNodes() const;
    QVariantList tableData() const;

    Q_INVOKABLE void toggleNodeStatus(int visibleIndex);
    Q_INVOKABLE void restartAllCritical();
    Q_INVOKABLE void cycleSortMode();
    Q_INVOKABLE QVariantMap getNodeAt(int visibleIndex) const;
    Q_INVOKABLE QString getNodeName(int visibleIndex) const;
    Q_INVOKABLE QString getNodeId(int visibleIndex) const;
    Q_INVOKABLE QString getNodeStatus(int visibleIndex) const;
    Q_INVOKABLE QString getNodeIp(int visibleIndex) const;
    Q_INVOKABLE int getNodeCpu(int visibleIndex) const;
    Q_INVOKABLE int getNodeMem(int visibleIndex) const;
    Q_INVOKABLE QString getSparkline(int visibleIndex) const;
    Q_INVOKABLE QStringList getLogs(int visibleIndex) const;

signals:
    void activeFilterChanged();
    void searchQueryChanged();
    void sortModeChanged();
    void statsChanged();
    void modelChanged();

private slots:
    void onTelemetryTimer();

private:
    void rebuildVisibleNodes();
    void applySort();

    QList<FleetNode> m_allNodes;
    QList<FleetNode> m_visibleNodes;
    int m_activeFilter = 0; // 0 = ALL, 1 = RUNNING, 2 = CRITICAL
    QString m_searchQuery;
    int m_sortMode = 0;     // 0 = Name, 1 = CPU, 2 = Mem, 3 = Status
    QTimer m_timer;
};
