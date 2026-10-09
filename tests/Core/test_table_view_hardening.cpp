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

class FetchingTableList final : public QAbstractListModel
{
public:
    int count = 0;
    int fetches = 0;
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : count; }
    QVariant data(const QModelIndex &index, int role) const override {
        return role == Qt::DisplayRole ? QVariant(QString::number(index.row())) : QVariant();
    }
    bool canFetchMore(const QModelIndex &parent) const override { return !parent.isValid() && count < 1000; }
    void fetchMore(const QModelIndex &) override {
        beginInsertRows({}, count, count + 1);
        count += 2;
        ++fetches;
        endInsertRows();
    }
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
    bool createBody(QObject *model, const QString &body, int initialHeight = 8) {
        const QString qml = QStringLiteral("pragma ComponentBehavior: Bound\nimport QtQml\nimport Qtui\n"
                "TableView { required property var inputModel; model: inputModel; width: 40; height: %1; ").arg(initialHeight)
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
    QList<Item *> items() const {
        QList<Item *> result;
        for (Item *container : table->childItems()) result.append(container->childItems());
        return result;
    }
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
        QTest::newRow("right-grid") << true << "right" << QString::fromUtf8("LATENCY│") << QString::fromUtf8("  12 ms│");
        QTest::newRow("right-no-grid") << false << "right" << QStringLiteral(" LATENCY") << QStringLiteral("   12 ms");
        QTest::newRow("center-grid") << true << "center" << QString::fromUtf8("LATENCY│") << QString::fromUtf8(" 12 ms │");
        QTest::newRow("left-grid") << true << "left" << QString::fromUtf8("LATENCY│") << QString::fromUtf8("12 ms  │");
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
        f.table->setColumns({QVariantMap{{"title", "LATENCY"}, {"width", grid ? 7 : 8}, {"align", alignment}},
                             QVariantMap{{"title", "TAIL"}, {"width", 8}, {"align", "right"}}});
        f.table->setShowColumnGrid(grid);
        f.table->setHeaderBackgroundColor(Color("#224466"));
        f.table->setCellFormat(f.engine.evaluate(
            "(function() { return {backgroundColor: '#446688'}; })"));
        f.paint();
        QCOMPARE(f.line(0).left(16), header + "    TAIL");
        QCOMPARE(f.line(1).left(16), grid ? QString::fromUtf8("───────┼────────") : QString::fromUtf8("────────────────"));
        QCOMPARE(f.line(2).left(16), body + "   12 ms");
        QCOMPARE(f.buffer.cellAt(7, 0).bgColor, 0xff224466u);
        QCOMPARE(f.buffer.cellAt(7, 2).bgColor, grid ? 0u : 0xff446688u);
        // Clipping does not change the logical content width.
        f.table->setWidth(8);
        f.paint();
        if (alignment == "right") {
            QCOMPARE(f.line(0).left(8), grid ? QString("LATENCY ") : QString(" LATENCY"));
            QCOMPARE(f.line(2).left(8), grid ? QString("  12 ms ") : QString("   12 ms"));
        }
    }

    void gridStyleOptions() {
        CountingTableList model;
        model.first = "val";
        TableFixture f;
        QVERIFY(f.create(&model, QString()));
        f.table->setColumns({QVariantMap{{"title", "COL1"}, {"width", 5}},
                             QVariantMap{{"title", "COL2"}, {"width", 6}}});
        QCOMPARE(f.table->gridStyle(), QStringLiteral("single"));

        // Single style (default)
        f.table->setShowColumnGrid(true);
        f.paint();
        QCOMPARE(f.line(0).left(12), QString::fromUtf8("COL1 │COL2  "));
        QCOMPARE(f.line(1).left(12), QString::fromUtf8("─────┼──────"));
        QCOMPARE(f.line(2).left(12), QString::fromUtf8("val  │val   "));

        // Double style
        f.table->setGridStyle(QStringLiteral("double"));
        QCOMPARE(f.table->gridStyle(), QStringLiteral("double"));
        f.paint();
        QCOMPARE(f.line(0).left(12), QString::fromUtf8("COL1 ║COL2  "));
        QCOMPARE(f.line(1).left(12), QString::fromUtf8("═════╬══════"));
        QCOMPARE(f.line(2).left(12), QString::fromUtf8("val  ║val   "));

        // Ascii style
        f.table->setGridStyle(QStringLiteral("ascii"));
        QCOMPARE(f.table->gridStyle(), QStringLiteral("ascii"));
        f.paint();
        QCOMPARE(f.line(0).left(12), QStringLiteral("COL1 |COL2  "));
        QCOMPARE(f.line(1).left(12), QStringLiteral("-----+------"));
        QCOMPARE(f.line(2).left(12), QStringLiteral("val  |val   "));

        // showColumnGrid false with double style: header divider still matches gridStyle
        f.table->setShowColumnGrid(false);
        f.table->setGridStyle(QStringLiteral("double"));
        f.paint();
        QCOMPARE(f.line(0).left(12), QStringLiteral("COL1 COL2   "));
        QCOMPARE(f.line(1).left(12), QString::fromUtf8("════════════"));
        QCOMPARE(f.line(2).left(12), QStringLiteral("val  val    "));
    }

    void cellAndColumnPadding() {
        CountingTableList model;
        model.first = "12";
        TableFixture f;
        QVERIFY(f.create(&model, QString()));
        f.table->setShowColumnGrid(true);
        // Column 0: left-aligned with padding 1
        // Column 1: right-aligned with padding 1
        f.table->setColumns({QVariantMap{{"title", "NAME"}, {"width", 7}, {"padding", 1}},
                             QVariantMap{{"title", "NUM"}, {"width", 8}, {"align", "right"}, {"padding", 1}}});
        f.paint();
        // Col 0: seven content cells plus one separator. Title "NAME" (4), pad 1 -> 1 space + "NAME" + 2 spaces = 7 chars + │
        // Col 1: width 8, sep 0 (last col) -> textWidth 8. Title "NUM" (3), pad 1, align right -> 4 spaces + "NUM" + 1 space = 8 chars
        QCOMPARE(f.line(0).left(16), QString::fromUtf8(" NAME  │    NUM "));
        // Body row 0:
        // Col 0: "12" (2), pad 1 -> 1 space + "12" + 4 spaces = 7 chars + │
        // Col 1: "12" (2), pad 1, align right -> 5 spaces + "12" + 1 space = 8 chars
        QCOMPARE(f.line(2).left(16), QString::fromUtf8(" 12    │     12 "));

        // Now override dynamically via cellFormat
        f.table->setCellFormat(f.engine.evaluate(
            "(function(row, col, val) {"
            "   if (col === 0) return { text: val, leftPadding: 2, rightPadding: 0 };"
            "   if (col === 1) return { text: val, padding: 0 };"
            "   return { text: val };"
            "})"));
        f.paint();
        // Col 0: leftPad 2, rightPad 0 -> 2 spaces + "12" + 3 spaces = 7 chars + │
        // Col 1: pad 0 -> 6 spaces + "12" = 8 chars
        QCOMPARE(f.line(2).left(16), QString::fromUtf8("  12   │      12"));
    }

    void boxGrid_data() {
        QTest::addColumn<int>("switches");
        QTest::addColumn<QString>("style");
        for (const QString &style : {QStringLiteral("single"), QStringLiteral("double"), QStringLiteral("ascii")})
            for (int switches = 0; switches < 8; ++switches)
                QTest::newRow(qPrintable(style + QString::number(switches))) << switches << style;
    }
    void boxGrid() {
        QFETCH(int, switches);
        QFETCH(QString, style);
        CountingTableList model;
        model.count = 2;
        model.first = "x";
        TableFixture f;
        QVERIFY(f.create(&model, QString()));
        const bool columns = switches & 1, rows = switches & 2, frame = switches & 4;
        const QString g = style == "double" ? QStringLiteral("═║╔╗╚╝╦╩╠╣╬")
            : style == "ascii" ? QStringLiteral("-|+++++++++") : QStringLiteral("─│┌┐└┘┬┴├┤┼");
        f.table->setColumns({QVariantMap{{"title", "A"}, {"width", 3}},
                             QVariantMap{{"title", "B"}, {"width", 3}}});
        f.table->setShowColumnGrid(columns);
        f.table->setShowRowGrid(rows);
        f.table->setShowOuterBorder(frame);
        f.table->setGridStyle(style);
        const int width = 6 + int(columns) + 2 * int(frame);
        const int height = 4 + int(rows) + 2 * int(frame);
        f.table->setWidth(width);
        f.table->setHeight(height);
        f.paint();
        const auto content = [&](const QString &a, const QString &b) {
            return (frame ? QString(g[1]) : QString()) + a + (columns ? QString(g[1]) : QString())
                + b + (frame ? QString(g[1]) : QString());
        };
        const auto line = [&](int left, int middle, int right) {
            return (frame ? QString(g[left]) : QString()) + QString(3, g[0])
                + (columns ? QString(g[middle]) : QString()) + QString(3, g[0])
                + (frame ? QString(g[right]) : QString());
        };
        int y = 0;
        if (frame) QCOMPARE(f.line(y++).left(width), line(2, 6, 3));
        QCOMPARE(f.line(y++).left(width), content("A  ", "B  "));
        QCOMPARE(f.line(y++).left(width), line(8, 10, 9));
        QCOMPARE(f.line(y++).left(width), content("x  ", "x  "));
        if (rows) QCOMPARE(f.line(y++).left(width), line(8, 10, 9));
        QCOMPARE(f.line(y++).left(width), content("1  ", "1  "));
        if (frame) QCOMPARE(f.line(y++).left(width), line(4, 7, 5));
        QCOMPARE(y, height);
    }

    void gridViewportAndInput() {
        CountingTableList model;
        TableFixture f;
        QVERIFY(f.create(&model, textDelegate));
        f.table->setColumns({QVariantMap{{"title", "A"}, {"width", 3}},
                             QVariantMap{{"title", "B"}, {"width", 3}}});
        f.table->setShowOuterBorder(true);
        f.table->setShowRowGrid(true);
        f.table->setShowColumnGrid(true);
        f.table->setWidth(9);
        f.table->setHeight(9); // Five body lines: three rows, two dividers.
        QVERIFY(f.buffer.resize(40, 12));
        f.paint();
        QCOMPARE(f.line(7).left(9), QStringLiteral("│2  │2  │"));
        QCOMPARE(f.items().size(), 10); // Three visible plus two overscan, two columns.
        QSignalSpy headers(f.table, &TableView::headerClicked);
        QSignalSpy activated(f.table, &TableView::rowActivated);
        for (const QPoint &point : {QPoint(0, 3), QPoint(8, 3), QPoint(2, 0), QPoint(2, 8),
                                   QPoint(4, 3), QPoint(2, 2), QPoint(2, 4)}) {
            MouseEvent click(MouseEvent::Type::Press, point.x(), point.y(), 0);
            f.table->mousePressEvent(&click);
            QVERIFY(!click.isAccepted());
        }
        QVERIFY(activated.isEmpty());
        MouseEvent header(MouseEvent::Type::Press, 6, 1, 0);
        f.table->mousePressEvent(&header);
        QCOMPARE(headers.size(), 1);
        QCOMPARE(headers.first().first().toInt(), 1);
        MouseEvent body(MouseEvent::Type::Press, 6, 7, 0);
        f.table->mousePressEvent(&body);
        QCOMPARE(f.table->selectedRow(), 2);
        QCOMPARE(f.table->currentColumn(), 1);
        KeyEvent page(Qt::Key_PageDown, QString());
        f.table->keyPressEvent(&page);
        QCOMPARE(f.table->selectedRow(), 5);
        f.paint();
        QCOMPARE(f.line(3).left(9), QStringLiteral("│3  │3  │"));
        QCOMPARE(f.line(7).left(9), QStringLiteral("│5  │5  │"));
        f.table->setHeight(8); // Four body lines: two rows and one empty line.
        f.paint();
        QCOMPARE(f.line(3).left(9), QStringLiteral("│4  │4  │"));
        QCOMPARE(f.line(6).left(9), QStringLiteral("│   │   │")); // Unused line keeps its separators.
        MouseEvent empty(MouseEvent::Type::Press, 2, 6, 0);
        f.table->mousePressEvent(&empty);
        QVERIFY(!empty.isAccepted());
        f.table->setShowRowGrid(false);
        f.paint();
        QCOMPARE(f.line(3).left(9), QStringLiteral("│2  │2  │"));
        QCOMPARE(f.line(6).left(9), QStringLiteral("│5  │5  │"));
        KeyEvent up(Qt::Key_PageUp, QString());
        f.table->keyPressEvent(&up);
        QCOMPARE(f.table->selectedRow(), 1);
    }

    void gridDelegates_data() {
        QTest::addColumn<bool>("rowDelegate");
        QTest::newRow("cells") << false;
        QTest::newRow("rows") << true;
    }
    void gridDelegates() {
        QFETCH(bool, rowDelegate);
        CountingTableList model;
        model.count = 2;
        TableFixture f;
        const QString delegate = (rowDelegate ? "rowDelegate" : "delegate") + QStringLiteral(R"(: Item {
            required property int row
            width: 99; height: 99
            Rectangle { x: -2; y: -2; width: 99; height: 99; color: "#123456" }
            Text { text: "ABCDEFGHIJKLM"; width: 99 }
        })");
        QVERIFY2(f.create(&model, delegate), qPrintable(f.component.errorString()));
        f.table->setColumns({QVariantMap{{"title", "A"}, {"width", 3}},
                             QVariantMap{{"title", "B"}, {"width", 3}}});
        f.table->setWidth(9); f.table->setHeight(7);
        f.table->setShowColumnGrid(true); f.table->setShowRowGrid(true); f.table->setShowOuterBorder(true);
        f.paint();
        QCOMPARE(f.line(0).left(9), QStringLiteral("┌───┬───┐"));
        QCOMPARE(f.line(3).left(9), rowDelegate ? QStringLiteral("│ABC│EFG│") : QStringLiteral("│ABC│ABC│"));
        QCOMPARE(f.line(4).left(9), QStringLiteral("├───┼───┤"));
        QCOMPARE(f.line(6).left(9), QStringLiteral("└───┴───┘"));
        QCOMPARE(f.buffer.cellAt(2, 3).bgColor, 0xff123456u);
        QCOMPARE(f.buffer.cellAt(2, 4).bgColor, 0u);
        QCOMPARE(f.buffer.cellAt(9, 3).bgColor, 0u);
        f.table->setHeight(2);
        f.paint();
        QVERIFY(f.items().isEmpty());
    }

    void wideDelegateClipping() {
        CountingTableList model;
        model.count = 1;
        for (bool rowMode : {false, true}) {
            TableFixture f;
            const QString delegate = (rowMode ? "rowDelegate" : "delegate") + QStringLiteral(R"(: Item {
                Text { x: 2; width: 20; text: "界" }
                Text { y: 1; width: 20; text: "overflow" }
            })");
            QVERIFY(f.create(&model, delegate));
            f.table->setColumns({QVariantMap{{"title", "A"}, {"width", 3}},
                                 QVariantMap{{"title", "B"}, {"width", 3}}});
            f.table->setWidth(9); f.table->setHeight(5);
            f.table->setShowOuterBorder(true); f.table->setShowColumnGrid(true);
            f.paint();
            QCOMPARE(f.line(3).left(9), QStringLiteral("│   │   │"));
            QCOMPARE(f.line(4).left(9), QStringLiteral("└───┴───┘"));
            for (int x = 0; x < 9; ++x) {
                QVERIFY(!f.buffer.cellAt(x, 3).continuation);
                QCOMPARE(f.buffer.cellAt(x, 3).displayWidth, 1);
            }
        }
    }

    void gridModelMutations() {
        QStandardItemModel model;
        for (const QString &label : {"c", "a", "b"}) model.appendRow(new QStandardItem(label));
        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&model); proxy.sort(0);
        TableFixture f;
        QVERIFY(f.createBody(&proxy, "showOuterBorder: true; showRowGrid: true; " + textDelegate));
        f.table->setWidth(6); f.table->setHeight(7);
        f.table->setSelectedRow(1); f.paint();
        QCOMPARE(f.line(3).left(6), QStringLiteral("│b   │"));
        const auto items = f.items();
        QSet<Item *> before(items.cbegin(), items.cend());
        proxy.sort(0, Qt::DescendingOrder); f.paint();
        const auto reordered = f.items();
        QCOMPARE(QSet<Item *>(reordered.cbegin(), reordered.cend()), before);
        QCOMPARE(f.line(1).left(6), QStringLiteral("│c   │"));
        model.insertRow(0, new QStandardItem("d")); f.paint();
        QCOMPARE(f.table->selectedRow(), 2);
        QCOMPARE(f.line(5).left(6), QStringLiteral("│b   │"));
        model.removeRow(0); f.paint();
        QCOMPARE(f.table->selectedRow(), 1);
        QCOMPARE(f.line(3).left(6), QStringLiteral("│b   │"));
        model.clear(); f.paint();
        QCOMPARE(f.table->selectedRow(), -1);
        QVERIFY(f.items().isEmpty());
        QCOMPARE(f.line(1).left(6), QStringLiteral("│    │"));
    }

    void gridLazyFetching() {
        FetchingTableList model;
        TableFixture f;
        QVERIFY(f.createBody(&model, "showRowGrid: true; showOuterBorder: true", 0));
        QCOMPARE(model.fetches, 0);
        f.table->setHeight(7); // Headerless: three visible content rows.
        f.paint(); f.paint();
        QCOMPARE(model.count, 8); // Three visible, two overscan, threshold two, batches of two.
        QCOMPARE(f.line(5).mid(1, 3).trimmed(), QStringLiteral("2"));
        f.table->setSelectedRow(7);
        f.paint(); f.paint();
        QCOMPARE(model.count, 14);
        QCOMPARE(f.line(5).mid(1, 3).trimmed(), QStringLiteral("7"));
        f.table->setWidth(0);
        const int fetches = model.fetches;
        f.table->setSelectedRow(11);
        f.paint();
        QCOMPARE(model.fetches, fetches);
    }

    void gridWidthsAndClipping() {
        CountingTableList model;
        model.count = 1; model.first = QStringLiteral("界ab");
        TableFixture f;
        QVERIFY(f.create(&model, QString()));
        f.table->setColumns({QVariantMap{{"title", "X"}, {"width", 4}, {"headerAlign", "center"}},
                             QVariantMap{{"title", "HIDDEN"}, {"width", 0}},
                             QVariantMap{{"title", "Y"}, {"width", 4}}});
        f.table->setWidth(11); f.table->setHeight(5);
        f.table->setShowColumnGrid(true); f.table->setShowOuterBorder(true);
        f.paint();
        QCOMPARE(f.line(0).left(11), QStringLiteral("┌────┬────┐"));
        QCOMPARE(f.line(1).left(11), QStringLiteral("│ X  │Y   │"));
        QCOMPARE(QString::fromUtf8(f.buffer.cellAt(1, 3).character()), QStringLiteral("界"));
        QVERIFY(f.buffer.cellAt(2, 3).continuation);
        QCOMPARE(QString::fromUtf8(f.buffer.cellAt(3, 3).character()), QStringLiteral("a"));
        f.table->setWidth(8); // The second column has only one visible content cell.
        f.paint();
        QCOMPARE(QString::fromUtf8(f.buffer.cellAt(6, 3).character()), QStringLiteral(" "));
        QCOMPARE(QString::fromUtf8(f.buffer.cellAt(7, 3).character()), QStringLiteral("│"));
        f.buffer.clear();
        testing::paintItem(f.table, f.buffer, 0, 0, {2, 1, 4, 3});
        QCOMPARE(QString::fromUtf8(f.buffer.cellAt(1, 3).character()), QStringLiteral(" "));
        QCOMPARE(QString::fromUtf8(f.buffer.cellAt(2, 3).character()), QStringLiteral(" "));
        QCOMPARE(QString::fromUtf8(f.buffer.cellAt(3, 3).character()), QStringLiteral("a"));
        QCOMPARE(QString::fromUtf8(f.buffer.cellAt(7, 3).character()), QStringLiteral(" "));
        f.table->setShowColumnGrid(false);
        f.table->setWidth(10);
        f.paint();
        QCOMPARE(f.line(0).left(10), QStringLiteral("┌────────┐"));
        QCOMPARE(f.line(1).left(10), QStringLiteral("│ X  Y   │"));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Unsupported TableView gridStyle:.*bogus.*"));
        f.table->setGridStyle("bogus");
        QCOMPARE(f.table->gridStyle(), QStringLiteral("single"));
    }

    void tinyAndEmptyGrids() {
        CountingTableList model;
        model.count = 0;
        TableFixture f;
        QVERIFY(f.create(&model, textDelegate));
        f.table->setShowOuterBorder(true); f.table->setShowRowGrid(true);
        f.table->setWidth(5); f.table->setHeight(7);
        f.paint();
        QCOMPARE(f.line(0).left(5), QStringLiteral("┌───┐"));
        QCOMPARE(f.line(3).left(5), QStringLiteral("│   │"));
        QCOMPARE(f.line(6).left(5), QStringLiteral("└───┘"));
        for (int width = 0; width <= 3; ++width) {
            for (int height = 0; height <= 3; ++height) {
                f.table->setWidth(width); f.table->setHeight(height); f.paint();
                QVERIFY(f.items().isEmpty());
                if (width == 0 || height == 0) QVERIFY(f.line(0).trimmed().isEmpty());
                else if (width == 1 && height == 1) QCOMPARE(f.line(0).left(1), QStringLiteral("┼"));
                else if (width == 1) QCOMPARE(f.line(0).left(1), QStringLiteral("│"));
                else if (height == 1) QCOMPARE(f.line(0).left(width), QString(width, QChar(u'─')));
                else {
                    QCOMPARE(f.line(0).left(width), QStringLiteral("┌") + QString(width - 2, QChar(u'─')) + QStringLiteral("┐"));
                    QCOMPARE(f.line(height - 1).left(width), QStringLiteral("└") + QString(width - 2, QChar(u'─')) + QStringLiteral("┘"));
                }
            }
        }
        // Headerless auto-sized model columns reserve the grid before distributing width.
        QStandardItemModel headerless(1, 2);
        headerless.setData(headerless.index(0, 0), "L");
        headerless.setData(headerless.index(0, 1), "R");
        TableFixture other;
        QVERIFY(other.createBody(&headerless, "showOuterBorder: true; showColumnGrid: true; showRowGrid: true"));
        other.table->setWidth(9); other.table->setHeight(3); other.paint();
        QCOMPARE(other.line(1).left(9), QStringLiteral("│L  │R  │"));
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

    void smartImplicitSize() {
        TableView table;
        // Default implicit size is 0x0
        QCOMPARE(table.implicitWidth(), 0);
        QCOMPARE(table.implicitHeight(), 0);

        QSignalSpy spy(&table, &TableView::implicitWidthChanged);

        // Columns define implicit width
        table.setColumns(QVariantList{
            QVariantMap{{"title", "ID"}, {"width", 10}},
            QVariantMap{{"title", "NAME"}, {"width", 20}}
        });
        QCOMPARE(table.implicitWidth(), 30);
        QCOMPARE(spy.count(), 1);

        // Setting same columns does not re-emit (deduplication)
        table.setColumns(table.columns());
        QCOMPARE(spy.count(), 1);

        // Column grid adds separators (2 columns -> 1 separator)
        table.setShowColumnGrid(true);
        QCOMPARE(table.implicitWidth(), 31);
        QCOMPARE(spy.count(), 2);

        // Outer border adds 2 cells (left + right borders)
        table.setShowOuterBorder(true);
        QCOMPARE(table.implicitWidth(), 33);
        QCOMPARE(spy.count(), 3);

        // Hidden column (width <= 0) is excluded from total width and grid separator count
        table.setColumns(QVariantList{
            QVariantMap{{"title", "A"}, {"width", 10}},
            QVariantMap{{"title", "B"}, {"width", 0}},
            QVariantMap{{"title", "C"}, {"width", 15}}
        });
        // 2 visible columns (width 10 + 15 = 25), 1 separator, 2 outer borders = 28
        QCOMPARE(table.implicitWidth(), 28);
        QCOMPARE(spy.count(), 4);

        // Clearing columns drops implicit width back to 0
        table.setColumns({});
        QCOMPARE(table.implicitWidth(), 0);
        QCOMPARE(spy.count(), 5);

        // Explicit user override in QML / C++ takes precedence
        table.setImplicitWidth(80);
        QCOMPARE(table.implicitWidth(), 80);
        QCOMPARE(spy.count(), 6);
    }

    void gridRenderedWhenHeightExceedsRows() {
        TableFixture f;
        QStandardItemModel model;
        model.appendRow({new QStandardItem(QStringLiteral("1")), new QStandardItem(QStringLiteral("2"))});
        QVERIFY(f.createBody(&model, QStringLiteral(
            "showOuterBorder: true; showColumnGrid: true; "
            "columns: [{title: 'A', width: 4}, {title: 'B', width: 4}]")));
        f.table->setWidth(11);
        f.table->setHeight(6);
        f.paint();

        // Line 3 is row 0 with data
        QCOMPARE(f.line(3).left(11), QStringLiteral("│1   │2   │"));
        // Line 4 is an empty row beyond the model, but column grid separator MUST be rendered
        QCOMPARE(f.line(4).left(11), QStringLiteral("│    │    │"));
        // Line 5 is bottom border with bottom tee junction
        QCOMPARE(f.line(5).left(11), QStringLiteral("└────┴────┘"));

        // Now test with showRowGrid: true and height 7
        TableFixture f2;
        QStandardItemModel model2;
        model2.appendRow({new QStandardItem(QStringLiteral("1")), new QStandardItem(QStringLiteral("2"))});
        QVERIFY(f2.createBody(&model2, QStringLiteral(
            "showOuterBorder: true; showColumnGrid: true; showRowGrid: true; "
            "columns: [{title: 'A', width: 4}, {title: 'B', width: 4}]")));
        f2.table->setWidth(11);
        f2.table->setHeight(7);
        f2.paint();

        QCOMPARE(f2.line(3).left(11), QStringLiteral("│1   │2   │"));
        QCOMPARE(f2.line(4).left(11), QStringLiteral("├────┼────┤"));
        QCOMPARE(f2.line(5).left(11), QStringLiteral("│    │    │"));
        QCOMPARE(f2.line(6).left(11), QStringLiteral("└────┴────┘"));
    }

    void stretchLastColumn() {
        TableFixture f;
        QStandardItemModel model;
        model.appendRow({new QStandardItem(QStringLiteral("A1")), new QStandardItem(QStringLiteral("B1"))});
        QVERIFY(f.createBody(&model, QStringLiteral(
            "stretchLastColumn: true; showColumnGrid: true; showOuterBorder: false; "
            "columns: [{title: 'COL1', width: 8}, {title: 'COL2', width: 6}]")));
        f.table->setWidth(30);
        f.table->setHeight(5);
        f.paint();

        // COL1 is width 8 (x: 0..7). Separator is at x=8.
        // Available width for COL2 is 30 - 8 - 1 = 21.
        // Line 0 is header: COL1(8) + │(1) + COL2 padded to 21
        QCOMPARE(f.line(0).left(30), QString::fromUtf8("COL1    │COL2                 "));
        // Line 1 is divider: 8 '─' + '┼' + 21 '─'
        QCOMPARE(f.line(1).left(30), QString::fromUtf8("────────┼─────────────────────"));
        // Line 2 is row 0: A1(8) + │(1) + B1 padded to 21
        QCOMPARE(f.line(2).left(30), QString::fromUtf8("A1      │B1                   "));
    }

    void columnDescriptorStretch() {
        TableFixture f;
        QStandardItemModel model;
        model.appendRow({new QStandardItem(QStringLiteral("A")), new QStandardItem(QStringLiteral("B")), new QStandardItem(QStringLiteral("C"))});
        QVERIFY(f.createBody(&model, QStringLiteral(
            "showColumnGrid: true; showOuterBorder: false; "
            "columns: [{title: 'A', width: 4}, {title: 'B', stretch: true}, {title: 'C', width: 5}]")));
        f.table->setWidth(25);
        f.table->setHeight(5);
        f.paint();

        // A is width 4 (x: 0..3). Separator at x=4.
        // C is width 5. Separator before C is 1.
        // So B starts at x=5, available space is 25 - 5 - (5 + 1) = 14.
        // Separator after B is at x=19. C is at x=20..24.
        QCOMPARE(f.line(0).left(25), QString::fromUtf8("A   │B             │C    "));
        QCOMPARE(f.line(1).left(25), QString::fromUtf8("────┼──────────────┼─────"));
        QCOMPARE(f.line(2).left(25), QString::fromUtf8("A   │B             │C    "));
    }

    void gridColorOption() {
        TableFixture f;
        QStandardItemModel model;
        model.appendRow({new QStandardItem(QStringLiteral("val1")), new QStandardItem(QStringLiteral("val2"))});
        QVERIFY(f.createBody(&model, QStringLiteral(
            "showColumnGrid: true; showOuterBorder: true; textColor: '#00ff00'; gridColor: '#ff0000'; "
            "columns: [{title: 'A', width: 5}, {title: 'B', width: 5}]")));
        f.table->setWidth(13);
        f.table->setHeight(5);
        f.paint();

        // Border at (0, 0) should have gridColor #ff0000
        QCOMPARE(f.buffer.cellAt(0, 0).fgColor, 0xffff0000u);
        // Separator at (6, 1) in header line should have gridColor #ff0000
        QCOMPARE(f.buffer.cellAt(6, 1).fgColor, 0xffff0000u);
        // Data text at (1, 3) ('v') should have textColor #00ff00
        QCOMPARE(f.buffer.cellAt(1, 3).fgColor, 0xff00ff00u);
    }

    void continuousRowAndHeaderBackground() {
        TableFixture f;
        QStandardItemModel model;
        model.appendRow({new QStandardItem(QStringLiteral("val1")), new QStandardItem(QStringLiteral("val2"))});
        QVERIFY(f.createBody(&model, QStringLiteral(
            "showColumnGrid: true; showOuterBorder: true; "
            "headerBackgroundColor: '#224466'; rowBackgroundColor: '#335577'; "
            "columns: [{title: 'A', width: 5}, {title: 'B', width: 5}]")));
        f.table->setWidth(20); // wider than 5+5+3 = 13
        f.table->setHeight(5);
        f.paint();

        // Header line is y=1. Check interior x from 1 to 18 (width 20, inset 1):
        for (int x = 1; x < 19; ++x) {
            QCOMPARE(f.buffer.cellAt(x, 1).bgColor, 0xff224466u);
        }
        // Body row 0 is y=3. Check interior x from 1 to 18:
        for (int x = 1; x < 19; ++x) {
            QCOMPARE(f.buffer.cellAt(x, 3).bgColor, 0xff335577u);
        }
    }
};

QTEST_MAIN(TestTableViewHardening)
#include "test_table_view_hardening.moc"
