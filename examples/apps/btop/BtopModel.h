#pragma once

#include <Item.h>
#include <Color.h>
#include <TextGridBuffer.h>

#include <QAbstractTableModel>
#include <QElapsedTimer>
#include <QHash>
#include <QQmlEngine>
#include <QTimer>
#include <QVariantList>

#include <sys/types.h>

struct BtopProcessRow
{
    int pid = 0;
    QString program;
    QString command;
    int threads = 0;
    QString user;
    quint64 memoryBytes = 0;
    QString memoryText;
    double cpuPercent = 0.0;
    quint64 cpuTicks = 0;
};

class BtopCpuGraph : public qtui::Item
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList values READ values WRITE setValues NOTIFY valuesChanged)
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qtui::Color bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)

public:
    explicit BtopCpuGraph(QObject *parent = nullptr);

    QVariantList values() const { return m_values; }
    void setValues(const QVariantList &values);

    qtui::Color color() const { return m_color; }
    void setColor(qtui::Color color);

    qtui::Color bgColor() const { return m_bgColor; }
    void setBgColor(qtui::Color color);

    void paint(qtui::TextGridBuffer &buffer, int absX, int absY, const qtui::ClipRect &clipRect) override;

signals:
    void valuesChanged();
    void colorChanged();
    void bgColorChanged();

private:
    QVariantList m_values;
    qtui::Color m_color = QStringLiteral("white");
    uint32_t m_resolvedColor = 0xFFFFFFFF;
    qtui::Color m_bgColor;
    uint32_t m_resolvedBgColor = 0x00000000;
};

class BtopModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString hostName READ hostName CONSTANT)
    Q_PROPERTY(QString userName READ userName CONSTANT)
    Q_PROPERTY(QString osName READ osName CONSTANT)
    Q_PROPERTY(QString uptimeText READ uptimeText NOTIFY statsChanged)
    Q_PROPERTY(QString loadAverageText READ loadAverageText NOTIFY statsChanged)
    Q_PROPERTY(QString cpuNameText READ cpuNameText NOTIFY statsChanged)
    Q_PROPERTY(QString cpuFrequencyText READ cpuFrequencyText NOTIFY statsChanged)
    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY statsChanged)
    Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY statsChanged)
    Q_PROPERTY(QVariantList coreUsage READ coreUsage NOTIFY statsChanged)
    Q_PROPERTY(QStringList coreHistoryText READ coreHistoryText NOTIFY statsChanged)
    Q_PROPERTY(double memoryPercent READ memoryPercent NOTIFY statsChanged)
    Q_PROPERTY(double availablePercent READ availablePercent NOTIFY statsChanged)
    Q_PROPERTY(double cachedPercent READ cachedPercent NOTIFY statsChanged)
    Q_PROPERTY(QString memoryUsedText READ memoryUsedText NOTIFY statsChanged)
    Q_PROPERTY(QString memoryTotalText READ memoryTotalText NOTIFY statsChanged)
    Q_PROPERTY(QString memoryAvailableText READ memoryAvailableText NOTIFY statsChanged)
    Q_PROPERTY(QString memoryCachedText READ memoryCachedText NOTIFY statsChanged)
    Q_PROPERTY(double swapPercent READ swapPercent NOTIFY statsChanged)
    Q_PROPERTY(QString swapUsedText READ swapUsedText NOTIFY statsChanged)
    Q_PROPERTY(QString swapTotalText READ swapTotalText NOTIFY statsChanged)
    Q_PROPERTY(double diskPercent READ diskPercent NOTIFY statsChanged)
    Q_PROPERTY(QString diskUsedText READ diskUsedText NOTIFY statsChanged)
    Q_PROPERTY(QString diskTotalText READ diskTotalText NOTIFY statsChanged)
    Q_PROPERTY(QString downloadRateText READ downloadRateText NOTIFY statsChanged)
    Q_PROPERTY(QString uploadRateText READ uploadRateText NOTIFY statsChanged)
    Q_PROPERTY(QString networkInterface READ networkInterface NOTIFY statsChanged)
    Q_PROPERTY(QVariantList downloadHistory READ downloadHistory NOTIFY statsChanged)
    Q_PROPERTY(QVariantList uploadHistory READ uploadHistory NOTIFY statsChanged)
    Q_PROPERTY(int totalProcesses READ totalProcesses NOTIFY statsChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(QString sortMode READ sortMode NOTIFY sortModeChanged)
    Q_PROPERTY(int refreshInterval READ refreshInterval NOTIFY refreshIntervalChanged)

public:
    explicit BtopModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QString hostName() const { return m_hostName; }
    QString userName() const { return m_userName; }
    QString osName() const { return m_osName; }
    QString uptimeText() const { return m_uptimeText; }
    QString loadAverageText() const { return m_loadAverageText; }
    QString cpuNameText() const { return m_cpuNameText; }
    QString cpuFrequencyText() const { return m_cpuFrequencyText; }
    double cpuUsage() const { return m_cpuUsage; }
    QVariantList cpuHistory() const { return m_cpuHistory; }
    QVariantList coreUsage() const { return m_coreUsage; }
    QStringList coreHistoryText() const { return m_coreHistoryText; }
    double memoryPercent() const { return percent(m_memoryUsed, m_memoryTotal); }
    double availablePercent() const { return percent(m_memoryAvailable, m_memoryTotal); }
    double cachedPercent() const { return percent(m_memoryCached, m_memoryTotal); }
    QString memoryUsedText() const;
    QString memoryTotalText() const;
    QString memoryAvailableText() const;
    QString memoryCachedText() const;
    double swapPercent() const { return percent(m_swapUsed, m_swapTotal); }
    QString swapUsedText() const;
    QString swapTotalText() const;
    double diskPercent() const { return percent(m_diskUsed, m_diskTotal); }
    QString diskUsedText() const;
    QString diskTotalText() const;
    QString downloadRateText() const;
    QString uploadRateText() const;
    QString networkInterface() const { return m_networkInterface; }
    QVariantList downloadHistory() const { return m_downloadHistory; }
    QVariantList uploadHistory() const { return m_uploadHistory; }
    int totalProcesses() const { return m_totalProcesses; }
    bool paused() const { return m_paused; }
    QString sortMode() const;
    int refreshInterval() const { return m_refreshInterval; }

    Q_INVOKABLE QVariantList columnDefinitions() const;
    Q_INVOKABLE void togglePaused();
    Q_INVOKABLE void sortByCpu();
    Q_INVOKABLE void sortByMemory();
    Q_INVOKABLE void sortByPid();
    Q_INVOKABLE void cycleRefreshInterval();
    Q_INVOKABLE void refresh();

signals:
    void statsChanged();
    void pausedChanged();
    void sortModeChanged();
    void refreshIntervalChanged();

private:
    enum class SortMode { Cpu, Memory, Pid };

    struct ProcessStaticInfo {
        QString program;
        QString command;
        QString user;
    };

    static double percent(quint64 part, quint64 total);
    static QString formatBytes(double bytes);
    static QString formatBrailleHistory(const QVariantList &history, int cellWidth = 10);
    static void appendHistory(QVariantList &history, double value);
    static QString usernameForUid(uid_t uid);

    void readCpuStats();
    void readMemoryStats();
    void readDiskStats();
    void readNetworkStats();
    void readProcessStats();
    void sortProcesses();

    QList<BtopProcessRow> m_processes;
    QHash<int, quint64> m_previousProcessTicks;
    QHash<int, ProcessStaticInfo> m_processStaticCache;
    static QHash<uid_t, QString> s_uidCache;

    QTimer m_timer;
    QElapsedTimer m_networkClock;

    QString m_hostName;
    QString m_userName;
    QString m_osName;
    QString m_uptimeText;
    QString m_loadAverageText;
    QString m_cpuNameText;
    QString m_cpuFrequencyText;
    QString m_networkInterface = QStringLiteral("loopback");

    double m_cpuUsage = 0.0;
    QVariantList m_cpuHistory;
    QVariantList m_coreUsage;
    QVector<QVariantList> m_coreHistoryValues;
    QStringList m_coreHistoryText;
    QVariantList m_downloadHistory;
    QVariantList m_uploadHistory;
    QVector<quint64> m_previousCoreTotals;
    QVector<quint64> m_previousCoreIdle;
    quint64 m_previousCpuTotal = 0;
    quint64 m_previousCpuIdle = 0;
    quint64 m_cpuDelta = 0;

    quint64 m_memoryTotal = 0;
    quint64 m_memoryUsed = 0;
    quint64 m_memoryAvailable = 0;
    quint64 m_memoryCached = 0;
    quint64 m_swapTotal = 0;
    quint64 m_swapUsed = 0;
    quint64 m_diskTotal = 0;
    quint64 m_diskUsed = 0;
    quint64 m_previousRxBytes = 0;
    quint64 m_previousTxBytes = 0;
    double m_downloadBytesPerSecond = 0.0;
    double m_uploadBytesPerSecond = 0.0;

    int m_totalProcesses = 0;
    int m_refreshInterval = 1000;
    bool m_paused = false;
    SortMode m_sortMode = SortMode::Cpu;
};
