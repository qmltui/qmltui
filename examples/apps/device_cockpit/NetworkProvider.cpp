#include "NetworkProvider.h"

NetworkProvider::NetworkProvider(int refreshIntervalMs, QObject *parent)
    : QObject(parent) {
    if (refreshIntervalMs > 0) {
        connect(&m_timer, &QTimer::timeout, this, &NetworkProvider::refresh);
        m_timer.start(refreshIntervalMs);
    }
}

void NetworkProvider::setInterfaces(QVariantList interfaces) {
    if (m_interfaces == interfaces)
        return;
    m_interfaces = std::move(interfaces);
    emit interfacesChanged();
}

void NetworkProvider::setCanConfigure(bool canConfigure) {
    if (m_canConfigure == canConfigure)
        return;
    m_canConfigure = canConfigure;
    emit canConfigureChanged();
}
