#include "LinuxSystemInfoProvider.h"

#include <QDir>
#include <QFile>
#include <QNetworkInterface>
#include <QStorageInfo>
#include <QSysInfo>

namespace {

QString readSmallFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll()).trimmed();
}

} // namespace

LinuxSystemInfoProvider::LinuxSystemInfoProvider(QObject *parent)
    : SystemInfoProvider(2000, parent) {
    refreshIdentity();
    refreshStats();
    refreshStorage();
    refreshNetwork();
}

void LinuxSystemInfoProvider::refresh() {
    refreshStats();
    emit statsChanged();

    // Mounts and links change rarely; poll them on every fifth tick.
    if (++m_tick % 5 == 0) {
        refreshStorage();
        refreshNetwork();
        emit storageChanged();
        emit networkChanged();
    }
}

void LinuxSystemInfoProvider::refreshIdentity() {
    m_hostname = QSysInfo::machineHostName();
    m_osName = QSysInfo::prettyProductName();
    m_kernelVersion = QSysInfo::kernelVersion();
    emit identityChanged();
}

void LinuxSystemInfoProvider::refreshStats() {
    const QString uptime = readSmallFile(QStringLiteral("/proc/uptime"));
    if (!uptime.isEmpty())
        m_uptimeSeconds = int(uptime.section(' ', 0, 0).toDouble());

    const QString loadavg = readSmallFile(QStringLiteral("/proc/loadavg"));
    if (!loadavg.isEmpty())
        m_loadAverage = loadavg.section(' ', 0, 0).toDouble();

    // First "cpu" line of /proc/stat: user nice system idle iowait irq softirq steal.
    const QString stat = readSmallFile(QStringLiteral("/proc/stat"));
    if (!stat.isEmpty()) {
        const QStringList fields =
            stat.section('\n', 0, 0).simplified().split(' ').mid(1);
        qulonglong total = 0;
        for (const QString &field : fields)
            total += field.toULongLong();
        qulonglong idle = 0;
        if (fields.size() > 4)
            idle = fields.at(3).toULongLong() + fields.at(4).toULongLong();
        const qulonglong busy = total - idle;
        if (m_lastCpuTotal > 0 && total > m_lastCpuTotal) {
            const double deltaTotal = double(total - m_lastCpuTotal);
            pushCpuSample(100.0 * double(busy - m_lastCpuBusy) / deltaTotal);
        }
        m_lastCpuTotal = total;
        m_lastCpuBusy = busy;
    }

    // Read whole /proc files at once: their reported size is zero, which
    // breaks QFile::atEnd() line loops.
    const QString meminfo = readSmallFile(QStringLiteral("/proc/meminfo"));
    if (!meminfo.isEmpty()) {
        qulonglong totalKb = 0;
        qulonglong availableKb = 0;
        const QStringList lines = meminfo.split('\n');
        for (const QString &line : lines) {
            if (line.startsWith(QStringLiteral("MemTotal:")))
                totalKb = line.simplified().section(' ', 1, 1).toULongLong();
            else if (line.startsWith(QStringLiteral("MemAvailable:")))
                availableKb = line.simplified().section(' ', 1, 1).toULongLong();
            if (totalKb > 0 && availableKb > 0)
                break;
        }
        if (totalKb > 0) {
            m_memTotalMb = int(totalKb / 1024);
            m_memUsedMb = int((totalKb - availableKb) / 1024);
        }
    }

    // Hottest thermal zone stands in for "CPU temperature" until a hwmon
    // mapping is configurable.
    double maxTemp = -1.0;
    const QDir thermal(QStringLiteral("/sys/class/thermal"));
    const QStringList zones =
        thermal.entryList({QStringLiteral("thermal_zone*")}, QDir::Dirs);
    for (const QString &zone : zones) {
        const QString raw = readSmallFile(thermal.filePath(zone + "/temp"));
        if (raw.isEmpty())
            continue;
        maxTemp = qMax(maxTemp, raw.toDouble() / 1000.0);
    }
    m_cpuTempC = maxTemp;
}

void LinuxSystemInfoProvider::refreshStorage() {
    static const QStringList ignoredFilesystems = {
        QStringLiteral("tmpfs"), QStringLiteral("devtmpfs"),
        QStringLiteral("efivarfs"), QStringLiteral("overlay")};

    QVariantList disks;
    QStringList seenDevices;
    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &volume : volumes) {
        if (!volume.isValid() || !volume.isReady() || volume.bytesTotal() <= 0)
            continue;
        const QString device = QString::fromUtf8(volume.device());
        const QString fsType = QString::fromUtf8(volume.fileSystemType());
        if (ignoredFilesystems.contains(fsType) || seenDevices.contains(device))
            continue;
        seenDevices.append(device);
        disks.append(QVariantMap{
            {"name", device.section('/', -1)},
            {"mount", volume.rootPath()},
            {"totalMb", int(volume.bytesTotal() / (1024 * 1024))},
            {"usedMb",
             int((volume.bytesTotal() - volume.bytesAvailable()) / (1024 * 1024))},
        });
    }
    m_disks = disks;
}

void LinuxSystemInfoProvider::refreshNetwork() {
    QVariantList interfaces;
    const auto all = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &interface : all) {
        if (interface.flags() & QNetworkInterface::IsLoopBack)
            continue;
        QString ipv4;
        const auto entries = interface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                ipv4 = entry.ip().toString();
                break;
            }
        }
        interfaces.append(QVariantMap{
            {"name", interface.humanReadableName()},
            {"up", bool(interface.flags() & QNetworkInterface::IsRunning)},
            {"ipv4", ipv4},
        });
    }
    m_interfaces = interfaces;
}
