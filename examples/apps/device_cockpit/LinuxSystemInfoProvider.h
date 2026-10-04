#pragma once

#include "SystemInfoProvider.h"

// Reads the live system through /proc, /sys, QStorageInfo, and
// QNetworkInterface. Values that cannot be read (for example on macOS, or a
// board without thermal zones) keep their "unknown" defaults instead of
// failing.
class LinuxSystemInfoProvider : public SystemInfoProvider {
    Q_OBJECT

public:
    explicit LinuxSystemInfoProvider(QObject *parent = nullptr);

protected slots:
    void refresh() override;

private:
    void refreshIdentity();
    void refreshStats();
    void refreshStorage();
    void refreshNetwork();

    // /proc/stat counters from the previous sample, for the load delta.
    qulonglong m_lastCpuTotal = 0;
    qulonglong m_lastCpuBusy = 0;
    int m_tick = 0;
};
