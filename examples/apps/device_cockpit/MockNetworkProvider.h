#pragma once

#include "NetworkProvider.h"

// Fully configurable simulated interfaces so the whole edit/apply flow is
// exercisable and testable without NetworkManager.
class MockNetworkProvider : public NetworkProvider {
    Q_OBJECT

public:
    explicit MockNetworkProvider(QObject *parent = nullptr);

    void applyIpv4(const QString &name, const QString &method,
                   const QString &address, int prefix,
                   const QString &gateway) override;

protected slots:
    void refresh() override {}
};
