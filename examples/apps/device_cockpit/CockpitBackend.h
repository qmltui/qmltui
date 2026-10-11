#pragma once

#include "LogProvider.h"
#include "NetworkProvider.h"
#include "ServiceProvider.h"
#include "SystemInfoProvider.h"

#include <QObject>
#include <QQmlEngine>

// Root of the provider layer. Chooses mock or live implementations once at
// startup (QAMUI_COCKPIT_MOCK=1 selects the mocks) and hands them to QML.
class CockpitBackend : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString productName READ productName CONSTANT)
    Q_PROPERTY(bool mockMode READ mockMode CONSTANT)
    Q_PROPERTY(SystemInfoProvider *systemInfo READ systemInfo CONSTANT)
    Q_PROPERTY(ServiceProvider *services READ services CONSTANT)
    Q_PROPERTY(LogProvider *logs READ logs CONSTANT)
    Q_PROPERTY(NetworkProvider *network READ network CONSTANT)

public:
    explicit CockpitBackend(QObject *parent = nullptr);

    static CockpitBackend *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    QString productName() const { return m_productName; }
    bool mockMode() const { return m_mockMode; }
    SystemInfoProvider *systemInfo() const { return m_systemInfo; }
    ServiceProvider *services() const { return m_services; }
    LogProvider *logs() const { return m_logs; }
    NetworkProvider *network() const { return m_network; }

private:
    QString m_productName;
    bool m_mockMode = false;
    SystemInfoProvider *m_systemInfo = nullptr;
    ServiceProvider *m_services = nullptr;
    LogProvider *m_logs = nullptr;
    NetworkProvider *m_network = nullptr;
};
