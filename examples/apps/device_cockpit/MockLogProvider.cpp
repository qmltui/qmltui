#include "MockLogProvider.h"

MockLogProvider::MockLogProvider(QObject *parent) : LogProvider(parent) {
    setAvailable(true);

    const QDateTime base(QDate(2026, 1, 1), QTime(9, 14, 0));
    appendLine(base.addSecs(0), 6, "systemd[1]", "Started Device telemetry agent.");
    appendLine(base.addSecs(1), 6, "device-agent[312]", "connected to mqtt://gateway:1883");
    appendLine(base.addSecs(2), 6, "device-agent[312]", "telemetry batch published (42 points)");
    appendLine(base.addSecs(3), 4, "chronyd[201]", "no reachable NTP sources");
    appendLine(base.addSecs(4), 6, "sshd[440]", "Accepted publickey for admin from 192.168.7.10");
    appendLine(base.addSecs(5), 3, "chronyd[201]", "exiting: could not step system clock");
    appendLine(base.addSecs(6), 6, "systemd[1]", "chronyd.service: Failed with result 'exit-code'.");
    appendLine(base.addSecs(7), 6, "modbus-poller[0]", "service is not enabled");

    connect(&m_timer, &QTimer::timeout, this, &MockLogProvider::emitNext);
    m_timer.start(900);
}

void MockLogProvider::emitNext() {
    ++m_tick;
    const QDateTime now = QDateTime::currentDateTime();
    switch (m_tick % 5) {
    case 0:
        appendLine(now, 4, "watchdog-feeder[98]",
                   QStringLiteral("feed latency %1 ms").arg(20 + m_tick % 7));
        break;
    case 1:
        appendLine(now, 6, "device-agent[312]",
                   QStringLiteral("telemetry batch published (%1 points)").arg(40 + m_tick % 5));
        break;
    case 2:
        appendLine(now, 6, "mosquitto[220]",
                   QStringLiteral("client cockpit-%1 connected").arg(m_tick));
        break;
    case 3:
        appendLine(now, 6, "kernel", "can0: bus-off recovered");
        break;
    default:
        appendLine(now, 6, "device-agent[312]", "sensor scan complete");
        break;
    }
}
