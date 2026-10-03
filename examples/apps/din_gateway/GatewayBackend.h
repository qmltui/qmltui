#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <QString>
#include <QQmlEngine>

class GatewayBackend : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList sensorValues READ sensorValues NOTIFY sensorsChanged)
    Q_PROPERTY(QVariantList sensorHistories READ sensorHistories NOTIFY sensorsChanged)
    Q_PROPERTY(bool relay1 READ relay1 NOTIFY relaysChanged)
    Q_PROPERTY(bool relay2 READ relay2 NOTIFY relaysChanged)
    Q_PROPERTY(bool relay3 READ relay3 NOTIFY relaysChanged)
    Q_PROPERTY(bool relay4 READ relay4 NOTIFY relaysChanged)

    Q_PROPERTY(int totalTx READ totalTx NOTIFY trafficStatsChanged)
    Q_PROPERTY(int totalRx READ totalRx NOTIFY trafficStatsChanged)
    Q_PROPERTY(int crcErrors READ crcErrors NOTIFY trafficStatsChanged)
    Q_PROPERTY(int packetsPerSec READ packetsPerSec NOTIFY trafficStatsChanged)
    Q_PROPERTY(double busLoadPct READ busLoadPct NOTIFY trafficStatsChanged)
    Q_PROPERTY(QVariantList packetLogs READ packetLogs NOTIFY packetLogsChanged)

    Q_PROPERTY(double cpuLoadPct READ cpuLoadPct NOTIFY systemStatsChanged)
    Q_PROPERTY(double cpuTemp READ cpuTemp NOTIFY systemStatsChanged)
    Q_PROPERTY(int memUsedMb READ memUsedMb NOTIFY systemStatsChanged)
    Q_PROPERTY(int uptimeSeconds READ uptimeSeconds NOTIFY uptimeChanged)

public:
    explicit GatewayBackend(QObject *parent = nullptr);

    static GatewayBackend *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static GatewayBackend *instance();

    QVariantList sensorValues() const { return m_sensorValues; }
    QVariantList sensorHistories() const;
    bool relay1() const { return m_relay1; }
    bool relay2() const { return m_relay2; }
    bool relay3() const { return m_relay3; }
    bool relay4() const { return m_relay4; }

    int totalTx() const { return m_totalTx; }
    int totalRx() const { return m_totalRx; }
    int crcErrors() const { return m_crcErrors; }
    int packetsPerSec() const { return m_packetsPerSec; }
    double busLoadPct() const { return m_busLoadPct; }
    QVariantList packetLogs() const { return m_packetLogs; }

    double cpuLoadPct() const { return m_cpuLoadPct; }
    double cpuTemp() const { return m_cpuTemp; }
    int memUsedMb() const { return m_memUsedMb; }
    int uptimeSeconds() const { return m_uptimeSeconds; }

    Q_INVOKABLE void toggleRelay(int relayIndex);
    Q_INVOKABLE void setModbusBaud(const QString &baud);
    Q_INVOKABLE void setCanBitrate(const QString &bitrate);
    Q_INVOKABLE void setPublishInterval(int seconds);

signals:
    void sensorsChanged();
    void relaysChanged();
    void trafficStatsChanged();
    void packetLogsChanged();
    void systemStatsChanged();
    void uptimeChanged();

private slots:
    void onTick();

private:
    static GatewayBackend *s_instance;

    QVariantList m_sensorValues;
    QList<QVariantList> m_sensorHistories;
    bool m_relay1 = true;
    bool m_relay2 = false;
    bool m_relay3 = true;
    bool m_relay4 = false;

    int m_totalTx = 48912;
    int m_totalRx = 48890;
    int m_crcErrors = 22;
    int m_packetsPerSec = 142;
    double m_busLoadPct = 12.4;
    QVariantList m_packetLogs;

    double m_cpuLoadPct = 14.5;
    double m_cpuTemp = 42.8;
    int m_memUsedMb = 122;
    int m_uptimeSeconds = 14520;

    QTimer m_timer;
};
