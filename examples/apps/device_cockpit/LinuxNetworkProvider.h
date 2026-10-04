#pragma once

#include "NetworkProvider.h"

// Observes the live system through QNetworkInterface and /proc/net/route.
// Read-only in Milestone 1: configuration apply (NetworkManager or
// systemd-networkd backend) is planned in DESIGN.md.
class LinuxNetworkProvider : public NetworkProvider {
    Q_OBJECT

public:
    explicit LinuxNetworkProvider(QObject *parent = nullptr);

    void applyIpv4(const QString &name, const QString &method,
                   const QString &address, int prefix,
                   const QString &gateway) override;

protected slots:
    void refresh() override;

private:
    static QHash<QString, QString> defaultGateways();
};
