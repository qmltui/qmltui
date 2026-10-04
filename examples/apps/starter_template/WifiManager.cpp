#include "WifiManager.h"
#include <QProcess>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QTime>
#include <QRegularExpression>
#include <memory>

WifiManager::WifiManager(QObject *parent)
    : QObject(parent)
{
    const QByteArray requestedBackend = qgetenv("QTUI_STARTER_BACKEND").trimmed().toLower();
    if (requestedBackend == "observe")
        m_backendMode = BackendMode::Observe;
    else if (requestedBackend == "mock")
        m_backendMode = BackendMode::Mock;

    m_interface = detectWirelessInterface();
    addLog(QString("Hardware initialized. Active wireless interface: %1").arg(m_interface));

    connect(&m_pollTimer, &QTimer::timeout, this, &WifiManager::checkConnectionStatus);
    m_pollTimer.start(3000);

    connect(&m_periodicScanTimer, &QTimer::timeout, this, &WifiManager::periodicScan);
    m_periodicScanTimer.start(18000); // Periodic background Wi-Fi scan every 18s

    connect(&m_sysInfoTimer, &QTimer::timeout, this, &WifiManager::updateSystemDiagnostics);
    m_sysInfoTimer.start(2000); // Update CPU/RAM/Uptime every 2s

    updateSystemDiagnostics();

    if (m_backendMode == BackendMode::Mock) {
        m_networks = {
            QVariantMap{{"ssid", "Qtui Lab"}, {"signal", 88}, {"frequency", 5180},
                        {"security", "WPA2"}, {"connected", false}},
            QVariantMap{{"ssid", "Device Setup"}, {"signal", 64}, {"frequency", 2412},
                        {"security", "WPA2"}, {"connected", false}}
        };
        m_statusMessage = QStringLiteral("Mock network backend ready");
        emit networksChanged();
        emit statusChanged();
        return;
    }

    if (allowsMutation()) {
        m_operationActive = true;
        prepareWifiStack([this](bool success) {
            m_operationActive = false;
            if (!success)
                addLog(QStringLiteral("[WARNING] Wi-Fi stack preparation was incomplete"));
            applyWifiPowerPolicy();
            refreshStatus();
        });
        return;
    }

    QTimer::singleShot(0, this, &WifiManager::refreshStatus);
}

void WifiManager::runCommand(const QString &program, const QStringList &arguments,
                             int timeoutMs, CommandCallback callback)
{
    auto *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    auto completed = std::make_shared<bool>(false);
    auto complete = [process, completed, callback = std::move(callback)](int exitCode) mutable {
        if (*completed)
            return;
        *completed = true;
        const QString standardOutput = QString::fromUtf8(process->readAllStandardOutput());
        const QString standardError = QString::fromUtf8(process->readAllStandardError());
        if (callback)
            callback(exitCode, standardOutput, standardError);
        process->deleteLater();
    };

    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [complete](int exitCode, QProcess::ExitStatus) mutable { complete(exitCode); });
    connect(process, &QProcess::errorOccurred, this,
            [complete](QProcess::ProcessError error) mutable {
        if (error == QProcess::FailedToStart)
            complete(-1);
    });
    QTimer::singleShot(timeoutMs, process, [process, complete]() mutable {
        if (process->state() != QProcess::NotRunning)
            process->kill();
        complete(-1);
    });
    process->start(program, arguments);
}

void WifiManager::runCommandSequence(const QList<Command> &commands,
                                     std::function<void(bool)> callback, int index)
{
    if (index >= commands.size()) {
        callback(true);
        return;
    }
    const Command command = commands.at(index);
    runCommand(command.first, command.second, 1500,
               [this, commands, callback = std::move(callback), index]
               (int exitCode, const QString &, const QString &) mutable {
        if (exitCode != 0) {
            callback(false);
            return;
        }
        runCommandSequence(commands, std::move(callback), index + 1);
    });
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

    return "mlan0";
}

void WifiManager::prepareWifiStack(std::function<void(bool)> callback)
{
    QList<Command> commands;
    commands.append({"ip", {"link", "set", m_interface, "up"}});

    // Dual-stack: make sure IPv6 is enabled and SLAAC accepts router
    // advertisements on the wireless interface.
    const auto writeSysctl = [](const QString &path, const char *value) {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly))
            file.write(value);
    };
    writeSysctl(QString("/proc/sys/net/ipv6/conf/%1/disable_ipv6").arg(m_interface), "0");
    writeSysctl(QString("/proc/sys/net/ipv6/conf/%1/accept_ra").arg(m_interface), "2");

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

        commands.append({"wpa_supplicant", {"-B", "-qq", "-i", m_interface,
                                              "-c", confPath, "-C", "/var/run/wpa_supplicant"}});
    }
    if (QFile::exists("/dev/rfkill")) {
        runCommand("rfkill", {"unblock", "all"}, 1000,
                   [this, commands, callback = std::move(callback)]
                   (int, const QString &, const QString &) mutable {
            runCommandSequence(commands, std::move(callback));
        });
    } else {
        runCommandSequence(commands, std::move(callback));
    }
}

void WifiManager::applyWifiPowerPolicy()
{
    if (qEnvironmentVariableIntValue("QTUI_WIFI_LOW_LATENCY") != 1)
        return;
    runCommand("iw", {"dev", m_interface, "set", "power_save", "off"}, 1000,
               [this](int exitCode, const QString &, const QString &error) {
        if (exitCode == 0)
            addLog("Low-latency Wi-Fi profile: power saving disabled");
        else
            addLog(QString("[WARNING] Wi-Fi power policy unsupported: %1").arg(error.trimmed()));
    });
}

void WifiManager::periodicScan()
{
    if (allowsMutation() && !m_connected && !m_scanning && !m_operationActive)
        scan();
}

void WifiManager::scan()
{
    if (!allowsMutation()) {
        m_lastError = QStringLiteral("Network changes are disabled in this backend mode");
        emit errorChanged();
        return;
    }
    if (m_connected || m_scanning || m_operationActive)
        return;

    m_interface = detectWirelessInterface();
    m_operationActive = true;
    m_scanning = true;
    m_scanResultAttempts = 0;
    m_statusMessage = QString("Scanning wireless networks on %1...").arg(m_interface);
    emit scanningChanged();
    emit statusChanged();

    runCommand("wpa_cli", {"-i", m_interface, "scan"}, 1500,
               [this](int exitCode, const QString &, const QString &error) {
        if (exitCode != 0) {
            failOperation(QString("Wi-Fi scan failed: %1").arg(error.trimmed()));
            return;
        }
        QTimer::singleShot(700, this, &WifiManager::handleScanFinished);
    });
}

void WifiManager::handleScanFinished()
{
    runCommand("wpa_cli", {"-i", m_interface, "scan_results"}, 1500,
               [this](int exitCode, const QString &output, const QString &error) {
        QVariantList networks;
        if (exitCode == 0)
            parseWpaCliScanOutput(output, networks);
        if (networks.isEmpty() && ++m_scanResultAttempts < 6) {
            QTimer::singleShot(500, this, &WifiManager::handleScanFinished);
            return;
        }
        if (exitCode != 0) {
            failOperation(QString("Could not read Wi-Fi scan results: %1").arg(error.trimmed()));
            return;
        }
        finishScan(networks);
    });
}

void WifiManager::finishScan(const QVariantList &networks)
{
    m_networks = networks;
    emit networksChanged();

    m_scanning = false;
    m_operationActive = false;
    if (m_networks.isEmpty()) {
        m_statusMessage = QString("No wireless networks found on %1").arg(m_interface);
    } else {
        m_statusMessage = QString("Found %1 wireless network(s)").arg(m_networks.size());
    }
    emit scanningChanged();
    emit statusChanged();
}

void WifiManager::failOperation(const QString &message)
{
    m_operationActive = false;
    if (m_scanning) {
        m_scanning = false;
        emit scanningChanged();
    }
    m_lastError = message;
    m_statusMessage = message;
    addLog("[ERROR] " + message);
    emit errorChanged();
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
    if (!allowsMutation()) {
        m_lastError = QStringLiteral("Network changes are disabled in this backend mode");
        emit errorChanged();
        emit connectionResult(false, m_lastError);
        return;
    }
    if (ssid.isEmpty()) {
        m_lastError = "No target SSID specified";
        addLog("[ERROR] " + m_lastError);
        emit errorChanged();
        return;
    }
    if (m_operationActive)
        return;

    m_operationActive = true;
    m_lastError.clear();
    emit errorChanged();

    m_statusMessage = QString("Connecting to '%1'...").arg(ssid);
    addLog(QString("Initiating connection to '%1'...").arg(ssid));
    emit statusChanged();

    runCommand("wpa_cli", {"-i", m_interface, "remove_network", "all"}, 1500,
               [this, ssid, password](int removeExit, const QString &, const QString &removeError) {
        if (removeExit != 0) {
            failOperation(QString("Could not reset Wi-Fi profiles: %1").arg(removeError.trimmed()));
            emit connectionResult(false, m_lastError);
            return;
        }
        runCommand("wpa_cli", {"-i", m_interface, "add_network"}, 1500,
                   [this, ssid, password](int addExit, const QString &output, const QString &addError) {
            if (addExit != 0) {
                failOperation(QString("Could not create Wi-Fi profile: %1").arg(addError.trimmed()));
                emit connectionResult(false, m_lastError);
                return;
            }
            QString networkId;
            for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
                bool ok = false;
                line.trimmed().toInt(&ok);
                if (ok) {
                    networkId = line.trimmed();
                    break;
                }
            }
            if (networkId.isEmpty()) {
                failOperation(QStringLiteral("wpa_supplicant returned no network identifier"));
                emit connectionResult(false, m_lastError);
                return;
            }

            QList<Command> commands;
            commands.append({"wpa_cli", {"-i", m_interface, "set_network", networkId,
                                           "ssid", QString("\"%1\"").arg(ssid)}});
            if (password.isEmpty())
                commands.append({"wpa_cli", {"-i", m_interface, "set_network", networkId,
                                               "key_mgmt", "NONE"}});
            else
                commands.append({"wpa_cli", {"-i", m_interface, "set_network", networkId,
                                               "psk", QString("\"%1\"").arg(password)}});
            commands.append({"wpa_cli", {"-i", m_interface, "enable_network", networkId}});
            commands.append({"wpa_cli", {"-i", m_interface, "select_network", networkId}});
            commands.append({"wpa_cli", {"-i", m_interface, "save_config"}});

            runCommandSequence(commands, [this, ssid, networkId](bool success) {
                if (!success) {
                    failOperation(QStringLiteral("Could not configure the Wi-Fi profile"));
                    emit connectionResult(false, m_lastError);
                    return;
                }
                addLog(QString("Configured profile %1 and began association with '%2'")
                           .arg(networkId, ssid));
                m_operationActive = false;
                m_pendingConnectionSsid = ssid;
                QTimer::singleShot(1500, this, &WifiManager::checkConnectionStatus);
                QTimer::singleShot(3500, this, &WifiManager::checkConnectionStatus);
                QTimer::singleShot(7000, this, [this, ssid] {
                    if (m_pendingConnectionSsid != ssid)
                        return;
                    m_pendingConnectionSsid.clear();
                    m_lastError = QString("Association timeout for '%1' - verify passphrase").arg(ssid);
                    addLog("[WARNING] " + m_lastError);
                    emit errorChanged();
                    emit connectionResult(false, m_lastError);
                });
            });
        });
    });
}

void WifiManager::disconnectNetwork()
{
    if (!allowsMutation() || m_operationActive)
        return;
    m_operationActive = true;
    const QList<Command> commands = {
        {"wpa_cli", {"-i", m_interface, "disconnect"}},
        {"wpa_cli", {"-i", m_interface, "remove_network", "all"}}
    };
    runCommandSequence(commands, [this](bool success) {
        m_operationActive = false;
        if (!success) {
            failOperation(QStringLiteral("Could not disconnect the wireless network"));
            return;
        }
        m_connected = false;
        m_currentSsid.clear();
        m_pendingConnectionSsid.clear();
        m_statusMessage = "Disconnected from wireless network";
        addLog("Wireless connection disconnected by user request");
        emit connectedChanged();
        emit statusChanged();
        requestAddresses();
        QTimer::singleShot(500, this, &WifiManager::scan);
    });
}

void WifiManager::refreshStatus()
{
    updateNetworkInterfaces();
    checkConnectionStatus();
}

void WifiManager::checkConnectionStatus()
{
    if (m_backendMode == BackendMode::Mock || m_statusCheckActive || m_operationActive)
        return;
    m_statusCheckActive = true;
    runCommand("wpa_cli", {"-i", m_interface, "status"}, 800,
               [this](int exitCode, const QString &output, const QString &) {
        bool connected = false;
        QString ssid;
        if (exitCode == 0) {
            for (const QString &line : output.split('\n')) {
                if (line.startsWith("wpa_state=COMPLETED"))
                    connected = true;
                else if (line.startsWith("ssid="))
                    ssid = line.mid(5).trimmed();
            }
        }
        if (!connected) {
            m_statusCheckActive = false;
            applyConnectionStatus(false, ssid, 0);
            return;
        }
        runCommand("wpa_cli", {"-i", m_interface, "signal_poll"}, 800,
                   [this, ssid](int, const QString &poll, const QString &) {
            static const QRegularExpression rssiRx(QStringLiteral("RSSI=(-?\\d+)"));
            const QRegularExpressionMatch match = rssiRx.match(poll);
            const int signal = match.hasMatch()
                ? qBound(0, 2 * (match.captured(1).toInt() + 100), 100) : 0;
            m_statusCheckActive = false;
            applyConnectionStatus(true, ssid, signal);
        });
    });
}

void WifiManager::requestAddresses()
{
    if (allowsMutation())
        runCommand("networkctl", {"reconfigure", m_interface}, 1000);
}

void WifiManager::applyConnectionStatus(bool connected, const QString &ssid, int signal)
{
    const bool connectionChanged = connected != m_connected || ssid != m_currentSsid;
    const bool signalChanged = signal != m_currentSignal;
    m_connected = connected;
    m_currentSsid = ssid;
    m_currentSignal = signal;
    if (connectionChanged)
        emit connectedChanged();
    if (connectionChanged || signalChanged)
        emit statusChanged();

    updateNetworkInterfaces();
    if (connected && connectionChanged) {
        requestAddresses();
        applyWifiPowerPolicy();
    }
    if (connected && !m_pendingConnectionSsid.isEmpty()) {
        const QString requestedSsid = m_pendingConnectionSsid;
        m_pendingConnectionSsid.clear();
        addLog(QString("[SUCCESS] Connected to '%1' | IPv4: %2 | IPv6: %3")
                   .arg(requestedSsid, m_ipAddress,
                        m_globalIp6.isEmpty() ? QStringLiteral("-") : m_globalIp6));
        emit connectionResult(true, QString("Connected to %1").arg(requestedSsid));
    }
    if (m_initialStatusPending) {
        m_initialStatusPending = false;
        if (!connected && allowsMutation())
            QTimer::singleShot(0, this, &WifiManager::scan);
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
