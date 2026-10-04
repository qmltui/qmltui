#include "SystemInfoProvider.h"

SystemInfoProvider::SystemInfoProvider(int refreshIntervalMs, QObject *parent)
    : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &SystemInfoProvider::refresh);
    m_timer.start(refreshIntervalMs);
}

void SystemInfoProvider::pushCpuSample(double pct) {
    m_cpuLoadPct = pct;
    m_cpuHistory.append(pct);
    while (m_cpuHistory.size() > kHistoryLength)
        m_cpuHistory.removeFirst();
}
