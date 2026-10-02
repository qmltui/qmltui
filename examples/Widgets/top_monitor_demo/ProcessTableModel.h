#pragma once

#include <QAbstractTableModel>
#include <QTimer>
#include <QList>
#include <QMap>
#include <QString>
#include <QQmlEngine>

struct ProcessInfo {
    int pid;
    QString user;
    int pr;
    int ni;
    QString virt;
    QString res;
    QString shr;
    double cpu;
    double mem;
    QString time;
    QString command;
    qint64 totalTicks = 0;
};

class ProcessTableModel : public QAbstractTableModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString uptimeText READ uptimeText NOTIFY systemStatsChanged)
    Q_PROPERTY(QString tasksText READ tasksText NOTIFY systemStatsChanged)
    Q_PROPERTY(QString cpuText READ cpuText NOTIFY systemStatsChanged)
    Q_PROPERTY(QString memText READ memText NOTIFY systemStatsChanged)
    Q_PROPERTY(QString swapText READ swapText NOTIFY systemStatsChanged)

public:
    explicit ProcessTableModel(QObject *parent = nullptr);
    ~ProcessTableModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QString uptimeText() const { return m_uptimeText; }
    QString tasksText() const { return m_tasksText; }
    QString cpuText() const { return m_cpuText; }
    QString memText() const { return m_memText; }
    QString swapText() const { return m_swapText; }

    Q_INVOKABLE QVariantList columnDefinitions() const;

signals:
    void systemStatsChanged();

private slots:
    void updateProcessStats();

private:
    void readSystemStats();

    QList<ProcessInfo> m_processes;
    QMap<int, qint64> m_prevProcTicks;
    qint64 m_prevSystemTotalTicks = 0;
    qint64 m_prevSystemIdleTicks = 0;

    QString m_uptimeText;
    QString m_tasksText;
    QString m_cpuText;
    QString m_memText;
    QString m_swapText;

    QTimer m_timer;
};
