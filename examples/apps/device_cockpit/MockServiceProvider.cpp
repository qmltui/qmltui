#include "MockServiceProvider.h"

#include <QTimer>

namespace {

QVariantMap unit(const QString &name, const QString &description,
                 const QString &active, const QString &sub) {
    return {{"unit", name},
            {"description", description},
            {"load", "loaded"},
            {"active", active},
            {"sub", sub}};
}

} // namespace

MockServiceProvider::MockServiceProvider(QObject *parent)
    : ServiceProvider(0, parent) {
    setCanManage(true);
    setAllUnits({
        unit("chronyd.service", "NTP client/server", "failed", "failed"),
        unit("device-agent.service", "Device telemetry agent", "active", "running"),
        unit("fw-updater.service", "Firmware update service", "inactive", "dead"),
        unit("getty@tty1.service", "Getty on tty1", "active", "running"),
        unit("modbus-poller.service", "Modbus RTU poller", "inactive", "dead"),
        unit("mosquitto.service", "Mosquitto MQTT broker", "active", "running"),
        unit("sshd.service", "OpenSSH server daemon", "active", "running"),
        unit("systemd-networkd.service", "Network configuration", "active", "running"),
        unit("systemd-timesyncd.service", "Network time synchronization", "active", "running"),
        unit("watchdog-feeder.service", "Hardware watchdog feeder", "active", "running"),
    });
}

void MockServiceProvider::startUnit(const QString &unit) { applyAction(unit, "start"); }
void MockServiceProvider::stopUnit(const QString &unit) { applyAction(unit, "stop"); }
void MockServiceProvider::restartUnit(const QString &unit) { applyAction(unit, "restart"); }

void MockServiceProvider::applyAction(const QString &unit, const QString &action) {
    QTimer::singleShot(400, this, [this, unit, action]() {
        bool found = false;
        QVariantList updated = m_allUnits;
        for (QVariant &entry : updated) {
            QVariantMap map = entry.toMap();
            if (map.value("unit").toString() != unit)
                continue;
            found = true;
            if (action == "stop") {
                map.insert("active", "inactive");
                map.insert("sub", "dead");
            } else {
                map.insert("active", "active");
                map.insert("sub", "running");
            }
            entry = map;
            break;
        }
        if (found)
            setAllUnits(std::move(updated));
        emit actionFinished(unit, action, found,
                            found ? QStringLiteral("%1: %2 succeeded").arg(unit, action)
                                  : QStringLiteral("%1: no such unit").arg(unit));
    });
}
