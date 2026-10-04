#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QTimer>
#include <QVariantList>

// Base class of the system-information domain. Pages bind to this interface
// only; CockpitBackend decides whether the mock or the Linux implementation
// is behind it, which keeps every page runnable without a device.
class SystemInfoProvider : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QString hostname READ hostname NOTIFY identityChanged)
    Q_PROPERTY(QString osName READ osName NOTIFY identityChanged)
    Q_PROPERTY(QString kernelVersion READ kernelVersion NOTIFY identityChanged)

    Q_PROPERTY(int uptimeSeconds READ uptimeSeconds NOTIFY statsChanged)
    Q_PROPERTY(double cpuLoadPct READ cpuLoadPct NOTIFY statsChanged)
    Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY statsChanged)
    Q_PROPERTY(double cpuTempC READ cpuTempC NOTIFY statsChanged)
    Q_PROPERTY(double loadAverage READ loadAverage NOTIFY statsChanged)
    Q_PROPERTY(int memTotalMb READ memTotalMb NOTIFY statsChanged)
    Q_PROPERTY(int memUsedMb READ memUsedMb NOTIFY statsChanged)

    Q_PROPERTY(QVariantList disks READ disks NOTIFY storageChanged)
    Q_PROPERTY(QVariantList interfaces READ interfaces NOTIFY networkChanged)

public:
    explicit SystemInfoProvider(int refreshIntervalMs, QObject *parent = nullptr);

    QString hostname() const { return m_hostname; }
    QString osName() const { return m_osName; }
    QString kernelVersion() const { return m_kernelVersion; }

    int uptimeSeconds() const { return m_uptimeSeconds; }
    double cpuLoadPct() const { return m_cpuLoadPct; }
    QVariantList cpuHistory() const { return m_cpuHistory; }
    /// Degrees Celsius; negative when no sensor is available.
    double cpuTempC() const { return m_cpuTempC; }
    double loadAverage() const { return m_loadAverage; }
    int memTotalMb() const { return m_memTotalMb; }
    int memUsedMb() const { return m_memUsedMb; }

    /// List of maps: { name, mount, totalMb, usedMb }.
    QVariantList disks() const { return m_disks; }
    /// List of maps: { name, up, ipv4 }.
    QVariantList interfaces() const { return m_interfaces; }

signals:
    void identityChanged();
    void statsChanged();
    void storageChanged();
    void networkChanged();

protected slots:
    virtual void refresh() = 0;

protected:
    void pushCpuSample(double pct);

    QString m_hostname;
    QString m_osName;
    QString m_kernelVersion;

    int m_uptimeSeconds = 0;
    double m_cpuLoadPct = 0.0;
    QVariantList m_cpuHistory;
    double m_cpuTempC = -1.0;
    double m_loadAverage = 0.0;
    int m_memTotalMb = 0;
    int m_memUsedMb = 0;

    QVariantList m_disks;
    QVariantList m_interfaces;

private:
    static constexpr int kHistoryLength = 60;

    QTimer m_timer;
};
