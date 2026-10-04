#include "LinuxNetworkProvider.h"

#include <QFile>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QtEndian>

LinuxNetworkProvider::LinuxNetworkProvider(QObject *parent)
    : NetworkProvider(5000, parent) {
    setCanConfigure(false);
    refresh();
}

void LinuxNetworkProvider::refresh() {
    const QHash<QString, QString> gateways = defaultGateways();

    QVariantList interfaces;
    const auto all = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &interface : all) {
        if (interface.flags() & QNetworkInterface::IsLoopBack)
            continue;

        QString ipv4;
        int prefix = 0;
        const auto entries = interface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                ipv4 = entry.ip().toString();
                prefix = entry.prefixLength();
                break;
            }
        }

        interfaces.append(QVariantMap{
            {"name", interface.humanReadableName()},
            {"up", bool(interface.flags() & QNetworkInterface::IsRunning)},
            {"method", "—"},
            {"ipv4", ipv4},
            {"prefix", prefix},
            {"gateway", gateways.value(interface.humanReadableName())},
            {"mac", interface.hardwareAddress().toLower()},
        });
    }
    setInterfaces(std::move(interfaces));
}

void LinuxNetworkProvider::applyIpv4(const QString &name, const QString &,
                                     const QString &, int, const QString &) {
    emit applyFinished(name, false,
                       QStringLiteral("configuration backend not available"));
}

QHash<QString, QString> LinuxNetworkProvider::defaultGateways() {
    // /proc/net/route: Iface Destination Gateway ... in little-endian hex;
    // destination 00000000 marks a default route.
    QHash<QString, QString> gateways;
    QFile route(QStringLiteral("/proc/net/route"));
    if (!route.open(QIODevice::ReadOnly | QIODevice::Text))
        return gateways;
    const QStringList lines =
        QString::fromUtf8(route.readAll()).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines.mid(1)) {
        const QStringList fields = line.simplified().split(' ');
        if (fields.size() < 3 || fields.at(1) != QStringLiteral("00000000"))
            continue;
        bool ok = false;
        const quint32 raw = fields.at(2).toUInt(&ok, 16);
        // The hex dump is the in-memory little-endian view of the
        // network-byte-order address; swap to QHostAddress host order.
        if (ok && raw != 0)
            gateways.insert(fields.at(0), QHostAddress(qbswap(raw)).toString());
    }
    return gateways;
}
