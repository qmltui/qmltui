#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <QPair>
#include <qqmlregistration.h>
#include <functional>

class WifiManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Wi-Fi State
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString currentSsid READ currentSsid NOTIFY statusChanged)
    Q_PROPERTY(QString ipAddress READ ipAddress NOTIFY statusChanged)
    Q_PROPERTY(int currentSignal READ currentSignal NOTIFY statusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(QString interfaceName READ interfaceName NOTIFY statusChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)
    Q_PROPERTY(QStringList logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(QVariantList networks READ networks NOTIFY networksChanged)

    // Multi-Interface Network Diagnostics ("Boot to Qtui")
    Q_PROPERTY(QVariantList allInterfaces READ allInterfaces NOTIFY interfacesChanged)
    Q_PROPERTY(QString hostname READ hostname NOTIFY systemInfoChanged)
    Q_PROPERTY(QString uptime READ uptime NOTIFY systemInfoChanged)
    Q_PROPERTY(QString memoryInfo READ memoryInfo NOTIFY systemInfoChanged)
    Q_PROPERTY(QString cpuLoad READ cpuLoad NOTIFY systemInfoChanged)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(bool asciiConsole READ asciiConsole CONSTANT)

    // Remote access for app deployment
    Q_PROPERTY(bool sshRunning READ sshRunning NOTIFY sshInfoChanged)
    Q_PROPERTY(QString sshServerName READ sshServerName NOTIFY sshInfoChanged)
    Q_PROPERTY(QString deployIp READ deployIp NOTIFY interfacesChanged)

public:
    explicit WifiManager(QObject *parent = nullptr);

    bool scanning() const { return m_scanning; }
    bool connected() const { return m_connected; }
    QString currentSsid() const { return m_currentSsid; }
    QString ipAddress() const { return m_ipAddress; }
    int currentSignal() const { return m_currentSignal; }
    QString statusMessage() const { return m_statusMessage; }
    QString interfaceName() const { return m_interface; }
    QString lastError() const { return m_lastError; }
    QStringList logs() const { return m_logs; }
    QVariantList networks() const { return m_networks; }

    QVariantList allInterfaces() const { return m_allInterfaces; }
    QString hostname() const { return m_hostname; }
    QString uptime() const { return m_uptime; }
    QString memoryInfo() const { return m_memoryInfo; }
    QString cpuLoad() const { return m_cpuLoad; }
    QString qtVersion() const { return QString::fromLatin1(qVersion()); }
    bool asciiConsole() const;

    bool sshRunning() const { return !m_sshServerName.isEmpty(); }
    QString sshServerName() const { return m_sshServerName; }
    QString deployIp() const {
        // IPv4 first for the shortest ssh command, else a global IPv6.
        return m_ipAddress != "-" ? m_ipAddress : m_globalIp6;
    }

    Q_INVOKABLE void scan();
    Q_INVOKABLE void connectToNetwork(const QString &ssid, const QString &password);
    Q_INVOKABLE void disconnectNetwork();
    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void clearLogs();

signals:
    void scanningChanged();
    void connectedChanged();
    void statusChanged();
    void errorChanged();
    void logsChanged();
    void networksChanged();
    void interfacesChanged();
    void systemInfoChanged();
    void sshInfoChanged();
    void connectionResult(bool success, const QString &message);

private slots:
    void handleScanFinished();
    void checkConnectionStatus();
    void periodicScan();
    void updateSystemDiagnostics();

private:
    enum class BackendMode { Live, Observe, Mock };
    using Command = QPair<QString, QStringList>;
    using CommandCallback = std::function<void(int, const QString &, const QString &)>;

    QString detectWirelessInterface();
    void prepareWifiStack(std::function<void(bool)> callback);
    void applyWifiPowerPolicy();
    void runCommand(const QString &program, const QStringList &arguments,
                    int timeoutMs, CommandCallback callback = {});
    void runCommandSequence(const QList<Command> &commands,
                            std::function<void(bool)> callback, int index = 0);
    void applyConnectionStatus(bool connected, const QString &ssid, int signal);
    void finishScan(const QVariantList &networks);
    void failOperation(const QString &message);
    bool allowsMutation() const { return m_backendMode == BackendMode::Live; }
    void parseIwScanOutput(const QString &output, QVariantList &outList);
    void parseWpaCliScanOutput(const QString &output, QVariantList &outList);
    void updateNetworkInterfaces();
    void requestAddresses();
    void updateSshStatus();
    void addLog(const QString &message);

    BackendMode m_backendMode = BackendMode::Live;
    bool m_operationActive = false;
    bool m_statusCheckActive = false;
    bool m_initialStatusPending = true;
    int m_scanResultAttempts = 0;
    QString m_pendingConnectionSsid;

    bool m_scanning = false;
    bool m_connected = false;
    QString m_interface = "mlan0";
    QString m_currentSsid;
    int m_currentSignal = 0;
    QString m_ipAddress = "-";
    QString m_statusMessage = "Ready";
    QString m_lastError;
    QStringList m_logs;
    QVariantList m_networks;
    QVariantList m_allInterfaces;

    QString m_hostname = "embedded-device";
    QString m_uptime = "0m";
    QString m_memoryInfo = "0 MB / 0 MB";
    QString m_cpuLoad = "0.00";
    QString m_sshServerName;
    QString m_globalIp6;

    QTimer m_pollTimer;
    QTimer m_periodicScanTimer;
    QTimer m_sysInfoTimer;
};
