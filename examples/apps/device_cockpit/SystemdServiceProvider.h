#pragma once

#include "ServiceProvider.h"

#include <QDBusInterface>

// Talks to systemd's manager on the system bus (org.freedesktop.systemd1).
// Falls back to a read-only "unavailable" state when the bus or the manager
// cannot be reached (non-systemd hosts, macOS, missing permissions).
class SystemdServiceProvider : public ServiceProvider {
    Q_OBJECT

public:
    explicit SystemdServiceProvider(QObject *parent = nullptr);

    void startUnit(const QString &unit) override;
    void stopUnit(const QString &unit) override;
    void restartUnit(const QString &unit) override;

protected slots:
    void refresh() override;

private:
    void callUnitAction(const QString &unit, const QString &method,
                        const QString &action);

    QDBusInterface m_manager;
};
