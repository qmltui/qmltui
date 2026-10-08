#include <QTest>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QQmlComponent>
#include <QQmlEngine>
#include <Qtui/Core/items/TableView.h>
#include <Qtui/Core/engine/KeyEvent.h>
#include <Qtui/Core/engine/MouseEvent.h>
#include "../TestPaint.h"
#include <memory>
#include <limits>

using namespace qtui;

class CountingTableList final : public QAbstractListModel
{
public:
    enum { Detail = Qt::UserRole + 1, Kind };
    int count = 1'000'000;
    int kind = 0;
    QString first = QStringLiteral("old");
    mutable QHash<int, int> reads;
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : count; }
    QVariant data(const QModelIndex &index, int role) const override {
        ++reads[role];
        if (!index.isValid()) return {};
        if (role == Qt::DisplayRole) return index.row() == 0 ? first : QString::number(index.row());
        if (role == Detail) return QStringLiteral("detail");
        if (role == Kind) return kind;
        return {};
    }
    QHash<int, QByteArray> roleNames() const override {
        return {{Qt::DisplayRole, "display"}, {Detail, "detail"}, {Kind, "kind"}};
    }
    void change(int firstRow, int lastRow, const QList<int> &roles) {
        emit dataChanged(index(firstRow, 0), index(lastRow, 0), roles);
    }
    void resetTo(int rows) { beginResetModel(); count = rows; endResetModel(); }
};

class TableFixture
{
public:
    QQmlEngine engine;
    QQmlComponent component{&engine};
    std::unique_ptr<QObject> object;
    TableView *table = nullptr;
    TextGridBuffer buffer;
    bool create(QObject *model, const QString &delegate, bool bound = true) {
        const QString qml = (bound ? QStringLiteral("pragma ComponentBehavior: Bound\n") : QString())
            + QStringLiteral("import QtQml\nimport Qtui\nimport Qt.labs.qmlmodels\n"
                "TableView { required property var inputModel; model: inputModel; width: 40; height: 8; "
                "columns: [{title:'A',width:12},{title:'B',width:12},{title:'C',width:12}]; ")
            + delegate + QStringLiteral(" }");
        component.setData(qml.toUtf8(), QUrl());
        if (!component.isReady()) return false;
        object.reset(component.createWithInitialProperties({{"inputModel", QVariant::fromValue(model)}}));
        table = qobject_cast<TableView *>(object.get());
        if (!table || !buffer.resize(40, 8)) return false;
        paint();
        return true;
    }
    // Same as create(), but the caller supplies the whole TableView body, so
    // tests can omit the columns descriptors.
    bool createBody(QObject *model, const QString &body) {
        const QString qml = QStringLiteral("pragma ComponentBehavior: Bound\nimport QtQml\nimport Qtui\n"
                "TableView { required property var inputModel; model: inputModel; width: 40; height: 8; ")
            + body + QStringLiteral(" }");
        component.setData(qml.toUtf8(), QUrl());
        if (!component.isReady()) return false;
        object.reset(component.createWithInitialProperties({{"inputModel", QVariant::fromValue(model)}}));
        table = qobject_cast<TableView *>(object.get());
        if (!table || !buffer.resize(40, 8)) return false;
        paint();
        return true;
    }
    void paint() { buffer.clear(); testing::paintItem(table, buffer, 0, 0); }
    QString line(int y) const {
        QString result;
        for (int x = 0; x < 40; ++x) result += QString::fromUtf8(buffer.cellAt(x, y).character());
        return result;
    }
    QList<Item *> items() const { return table->childItems(); }
};

static const QString textDelegate = QStringLiteral(
    "delegate: Text { required property string display; text: display }");

class TestTableViewHardening : public QObject
{
    Q_OBJECT
private slots:
    void alignedGridCells_data() {
        QTest::addColumn<bool>("grid");
        QTest::addColumn<QString>("alignment");
        QTest::addColumn<QString>("header");
        QTest::addColumn<QString>("body");
        QTest::newRow("right-grid") << true << "right" << "LATENCY|" << "  12 ms|";
        QTest::newRow("right-no-grid") << false << "right" << " LATENCY" << "   12 ms";
        QTest::newRow("center-grid") << true << "center" << "LATENCY|" << " 12 ms |";
        QTest::newRow("left-grid") << true << "left" << "LATENCY|" << "12 ms  |";
    }

    void alignedGridCells() {
        QFETCH(bool, grid);
        QFETCH(QString, alignment);
        QFETCH(QString, header);
        QFETCH(QString, body);
        CountingTableList model;
        model.first = "12 ms";
        TableFixture f;
        QVERIFY(f.create(&model, QString()));
        f.table->setColumns({QVariantMap{{"title", "LATENCY"}, {"width", 8}, {"align", alignment}},
                             QVariantMap{{"title", "TAIL"}, {"width", 8}, {"align", "right"}}});
        f.table->setShowGrid(grid);
        f.table->setHeaderBackgroundColor(Color("#224466"));
        f.table->setCellFormat(f.engine.evaluate(
            "(function() { return {backgroundColor: '#446688'}; })"));
        f.paint();
        QCOMPARE(f.line(0).left(16), header + "    TAIL");
        QCOMPARE(f.line(2).left(16), body + "   12 ms");
        QCOMPARE(f.buffer.cellAt(7, 0).bgColor, 0xff224466u);
        QCOMPARE(f.buffer.cellAt(7, 2).bgColor, 0xff446688u);
        // A viewport ending at the first column has no separator to reserve.
        f.table->setWidth(8);
        f.paint();
        if (alignment == "right") {
            QCOMPARE(f.line(0).left(8), QString(" LATENCY"));
            QCOMPARE(f.line(2).left(8), QString("   12 ms"));
        }
    }

    void rolesAndViewport_data() {
        QTest::addColumn<bool>("bound");
        QTest::newRow("bound") << true;
        QTest::newRow("unbound") << false;
    }
    void rolesAndViewport() {
        QFETCH(bool, bound);
        CountingTableList model;
        TableFixture f;
        QVERIFY2(f.create(&model, textDelegate, bound), qPrintable(f.component.errorString()));
        const auto items = f.items();
        QCOMPARE(items.size(), 24); // Six body rows plus two overscan, three columns.
        QCOMPARE(f.line(2).count("old"), 3);
        model.reads.clear();
        model.first = QStringLiteral("new");
        model.change(0, 0, {Qt::DisplayRole});
        f.paint();
        QCOMPARE(f.line(2).count("new"), 3);
        QCOMPARE(model.reads.value(Qt::DisplayRole), 1); // Shared source column read once.
        QCOMPARE(model.reads.value(CountingTableList::Detail), 0);
        QCOMPARE(f.items(), items);
        model.reads.clear();
        model.change(0, model.count - 1, {Qt::DisplayRole});
        f.paint();
        QCOMPARE(model.reads.value(Qt::DisplayRole), 8);
        QCOMPARE(model.reads.value(CountingTableList::Detail), 0);
        QCOMPARE(f.items(), items);
        model.reads.clear();
        model.change(900'000, 999'999, {});
        f.paint();
        QVERIFY(model.reads.isEmpty());
        QCOMPARE(f.items(), items);
        f.table->setSelectedRow(1);
        f.table->setCurrentColumn(2);
        f.paint();
        QVERIFY(model.reads.isEmpty());
        model.change(0, 0, {});
        f.paint();
        QCOMPARE(model.reads.value(Qt::DisplayRole), 1);
        QCOMPARE(model.reads.value(CountingTableList::Detail), 1);
    }

    void chooserChanges() {
        CountingTableList model;
        model.count = 1;
        TableFixture f;
        QVERIFY2(f.create(&model, QStringLiteral(R"(
            delegate: DelegateChooser {
                role: "kind"
                DelegateChoice { roleValue: 0; delegate: Text {
                    required property string display; text: "A:" + display
                } }
                DelegateChoice { roleValue: 1; delegate: Text {
                    required property string display; text: "B:" + display
                } }
            }
        )")), qPrintable(f.component.errorString()));
        QCOMPARE(f.line(2).count("A:old"), 3);
        QPointer<Item> original = f.items().first();
        model.kind = 1;
        model.change(0, 0, {CountingTableList::Kind});
        f.paint();
        QVERIFY(original.isNull());
        QCOMPARE(f.line(2).count("B:old"), 3);
        model.kind = 2;
        model.change(0, 0, {CountingTableList::Kind});
        f.paint();
        QVERIFY(f.items().isEmpty());
        QVERIFY(f.line(2).trimmed().isEmpty());
        model.kind = 0;
        model.change(0, 0, {CountingTableList::Kind});
        f.paint();
        QCOMPARE(f.line(2).count("A:old"), 3);
    }

    void proxySelectionAndIdentity() {
        QStandardItemModel source;
        for (const QString &label : {"charlie", "alpha", "bravo"})
            source.appendRow(new QStandardItem(label));
        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&source);
        proxy.sort(0);
        TableFixture f;
        QVERIFY2(f.create(&proxy, textDelegate), qPrintable(f.component.errorString()));
        QVERIFY(f.line(2).contains("alpha"));
        f.table->setSelectedRow(0);
        f.paint();
        const auto items = f.items();
        QSet<Item *> before(items.cbegin(), items.cend());
        proxy.sort(0, Qt::DescendingOrder);
        QCOMPARE(f.table->selectedRow(), 2);
        f.paint();
        const auto reordered = f.items();
        QCOMPARE(QSet<Item *>(reordered.cbegin(), reordered.cend()), before);
        QVERIFY(f.line(4).contains("alpha"));
        QSignalSpy activated(f.table, &TableView::rowActivated);
        KeyEvent enter(Qt::Key_Return, QString());
        f.table->keyPressEvent(&enter);
        QCOMPARE(activated.size(), 1);
        QCOMPARE(proxy.mapToSource(proxy.index(activated.first().first().toInt(), 0)).row(), 1);
        source.setData(source.index(1, 0), "zulu");
        f.paint();
        QCOMPARE(f.table->selectedRow(), 0);
        QVERIFY(f.line(2).contains("zulu"));
        proxy.setFilterFixedString("bravo");
        QCOMPARE(f.table->selectedRow(), -1);
        f.paint();
        QVERIFY(f.line(2).contains("bravo"));
        proxy.setFilterFixedString(QString());
        QCOMPARE(f.table->selectedRow(), -1);
        f.table->keyPressEvent(&enter);
        QCOMPARE(activated.size(), 1);
        f.table->setSelectedRow(0);
        source.removeRow(proxy.mapToSource(proxy.index(0, 0)).row());
        QCOMPARE(f.table->selectedRow(), -1);
    }

    void resetReplacementAndDestruction() {
        auto model = std::make_unique<CountingTableList>();
        TableFixture f;
        QVERIFY(f.create(model.get(), textDelegate));
        f.table->setSelectedRow(99);
        QSignalSpy selection(f.table, &TableView::selectedRowChanged);
        model->resetTo(2);
        QCOMPARE(f.table->selectedRow(), 1);
        QCOMPARE(selection.size(), 1);
        // Reset without changing count keeps selection
        model->resetTo(2);
        QCOMPARE(f.table->selectedRow(), 1);
        QCOMPARE(selection.size(), 1);
        // Reset to 0 clears selection
        model->resetTo(0);
        QCOMPARE(f.table->selectedRow(), -1);
        QCOMPARE(selection.size(), 2);
        model->resetTo(5);
        f.table->setSelectedRow(1);
        model.reset();
        QCOMPARE(f.table->selectedRow(), -1);
        f.paint();
        QVERIFY(f.items().isEmpty());
        f.table->setDelegate(nullptr);
        f.table->setModel(QVariantList{QVariantList{"z"}, QVariantList{"a"}});
        f.table->setSelectedRow(1);
        f.table->setModel(QVariantList{QVariantList{"z"}, QVariantList{"a"}});
        QCOMPARE(f.table->selectedRow(), 1);
        f.paint();
        QVERIFY(f.line(2).startsWith("z"));
        QVERIFY(f.line(3).startsWith("a"));
        QVERIFY(f.table->metaObject()->indexOfProperty("sortColumn") < 0);
    }

    void mouseAndPageNavigation() {
        CountingTableList model;
        TableFixture f;
        QVERIFY(f.create(&model, textDelegate));
        QSignalSpy headers(f.table, &TableView::headerClicked);
        MouseEvent header(MouseEvent::Type::Press, 15, 0, 0);
        f.table->mousePressEvent(&header);
        QCOMPARE(headers.size(), 1);
        QCOMPARE(headers.first().first().toInt(), 1);
        MouseEvent divider(MouseEvent::Type::Press, 15, 1, 0);
        f.table->mousePressEvent(&divider);
        QCOMPARE(headers.size(), 1);
        MouseEvent right(MouseEvent::Type::Press, 15, 0, 2);
        f.table->mousePressEvent(&right);
        QCOMPARE(headers.size(), 1);
        MouseEvent body(MouseEvent::Type::Press, 15, 2, 0);
        f.table->mousePressEvent(&body);
        QCOMPARE(f.table->selectedRow(), 0);
        QCOMPARE(f.table->currentColumn(), 1);
        KeyEvent page(Qt::Key_PageDown, QString());
        f.table->keyPressEvent(&page);
        QCOMPARE(f.table->selectedRow(), 6);
    }

    void geometryCacheAndVisibility() {
        CountingTableList model;
        TableFixture f;
        QVERIFY(f.create(&model, textDelegate));
        f.engine.globalObject().setProperty("widthCalls", 0);
        f.engine.globalObject().setProperty("cellWidth", 12);
        f.table->setColumnWidthProvider(f.engine.evaluate("(function(c) { ++widthCalls; return c === 1 ? 0 : cellWidth; })"));
        f.paint();
        QCOMPARE(f.engine.globalObject().property("widthCalls").toInt(), 3);
        QCOMPARE(f.items().size(), 16);
        QVERIFY(f.line(0).startsWith("A           C"));
        f.table->setSelectedRow(1);
        f.paint();
        QCOMPARE(f.engine.globalObject().property("widthCalls").toInt(), 3);
        const auto before = f.items();
        f.table->setColumns(f.table->columns());
        f.paint();
        QCOMPARE(f.items(), before);
        f.engine.globalObject().setProperty("cellWidth", 8);
        f.table->forceLayout();
        f.paint();
        QCOMPARE(f.engine.globalObject().property("widthCalls").toInt(), 6);
        QVERIFY(f.line(0).startsWith("A       C"));
        QCOMPARE(f.items(), before);
        f.table->setHeight(1);
        f.paint();
        QVERIFY(f.items().isEmpty());
        f.table->setHeight(8);
        f.table->setColumnWidthProvider(QJSValue());
        f.table->setDelegate(nullptr);
        f.table->setColumns({QVariantMap{{"title", "wide"}, {"width", (std::numeric_limits<int>::max)()}}});
        f.paint();
        QVERIFY(f.line(0).startsWith("wide"));
    }

    void callbacksAndFallback() {
        CountingTableList model;
        TableFixture f;
        QVERIFY(f.create(&model, QString()));
        f.table->setColumnWidthProvider(f.engine.evaluate("(function(c) { return c === 0 ? -1 : undefined; })"));
        f.paint();
        QVERIFY(f.line(0).startsWith("A           B"));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*columnWidthProvider: Error: width-failure.*", QRegularExpression::DotMatchesEverythingOption));
        f.table->setColumnWidthProvider(f.engine.evaluate("(function(c) { throw new Error('width-failure'); })"));
        f.paint();
        f.table->forceLayout();
        f.paint(); // No repeat of the same exception.
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*cellFormat: Error: format-failure.*", QRegularExpression::DotMatchesEverythingOption));
        f.table->setCellFormat(f.engine.evaluate("(function(r,c,v) { throw new Error('format-failure'); })"));
        f.paint();
        f.paint();
        QVERIFY(f.line(2).contains("old"));
        f.table->setColumnWidthProvider(QJSValue());
        f.table->setColumns({QVariantMap{{"title", "bad"}, {"width", -3}}});
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Invalid TableView column width; using 12.*"));
        f.paint();
        QVERIFY(f.line(0).startsWith("bad"));
    }

    void rowDelegateUpdates() {
        CountingTableList model;
        model.count = 2;
        TableFixture f;
        QVERIFY2(f.create(&model, QStringLiteral("rowDelegate: Text { required property string display; text: display }")),
            qPrintable(f.component.errorString()));
        const auto before = f.items();
        model.first = "changed";
        model.change(0, 0, {Qt::DisplayRole});
        f.paint();
        QVERIFY(f.line(2).startsWith("changed"));
        QCOMPARE(f.items(), before);
    }
    // Plan 07 WP1: header data and column count come from the model when no
    // columns descriptors are set.
    void modelHeaderDataDrivesColumns() {
        QStandardItemModel model(2, 3);
        for (int row = 0; row < 2; ++row)
            for (int column = 0; column < 3; ++column)
                model.setItem(row, column, new QStandardItem(QStringLiteral("%1%2").arg(QChar('a' + column)).arg(row)));
        model.setHorizontalHeaderLabels({"Name", "Size", "Kind"});
        TableFixture f;
        QVERIFY2(f.createBody(&model, QString()), qPrintable(f.component.errorString()));
        // Header row shows the model titles, one column each (40 / 3 = 13, 13, 14).
        QCOMPARE(f.line(0).mid(0, 13).trimmed(), QStringLiteral("Name"));
        QCOMPARE(f.line(0).mid(13, 13).trimmed(), QStringLiteral("Size"));
        QCOMPARE(f.line(0).mid(26, 14).trimmed(), QStringLiteral("Kind"));
        QCOMPARE(f.line(2).mid(0, 13).trimmed(), QStringLiteral("a0"));
        QCOMPARE(f.line(2).mid(13, 13).trimmed(), QStringLiteral("b0"));
        QCOMPARE(f.line(2).mid(26, 14).trimmed(), QStringLiteral("c0"));
        QCOMPARE(f.line(3).mid(0, 13).trimmed(), QStringLiteral("a1"));
        // headerDataChanged repaints the title.
        model.setHeaderData(0, Qt::Horizontal, QStringLiteral("Path"));
        f.paint();
        QCOMPARE(f.line(0).mid(0, 13).trimmed(), QStringLiteral("Path"));
        // Header alignment role becomes the column default for header and cells.
        model.setHeaderData(1, Qt::Horizontal, int(Qt::AlignRight | Qt::AlignVCenter), Qt::TextAlignmentRole);
        f.paint();
        QVERIFY2(f.line(0).mid(13, 13).endsWith("Size"), qPrintable(f.line(0)));
        QVERIFY2(f.line(2).mid(13, 13).endsWith("b0"), qPrintable(f.line(2)));
        // Explicit columns still win over model header data.
        f.table->setColumns({QVariantMap{{"title", "X"}, {"width", 10}}});
        f.paint();
        QCOMPARE(f.line(0).mid(0, 10).trimmed(), QStringLiteral("X"));
        QVERIFY(!f.line(0).contains("Size"));
        QCOMPARE(f.line(2).mid(0, 10).trimmed(), QStringLiteral("a0"));
    }

    void modelWithoutHeaderLabelsShowsNoHeader() {
        QStandardItemModel model(1, 3);
        for (int column = 0; column < 3; ++column)
            model.setItem(0, column, new QStandardItem(QStringLiteral("v%1").arg(column)));
        TableFixture f;
        QVERIFY2(f.createBody(&model, QString()), qPrintable(f.component.errorString()));
        // Default headerData() returns section numbers, which is not a header.
        QCOMPARE(f.line(0).mid(0, 13).trimmed(), QStringLiteral("v0"));
        QCOMPARE(f.line(0).mid(13, 13).trimmed(), QStringLiteral("v1"));
        QCOMPARE(f.line(0).mid(26, 14).trimmed(), QStringLiteral("v2"));
        QVERIFY(f.line(1).trimmed().isEmpty());
        // Adding labels later turns the header on.
        model.setHorizontalHeaderLabels({"A", "B", "C"});
        f.paint();
        QCOMPARE(f.line(0).mid(0, 13).trimmed(), QStringLiteral("A"));
        QCOMPARE(f.line(2).mid(0, 13).trimmed(), QStringLiteral("v0"));
        // Column insertion is reflected without descriptors.
        model.insertColumn(3);
        model.setItem(0, 3, new QStandardItem(QStringLiteral("v3")));
        f.paint();
        QCOMPARE(f.line(2).mid(30, 10).trimmed(), QStringLiteral("v3"));
    }

    void listModelWithoutColumnsIsSingleColumn() {
        CountingTableList model;
        model.count = 3;
        TableFixture f;
        QVERIFY2(f.createBody(&model, QString()), qPrintable(f.component.errorString()));
        // One column filling the width, no header, and no repeated phantom cells.
        QCOMPARE(f.line(0).count("old"), 1);
        QCOMPARE(f.line(0).trimmed(), QStringLiteral("old"));
        QCOMPARE(f.line(1).trimmed(), QStringLiteral("1"));
        QCOMPARE(f.line(2).trimmed(), QStringLiteral("2"));
    }
};

QTEST_MAIN(TestTableViewHardening)
#include "test_table_view_hardening.moc"
