#include <QCoreApplication>
#include <QTimer>
#include <QList>
#include <QVariantList>
#include <algorithm>

#include "Terminal.h"
#include "Column.h"
#include "Row.h"
#include "Text.h"
#include "TextInput.h"
#include "TableView.h"
#include "Shortcut.h"

using namespace qtui;

struct ServiceEntry {
    QString name;
    int baseLatency;
    QString status;
};

int main(int argc, char *argv[]) {
    // 1. Initialize standard QCoreApplication (no QML engine required)
    QCoreApplication app(argc, argv);

    // 2. Root Terminal container
    Terminal terminal;
    terminal.setTitle(QStringLiteral("Qtui C++ Service Dashboard"));
    terminal.setFillTerminal(true);
    terminal.setColor(Color::fromString(QStringLiteral("#0d1117")));

    // 3. Main layout column with margins and spacing
    auto *rootLayout = new Column(&terminal);
    rootLayout->anchors()->setFill(&terminal);
    rootLayout->anchors()->setMargins(1);
    rootLayout->setSpacing(1);

    // 4. Header bar: Title + Badge
    auto *headerRow = new Row(rootLayout);
    headerRow->setSpacing(2);

    auto *titleText = new Text(headerRow);
    titleText->setText(QStringLiteral("⚡ SERVICE MONITOR"));
    titleText->setColor(Color::fromString(QStringLiteral("#58a6ff")));
    titleText->setBold(true);

    auto *badgeText = new Text(headerRow);
    badgeText->setText(QStringLiteral("[C++ NATIVE • NO QML]"));
    badgeText->setColor(Color::fromString(QStringLiteral("#7ee787")));

    // 5. Data list of services
    QList<ServiceEntry> services = {
        {QStringLiteral("auth-service"), 12, QStringLiteral("● HEALTHY")},
        {QStringLiteral("database-primary"), 6, QStringLiteral("● HEALTHY")},
        {QStringLiteral("api-gateway"), 28, QStringLiteral("● HEALTHY")},
        {QStringLiteral("redis-cache"), 2, QStringLiteral("● HEALTHY")},
        {QStringLiteral("worker-queue"), 85, QStringLiteral("▲ DEGRADED")},
        {QStringLiteral("payment-gateway"), 42, QStringLiteral("● HEALTHY")},
        {QStringLiteral("analytics-pipeline"), 140, QStringLiteral("▲ HIGH LOAD")}
    };

    // 6. Interactive Search / Filter row
    auto *searchRow = new Row(rootLayout);
    searchRow->setSpacing(1);

    auto *searchLabel = new Text(searchRow);
    searchLabel->setText(QStringLiteral("🔍 Filter:"));
    searchLabel->setColor(Color::fromString(QStringLiteral("#8b949e")));

    auto *searchInput = new TextInput(searchRow);
    searchInput->setPlaceholderText(QStringLiteral("type to filter services..."));
    searchInput->setColor(Color::fromString(QStringLiteral("#f0f6fc")));
    searchInput->setPlaceholderColor(Color::fromString(QStringLiteral("#484f58")));
    searchInput->setWidth(32);
    searchInput->setFocus(true);

    // 7. TableView for services
    auto *table = new TableView(rootLayout);
    table->setWidth(60);
    table->setHeight(9);
    table->setShowGrid(true);
    table->setAlternatingRows(true);
    table->setHeaderColor(Color::fromString(QStringLiteral("#58a6ff")));
    table->setHeaderBgColor(Color::fromString(QStringLiteral("#161b22")));
    table->setRowBgColor(Color::fromString(QStringLiteral("#0d1117")));
    table->setAlternatingRowBgColor(Color::fromString(QStringLiteral("#161b22")));
    table->setTextColor(Color::fromString(QStringLiteral("#c9d1d9")));

    QVariantList columns;
    columns.append(QVariantMap{
        {QStringLiteral("title"), QStringLiteral("SERVICE")},
        {QStringLiteral("width"), 22}
    });
    columns.append(QVariantMap{
        {QStringLiteral("title"), QStringLiteral("LATENCY")},
        {QStringLiteral("width"), 14},
        {QStringLiteral("align"), QStringLiteral("right")}
    });
    columns.append(QVariantMap{
        {QStringLiteral("title"), QStringLiteral("STATUS")},
        {QStringLiteral("width"), 18},
        {QStringLiteral("align"), QStringLiteral("center")}
    });
    table->setColumns(columns);

    // 8. Status footer bar
    auto *footer = new Text(rootLayout);
    footer->setText(QStringLiteral("Type to filter • Q or Esc to quit"));
    footer->setColor(Color::fromString(QStringLiteral("#8b949e")));

    // Reactive table update helper
    auto updateTable = [&]() {
        const QString filter = searchInput->text().trimmed();
        QVariantList rows;
        int matchCount = 0;

        for (const auto &svc : services) {
            if (!filter.isEmpty() && !svc.name.contains(filter, Qt::CaseInsensitive)) {
                continue;
            }
            matchCount++;
            rows.append(QVariantList{
                svc.name,
                QStringLiteral("%1 ms").arg(svc.baseLatency),
                svc.status
            });
        }
        table->setModel(rows);

        if (!filter.isEmpty()) {
            footer->setText(QStringLiteral("Showing %1 / %2 services matching \"%3\" • Q or Esc to quit")
                .arg(matchCount)
                .arg(services.size())
                .arg(filter));
        } else {
            footer->setText(QStringLiteral("Type to filter • Q or Esc to quit"));
        }
    };

    updateTable();

    // Live reactivity: typing in searchInput instantly filters the TableView
    QObject::connect(searchInput, &TextInput::textChanged, updateTable);

    // 9. Periodic timer simulating live latency jitter
    auto *timer = new QTimer(&terminal);
    int tick = 0;
    QObject::connect(timer, &QTimer::timeout, [&]() {
        tick++;
        for (auto &svc : services) {
            int jitter = ((tick * 17 + svc.name.length() * 11) % 7) - 3;
            svc.baseLatency = std::max(1, svc.baseLatency + jitter);
        }
        updateTable();
    });
    timer->start(1500);

    // 10. Clean keyboard quit shortcuts
    auto *quitQ = new Shortcut(&terminal);
    quitQ->setSequence(Qt::Key_Q);
    QObject::connect(quitQ, &Shortcut::activated, &app, &QCoreApplication::quit);

    auto *quitEsc = new Shortcut(&terminal);
    quitEsc->setSequence(Qt::Key_Escape);
    QObject::connect(quitEsc, &Shortcut::activated, &app, &QCoreApplication::quit);

    // 11. Run Qt event loop
    return app.exec();
}
