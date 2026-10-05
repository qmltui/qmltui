#include "BtopModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QSysInfo>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>

#include <unistd.h>

#ifdef Q_OS_MACOS
#include <ifaddrs.h>
#include <mach/mach.h>
#include <net/if.h>
#include <net/if_dl.h>
#endif

namespace {
constexpr int kHistoryLength = 120;
constexpr int kMaximumVisibleProcesses = 256;

QString readFirstLine(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readLine()).trimmed();
}
}

BtopModel::BtopModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_hostName(QSysInfo::machineHostName())
    , m_userName(qEnvironmentVariable("USER", QStringLiteral("user")))
    , m_osName(QSysInfo::prettyProductName())
{
    if (m_hostName.isEmpty())
        m_hostName = qEnvironmentVariable("HOSTNAME", QStringLiteral("localhost"));

    m_networkClock.start();
    connect(&m_timer, &QTimer::timeout, this, &BtopModel::refresh);
    m_timer.start(m_refreshInterval);
    refresh();
}

int BtopModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_processes.size();
}

int BtopModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 7;
}

QVariant BtopModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole || index.row() >= m_processes.size())
        return {};

    const BtopProcessRow &process = m_processes.at(index.row());
    switch (index.column()) {
    case 0: return process.pid;
    case 1: return process.program;
    case 2: return process.command;
    case 3: return process.threads;
    case 4: return process.user;
    case 5: return process.memoryText;
    case 6: return QString::number(process.cpuPercent, 'f', 1);
    default: return {};
    }
}

QVariant BtopModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    static const QStringList headers = {
        QStringLiteral("PID"), QStringLiteral("Program"), QStringLiteral("Command"),
        QStringLiteral("Thr"), QStringLiteral("User"), QStringLiteral("Mem"),
        QStringLiteral("Cpu%")
    };
    return section >= 0 && section < headers.size() ? headers.at(section) : QVariant();
}

QVariantList BtopModel::columnDefinitions() const
{
    return {
        QVariantMap{{"title", "PID"}, {"width", 7}, {"alignment", "right"}},
        QVariantMap{{"title", "Program"}, {"width", 13}},
        QVariantMap{{"title", "Command"}, {"width", 22}},
        QVariantMap{{"title", "Thr"}, {"width", 4}, {"alignment", "right"}},
        QVariantMap{{"title", "User"}, {"width", 8}},
        QVariantMap{{"title", "Mem"}, {"width", 7}, {"alignment", "right"}},
        QVariantMap{{"title", "Cpu%"}, {"width", 5}, {"alignment", "right"}}
    };
}

void BtopModel::togglePaused()
{
    m_paused = !m_paused;
    emit pausedChanged();
    if (!m_paused)
        refresh();
}

void BtopModel::sortByCpu()
{
    if (m_sortMode == SortMode::Cpu)
        return;
    m_sortMode = SortMode::Cpu;
    beginResetModel();
    sortProcesses();
    endResetModel();
    emit sortModeChanged();
}

void BtopModel::sortByMemory()
{
    if (m_sortMode == SortMode::Memory)
        return;
    m_sortMode = SortMode::Memory;
    beginResetModel();
    sortProcesses();
    endResetModel();
    emit sortModeChanged();
}

void BtopModel::sortByPid()
{
    if (m_sortMode == SortMode::Pid)
        return;
    m_sortMode = SortMode::Pid;
    beginResetModel();
    sortProcesses();
    endResetModel();
    emit sortModeChanged();
}

void BtopModel::cycleRefreshInterval()
{
    static constexpr int intervals[] = {500, 1000, 2000, 5000};
    auto it = std::find(std::begin(intervals), std::end(intervals), m_refreshInterval);
    const auto next = it == std::end(intervals) || std::next(it) == std::end(intervals)
        ? std::begin(intervals) : std::next(it);
    m_refreshInterval = *next;
    m_timer.setInterval(m_refreshInterval);
    emit refreshIntervalChanged();
}

void BtopModel::refresh()
{
    if (m_paused)
        return;
    readCpuStats();
    readMemoryStats();
    readDiskStats();
    readNetworkStats();
    readProcessStats();
    emit statsChanged();
}

QString BtopModel::memoryUsedText() const { return formatBytes(m_memoryUsed); }
QString BtopModel::memoryTotalText() const { return formatBytes(m_memoryTotal); }
QString BtopModel::memoryAvailableText() const { return formatBytes(m_memoryAvailable); }
QString BtopModel::memoryCachedText() const { return formatBytes(m_memoryCached); }
QString BtopModel::swapUsedText() const { return formatBytes(m_swapUsed); }
QString BtopModel::swapTotalText() const { return formatBytes(m_swapTotal); }
QString BtopModel::diskUsedText() const { return formatBytes(m_diskUsed); }
QString BtopModel::diskTotalText() const { return formatBytes(m_diskTotal); }
QString BtopModel::downloadRateText() const { return formatBytes(m_downloadBytesPerSecond) + QStringLiteral("/s"); }
QString BtopModel::uploadRateText() const { return formatBytes(m_uploadBytesPerSecond) + QStringLiteral("/s"); }

QString BtopModel::sortMode() const
{
    switch (m_sortMode) {
    case SortMode::Cpu: return QStringLiteral("cpu");
    case SortMode::Memory: return QStringLiteral("memory");
    case SortMode::Pid: return QStringLiteral("pid");
    }
    return QStringLiteral("cpu");
}

double BtopModel::percent(quint64 part, quint64 total)
{
    return total == 0 ? 0.0 : std::clamp(part * 100.0 / total, 0.0, 100.0);
}

QString BtopModel::formatBytes(double bytes)
{
    static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    int unit = 0;
    while (bytes >= 1024.0 && unit < 4) {
        bytes /= 1024.0;
        ++unit;
    }
    const int precision = unit == 0 ? 0 : (bytes < 10.0 ? 2 : 1);
    return QStringLiteral("%1 %2").arg(bytes, 0, 'f', precision).arg(QString::fromLatin1(units[unit]));
}

QString BtopModel::formatBrailleHistory(const QVariantList &history, int cellWidth)
{
    static constexpr int leftMasks[] = {0x40, 0x40, 0x44, 0x46, 0x47};
    static constexpr int rightMasks[] = {0x80, 0x80, 0xA0, 0xB0, 0xB8};
    const int sampleCount = cellWidth * 2;
    const int firstSample = std::max(0, static_cast<int>(history.size()) - sampleCount);
    const int leftPadding = std::max(0, sampleCount - static_cast<int>(history.size()));
    QString graph;
    graph.reserve(cellWidth);
    for (int cell = 0; cell < cellWidth; ++cell) {
        const int leftSlot = cell * 2 - leftPadding;
        const int rightSlot = leftSlot + 1;
        const int leftIndex = firstSample + leftSlot;
        const int rightIndex = firstSample + rightSlot;
        const double leftValue = leftSlot >= 0 && leftIndex < history.size()
            ? history.at(leftIndex).toDouble() : 0.0;
        const double rightValue = rightSlot >= 0 && rightIndex < history.size()
            ? history.at(rightIndex).toDouble() : 0.0;
        const int leftLevel = std::clamp(static_cast<int>(std::ceil(leftValue / 25.0)), 1, 4);
        const int rightLevel = std::clamp(static_cast<int>(std::ceil(rightValue / 25.0)), 1, 4);
        graph.append(QChar(0x2800 + leftMasks[leftLevel] + rightMasks[rightLevel]));
    }
    return graph;
}

void BtopModel::appendHistory(QVariantList &history, double value)
{
    history.append(value);
    while (history.size() > kHistoryLength)
        history.removeFirst();
}

void BtopModel::readCpuStats()
{
#ifdef Q_OS_LINUX
    QFile statFile(QStringLiteral("/proc/stat"));
    if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QVector<quint64> totals;
        QVector<quint64> idles;
        const QStringList statLines = QString::fromUtf8(statFile.readAll())
                                          .split('\n', Qt::SkipEmptyParts);
        for (const QString &rawLine : statLines) {
            const QString line = rawLine.trimmed();
            if (!line.startsWith(QStringLiteral("cpu")))
                break;
            const QStringList fields = line.split(' ', Qt::SkipEmptyParts);
            if (fields.size() < 5)
                continue;
            quint64 total = 0;
            for (int i = 1; i < fields.size(); ++i)
                total += fields.at(i).toULongLong();
            const quint64 idle = fields.at(4).toULongLong()
                + (fields.size() > 5 ? fields.at(5).toULongLong() : 0);
            totals.append(total);
            idles.append(idle);
        }

        if (!totals.isEmpty()) {
            m_cpuDelta = totals.first() - m_previousCpuTotal;
            const quint64 idleDelta = idles.first() - m_previousCpuIdle;
            if (m_previousCpuTotal != 0 && m_cpuDelta != 0)
                m_cpuUsage = std::clamp((m_cpuDelta - idleDelta) * 100.0 / m_cpuDelta, 0.0, 100.0);
            m_previousCpuTotal = totals.first();
            m_previousCpuIdle = idles.first();

            QVariantList cores;
            const int coreCount = totals.size() - 1;
            if (m_previousCoreTotals.size() != coreCount) {
                m_previousCoreTotals.fill(0, coreCount);
                m_previousCoreIdle.fill(0, coreCount);
            }
            for (int i = 0; i < coreCount; ++i) {
                const quint64 totalDelta = totals.at(i + 1) - m_previousCoreTotals.at(i);
                const quint64 coreIdleDelta = idles.at(i + 1) - m_previousCoreIdle.at(i);
                double usage = 0.0;
                if (m_previousCoreTotals.at(i) != 0 && totalDelta != 0)
                    usage = std::clamp((totalDelta - coreIdleDelta) * 100.0 / totalDelta, 0.0, 100.0);
                cores.append(usage);
                m_previousCoreTotals[i] = totals.at(i + 1);
                m_previousCoreIdle[i] = idles.at(i + 1);
            }
            m_coreUsage = cores;
            while (m_coreHistoryValues.size() < coreCount)
                m_coreHistoryValues.append(QVariantList{});
            while (m_coreHistoryValues.size() > coreCount)
                m_coreHistoryValues.removeLast();
            m_coreHistoryText.clear();
            for (int i = 0; i < coreCount; ++i) {
                QVariantList history = m_coreHistoryValues.at(i);
                appendHistory(history, cores.at(i).toDouble());
                m_coreHistoryValues[i] = history;
                m_coreHistoryText.append(formatBrailleHistory(history));
            }
        }
    }

    const QString loadLine = readFirstLine(QStringLiteral("/proc/loadavg"));
    const QStringList loads = loadLine.split(' ', Qt::SkipEmptyParts);
    m_loadAverageText = loads.mid(0, 3).join(' ');

    const double uptimeSeconds = readFirstLine(QStringLiteral("/proc/uptime"))
                                     .section(' ', 0, 0).toDouble();
    const int days = static_cast<int>(uptimeSeconds / 86400.0);
    const int hours = static_cast<int>(uptimeSeconds / 3600.0) % 24;
    const int minutes = static_cast<int>(uptimeSeconds / 60.0) % 60;
    const int seconds = static_cast<int>(uptimeSeconds) % 60;
    m_uptimeText = days > 0
        ? QStringLiteral("%1d %2:%3:%4").arg(days)
              .arg(hours, 2, 10, QLatin1Char('0'))
              .arg(minutes, 2, 10, QLatin1Char('0'))
              .arg(seconds, 2, 10, QLatin1Char('0'))
        : QStringLiteral("%1:%2:%3")
              .arg(hours, 2, 10, QLatin1Char('0'))
              .arg(minutes, 2, 10, QLatin1Char('0'))
              .arg(seconds, 2, 10, QLatin1Char('0'));

    QFile cpuInfo(QStringLiteral("/proc/cpuinfo"));
    double frequencyTotal = 0.0;
    int frequencyCount = 0;
    if (cpuInfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QStringList cpuInfoLines = QString::fromUtf8(cpuInfo.readAll())
                                             .split('\n', Qt::SkipEmptyParts);
        for (const QString &line : cpuInfoLines) {
            if (m_cpuNameText.isEmpty() && line.startsWith(QStringLiteral("model name"))) {
                m_cpuNameText = line.section(':', 1).trimmed();
                m_cpuNameText.remove(QRegularExpression(QStringLiteral("\\s+CPU(?:\\s+@.*)?$")));
                m_cpuNameText.replace(QStringLiteral("(R)"), QString());
                m_cpuNameText.replace(QStringLiteral("(TM)"), QString());
                m_cpuNameText = m_cpuNameText.simplified();
                m_cpuNameText.remove(QRegularExpression(QStringLiteral("^AMD\\s+")));
            }
            if (line.startsWith(QStringLiteral("cpu MHz"))) {
                frequencyTotal += line.section(':', 1).trimmed().toDouble();
                ++frequencyCount;
            }
        }
    }
    m_cpuFrequencyText = frequencyCount == 0 ? QStringLiteral("n/a")
        : QStringLiteral("%1 GHz").arg(frequencyTotal / frequencyCount / 1000.0, 0, 'f', 2);
    if (m_cpuNameText.isEmpty())
        m_cpuNameText = QSysInfo::currentCpuArchitecture();
#elif defined(Q_OS_MACOS)
    host_cpu_load_info_data_t loadInfo{};
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                        reinterpret_cast<host_info_t>(&loadInfo), &count) == KERN_SUCCESS) {
        const quint64 idle = loadInfo.cpu_ticks[CPU_STATE_IDLE];
        quint64 total = 0;
        for (int state = 0; state < CPU_STATE_MAX; ++state)
            total += loadInfo.cpu_ticks[state];
        m_cpuDelta = total - m_previousCpuTotal;
        const quint64 idleDelta = idle - m_previousCpuIdle;
        if (m_previousCpuTotal != 0 && m_cpuDelta != 0)
            m_cpuUsage = std::clamp((m_cpuDelta - idleDelta) * 100.0 / m_cpuDelta, 0.0, 100.0);
        m_previousCpuTotal = total;
        m_previousCpuIdle = idle;
        m_coreUsage = {m_cpuUsage};
        QVariantList history = m_coreHistoryValues.isEmpty()
            ? QVariantList{} : m_coreHistoryValues.first();
        appendHistory(history, m_cpuUsage);
        m_coreHistoryValues = {history};
        m_coreHistoryText = {formatBrailleHistory(history)};
    }
    double loads[3]{};
    getloadavg(loads, 3);
    m_loadAverageText = QStringLiteral("%1 %2 %3").arg(loads[0], 0, 'f', 2)
                            .arg(loads[1], 0, 'f', 2).arg(loads[2], 0, 'f', 2);
    m_uptimeText = QStringLiteral("macOS");
    m_cpuNameText = QStringLiteral("Apple %1").arg(QSysInfo::currentCpuArchitecture());
    m_cpuFrequencyText = QStringLiteral("Apple CPU");
#else
    m_loadAverageText = QStringLiteral("n/a");
    m_uptimeText = QStringLiteral("n/a");
    m_cpuNameText = QSysInfo::currentCpuArchitecture();
    m_cpuFrequencyText = QStringLiteral("n/a");
#endif
    appendHistory(m_cpuHistory, m_cpuUsage);
}

void BtopModel::readMemoryStats()
{
#ifdef Q_OS_LINUX
    QFile file(QStringLiteral("/proc/meminfo"));
    QHash<QString, quint64> values;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QStringList lines = QString::fromUtf8(file.readAll())
                                      .split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList fields = line
                                           .split(QRegularExpression(QStringLiteral("[:\\s]+")),
                                                  Qt::SkipEmptyParts);
            if (fields.size() >= 2)
                values.insert(fields.at(0), fields.at(1).toULongLong() * 1024ULL);
        }
    }
    m_memoryTotal = values.value(QStringLiteral("MemTotal"));
    m_memoryAvailable = values.value(QStringLiteral("MemAvailable"));
    m_memoryCached = values.value(QStringLiteral("Cached"))
        + values.value(QStringLiteral("SReclaimable"));
    m_memoryUsed = m_memoryTotal > m_memoryAvailable ? m_memoryTotal - m_memoryAvailable : 0;
    m_swapTotal = values.value(QStringLiteral("SwapTotal"));
    const quint64 swapFree = values.value(QStringLiteral("SwapFree"));
    m_swapUsed = m_swapTotal > swapFree ? m_swapTotal - swapFree : 0;
#elif defined(Q_OS_MACOS)
    vm_statistics64_data_t vmStats{};
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_size_t pageSize = 0;
    host_page_size(mach_host_self(), &pageSize);
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          reinterpret_cast<host_info64_t>(&vmStats), &count) == KERN_SUCCESS) {
        m_memoryAvailable = (vmStats.free_count + vmStats.inactive_count) * pageSize;
        m_memoryCached = vmStats.purgeable_count * pageSize;
        const quint64 used = (vmStats.active_count + vmStats.wire_count
                              + vmStats.compressor_page_count) * pageSize;
        m_memoryUsed = used;
        m_memoryTotal = used + m_memoryAvailable;
    }
#endif
}

void BtopModel::readDiskStats()
{
    QStorageInfo rootStorage = QStorageInfo::root();
    rootStorage.refresh();
    m_diskTotal = rootStorage.bytesTotal();
    const quint64 available = rootStorage.bytesAvailable();
    m_diskUsed = m_diskTotal > available ? m_diskTotal - available : 0;
}

void BtopModel::readNetworkStats()
{
    quint64 rxBytes = 0;
    quint64 txBytes = 0;
    QString selectedInterface;
#ifdef Q_OS_LINUX
    QFile file(QStringLiteral("/proc/net/dev"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QStringList lines = QString::fromUtf8(file.readAll())
                                      .split('\n', Qt::SkipEmptyParts);
        for (int lineIndex = 2; lineIndex < lines.size(); ++lineIndex) {
            const QString line = lines.at(lineIndex).trimmed();
            const QString name = line.section(':', 0, 0).trimmed();
            if (name.isEmpty() || name == QStringLiteral("lo"))
                continue;
            const QStringList fields = line.section(':', 1).split(' ', Qt::SkipEmptyParts);
            if (fields.size() >= 9) {
                rxBytes += fields.at(0).toULongLong();
                txBytes += fields.at(8).toULongLong();
                if (selectedInterface.isEmpty())
                    selectedInterface = name;
            }
        }
    }
#elif defined(Q_OS_MACOS)
    ifaddrs *addresses = nullptr;
    if (getifaddrs(&addresses) == 0) {
        for (ifaddrs *address = addresses; address; address = address->ifa_next) {
            if (!address->ifa_data || (address->ifa_flags & IFF_LOOPBACK))
                continue;
            const auto *data = static_cast<const if_data *>(address->ifa_data);
            rxBytes += data->ifi_ibytes;
            txBytes += data->ifi_obytes;
            if (selectedInterface.isEmpty())
                selectedInterface = QString::fromLocal8Bit(address->ifa_name);
        }
        freeifaddrs(addresses);
    }
#endif
    if (!selectedInterface.isEmpty())
        m_networkInterface = selectedInterface;

    const qint64 elapsedMs = m_networkClock.restart();
    if (m_previousRxBytes != 0 && rxBytes >= m_previousRxBytes
        && txBytes >= m_previousTxBytes && elapsedMs > 0) {
        m_downloadBytesPerSecond = (rxBytes - m_previousRxBytes) * 1000.0 / elapsedMs;
        m_uploadBytesPerSecond = (txBytes - m_previousTxBytes) * 1000.0 / elapsedMs;
    }
    m_previousRxBytes = rxBytes;
    m_previousTxBytes = txBytes;
    appendHistory(m_downloadHistory, m_downloadBytesPerSecond / 1024.0);
    appendHistory(m_uploadHistory, m_uploadBytesPerSecond / 1024.0);
}

void BtopModel::readProcessStats()
{
    QList<BtopProcessRow> processes;
    QHash<int, quint64> currentTicks;
    m_totalProcesses = 0;

#ifdef Q_OS_LINUX
    const QStringList entries = QDir(QStringLiteral("/proc"))
                                    .entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    const long pageSize = sysconf(_SC_PAGESIZE);
    const int cpuCount = std::max(1, static_cast<int>(m_coreUsage.size()));
    for (const QString &entry : entries) {
        bool ok = false;
        const int pid = entry.toInt(&ok);
        if (!ok || pid <= 0)
            continue;

        const QString stat = readFirstLine(QStringLiteral("/proc/%1/stat").arg(pid));
        const int commandStart = stat.indexOf('(');
        const int commandEnd = stat.lastIndexOf(')');
        if (commandStart < 0 || commandEnd <= commandStart)
            continue;
        const QStringList fields = stat.mid(commandEnd + 2).split(' ', Qt::SkipEmptyParts);
        if (fields.size() < 22)
            continue;

        ++m_totalProcesses;
        BtopProcessRow process;
        process.pid = pid;
        process.program = stat.mid(commandStart + 1, commandEnd - commandStart - 1);
        process.threads = fields.at(17).toInt();
        process.cpuTicks = fields.at(11).toULongLong() + fields.at(12).toULongLong();
        currentTicks.insert(pid, process.cpuTicks);
        const quint64 previousTicks = m_previousProcessTicks.value(pid);
        if (previousTicks != 0 && process.cpuTicks >= previousTicks && m_cpuDelta != 0) {
            process.cpuPercent = std::clamp((process.cpuTicks - previousTicks) * 100.0
                                                * cpuCount / m_cpuDelta,
                                            0.0, cpuCount * 100.0);
        }

        const qint64 residentPages = fields.at(21).toLongLong();
        process.memoryBytes = residentPages > 0 ? residentPages * pageSize : 0;
        process.memoryText = formatBytes(process.memoryBytes);
        process.user = QFileInfo(QStringLiteral("/proc/%1").arg(pid)).owner();

        QFile commandFile(QStringLiteral("/proc/%1/cmdline").arg(pid));
        if (commandFile.open(QIODevice::ReadOnly)) {
            QByteArray commandLine = commandFile.readAll();
            commandLine.replace('\0', ' ');
            process.command = QString::fromLocal8Bit(commandLine).trimmed();
        }
        if (process.command.isEmpty())
            process.command = QStringLiteral("[%1]").arg(process.program);
        processes.append(process);
    }
#else
    QProcess processList;
    processList.start(QStringLiteral("/bin/ps"),
                      {QStringLiteral("-axo"),
                       QStringLiteral("pid=,comm=,user=,%cpu=,%mem=")});
    if (processList.waitForFinished(1000)) {
        const QStringList lines = QString::fromLocal8Bit(processList.readAllStandardOutput())
                                      .split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList fields = line.simplified().split(' ');
            if (fields.size() < 5)
                continue;
            BtopProcessRow process;
            process.pid = fields.at(0).toInt();
            process.program = QFileInfo(fields.at(1)).fileName();
            process.command = fields.at(1);
            process.user = fields.at(2);
            process.cpuPercent = fields.at(3).toDouble();
            process.memoryBytes = static_cast<quint64>(fields.at(4).toDouble()
                                                        * m_memoryTotal / 100.0);
            process.memoryText = formatBytes(process.memoryBytes);
            processes.append(process);
        }
        m_totalProcesses = processes.size();
    }
#endif

    m_previousProcessTicks = currentTicks;
    beginResetModel();
    m_processes = processes;
    sortProcesses();
    if (m_processes.size() > kMaximumVisibleProcesses)
        m_processes = m_processes.mid(0, kMaximumVisibleProcesses);
    endResetModel();
}

void BtopModel::sortProcesses()
{
    std::sort(m_processes.begin(), m_processes.end(), [this](const BtopProcessRow &left,
                                                              const BtopProcessRow &right) {
        switch (m_sortMode) {
        case SortMode::Cpu:
            if (!qFuzzyCompare(left.cpuPercent, right.cpuPercent))
                return left.cpuPercent > right.cpuPercent;
            break;
        case SortMode::Memory:
            if (left.memoryBytes != right.memoryBytes)
                return left.memoryBytes > right.memoryBytes;
            break;
        case SortMode::Pid:
            return left.pid < right.pid;
        }
        return left.pid < right.pid;
    });
}
