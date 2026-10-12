#include "ServiceDashboard.h"

#include <Qamui/Core/Engine/KeyEvent.h>
#include <Qamui/Core/Items/Column.h>
#include <Qamui/Core/Items/Row.h>
#include <Qamui/Core/Items/Text.h>
#include <Qamui/Core/Items/TextInput.h>

#include <QBindable>
#include <QCoreApplication>

#include <algorithm>

using namespace qamui;

namespace {

const Color kAccent("#58a6ff");
const Color kMuted("#8b949e");

// The C++ counterpart of a ListView delegate: a function that builds one row.
// Items are QObjects, so parenting a row to the column both owns and lays it out.
void addRow(Column *column, const ServiceEntry &service)
{
    auto *row = new Row(column);
    row->setSpacing(1);

    auto *name = new Text(row);
    name->setText(service.name);
    name->setWidth(22);

    auto *latency = new Text(row);
    latency->setText(QStringLiteral("%1 ms").arg(service.latencyMs));
    latency->setWidth(7);
    latency->setHorizontalAlignment(Qt::AlignRight);

    auto *status = new Text(row);
    const bool healthy = service.latencyMs <= 60;
    status->setText(healthy ? QStringLiteral("● healthy") : QStringLiteral("▲ degraded"));
    status->setColor(Color(healthy ? "#3fb950" : "#d29922"));
    status->setBold(!healthy);
}

} // namespace

ServiceDashboard::ServiceDashboard(QObject *parent)
    : Terminal(parent)
{
    setTitle(QStringLiteral("Qamui C++ Service Dashboard"));
    setColor(Color("#0d1117"));

    m_services = QList<ServiceEntry>{
        {QStringLiteral("auth-service"), 12},
        {QStringLiteral("database-primary"), 6},
        {QStringLiteral("api-gateway"), 28},
        {QStringLiteral("redis-cache"), 2},
        {QStringLiteral("worker-queue"), 85},
        {QStringLiteral("payment-gateway"), 42},
        {QStringLiteral("analytics-pipeline"), 140},
    };

    // --- Item tree -------------------------------------------------------
    auto *page = new Column(this);
    page->anchors()->setFill(this);
    page->anchors()->setMargins(1);
    page->setSpacing(1);

    auto *title = new Text(page);
    title->setText(QStringLiteral("SERVICE MONITOR  ·  C++ only, no QML engine"));
    title->setColor(kAccent);
    title->setBold(true);

    auto *filterRow = new Row(page);
    filterRow->setSpacing(1);
    auto *filterLabel = new Text(filterRow);
    filterLabel->setText(QStringLiteral("Filter:"));
    filterLabel->setColor(kMuted);
    m_filter = new TextInput(filterRow);
    m_filter->setPlaceholderText(QStringLiteral("type part of a service name"));
    m_filter->setPlaceholderColor(kMuted);
    m_filter->setWidth(40);
    m_filter->setFocus(true);

    m_rows = new Column(page);

    auto *legend = new Text(page);
    legend->setText(QStringLiteral("Esc clears the filter, then quits  ·  Ctrl+Q quits anywhere"));
    legend->setColor(kMuted);

    // --- Reactive state --------------------------------------------------
    // QBindable adapts the ordinary text Q_PROPERTY (NOTIFY textChanged) so a
    // binding can read it as a dependency.
    m_filterText.setBinding([text = QBindable<QString>(m_filter, "text")] { return text.value(); });

    // Re-evaluates when either the filter or the services change.
    m_visibleServices.setBinding([this] {
        const QString needle = m_filterText.value().trimmed();
        QList<ServiceEntry> visible;
        for (const ServiceEntry &service : m_services.value()) {
            if (needle.isEmpty() || service.name.contains(needle, Qt::CaseInsensitive))
                visible.append(service);
        }
        return visible;
    });

    // The one place where state flows into the view.
    rebuildRows();
    m_rowsNotifier = m_visibleServices.addNotifier([this] { rebuildRows(); });

    m_telemetry.setInterval(1500);
    connect(&m_telemetry, &QTimer::timeout, this, &ServiceDashboard::tick);
}

void ServiceDashboard::rebuildRows()
{
    // Seven rows: rebuilding is simpler than diffing. A ListView would
    // virtualize this; it needs a QML delegate, which this example avoids.
    qDeleteAll(m_rows->childItems());
    const QList<ServiceEntry> visible = m_visibleServices.value();
    for (const ServiceEntry &service : visible)
        addRow(m_rows, service);
    if (visible.isEmpty()) {
        auto *empty = new Text(m_rows);
        empty->setText(QStringLiteral("no service matches"));
        empty->setColor(kMuted);
    }
}

void ServiceDashboard::setLiveTelemetry(bool enabled)
{
    if (enabled)
        m_telemetry.start();
    else
        m_telemetry.stop();
}

void ServiceDashboard::tick()
{
    ++m_samples;
    QList<ServiceEntry> services = m_services.value();
    for (int i = 0; i < services.size(); ++i) {
        // Deterministic jitter, so every run draws the same numbers.
        const int jitter = ((m_samples * 37 + i * 101) % 11) - 5;
        services[i].latencyMs = std::max(1, services[i].latencyMs + jitter);
    }
    m_services = services; // one assignment; the binding and the rows follow
}

void ServiceDashboard::keyPressEvent(KeyEvent *event)
{
    // The focused TextInput accepts every printable character, so a bare "q"
    // never arrives here while typing. A global Shortcut would fire *before*
    // the widget and steal the letter; do not use one for printable keys.
    const bool control = event->modifiers() & Qt::ControlModifier;
    if (control && event->key() == Qt::Key_Q) {
        QCoreApplication::quit();
    } else if (event->key() == Qt::Key_Escape) {
        // Contextual Escape: undo the filter first, quit when nothing is left.
        if (!m_filter->text().isEmpty())
            m_filter->clear();
        else
            QCoreApplication::quit();
    } else {
        return;
    }
    event->setAccepted(true);
}
