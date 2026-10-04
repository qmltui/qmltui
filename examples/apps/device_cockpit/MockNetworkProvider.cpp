#include "MockNetworkProvider.h"

#include <QTimer>

namespace {

QVariantMap nic(const QString &name, bool up, const QString &method,
                const QString &ipv4, int prefix, const QString &gateway,
                const QString &mac) {
    return {{"name", name},   {"up", up},         {"method", method},
            {"ipv4", ipv4},   {"prefix", prefix}, {"gateway", gateway},
            {"mac", mac}};
}

} // namespace

MockNetworkProvider::MockNetworkProvider(QObject *parent)
    : NetworkProvider(0, parent) {
    setCanConfigure(true);
    setInterfaces({
        nic("eth0", true, "dhcp", "192.168.7.2", 24, "192.168.7.1",
            "02:42:c0:a8:07:02"),
        nic("can0", true, "—", QString(), 0, QString(), QString()),
        nic("wlan0", false, "dhcp", QString(), 0, QString(),
            "04:e8:b9:11:22:33"),
    });
}

void MockNetworkProvider::applyIpv4(const QString &name, const QString &method,
                                    const QString &address, int prefix,
                                    const QString &gateway) {
    QTimer::singleShot(300, this, [this, name, method, address, prefix, gateway]() {
        bool found = false;
        QVariantList updated = m_interfaces;
        for (QVariant &entry : updated) {
            QVariantMap map = entry.toMap();
            if (map.value("name").toString() != name)
                continue;
            found = true;
            map.insert("method", method);
            if (method == "static") {
                map.insert("ipv4", address);
                map.insert("prefix", prefix);
                map.insert("gateway", gateway);
            } else {
                // Simulated DHCP lease.
                map.insert("ipv4", "192.168.7.2");
                map.insert("prefix", 24);
                map.insert("gateway", "192.168.7.1");
            }
            entry = map;
            break;
        }
        if (found)
            setInterfaces(std::move(updated));
        emit applyFinished(name, found,
                           found ? QStringLiteral("%1: %2 configuration applied")
                                       .arg(name, method)
                                 : QStringLiteral("%1: no such interface").arg(name));
    });
}
