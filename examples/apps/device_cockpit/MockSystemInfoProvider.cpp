#include "MockSystemInfoProvider.h"

#include <QtMath>

namespace {

QVariantMap disk(const QString &name, const QString &mount, int totalMb, int usedMb) {
    return {{"name", name}, {"mount", mount}, {"totalMb", totalMb}, {"usedMb", usedMb}};
}

QVariantMap nic(const QString &name, bool up, const QString &ipv4) {
    return {{"name", name}, {"up", up}, {"ipv4", ipv4}};
}

} // namespace

MockSystemInfoProvider::MockSystemInfoProvider(QObject *parent)
    : SystemInfoProvider(1000, parent) {
    m_hostname = QStringLiteral("cockpit-demo");
    m_osName = QStringLiteral("Cockpit Demo OS 1.0");
    m_kernelVersion = QStringLiteral("6.6.0-mock");

    m_uptimeSeconds = 3 * 3600 + 25 * 60;
    m_memTotalMb = 2048;
    m_memUsedMb = 512;
    m_loadAverage = 0.42;
    m_cpuTempC = 48.0;

    m_disks = {disk("mmcblk0p2", "/", 7629, 2917),
               disk("mmcblk0p3", "/data", 29491, 8123)};
    m_interfaces = {nic("eth0", true, "192.168.7.2"),
                    nic("can0", true, QString()),
                    nic("wlan0", false, QString())};

    for (int i = 0; i < 30; ++i)
        pushCpuSample(35.0 + 18.0 * qSin(i / 6.0));
    m_tick = 30;

    refresh();
}

void MockSystemInfoProvider::refresh() {
    ++m_tick;
    ++m_uptimeSeconds;

    pushCpuSample(35.0 + 18.0 * qSin(m_tick / 6.0));
    m_memUsedMb = 512 + int(40.0 * qSin(m_tick / 9.0));
    m_cpuTempC = 48.0 + 4.0 * qSin(m_tick / 7.0);
    m_loadAverage = 0.42 + 0.3 * (qSin(m_tick / 6.0) + 1.0) / 2.0;

    emit statsChanged();
}
