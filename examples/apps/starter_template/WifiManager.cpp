#include "WifiManager.h"
#include <QProcess>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QTime>
#include <QRegularExpression>
#include <QDebug>

namespace {
QString runQuiet(const QString &program, const QStringList &arguments, int timeoutMs = 2000, int *exitCode = nullptr)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::SeparateChannels);
    proc.start(program, arguments);
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(200);
        if (exitCode) *exitCode = -1;
        return QString();
    }
    if (exitCode) *exitCode = proc.exitCode();
    return QString::fromUtf8(proc.readAllStandardOutput());
}

void executeQuiet(const QString &program, const QStringList &arguments, int timeoutMs = 1500)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::SeparateChannels);
    proc.start(program, arguments);
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(100);
    }
}
} // namespace

WifiManager::WifiManager(QObject *parent)
    : QObject(parent)
{
    m_interface = detectWirelessInterface();
    addLog(QString("Hardware initialized. Active wireless interface: %1").arg(m_interface));

    connect(&m_pollTimer, &QTimer::timeout, this, &WifiManager::checkConnectionStatus);
    m_pollTimer.start(3000);

    connect(&m_periodicScanTimer, &QTimer::timeout, this, &WifiManager::periodicScan);
    m_periodicScanTimer.start(18000); // Periodic background Wi-Fi scan every 18s

    connect(&m_sysInfoTimer, &QTimer::timeout, this, &WifiManager::updateSystemDiagnostics);
    m_sysInfoTimer.start(2000); // Update CPU/RAM/Uptime every 2s

    ensureInterfaceUp();
    ensureWpaSupplicantRunning();

    updateSystemDiagnostics();
    refreshStatus();
    handleScanFinished(); // Load cached networks immediately
    scan();
}

void WifiManager::addLog(const QString &message)
{
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    m_logs.prepend(QString("[%1] %2").arg(timestamp, message));
    if (m_logs.size() > 25) {
        m_logs.removeLast();
    }
    emit logsChanged();
}

void WifiManager::clearLogs()
{
    m_logs.clear();
    emit logsChanged();
}

QString WifiManager::detectWirelessInterface()
{
    if (QFile::exists("/sys/class/net/mlan0")) return "mlan0";
    if (QFile::exists("/sys/class/net/wlan0")) return "wlan0";

    QDir netDir("/sys/class/net");
    QStringList entries = netDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &iface : entries) {
        if (QFile::exists(QString("/sys/class/net/%1/wireless").arg(iface)) ||
            QFile::exists(QString("/sys/class/net/%1/phy80211").arg(iface)) ||
            iface.startsWith("mlan") || iface.startsWith("wlan")) {
            return iface;
        }
    }

    QString out = runQuiet("iw", QStringList() << "dev", 800);
    if (!out.isEmpty()) {
        QRegularExpression rx(R"(Interface\s+([a-zA-Z0-9_-]+))");
        auto match = rx.match(out);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }

    return "mlan0";
}

void WifiManager::ensureInterfaceUp()
{
    if (QFile::exists("/dev/rfkill")) {
        executeQuiet("rfkill", QStringList() << "unblock" << "wifi");
        executeQuiet("rfkill", QStringList() << "unblock" << "all");
    }
    executeQuiet("ip", QStringList() << "link" << "set" << m_interface << "up");

    // Dual-stack: make sure IPv6 is enabled and SLAAC accepts router
    // advertisements on the wireless interface.
    const auto writeSysctl = [](const QString &path, const char *value) {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly))
            file.write(value);
    };
    writeSysctl(QString("/proc/sys/net/ipv6/conf/%1/disable_ipv6").arg(m_interface), "0");
    writeSysctl(QString("/proc/sys/net/ipv6/conf/%1/accept_ra").arg(m_interface), "2");
}

void WifiManager::ensureWpaSupplicantRunning()
{
    QString ctrlSocket = QString("/var/run/wpa_supplicant/%1").arg(m_interface);
    if (!QFile::exists(ctrlSocket)) {
        QDir().mkpath("/var/run/wpa_supplicant");
        QDir().mkpath("/etc/wpa_supplicant");

        QString confPath = QString("/etc/wpa_supplicant/wpa_supplicant-%1.conf").arg(m_interface);
        if (!QFile::exists(confPath)) {
            QFile file(confPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << "ctrl_interface=/var/run/wpa_supplicant\n";
                out << "ctrl_interface_group=0\n";
                out << "update_config=1\n";
            }
        }

        executeQuiet("wpa_supplicant", QStringList()
            << "-B" << "-qq"
            << "-i" << m_interface
            << "-c" << confPath
            << "-C" << "/var/run/wpa_supplicant");
    }
}

void WifiManager::periodicScan()
{
    // When connected to Wi-Fi, stop auto-discovery scanning
    if (m_connected) {
        return;
    }

    if (!m_scanning) {
        scan();
    }
}

void WifiManager::scan()
{
    if (m_scanning)
        return;

    m_interface = detectWirelessInterface();
    ensureInterfaceUp();
    ensureWpaSupplicantRunning();

    m_scanning = true;
    m_statusMessage = QString("Scanning wireless networks on %1...").arg(m_interface);
    emit scanningChanged();
    emit statusChanged();

    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int, QProcess::ExitStatus) {
        process->deleteLater();
        QTimer::singleShot(2500, this, &WifiManager::handleScanFinished);
        QTimer::singleShot(5000, this, &WifiManager::handleScanFinished);
    });

    process->start("wpa_cli", QStringList() << "-i" << m_interface << "scan");
    if (!process->waitForStarted(500)) {
        process->deleteLater();
        QTimer::singleShot(2500, this, &WifiManager::handleScanFinished);
        QTimer::singleShot(5000, this, &WifiManager::handleScanFinished);
    }
}

void WifiManager::handleScanFinished()
{
    QVariantList newNetworks;

    // 1. Try wpa_cli scan_results
    QString wpaOutput = runQuiet("wpa_cli", QStringList() << "-i" << m_interface << "scan_results", 1500);
    if (!wpaOutput.isEmpty()) {
        parseWpaCliScanOutput(wpaOutput, newNetworks);
    }

    // 2. If wpa_cli had no results, try direct iw dev scan
    if (newNetworks.isEmpty()) {
        QString iwOutput = runQuiet("iw", QStringList() << "dev" << m_interface << "scan", 3000);
        if (!iwOutput.isEmpty()) {
            parseIwScanOutput(iwOutput, newNetworks);
        }
    }

    m_networks = newNetworks;
    emit networksChanged();

    m_scanning = false;
    if (m_networks.isEmpty()) {
        m_statusMessage = QString("No wireless networks found on %1").arg(m_interface);
    } else {
        m_statusMessage = QString("Found %1 wireless network(s)").arg(m_networks.size());
    }
    emit scanningChanged();
    emit statusChanged();
}

void WifiManager::parseWpaCliScanOutput(const QString &output, QVariantList &outList)
{
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (int i = 1; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split('\t');
        if (parts.size() >= 5) {
            QString bssid = parts[0];
            int freq = parts[1].toInt();
            int level = parts[2].toInt(); // dBm
            QString flags = parts[3];
            QString ssid = parts[4];

            if (ssid.isEmpty() || ssid == "\\x00") continue;

            int signalPct = qBound(0, 2 * (level + 100), 100);

            QString security = "Open";
            if (flags.contains("WPA3") || flags.contains("SAE"))
                security = "WPA3";
            else if (flags.contains("WPA2"))
                security = "WPA2";
            else if (flags.contains("WPA"))
                security = "WPA";
            else if (flags.contains("WEP"))
                security = "WEP";

            QVariantMap net;
            net["ssid"] = ssid;
            net["bssid"] = bssid;
            net["signal"] = signalPct;
            net["frequency"] = freq;
            net["security"] = security;
            net["connected"] = (ssid == m_currentSsid && m_connected);
            outList.append(net);
        }
    }
}

void WifiManager::parseIwScanOutput(const QString &output, QVariantList &outList)
{
    QStringList bssBlocks = output.split("BSS ", Qt::SkipEmptyParts);
    for (const QString &block : bssBlocks) {
        QStringList lines = block.split('\n');
        QString bssid = lines.value(0).split('(').value(0).trimmed();
        QString ssid;
        int signalPct = 50;
        int freq = 2412;
        QString security = "Open";

        for (const QString &rawLine : lines) {
            QString line = rawLine.trimmed();
            if (line.startsWith("SSID:")) {
                ssid = line.mid(5).trimmed();
            } else if (line.startsWith("freq:")) {
                freq = line.mid(5).trimmed().toInt();
            } else if (line.startsWith("signal:")) {
                double dbm = line.mid(7).split(' ').value(0).toDouble();
                signalPct = qBound(0, static_cast<int>(2 * (dbm + 100)), 100);
            } else if (line.contains("RSN:") || line.contains("WPA2")) {
                security = "WPA2";
            } else if (line.contains("WPA3") || line.contains("SAE")) {
                security = "WPA3";
            } else if (line.contains("WPA:")) {
                if (security == "Open") security = "WPA";
            }
        }

        if (!ssid.isEmpty() && ssid != "\\x00") {
            QVariantMap net;
            net["ssid"] = ssid;
            net["bssid"] = bssid;
            net["signal"] = signalPct;
            net["frequency"] = freq;
            net["security"] = security;
            net["connected"] = (ssid == m_currentSsid && m_connected);
            outList.append(net);
        }
    }
}

void WifiManager::connectToNetwork(const QString &ssid, const QString &password)
{
    if (ssid.isEmpty()) {
        m_lastError = "No target SSID specified";
        addLog("[ERROR] " + m_lastError);
        emit errorChanged();
        return;
    }

    m_lastError.clear();
    emit errorChanged();

    m_statusMessage = QString("Connecting to '%1'...").arg(ssid);
    addLog(QString("Initiating connection to '%1'...").arg(ssid));
    emit statusChanged();

    ensureInterfaceUp();
    ensureWpaSupplicantRunning();

    // 1. Remove previous network profiles
    executeQuiet("wpa_cli", QStringList() << "-i" << m_interface << "remove_network" << "all");

    // 2. Add new network profile via wpa_cli
    QString netIdRaw = runQuiet("wpa_cli", QStringList() << "-i" << m_interface << "add_network", 1500).trimmed();
    QString netId = "0";
    for (const QString &line : netIdRaw.split('\n', Qt::SkipEmptyParts)) {
        QString trimmed = line.trimmed();
        bool ok = false;
        int val = trimmed.toInt(&ok);
        if (ok && val >= 0) {
            netId = trimmed;
            break;
        }
    }

    // 3. Configure SSID and PSK
    addLog(QString("Configured profile %1 (SSID: '%2')").arg(netId, ssid));
    executeQuiet("wpa_cli", QStringList() << "-i" << m_interface << "set_network" << netId << "ssid" << QString("\"%1\"").arg(ssid));
    if (password.isEmpty()) {
        executeQuiet("wpa_cli", QStringList() << "-i" << m_interface << "set_network" << netId << "key_mgmt" << "NONE");
    } else {
        executeQuiet("wpa_cli", QStringList() << "-i" << m_interface << "set_network" << netId << "psk" << QString("\"%1\"").arg(password));
    }

    // 4. Enable and select
    addLog("Selected network profile and began 4-way authentication...");
    executeQuiet("wpa_cli", QStringList() << "-i" << m_interface << "enable_network" << netId);
    executeQuiet("wpa_cli", QStringList() << "-i" << m_interface << "select_network" << netId);
    executeQuiet("wpa_cli", QStringList() << "-i" << m_interface << "save_config");

    m_currentSsid = ssid;
    emit connectedChanged();
    emit statusChanged();

    // 5. Poll connection progress and trigger DHCP via udhcpc
    QTimer::singleShot(2500, this, [this]() {
        addLog("Requesting addresses (IPv4: udhcpc, IPv6: SLAAC/DHCPv6)...");
        requestAddresses();
        refreshStatus();
    });

    QTimer::singleShot(5000, this, [this, ssid]() {
        requestAddresses();
        refreshStatus();
        if (m_connected) {
            addLog(QString("[SUCCESS] Connected to '%1' | IPv4: %2 | IPv6: %3")
                       .arg(ssid, m_ipAddress, m_globalIp6.isEmpty() ? QStringLiteral("-") : m_globalIp6));
            emit connectionResult(true, QString("Connected to %1").arg(ssid));
        } else {
            m_lastError = QString("Association timeout for '%1' - verify passphrase").arg(ssid);
            addLog("[WARNING] " + m_lastError);
            emit errorChanged();
            emit connectionResult(false, m_lastError);
        }
    });
}

void WifiManager::disconnectNetwork()
{
    executeQuiet("wpa_cli", QStringList() << "-i" << m_interface << "disconnect");
    executeQuiet("wpa_cli", QStringList() << "-i" << m_interface << "remove_network" << "all");
    m_connected = false;
    m_currentSsid.clear();
    m_statusMessage = "Disconnected from wireless network";
    addLog("Wireless connection disconnected by user request");
    emit connectedChanged();
    emit statusChanged();

    // Trigger immediate scan when returning to disconnected state
    scan();
}

void WifiManager::refreshStatus()
{
    updateNetworkInterfaces();
    checkConnectionStatus();
}

void WifiManager::checkConnectionStatus()
{
    QString output = runQuiet("wpa_cli", QStringList() << "-i" << m_interface << "status", 800);
    bool wasConnected = m_connected;
    bool isConnected = false;
    QString currentSsid;

    if (!output.isEmpty()) {
        QStringList lines = output.split('\n');
        for (const QString &line : lines) {
            if (line.startsWith("wpa_state=COMPLETED")) {
                isConnected = true;
            } else if (line.startsWith("ssid=")) {
                currentSsid = line.mid(5).trimmed();
            }
        }
    }

    // Live link quality of the active association (RSSI dBm -> percent),
    // shown by the connected view's signal meter.
    int signal = 0;
    if (isConnected) {
        const QString poll = runQuiet("wpa_cli", QStringList() << "-i" << m_interface << "signal_poll", 800);
        static const QRegularExpression rssiRx(QStringLiteral("RSSI=(-?\\d+)"));
        const QRegularExpressionMatch match = rssiRx.match(poll);
        if (match.hasMatch())
            signal = qBound(0, 2 * (match.captured(1).toInt() + 100), 100);
    }
    if (signal != m_currentSignal) {
        m_currentSignal = signal;
        emit statusChanged();
    }

    if (isConnected != wasConnected || currentSsid != m_currentSsid) {
        m_connected = isConnected;
        m_currentSsid = currentSsid;
        emit connectedChanged();
        emit statusChanged();

        if (m_connected) {
            requestAddresses();
        }
    }

    updateNetworkInterfaces();

    if (m_connected && m_ipAddress == "-") {
        requestAddresses();
    }
}

void WifiManager::requestAddresses()
{
    // 1. Tell systemd-networkd to reconfigure interface
    executeQuiet("networkctl", QStringList() << "reconfigure" << m_interface, 500);

    // 2. Launch background udhcpc daemon completely silent with stdout/stderr redirected to /dev/null
    const QString pidFile = QString("/var/run/udhcpc.%1.pid").arg(m_interface);
    if (!QFile::exists(pidFile)) {
        const QString cmd = QString("udhcpc -i %1 -b -q -p %2 -t 8 -A 3 -x hostname:%3 >/dev/null 2>&1")
            .arg(m_interface, pidFile, m_hostname);
        QProcess::startDetached("sh", QStringList() << "-c" << cmd);
    }
}

void WifiManager::updateNetworkInterfaces()
{
    QVariantList ifaceList;
    QString wifiIp = "-";
    QString primaryIp = "-";
    QString globalIp6 = QString();

    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        QString name = iface.name();
        if (name == "lo") continue; // Exclude loopback from visual list

        bool isUp = iface.flags().testFlag(QNetworkInterface::IsUp);
        bool isRunning = iface.flags().testFlag(QNetworkInterface::IsRunning);
        QString mac = iface.hardwareAddress();
        QString ip = "-";
        QString ip6 = "-";
        bool hasGlobalIp6 = false;

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress addr = entry.ip();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                if (ip == "-")
                    ip = addr.toString();
            } else if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
                if (addr.isLinkLocal()) {
                    if (ip6 == "-")
                        ip6 = addr.toString(); // fallback: link-local
                } else if (!hasGlobalIp6) {
                    ip6 = addr.toString();
                    hasGlobalIp6 = true;
                }
            }
        }

        QString type = "ethernet";
        QString displayName = QString("Ethernet (%1)").arg(name);

        if (name.startsWith("mlan") || name.startsWith("wlan")) {
            type = "wifi";
            displayName = QString("Wi-Fi (%1)").arg(name);
            if (ip != "-") wifiIp = ip;
            if (hasGlobalIp6) globalIp6 = ip6; // Wi-Fi wins as deploy fallback
        } else if (name.startsWith("end") || name.startsWith("eth")) {
            type = "ethernet";
            displayName = QString("Ethernet (%1)").arg(name);
            if (primaryIp == "-" && ip != "-") primaryIp = ip;
            if (globalIp6.isEmpty() && hasGlobalIp6) globalIp6 = ip6;
        }

        QVariantMap ifaceMap;
        ifaceMap["name"] = name;
        ifaceMap["displayName"] = displayName;
        ifaceMap["type"] = type;
        ifaceMap["ip"] = ip;
        ifaceMap["ip6"] = ip6;
        ifaceMap["hasGlobalIp6"] = hasGlobalIp6;
        ifaceMap["mac"] = mac;
        ifaceMap["isUp"] = isUp;
        ifaceMap["hasLink"] = isRunning;
        ifaceMap["connected"] = (ip != "-") || hasGlobalIp6;

        ifaceList.append(ifaceMap);
    }

    m_allInterfaces = ifaceList;
    m_globalIp6 = globalIp6;
    emit interfacesChanged();

    if (wifiIp != "-") {
        m_ipAddress = wifiIp;
    } else if (primaryIp != "-") {
        m_ipAddress = primaryIp;
    } else {
        m_ipAddress = "-";
    }
}

void WifiManager::updateSystemDiagnostics()
{
    // 1. Hostname
    QFile hostFile("/etc/hostname");
    if (hostFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_hostname = QString::fromUtf8(hostFile.readAll()).trimmed();
    }

    // 2. Uptime
    QFile upFile("/proc/uptime");
    if (upFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        double seconds = QString::fromUtf8(upFile.readAll()).split(' ').value(0).toDouble();
        int mins = static_cast<int>(seconds) / 60;
        int hours = mins / 60;
        int m = mins % 60;
        int s = static_cast<int>(seconds) % 60;
        if (hours > 0) {
            m_uptime = QString("%1h %2m").arg(hours).arg(m);
        } else {
            m_uptime = QString("%1m %2s").arg(m).arg(s);
        }
    }

    // 3. Memory Info
    QFile memFile("/proc/meminfo");
    if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(memFile.readAll());
        long totalKb = 0, availKb = 0;
        for (const QString &line : content.split('\n')) {
            if (line.startsWith("MemTotal:")) {
                totalKb = line.split(' ', Qt::SkipEmptyParts).value(1).toLong();
            } else if (line.startsWith("MemAvailable:")) {
                availKb = line.split(' ', Qt::SkipEmptyParts).value(1).toLong();
            }
        }
        if (totalKb > 0) {
            long usedMb = (totalKb - availKb) / 1024;
            long totalMb = totalKb / 1024;
            m_memoryInfo = QString("%1 / %2 MB").arg(usedMb).arg(totalMb);
        }
    }

    // 4. Load Average
    QFile loadFile("/proc/loadavg");
    if (loadFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_cpuLoad = QString::fromUtf8(loadFile.readAll()).split(' ').value(0).trimmed();
    }

    emit systemInfoChanged();

    updateSshStatus();
}

bool WifiManager::asciiConsole() const
{
    // Emoji only render on real terminal emulators; the framebuffer and
    // serial consoles use fixed fonts without them. QTUI_SIMPLE_GLYPHS=1/0
    // forces either mode.
    const QByteArray forced = qgetenv("QTUI_SIMPLE_GLYPHS");
    if (!forced.isEmpty())
        return forced != "0";
    const QByteArray term = qgetenv("TERM");
    return term.isEmpty() || term == "linux" || term == "dumb" || term.startsWith("vt");
}

void WifiManager::updateSshStatus()
{
    // Detect a running SSH server (OpenSSH or dropbear) by scanning /proc,
    // so it works without pidof/ps on minimal images.
    QString serverName;
    const QStringList pids = QDir("/proc").entryList(QStringList() << "[0-9]*", QDir::Dirs);
    for (const QString &pid : pids) {
        QFile commFile("/proc/" + pid + "/comm");
        if (!commFile.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const QString comm = QString::fromUtf8(commFile.readAll()).trimmed();
        if (comm == "sshd" || comm == "sshd-session") {
            serverName = "sshd";
            break;
        }
        if (comm == "dropbear") {
            serverName = "dropbear";
            break;
        }
    }

    if (serverName != m_sshServerName) {
        m_sshServerName = serverName;
        emit sshInfoChanged();
    }
}
