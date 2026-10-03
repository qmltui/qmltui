#include "GatewayBackend.h"
#include <QTime>
#include <QRandomGenerator>

GatewayBackend *GatewayBackend::s_instance = nullptr;

GatewayBackend::GatewayBackend(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    // Initial sensor telemetry
    m_sensorValues = { 48.2, 53.6, 14.8, 3.42, 62.1, 12.5, 0.45, 101.3 };

    for (int i = 0; i < 8; ++i) {
        QVariantList hist;
        double baseVal = m_sensorValues[i].toDouble();
        for (int k = 0; k < 10; ++k) {
            hist.append(baseVal);
        }
        m_sensorHistories.append(hist);
    }

    // Initial packet logs
    QVariantMap p1;
    p1["time"] = "14:45:01.012"; p1["bus"] = "MODBUS-1"; p1["addr"] = "0x01";
    p1["func"] = "FC03 (ReadReg)"; p1["payload"] = "0x0001 0x0004 [48.2C, 53.6V]"; p1["status"] = "OK (CRC:0x4A12)";

    QVariantMap p2;
    p2["time"] = "14:45:01.045"; p2["bus"] = "CAN-0"; p2["addr"] = "0x18FF50e5";
    p2["func"] = "BMS_VOLT"; p2["payload"] = "53.60V | 14.8A | SOC:88%"; p2["status"] = "ACK";

    QVariantMap p3;
    p3["time"] = "14:45:01.088"; p3["bus"] = "MODBUS-1"; p3["addr"] = "0x02";
    p3["func"] = "FC03 (ReadReg)"; p3["payload"] = "0x0010 0x0002 [14.8A]"; p3["status"] = "OK (CRC:0x98F1)";

    QVariantMap p4;
    p4["time"] = "14:45:01.120"; p4["bus"] = "CAN-0"; p4["addr"] = "0x0c08a7f4";
    p4["func"] = "INV_STATUS"; p4["payload"] = "3.42kW | 60.0Hz | P=1.00"; p4["status"] = "ACK";

    m_packetLogs.append(p1);
    m_packetLogs.append(p2);
    m_packetLogs.append(p3);
    m_packetLogs.append(p4);

    connect(&m_timer, &QTimer::timeout, this, &GatewayBackend::onTick);
    m_timer.start(1000);
}

GatewayBackend *GatewayBackend::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return instance();
}

GatewayBackend *GatewayBackend::instance()
{
    if (!s_instance) {
        s_instance = new GatewayBackend();
    }
    return s_instance;
}

QVariantList GatewayBackend::sensorHistories() const
{
    QVariantList res;
    for (const auto &h : m_sensorHistories) {
        res.append(QVariant::fromValue(h));
    }
    return res;
}

void GatewayBackend::toggleRelay(int relayIndex)
{
    if (relayIndex == 1) m_relay1 = !m_relay1;
    else if (relayIndex == 2) m_relay2 = !m_relay2;
    else if (relayIndex == 3) m_relay3 = !m_relay3;
    else if (relayIndex == 4) m_relay4 = !m_relay4;

    emit relaysChanged();
}

void GatewayBackend::setModbusBaud(const QString &baud)
{
    Q_UNUSED(baud)
    m_totalTx += 5;
    emit trafficStatsChanged();
}

void GatewayBackend::setCanBitrate(const QString &bitrate)
{
    Q_UNUSED(bitrate)
    m_totalTx += 5;
    emit trafficStatsChanged();
}

void GatewayBackend::setPublishInterval(int seconds)
{
    Q_UNUSED(seconds)
}

void GatewayBackend::onTick()
{
    m_uptimeSeconds += 1;
    emit uptimeChanged();

    // Simulate sensor fluctuations in C++
    auto *rng = QRandomGenerator::global();

    double t = m_sensorValues[0].toDouble() + (rng->bounded(1.0) - 0.5);
    double v = m_sensorValues[1].toDouble() + (rng->bounded(0.4) - 0.2);
    double c = m_sensorValues[2].toDouble() + (rng->bounded(0.6) - 0.3);
    double p = m_sensorValues[3].toDouble() + (rng->bounded(0.1) - 0.05);

    m_sensorValues[0] = qBound(20.0, t, 85.0);
    m_sensorValues[1] = qBound(40.0, v, 60.0);
    m_sensorValues[2] = qBound(0.0, c, 30.0);
    m_sensorValues[3] = qBound(0.0, p, 5.0);

    for (int i = 0; i < 4; ++i) {
        QVariantList hist = m_sensorHistories[i];
        if (!hist.isEmpty()) hist.removeFirst();
        hist.append(m_sensorValues[i]);
        m_sensorHistories[i] = hist;
    }
    emit sensorsChanged();

    // Simulate packet traffic stats
    int newPackets = 120 + rng->bounded(40);
    m_totalTx += newPackets;
    m_totalRx += newPackets - (rng->bounded(5) == 0 ? 1 : 0);
    m_packetsPerSec = newPackets;
    m_busLoadPct = qBound(5.0, 10.0 + (newPackets / 20.0), 45.0);
    emit trafficStatsChanged();

    // Simulate CPU and Hardware stats
    m_cpuLoadPct = qBound(5.0, 12.0 + rng->bounded(6.0), 30.0);
    m_cpuTemp = qBound(38.0, 42.0 + rng->bounded(2.0), 65.0);
    emit systemStatsChanged();
}
