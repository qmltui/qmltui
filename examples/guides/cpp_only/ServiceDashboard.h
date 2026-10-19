#pragma once

// A small QmlTui application without QML: a filterable list of services with
// live latencies, built from QmlTui's C++ items.

#include <QmlTui/Core/Items/Terminal.h>

#include <QList>
#include <QProperty>
#include <QString>
#include <QTimer>

namespace qmltui {
class Column;
class KeyEvent;
class TextInput;
} // namespace qmltui

struct ServiceEntry {
    QString name;
    int latencyMs = 0;
    friend bool operator==(const ServiceEntry&, const ServiceEntry&) = default;
};

// The root Terminal is the last stop for keyboard events, so subclassing it is
// the C++ counterpart of `Keys.onPressed` on a QML root.
class ServiceDashboard : public qmltui::Terminal {
    Q_OBJECT
public:
    explicit ServiceDashboard(QObject* parent = nullptr);

    // One simulated latency sample per service. The live timer calls it; a
    // Runner scenario calls it directly so frames do not depend on the clock.
    Q_INVOKABLE void tick();

    void setLiveTelemetry(bool enabled);

protected:
    // Receives only the keys the focused widget declined.
    void keyPressEvent(qmltui::KeyEvent* event) override;

private:
    void rebuildRows();

    // Written by the application and the filter field...
    QProperty<QList<ServiceEntry>> m_services;
    QProperty<QString> m_filterText;
    // ...and derived from them by a binding.
    QProperty<QList<ServiceEntry>> m_visibleServices;
    QPropertyNotifier m_rowsNotifier;

    qmltui::TextInput* m_filter = nullptr;
    qmltui::Column* m_rows = nullptr;
    QTimer m_telemetry;
    int m_samples = 0;
};
