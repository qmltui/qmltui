#include "CockpitBackend.h"

#include "JournaldLogProvider.h"
#include "LinuxNetworkProvider.h"
#include "LinuxSystemInfoProvider.h"
#include "MockLogProvider.h"
#include "MockNetworkProvider.h"
#include "MockServiceProvider.h"
#include "MockSystemInfoProvider.h"
#include "SystemdServiceProvider.h"

CockpitBackend::CockpitBackend(QObject *parent)
    : QObject(parent),
      m_productName(QStringLiteral("DEVICE COCKPIT")),
      m_mockMode(qEnvironmentVariableIntValue("QAMUI_COCKPIT_MOCK") == 1) {
    if (m_mockMode) {
        m_systemInfo = new MockSystemInfoProvider(this);
        m_services = new MockServiceProvider(this);
        m_logs = new MockLogProvider(this);
        m_network = new MockNetworkProvider(this);
    } else {
        m_systemInfo = new LinuxSystemInfoProvider(this);
        m_services = new SystemdServiceProvider(this);
        m_logs = new JournaldLogProvider(this);
        m_network = new LinuxNetworkProvider(this);
    }
}

CockpitBackend *CockpitBackend::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return new CockpitBackend();
}
