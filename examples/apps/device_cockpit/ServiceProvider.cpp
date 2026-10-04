#include "ServiceProvider.h"

#include <algorithm>

ServiceProvider::ServiceProvider(int refreshIntervalMs, QObject *parent)
    : QObject(parent) {
    if (refreshIntervalMs > 0) {
        connect(&m_timer, &QTimer::timeout, this, &ServiceProvider::refresh);
        m_timer.start(refreshIntervalMs);
    }
}

QVariantList ServiceProvider::units() const {
    if (m_filter.isEmpty())
        return m_allUnits;
    QVariantList filtered;
    for (const QVariant &entry : m_allUnits) {
        const QVariantMap unit = entry.toMap();
        if (unit.value("unit").toString().contains(m_filter, Qt::CaseInsensitive)
            || unit.value("description").toString().contains(m_filter, Qt::CaseInsensitive)) {
            filtered.append(entry);
        }
    }
    return filtered;
}

void ServiceProvider::setFilter(const QString &filter) {
    if (m_filter == filter)
        return;
    m_filter = filter;
    emit filterChanged();
    emit unitsChanged();
}

void ServiceProvider::setAllUnits(QVariantList units) {
    std::sort(units.begin(), units.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value("unit").toString() < b.toMap().value("unit").toString();
    });
    if (m_allUnits == units)
        return;
    m_allUnits = std::move(units);
    emit unitsChanged();
}

void ServiceProvider::setCanManage(bool canManage) {
    if (m_canManage == canManage)
        return;
    m_canManage = canManage;
    emit canManageChanged();
}
