#include "BtopModel.h"
#include <Qtui/Core/primitives/ColorMapper.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QSysInfo>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iterator>

#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef Q_OS_LINUX
#include <dirent.h>
#endif

#ifdef Q_OS_MACOS
#include <ifaddrs.h>
#include <mach/mach.h>
#include <net/if.h>
#include <net/if_dl.h>
#endif

namespace {
constexpr int kHistoryLength = 120;
constexpr int kMaximumVisibleProcesses = 256;
} // namespace

using namespace qtui;

QHash<uid_t, QString> BtopModel::s_uidCache;

BtopCpuGraph::BtopCpuGraph(QObject *parent) : Item(parent) {}

void BtopCpuGraph::setValues(const QVariantList &values) {
  if (m_values == values)
    return;
  m_values = values;
  emit valuesChanged();
  markDirty();
}

void BtopCpuGraph::setColor(Color color) {
  if (m_color == color)
    return;
  m_color = color;
  m_resolvedColor = ColorMapper::fromString(color, 0xFFFFFFFF);
  emit colorChanged();
  markDirty();
}

void BtopCpuGraph::setBackgroundColor(Color color) {
  if (m_backgroundColor == color)
    return;
  m_backgroundColor = color;
  m_resolvedBackgroundColor = ColorMapper::fromString(color, 0x00000000);
  emit backgroundColorChanged();
  markDirty();
}

void BtopCpuGraph::paint(TextGridBuffer &buffer, int absX, int absY,
                         const ClipRect &) const {
  if (!visible() || width() <= 0 || height() <= 0 || m_values.isEmpty())
    return;

  const int cellWidth = width();
  const int cellHeight = height();
  const int pixelW = cellWidth * 2;
  const int pixelH = cellHeight * 4;

  static const auto brailleTable = []() {
    std::array<QByteArray, 256> table;
    for (int i = 0; i < 256; ++i) {
      char bytes[3] = {static_cast<char>(0xE2),
                       static_cast<char>(0xA0 | (i >> 6)),
                       static_cast<char>(0x80 | (i & 0x3F))};
      table[i] = QByteArray(bytes, 3);
    }
    return table;
  }();

  constexpr uint8_t masks[2][4] = {{0x01, 0x02, 0x04, 0x40},
                                   {0x08, 0x10, 0x20, 0x80}};

  std::vector<uint8_t> cells(static_cast<size_t>(cellWidth * cellHeight), 0);

  const int visibleSamples = std::min<int>(m_values.size(), cellWidth);
  const int firstCell = cellWidth - visibleSamples;
  const int firstPixel = pixelW - cellWidth * 2;

  for (int sample = 0; sample < visibleSamples; ++sample) {
    double value = 0.0;
    if (m_values.size() <= cellWidth) {
      value = m_values.at(sample).toDouble();
    } else {
      const int bucketStart =
          sample * static_cast<int>(m_values.size()) / visibleSamples;
      const int bucketEnd = std::max(
          bucketStart + 1,
          (sample + 1) * static_cast<int>(m_values.size()) / visibleSamples);
      double total = 0.0;
      for (int idx = bucketStart; idx < bucketEnd; ++idx)
        total += m_values.at(idx).toDouble();
      value = total / (bucketEnd - bucketStart);
    }

    value = std::clamp(value, 0.0, 100.0);
    const int x = firstPixel + (firstCell + sample) * 2;
    int top = (pixelH - 1) / 2;
    int bottom = top;
    if (value > 0.0) {
      const int inset = std::min(
          (pixelH - 1) / 2,
          static_cast<int>(std::round((100.0 - value) * pixelH / 200.0)));
      top = inset;
      bottom = pixelH - 1 - inset;
    }

    for (int col = 0; col < 2; ++col) {
      const int curX = x + col;
      const int cX = curX / 2;
      const int dotX = curX % 2;
      if (cX < 0 || cX >= cellWidth)
        continue;

      for (int y = top; y <= bottom; ++y) {
        if (y < 0 || y >= pixelH)
          continue;
        const int cY = y / 4;
        const int dotY = y % 4;
        if (cY < 0 || cY >= cellHeight)
          continue;
        cells[static_cast<size_t>(cY * cellWidth + cX)] |= masks[dotX][dotY];
      }
    }
  }

  for (int cY = 0; cY < cellHeight; ++cY) {
    for (int cX = 0; cX < cellWidth; ++cX) {
      const uint8_t val = cells[static_cast<size_t>(cY * cellWidth + cX)];
      if (val > 0) {
        Cell *cell = buffer.tryCellAt(absX + cX, absY + cY);
        if (cell) {
          cell->setCharacter(brailleTable[val]);
          cell->fgColor = m_resolvedColor;
          if (m_backgroundColor.isValid())
            cell->bgColor = m_resolvedBackgroundColor;
          cell->displayWidth = 1;
          cell->continuation = false;
        }
      }
    }
  }
}

BtopModel::BtopModel(QObject *parent)
    : QAbstractTableModel(parent), m_hostName(QSysInfo::machineHostName()),
      m_userName(qEnvironmentVariable("USER", QStringLiteral("user"))),
      m_osName(QSysInfo::prettyProductName()) {
  if (m_hostName.isEmpty())
    m_hostName = qEnvironmentVariable("HOSTNAME", QStringLiteral("localhost"));

  m_networkClock.start();
  connect(&m_timer, &QTimer::timeout, this, &BtopModel::refresh);
  m_timer.start(m_refreshInterval);
  refresh();
}

int BtopModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : m_processes.size();
}

int BtopModel::columnCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : 7;
}

QVariant BtopModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || role != Qt::DisplayRole ||
      index.row() >= m_processes.size())
    return {};

  const BtopProcessRow &process = m_processes.at(index.row());
  switch (index.column()) {
  case 0:
    return process.pid;
  case 1:
    return process.program;
  case 2:
    return process.command;
  case 3:
    return process.threads;
  case 4:
    return process.user;
  case 5:
    return process.memoryText;
  case 6:
    return QString::number(process.cpuPercent, 'f', 1);
  default:
    return {};
  }
}

QVariant BtopModel::headerData(int section, Qt::Orientation orientation,
                               int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    return {};
  static const QStringList headers = {
      QStringLiteral("PID"),     QStringLiteral("Program"),
      QStringLiteral("Command"), QStringLiteral("Thr"),
      QStringLiteral("User"),    QStringLiteral("Mem"),
      QStringLiteral("Cpu%")};
  return section >= 0 && section < headers.size() ? headers.at(section)
                                                  : QVariant();
}

QVariantList BtopModel::columnDefinitions() const {
  return {QVariantMap{{"title", "PID"}, {"width", 8}, {"align", "left"}, {"alignment", "left"}},
          QVariantMap{{"title", "Program"}, {"width", 15}, {"align", "left"}, {"alignment", "left"}},
          QVariantMap{{"title", "Command"}, {"width", 24}, {"align", "left"}, {"alignment", "left"}},
          QVariantMap{{"title", "Thr"}, {"width", 6}, {"align", "left"}, {"alignment", "left"}},
          QVariantMap{{"title", "User"}, {"width", 9}, {"align", "left"}, {"alignment", "left"}},
          QVariantMap{{"title", "Mem"}, {"width", 10}, {"align", "right"}, {"alignment", "right"}},
          QVariantMap{{"title", "Cpu%"}, {"width", 7}, {"align", "right"}, {"alignment", "right"}}};
}

void BtopModel::togglePaused() {
  m_paused = !m_paused;
  emit pausedChanged();
  if (!m_paused)
    refresh();
}

void BtopModel::sortByCpu() {
  if (m_sortMode == SortMode::Cpu)
    return;
  m_sortMode = SortMode::Cpu;
  beginResetModel();
  sortProcesses();
  endResetModel();
  emit sortModeChanged();
}

void BtopModel::sortByMemory() {
  if (m_sortMode == SortMode::Memory)
    return;
  m_sortMode = SortMode::Memory;
  beginResetModel();
  sortProcesses();
  endResetModel();
  emit sortModeChanged();
}

void BtopModel::sortByPid() {
  if (m_sortMode == SortMode::Pid)
    return;
  m_sortMode = SortMode::Pid;
  beginResetModel();
  sortProcesses();
  endResetModel();
  emit sortModeChanged();
}

void BtopModel::cycleRefreshInterval() {
  static constexpr int intervals[] = {500, 1000, 2000, 5000};
  auto it =
      std::find(std::begin(intervals), std::end(intervals), m_refreshInterval);
  const auto next =
      it == std::end(intervals) || std::next(it) == std::end(intervals)
          ? std::begin(intervals)
          : std::next(it);
  m_refreshInterval = *next;
  m_timer.setInterval(m_refreshInterval);
  emit refreshIntervalChanged();
}

void BtopModel::refresh() {
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
QString BtopModel::memoryTotalText() const {
  return formatBytes(m_memoryTotal);
}
QString BtopModel::memoryAvailableText() const {
  return formatBytes(m_memoryAvailable);
}
QString BtopModel::memoryCachedText() const {
  return formatBytes(m_memoryCached);
}
QString BtopModel::swapUsedText() const { return formatBytes(m_swapUsed); }
QString BtopModel::swapTotalText() const { return formatBytes(m_swapTotal); }
QString BtopModel::diskUsedText() const { return formatBytes(m_diskUsed); }
QString BtopModel::diskTotalText() const { return formatBytes(m_diskTotal); }
QString BtopModel::downloadRateText() const {
  return formatBytes(m_downloadBytesPerSecond) + QStringLiteral("/s");
}
QString BtopModel::uploadRateText() const {
  return formatBytes(m_uploadBytesPerSecond) + QStringLiteral("/s");
}

QString BtopModel::sortMode() const {
  switch (m_sortMode) {
  case SortMode::Cpu:
    return QStringLiteral("cpu");
  case SortMode::Memory:
    return QStringLiteral("memory");
  case SortMode::Pid:
    return QStringLiteral("pid");
  }
  return QStringLiteral("cpu");
}

double BtopModel::percent(quint64 part, quint64 total) {
  return total == 0 ? 0.0 : std::clamp(part * 100.0 / total, 0.0, 100.0);
}

QString BtopModel::formatBytes(double bytes) {
  static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
  int unit = 0;
  while (bytes >= 1024.0 && unit < 4) {
    bytes /= 1024.0;
    ++unit;
  }
  const int precision = unit == 0 ? 0 : (bytes < 10.0 ? 2 : 1);
  return QStringLiteral("%1 %2")
      .arg(bytes, 0, 'f', precision)
      .arg(QString::fromLatin1(units[unit]));
}

QString BtopModel::formatBrailleHistory(const QVariantList &history,
                                        int cellWidth) {
  static constexpr int leftMasks[] = {0x40, 0x40, 0x44, 0x46, 0x47};
  static constexpr int rightMasks[] = {0x80, 0x80, 0xA0, 0xB0, 0xB8};
  const int sampleCount = cellWidth * 2;
  const int firstSample =
      std::max(0, static_cast<int>(history.size()) - sampleCount);
  const int leftPadding =
      std::max(0, sampleCount - static_cast<int>(history.size()));
  QString graph;
  graph.reserve(cellWidth);
  for (int cell = 0; cell < cellWidth; ++cell) {
    const int leftSlot = cell * 2 - leftPadding;
    const int rightSlot = leftSlot + 1;
    const int leftIndex = firstSample + leftSlot;
    const int rightIndex = firstSample + rightSlot;
    const double leftValue = leftSlot >= 0 && leftIndex < history.size()
                                 ? history.at(leftIndex).toDouble()
                                 : 0.0;
    const double rightValue = rightSlot >= 0 && rightIndex < history.size()
                                  ? history.at(rightIndex).toDouble()
                                  : 0.0;
    const int leftLevel =
        std::clamp(static_cast<int>(std::ceil(leftValue / 25.0)), 1, 4);
    const int rightLevel =
        std::clamp(static_cast<int>(std::ceil(rightValue / 25.0)), 1, 4);
    graph.append(QChar(0x2800 + leftMasks[leftLevel] + rightMasks[rightLevel]));
  }
  return graph;
}

void BtopModel::appendHistory(QVariantList &history, double value) {
  history.append(value);
  while (history.size() > kHistoryLength)
    history.removeFirst();
}

QString BtopModel::usernameForUid(uid_t uid) {
  auto it = s_uidCache.find(uid);
  if (it != s_uidCache.end())
    return it.value();

  struct passwd pwd;
  struct passwd *result = nullptr;
  char buf[1024];
  if (getpwuid_r(uid, &pwd, buf, sizeof(buf), &result) == 0 && result &&
      pwd.pw_name) {
    QString name = QString::fromUtf8(pwd.pw_name);
    s_uidCache.insert(uid, name);
    return name;
  }
  QString name = QString::number(uid);
  s_uidCache.insert(uid, name);
  return name;
}

void BtopModel::readCpuStats() {
#ifdef Q_OS_LINUX
  int fd = ::open("/proc/stat", O_RDONLY);
  if (fd >= 0) {
    char buf[8192];
    ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
    ::close(fd);
    if (n > 0) {
      buf[n] = '\0';
      QVector<quint64> totals;
      QVector<quint64> idles;
      const char *line = buf;
      while (line && *line) {
        if (strncmp(line, "cpu", 3) != 0)
          break;
        const char *p = line + 3;
        while (*p && *p != ' ' && *p != '\n')
          ++p; // skip core suffix (e.g. '0', '1')
        quint64 lineTotal = 0;
        quint64 lineIdle = 0;
        int field = 0;
        while (*p && *p != '\n') {
          while (*p == ' ')
            ++p;
          if (*p == '\n' || !*p)
            break;
          char *end = nullptr;
          unsigned long long val = strtoull(p, &end, 10);
          if (p == end)
            break;
          ++field;
          lineTotal += val;
          if (field == 4 || field == 5) // idle or iowait
            lineIdle += val;
          p = end;
        }
        if (field >= 4) {
          totals.append(lineTotal);
          idles.append(lineIdle);
        }

        const char *nextLine = strchr(line, '\n');
        line = nextLine ? nextLine + 1 : nullptr;
      }

      if (!totals.isEmpty()) {
        m_cpuDelta = totals.first() - m_previousCpuTotal;
        const quint64 idleDelta = idles.first() - m_previousCpuIdle;
        if (m_previousCpuTotal != 0 && m_cpuDelta != 0)
          m_cpuUsage = std::clamp((m_cpuDelta - idleDelta) * 100.0 / m_cpuDelta,
                                  0.0, 100.0);
        m_previousCpuTotal = totals.first();
        m_previousCpuIdle = idles.first();

        const int coreCount = totals.size() - 1;
        QVariantList cores;
        if (m_previousCoreTotals.size() != coreCount) {
          m_previousCoreTotals.fill(0, coreCount);
          m_previousCoreIdle.fill(0, coreCount);
        }
        for (int i = 0; i < coreCount; ++i) {
          const quint64 totalDelta =
              totals.at(i + 1) - m_previousCoreTotals.at(i);
          const quint64 coreIdleDelta =
              idles.at(i + 1) - m_previousCoreIdle.at(i);
          double usage = 0.0;
          if (m_previousCoreTotals.at(i) != 0 && totalDelta != 0)
            usage = std::clamp(
                (totalDelta - coreIdleDelta) * 100.0 / totalDelta, 0.0, 100.0);
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
  }

  char loadBuf[128];
  int loadFd = ::open("/proc/loadavg", O_RDONLY);
  if (loadFd >= 0) {
    ssize_t n = ::read(loadFd, loadBuf, sizeof(loadBuf) - 1);
    ::close(loadFd);
    if (n > 0) {
      loadBuf[n] = '\0';
      char *p1 = strchr(loadBuf, ' ');
      char *p2 = p1 ? strchr(p1 + 1, ' ') : nullptr;
      char *p3 = p2 ? strchr(p2 + 1, ' ') : nullptr;
      if (p3)
        *p3 = '\0';
      m_loadAverageText = QString::fromLatin1(loadBuf);
    }
  }

  char upBuf[64];
  int upFd = ::open("/proc/uptime", O_RDONLY);
  if (upFd >= 0) {
    ssize_t n = ::read(upFd, upBuf, sizeof(upBuf) - 1);
    ::close(upFd);
    if (n > 0) {
      upBuf[n] = '\0';
      double uptimeSeconds = strtod(upBuf, nullptr);
      const int days = static_cast<int>(uptimeSeconds / 86400.0);
      const int hours = static_cast<int>(uptimeSeconds / 3600.0) % 24;
      const int minutes = static_cast<int>(uptimeSeconds / 60.0) % 60;
      const int seconds = static_cast<int>(uptimeSeconds) % 60;
      m_uptimeText = days > 0 ? QStringLiteral("%1d %2:%3:%4")
                                    .arg(days)
                                    .arg(hours, 2, 10, QLatin1Char('0'))
                                    .arg(minutes, 2, 10, QLatin1Char('0'))
                                    .arg(seconds, 2, 10, QLatin1Char('0'))
                              : QStringLiteral("%1:%2:%3")
                                    .arg(hours, 2, 10, QLatin1Char('0'))
                                    .arg(minutes, 2, 10, QLatin1Char('0'))
                                    .arg(seconds, 2, 10, QLatin1Char('0'));
    }
  }

  int cpuInfoFd = ::open("/proc/cpuinfo", O_RDONLY);
  if (cpuInfoFd >= 0) {
    char cpuInfoBuf[8192];
    ssize_t n = ::read(cpuInfoFd, cpuInfoBuf, sizeof(cpuInfoBuf) - 1);
    ::close(cpuInfoFd);
    if (n > 0) {
      cpuInfoBuf[n] = '\0';
      if (m_cpuNameText.isEmpty()) {
        const char *model = strstr(cpuInfoBuf, "model name");
        if (model) {
          const char *colon = strchr(model, ':');
          if (colon) {
            const char *end = strchr(colon, '\n');
            size_t len = end ? (end - colon - 1) : strlen(colon + 1);
            m_cpuNameText =
                QString::fromUtf8(colon + 1, static_cast<int>(len)).trimmed();
            m_cpuNameText.remove(
                QRegularExpression(QStringLiteral("\\s+CPU(?:\\s+@.*)?$")));
            m_cpuNameText.replace(QStringLiteral("(R)"), QString());
            m_cpuNameText.replace(QStringLiteral("(TM)"), QString());
            m_cpuNameText = m_cpuNameText.simplified();
            m_cpuNameText.remove(
                QRegularExpression(QStringLiteral("^AMD\\s+")));
          }
        }
      }

      double frequencyTotal = 0.0;
      int frequencyCount = 0;
      const char *cur = cpuInfoBuf;
      while (cur && (cur = strstr(cur, "cpu MHz")) != nullptr) {
        const char *colon = strchr(cur, ':');
        if (colon) {
          frequencyTotal += strtod(colon + 1, nullptr);
          ++frequencyCount;
        }
        cur = strchr(cur, '\n');
      }
      m_cpuFrequencyText =
          frequencyCount == 0
              ? QStringLiteral("n/a")
              : QStringLiteral("%1 GHz").arg(
                    frequencyTotal / frequencyCount / 1000.0, 0, 'f', 2);
    }
  }
  if (m_cpuNameText.isEmpty())
    m_cpuNameText = QSysInfo::currentCpuArchitecture();
#elif defined(Q_OS_MACOS)
  host_cpu_load_info_data_t loadInfo{};
  mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
  if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                      reinterpret_cast<host_info_t>(&loadInfo),
                      &count) == KERN_SUCCESS) {
    const quint64 idle = loadInfo.cpu_ticks[CPU_STATE_IDLE];
    quint64 total = 0;
    for (int state = 0; state < CPU_STATE_MAX; ++state)
      total += loadInfo.cpu_ticks[state];
    m_cpuDelta = total - m_previousCpuTotal;
    const quint64 idleDelta = idle - m_previousCpuIdle;
    if (m_previousCpuTotal != 0 && m_cpuDelta != 0)
      m_cpuUsage =
          std::clamp((m_cpuDelta - idleDelta) * 100.0 / m_cpuDelta, 0.0, 100.0);
    m_previousCpuTotal = total;
    m_previousCpuIdle = idle;
    m_coreUsage = {m_cpuUsage};
    QVariantList history = m_coreHistoryValues.isEmpty()
                               ? QVariantList{}
                               : m_coreHistoryValues.first();
    appendHistory(history, m_cpuUsage);
    m_coreHistoryValues = {history};
    m_coreHistoryText = {formatBrailleHistory(history)};
  }
  double loads[3]{};
  getloadavg(loads, 3);
  m_loadAverageText = QStringLiteral("%1 %2 %3")
                          .arg(loads[0], 0, 'f', 2)
                          .arg(loads[1], 0, 'f', 2)
                          .arg(loads[2], 0, 'f', 2);
  m_uptimeText = QStringLiteral("macOS");
  m_cpuNameText =
      QStringLiteral("Apple %1").arg(QSysInfo::currentCpuArchitecture());
  m_cpuFrequencyText = QStringLiteral("Apple CPU");
#else
  m_loadAverageText = QStringLiteral("n/a");
  m_uptimeText = QStringLiteral("n/a");
  m_cpuNameText = QSysInfo::currentCpuArchitecture();
  m_cpuFrequencyText = QStringLiteral("n/a");
#endif
  appendHistory(m_cpuHistory, m_cpuUsage);
}

void BtopModel::readMemoryStats() {
#ifdef Q_OS_LINUX
  int fd = ::open("/proc/meminfo", O_RDONLY);
  if (fd >= 0) {
    char buf[4096];
    ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
    ::close(fd);
    if (n > 0) {
      buf[n] = '\0';
      auto getVal = [](const char *str, const char *key) -> quint64 {
        const char *p = strstr(str, key);
        if (!p)
          return 0;
        p += strlen(key);
        while (*p == ' ' || *p == ':')
          ++p;
        return strtoull(p, nullptr, 10) * 1024ULL;
      };
      m_memoryTotal = getVal(buf, "MemTotal:");
      m_memoryAvailable = getVal(buf, "MemAvailable:");
      m_memoryCached = getVal(buf, "Cached:") + getVal(buf, "SReclaimable:");
      m_memoryUsed = m_memoryTotal > m_memoryAvailable
                         ? m_memoryTotal - m_memoryAvailable
                         : 0;
      m_swapTotal = getVal(buf, "SwapTotal:");
      const quint64 swapFree = getVal(buf, "SwapFree:");
      m_swapUsed = m_swapTotal > swapFree ? m_swapTotal - swapFree : 0;
    }
  }
#elif defined(Q_OS_MACOS)
  vm_statistics64_data_t vmStats{};
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  vm_size_t pageSize = 0;
  host_page_size(mach_host_self(), &pageSize);
  if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                        reinterpret_cast<host_info64_t>(&vmStats),
                        &count) == KERN_SUCCESS) {
    m_memoryAvailable =
        (vmStats.free_count + vmStats.inactive_count) * pageSize;
    m_memoryCached = vmStats.purgeable_count * pageSize;
    const quint64 used = (vmStats.active_count + vmStats.wire_count +
                          vmStats.compressor_page_count) *
                         pageSize;
    m_memoryUsed = used;
    m_memoryTotal = used + m_memoryAvailable;
  }
#endif
}

void BtopModel::readDiskStats() {
  QStorageInfo rootStorage = QStorageInfo::root();
  rootStorage.refresh();
  m_diskTotal = rootStorage.bytesTotal();
  const quint64 available = rootStorage.bytesAvailable();
  m_diskUsed = m_diskTotal > available ? m_diskTotal - available : 0;
}

void BtopModel::readNetworkStats() {
  quint64 rxBytes = 0;
  quint64 txBytes = 0;
  QString selectedInterface;
#ifdef Q_OS_LINUX
  int fd = ::open("/proc/net/dev", O_RDONLY);
  if (fd >= 0) {
    char buf[4096];
    ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
    ::close(fd);
    if (n > 0) {
      buf[n] = '\0';
      const char *line = strchr(buf, '\n');
      if (line)
        line = strchr(line + 1, '\n'); // Skip 2 header lines
      if (line)
        ++line;
      while (line && *line) {
        while (*line == ' ')
          ++line;
        const char *colon = strchr(line, ':');
        if (!colon)
          break;
        char ifName[32];
        size_t nameLen = std::min<size_t>(sizeof(ifName) - 1,
                                          static_cast<size_t>(colon - line));
        memcpy(ifName, line, nameLen);
        ifName[nameLen] = '\0';

        if (strcmp(ifName, "lo") != 0 && nameLen > 0) {
          const char *p = colon + 1;
          char *end = nullptr;
          unsigned long long rx = strtoull(p, &end, 10);
          if (p != end) {
            p = end;
            for (int f = 0; f < 7 && *p; ++f) {
              while (*p == ' ')
                ++p;
              strtoull(p, &end, 10);
              p = end;
            }
            while (*p == ' ')
              ++p;
            unsigned long long tx = strtoull(p, &end, 10);
            rxBytes += rx;
            txBytes += tx;
            if (selectedInterface.isEmpty())
              selectedInterface = QString::fromLatin1(ifName);
          }
        }
        const char *nextLine = strchr(colon, '\n');
        line = nextLine ? nextLine + 1 : nullptr;
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
  if (m_previousRxBytes != 0 && rxBytes >= m_previousRxBytes &&
      txBytes >= m_previousTxBytes && elapsedMs > 0) {
    m_downloadBytesPerSecond =
        (rxBytes - m_previousRxBytes) * 1000.0 / elapsedMs;
    m_uploadBytesPerSecond = (txBytes - m_previousTxBytes) * 1000.0 / elapsedMs;
  }
  m_previousRxBytes = rxBytes;
  m_previousTxBytes = txBytes;
  appendHistory(m_downloadHistory, m_downloadBytesPerSecond / 1024.0);
  appendHistory(m_uploadHistory, m_uploadBytesPerSecond / 1024.0);
}

void BtopModel::readProcessStats() {
  QList<BtopProcessRow> processes;
  QHash<int, quint64> currentTicks;
  m_totalProcesses = 0;

#ifdef Q_OS_LINUX
  DIR *procDir = opendir("/proc");
  if (procDir) {
    const long pageSize = sysconf(_SC_PAGESIZE);
    const int cpuCount = std::max(1, static_cast<int>(m_coreUsage.size()));
    struct dirent *entry = nullptr;

    while ((entry = readdir(procDir)) != nullptr) {
      if (!isdigit(entry->d_name[0]))
        continue;
      const int pid = atoi(entry->d_name);
      if (pid <= 0)
        continue;

      char statPath[64];
      snprintf(statPath, sizeof(statPath), "/proc/%d/stat", pid);
      int fd = ::open(statPath, O_RDONLY);
      if (fd < 0)
        continue;

      char buf[1024];
      ssize_t bytesRead = ::read(fd, buf, sizeof(buf) - 1);
      ::close(fd);
      if (bytesRead <= 0)
        continue;
      buf[bytesRead] = '\0';

      const char *openParen = strchr(buf, '(');
      const char *closeParen = strrchr(buf, ')');
      if (!openParen || !closeParen || closeParen <= openParen)
        continue;

      const char *p = closeParen + 1;
      if (*p == ' ')
        ++p;
      while (*p && *p != ' ')
        ++p; // skip state (field 3)
      while (*p == ' ')
        ++p;

      quint64 utime = 0, stime = 0;
      int threads = 1;
      qint64 residentPages = 0;
      bool valid = true;

      for (int field = 4; field <= 24 && *p; ++field) {
        char *end = nullptr;
        unsigned long long val = strtoull(p, &end, 10);
        if (p == end) {
          valid = false;
          break;
        }
        if (field == 14)
          utime = val;
        else if (field == 15)
          stime = val;
        else if (field == 20)
          threads = static_cast<int>(val);
        else if (field == 24)
          residentPages = static_cast<qint64>(val);
        p = end;
        while (*p == ' ')
          ++p;
      }
      if (!valid)
        continue;

      ++m_totalProcesses;
      BtopProcessRow process;
      process.pid = pid;
      process.threads = threads;
      process.cpuTicks = utime + stime;
      currentTicks.insert(pid, process.cpuTicks);

      const quint64 previousTicks = m_previousProcessTicks.value(pid);
      if (previousTicks != 0 && process.cpuTicks >= previousTicks &&
          m_cpuDelta != 0) {
        process.cpuPercent = std::clamp((process.cpuTicks - previousTicks) *
                                            100.0 * cpuCount / m_cpuDelta,
                                        0.0, cpuCount * 100.0);
      }

      process.memoryBytes = residentPages > 0 ? residentPages * pageSize : 0;
      process.memoryText = formatBytes(process.memoryBytes);

      auto staticIt = m_processStaticCache.find(pid);
      if (staticIt != m_processStaticCache.end()) {
        process.program = staticIt->program;
        process.command = staticIt->command;
        process.user = staticIt->user;
      } else {
        process.program = QString::fromUtf8(
            openParen + 1, static_cast<int>(closeParen - openParen - 1));

        char cmdPath[64];
        snprintf(cmdPath, sizeof(cmdPath), "/proc/%d/cmdline", pid);
        int cmdFd = ::open(cmdPath, O_RDONLY);
        if (cmdFd >= 0) {
          char cmdBuf[2048];
          ssize_t cmdBytes = ::read(cmdFd, cmdBuf, sizeof(cmdBuf) - 1);
          ::close(cmdFd);
          if (cmdBytes > 0) {
            for (ssize_t i = 0; i < cmdBytes - 1; ++i) {
              if (cmdBuf[i] == '\0')
                cmdBuf[i] = ' ';
            }
            cmdBuf[cmdBytes] = '\0';
            process.command = QString::fromLocal8Bit(cmdBuf).trimmed();
          }
        }
        if (process.command.isEmpty())
          process.command = QStringLiteral("[%1]").arg(process.program);

        char pidPath[64];
        snprintf(pidPath, sizeof(pidPath), "/proc/%d", pid);
        struct stat st;
        if (stat(pidPath, &st) == 0) {
          process.user = usernameForUid(st.st_uid);
        } else {
          process.user = QStringLiteral("?");
        }

        m_processStaticCache.insert(
            pid, {process.program, process.command, process.user});
      }

      processes.append(process);
    }
    closedir(procDir);

    for (auto it = m_processStaticCache.begin();
         it != m_processStaticCache.end();) {
      if (!currentTicks.contains(it.key()))
        it = m_processStaticCache.erase(it);
      else
        ++it;
    }
  }
#else
  QProcess processList;
  processList.start(
      QStringLiteral("/bin/ps"),
      {QStringLiteral("-axo"), QStringLiteral("pid=,comm=,user=,%cpu=,%mem=")});
  if (processList.waitForFinished(1000)) {
    const QStringList lines =
        QString::fromLocal8Bit(processList.readAllStandardOutput())
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
      process.memoryBytes =
          static_cast<quint64>(fields.at(4).toDouble() * m_memoryTotal / 100.0);
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

void BtopModel::sortProcesses() {
  std::sort(m_processes.begin(), m_processes.end(),
            [this](const BtopProcessRow &left, const BtopProcessRow &right) {
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
