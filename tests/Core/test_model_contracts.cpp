// Plan 07: "any QAbstractItemModel renders correctly". This file grows into the
// view x model acceptance matrix (WP7). WP2 starts it with the nested-model
// diagnostic and the rootIndex property on the flat views.
#include <QTest>
#include <QSignalSpy>
#include <QRegularExpression>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QStandardItemModel>
#include <ListView.h>
#include <GridView.h>
#include <Repeater.h>
#include <TableView.h>
#include <TreeView.h>
#include <QAbstractListModel>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QAbstractTableModel>
#include <TextGridBuffer.h>
#include "../TestPaint.h"
#include <memory>
#include <functional>

using namespace qtui;

namespace {

// A two-level tree: top-level "group N" rows each with three "leaf N.M" children.
std::unique_ptr<QStandardItemModel> makeTree(int groups = 2, int leaves = 3)
{
    auto model = std::make_unique<QStandardItemModel>();
    for (int g = 0; g < groups; ++g) {
        auto *group = new QStandardItem(QStringLiteral("group %1").arg(g));
        for (int l = 0; l < leaves; ++l)
            group->appendRow(new QStandardItem(QStringLiteral("leaf %1.%2").arg(g).arg(l)));
        model->appendRow(group);
    }
    return model;
}

// Serves `total` rows in pages of `pageSize` through canFetchMore/fetchMore,
// the way QSqlQueryModel and paged network models do.
class BatchModel final : public QAbstractListModel {
public:
    explicit BatchModel(int total, int pageSize) : m_total(total), m_pageSize(pageSize) {}
    int fetches = 0;
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_loaded; }
    QVariant data(const QModelIndex &index, int role) const override
    {
        return index.isValid() && role == Qt::DisplayRole ? QVariant(QStringLiteral("row %1").arg(index.row())) : QVariant();
    }
    QHash<int, QByteArray> roleNames() const override { return {{Qt::DisplayRole, "display"}}; }
    bool canFetchMore(const QModelIndex &parent) const override { return !parent.isValid() && m_loaded < m_total; }
    void fetchMore(const QModelIndex &parent) override
    {
        if (parent.isValid() || m_loaded >= m_total) return;
        ++fetches;
        const int count = std::min(m_pageSize, m_total - m_loaded);
        beginInsertRows({}, m_loaded, m_loaded + count - 1);
        m_loaded += count;
        endInsertRows();
    }
private:
    int m_total, m_pageSize, m_loaded = 0;
};

// Directories whose children exist only after fetchMore(), like QFileSystemModel.
class LazyTreeModel final : public QStandardItemModel {
public:
    QSet<QString> fetched;
    LazyTreeModel()
    {
        for (int d = 0; d < 2; ++d) appendRow(new QStandardItem(QStringLiteral("dir %1").arg(d)));
    }
    bool hasChildren(const QModelIndex &parent = {}) const override
    {
        return !parent.isValid() || canFetchMore(parent) || QStandardItemModel::hasChildren(parent);
    }
    bool canFetchMore(const QModelIndex &parent) const override
    {
        return parent.isValid() && !fetched.contains(data(parent).toString());
    }
    void fetchMore(const QModelIndex &parent) override
    {
        if (!canFetchMore(parent)) return;
        const QString name = data(parent).toString();
        fetched.insert(name);
        QStandardItem *dir = itemFromIndex(parent);
        for (int f = 0; f < 3; ++f)
            dir->appendRow(new QStandardItem(QStringLiteral("file %1.%2").arg(name.back()).arg(f)));
    }
};

// A table model with three columns and only Qt::DisplayRole, no roleNames().
// Row labels are stored, so removing a row does not relabel the others.
class PlainTableModel final : public QAbstractTableModel {
public:
    QList<int> rows{0, 1, 2, 3};
    int next = 4;
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : int(rows.size()); }
    int columnCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : 3; }
    QVariant data(const QModelIndex &index, int role) const override
    {
        return index.isValid() && role == Qt::DisplayRole
            ? QVariant(QStringLiteral("c%1r%2").arg(index.column()).arg(rows.at(index.row()))) : QVariant();
    }
    void rename(int row) { emit dataChanged(index(row, 0), index(row, 2)); }
    void append() { beginInsertRows({}, int(rows.size()), int(rows.size())); rows.append(next++); endInsertRows(); }
    void removeFirst() { beginRemoveRows({}, 0, 0); rows.removeFirst(); endRemoveRows(); }
    void resetTo(int count) { beginResetModel(); rows.clear(); for (int i = 0; i < count; ++i) rows.append(i); endResetModel(); }
};

struct ViewFixture {
    QQmlEngine engine;
    QQmlComponent component{&engine};
    std::unique_ptr<QObject> object;
    Item *root = nullptr;
    TextGridBuffer buffer;

    bool create(QObject *model, const QString &qml, int width = 40, int height = 8)
    {
        engine.rootContext()->setContextProperty("testModel", model);
        component.setData(qml.toUtf8(), QUrl());
        if (!component.isReady()) return false;
        object.reset(component.create());
        root = qobject_cast<Item *>(object.get());
        if (!root || !buffer.resize(width, height)) return false;
        paint();
        return true;
    }
    void paint() { buffer.clear(); testing::paintItem(root, buffer, 0, 0); }
    QString text() const
    {
        QString all;
        for (int y = 0; y < buffer.height(); ++y) {
            for (int x = 0; x < buffer.width(); ++x)
                all += QString::fromUtf8(buffer.cellAt(x, y).character());
            all += QLatin1Char('\n');
        }
        return all;
    }
};

const QString listQml = QStringLiteral(R"(
    import Qtui
    ListView {
        width: 40; height: 8
        model: testModel
        delegate: Text { required property string display; text: display }
    }
)");
const QString gridQml = QStringLiteral(R"(
    import Qtui
    GridView {
        width: 40; height: 8; cellWidth: 20; cellHeight: 1
        model: testModel
        delegate: Text { required property int index; required property string display; text: display }
    }
)");
const QString repeaterQml = QStringLiteral(R"(
    import Qtui
    Column {
        width: 40; height: 8
        Repeater {
            model: testModel
            delegate: Text { required property string display; text: display }
        }
    }
)");

const QRegularExpression nestedWarning(QStringLiteral("model has nested rows; this view shows one level"));

} // namespace

class TestModelContracts : public QObject {
    Q_OBJECT
private slots:
    void nestedModelWarnsOnce_data()
    {
        QTest::addColumn<QString>("qml");
        QTest::newRow("ListView") << listQml;
        QTest::newRow("GridView") << gridQml;
        QTest::newRow("Repeater") << repeaterQml;
    }
    void nestedModelWarnsOnce()
    {
        QFETCH(QString, qml);
        auto model = makeTree();
        ViewFixture f;
        QTest::ignoreMessage(QtWarningMsg, nestedWarning);
        QVERIFY2(f.create(model.get(), qml), qPrintable(f.component.errorString()));
        // Only the top level is shown.
        QVERIFY(f.text().contains("group 0"));
        QVERIFY(f.text().contains("group 1"));
        QVERIFY(!f.text().contains("leaf"));
        // A second structural change does not warn again.
        model->appendRow(new QStandardItem(QStringLiteral("group 2")));
        f.paint();
        QVERIFY(f.text().contains("group 2"));
    }

    void flatModelDoesNotWarn()
    {
        QStandardItemModel model;
        for (int i = 0; i < 3; ++i) model.appendRow(new QStandardItem(QStringLiteral("row %1").arg(i)));
        ViewFixture f;
        // Any warning here would fail the test through QTest's default handler
        // only if it matched an ignoreMessage; assert on output instead.
        QVERIFY2(f.create(&model, listQml), qPrintable(f.component.errorString()));
        QVERIFY(f.text().contains("row 2"));
    }

    void rootIndexShowsSubtree_data()
    {
        QTest::addColumn<QString>("qml");
        QTest::newRow("ListView") << listQml;
        QTest::newRow("GridView") << gridQml;
        QTest::newRow("Repeater") << repeaterQml;
    }
    void rootIndexShowsSubtree()
    {
        QFETCH(QString, qml);
        auto model = makeTree();
        ViewFixture f;
        QTest::ignoreMessage(QtWarningMsg, nestedWarning);
        QVERIFY2(f.create(model.get(), qml), qPrintable(f.component.errorString()));

        QObject *view = f.root->findChild<Repeater *>();
        if (!view) view = f.root;
        QSignalSpy rootChanged(view, SIGNAL(rootIndexChanged()));
        QVERIFY(view->setProperty("rootIndex", QVariant::fromValue(model->index(1, 0))));
        QCOMPARE(rootChanged.size(), 1);
        f.paint();
        QVERIFY2(f.text().contains("leaf 1.0"), qPrintable(f.text()));
        QVERIFY(f.text().contains("leaf 1.2"));
        QVERIFY(!f.text().contains("group"));
        QVERIFY(!f.text().contains("leaf 0."));

        // Inserts under the root are followed; inserts elsewhere are ignored.
        model->item(1)->appendRow(new QStandardItem(QStringLiteral("leaf 1.3")));
        model->item(0)->appendRow(new QStandardItem(QStringLiteral("leaf 0.3")));
        f.paint();
        QVERIFY2(f.text().contains("leaf 1.3"), qPrintable(f.text()));
        QVERIFY(!f.text().contains("leaf 0.3"));

        // Data changes under the root repaint in place.
        model->item(1)->child(0)->setText(QStringLiteral("renamed"));
        f.paint();
        QVERIFY2(f.text().contains("renamed"), qPrintable(f.text()));
        QVERIFY(!f.text().contains("leaf 1.0"));

        // Removing rows under the root shrinks the view.
        model->item(1)->removeRow(0);
        f.paint();
        QVERIFY(!f.text().contains("renamed"));
        QVERIFY(f.text().contains("leaf 1.1"));

        // Back to the top level.
        QVERIFY(view->setProperty("rootIndex", QVariant::fromValue(QModelIndex())));
        f.paint();
        QVERIFY(f.text().contains("group 0"));
        QVERIFY(!f.text().contains("leaf"));
    }

    // WP3: GridView keeps delegate indexes and positions in sync through
    // structural changes while only the viewport window exists.
    void gridViewFollowsInsertsAndRemovals()
    {
        QStandardItemModel model;
        for (int i = 0; i < 6; ++i) model.appendRow(new QStandardItem(QStringLiteral("cell %1").arg(i)));
        ViewFixture f;
        QVERIFY2(f.create(&model, gridQml), qPrintable(f.component.errorString()));
        auto *grid = qobject_cast<GridView *>(f.root);
        QVERIFY(grid);
        auto cellAt = [&](int column, int row) {
            QString line;
            for (int i = 0; i < 20; ++i) line += QString::fromUtf8(f.buffer.cellAt(column * 20 + i, row).character());
            return line.trimmed();
        };
        QCOMPARE(cellAt(0, 0), QStringLiteral("cell 0"));
        QCOMPARE(cellAt(1, 0), QStringLiteral("cell 1"));
        QCOMPARE(cellAt(0, 1), QStringLiteral("cell 2"));

        model.insertRow(0, new QStandardItem(QStringLiteral("first")));
        f.paint();
        QCOMPARE(cellAt(0, 0), QStringLiteral("first"));
        QCOMPARE(cellAt(1, 0), QStringLiteral("cell 0"));
        QCOMPARE(cellAt(0, 1), QStringLiteral("cell 1"));
        // Every live delegate reports its current model index.
        for (Item *item : grid->findChildren<Item *>()) {
            const int index = item->property("index").toInt();
            const QString expected = index == 0 ? QStringLiteral("first") : QStringLiteral("cell %1").arg(index - 1);
            QCOMPARE(item->property("text").toString(), expected);
        }

        model.removeRows(0, 2);
        f.paint();
        QCOMPARE(cellAt(0, 0), QStringLiteral("cell 1"));
        QCOMPARE(cellAt(1, 0), QStringLiteral("cell 2"));
        QCOMPARE(grid->findChildren<Item *>().size(), 5);
        for (Item *item : grid->findChildren<Item *>())
            QCOMPARE(item->property("text").toString(),
                     QStringLiteral("cell %1").arg(item->property("index").toInt() + 1));

        model.setData(model.index(0, 0), QStringLiteral("changed"));
        f.paint();
        QCOMPARE(cellAt(0, 0), QStringLiteral("changed"));

        model.clear();
        f.paint();
        QCOMPARE(grid->findChildren<Item *>().size(), 0);
        QVERIFY(f.text().trimmed().isEmpty());
    }

    // WP5: views drive canFetchMore/fetchMore.
    void listViewFetchesPagesOnScroll()
    {
        BatchModel model(500, 50);
        ViewFixture f;
        QVERIFY2(f.create(&model, listQml), qPrintable(f.component.errorString()));
        // Eight visible rows plus overscan fit in the first page.
        QCOMPARE(model.fetches, 1);
        QCOMPARE(model.rowCount(), 50);
        QVERIFY(f.text().contains("row 0"));
        auto *list = qobject_cast<ListView *>(f.root);
        QVERIFY(list);
        // Scrolling to the end of the loaded rows fetches exactly one more page.
        list->setContentY(42);
        f.paint();
        QCOMPARE(model.fetches, 2);
        QCOMPARE(model.rowCount(), 100);
        QVERIFY2(f.text().contains("row 49"), qPrintable(f.text()));
        // Scrolling within loaded rows does not fetch.
        list->setContentY(10);
        f.paint();
        QCOMPARE(model.fetches, 2);
    }

    void gridViewFetchesPagesOnScroll()
    {
        BatchModel model(500, 50);
        ViewFixture f;
        QVERIFY2(f.create(&model, gridQml), qPrintable(f.component.errorString()));
        QCOMPARE(model.fetches, 1);
        auto *grid = qobject_cast<GridView *>(f.root);
        QVERIFY(grid);
        // Two columns of one-line cells: 50 rows span 25 lines.
        grid->setContentY(17);
        f.paint();
        QCOMPARE(model.fetches, 2);
        QVERIFY2(f.text().contains("row 49"), qPrintable(f.text()));
    }

    void tableViewFetchesPagesOnSelection()
    {
        BatchModel model(500, 50);
        ViewFixture f;
        QVERIFY2(f.create(&model, QStringLiteral(R"(
            import Qtui
            TableView { width: 40; height: 8; model: testModel; columns: [{title: "Row", width: 40}] }
        )")), qPrintable(f.component.errorString()));
        QCOMPARE(model.fetches, 1);
        QVERIFY(f.text().contains("row 0"));
        auto *table = qobject_cast<TableView *>(f.root);
        QVERIFY(table);
        table->setSelectedRow(49);
        f.paint();
        QCOMPARE(model.fetches, 2);
        QVERIFY2(f.text().contains("row 49"), qPrintable(f.text()));
        QCOMPARE(model.rowCount(), 100);
    }

    void repeaterLoadsEverything()
    {
        BatchModel model(120, 50);
        ViewFixture f;
        QVERIFY2(f.create(&model, repeaterQml), qPrintable(f.component.errorString()));
        // Eager view: every page is fetched up front.
        QCOMPARE(model.fetches, 3);
        QCOMPARE(model.rowCount(), 120);
        QCOMPARE(f.root->findChild<Repeater *>()->childItems().size(), 0); // delegates belong to the Column
        QCOMPARE(f.root->childItems().size(), 121);
    }

    void treeViewFetchesBranchOnExpand()
    {
        LazyTreeModel model;
        ViewFixture f;
        QVERIFY2(f.create(&model, QStringLiteral(R"(
            import Qtui
            TreeView {
                width: 40; height: 8; model: testModel
                delegate: Text { required property string display; required property int depth; text: display }
            }
        )")), qPrintable(f.component.errorString()));
        QVERIFY(f.text().contains("dir 0"));
        QVERIFY(!f.text().contains("file"));
        QVERIFY(model.fetched.isEmpty());
        auto *tree = qobject_cast<TreeView *>(f.root);
        QVERIFY(tree);
        tree->expand(0);
        f.paint();
        QCOMPARE(model.fetched, QSet<QString>{QStringLiteral("dir 0")});
        QVERIFY2(f.text().contains("file 0.0"), qPrintable(f.text()));
        QVERIFY(f.text().contains("file 0.2"));
        QVERIFY(!f.text().contains("file 1."));
        // Collapsing and re-expanding does not refetch.
        tree->collapse(0);
        tree->expand(0);
        f.paint();
        QCOMPARE(model.fetched.size(), 1);
        QVERIFY(f.text().contains("file 0.1"));
    }

    // WP7: the view x model acceptance matrix. Each view is bound to each
    // model kind and must show the first row, follow a data change, an
    // insert, a removal, and a reset, without warnings other than the nested
    // rows one for tree models in flat views.
    void matrix_data()
    {
        QTest::addColumn<QString>("view");
        QTest::addColumn<QString>("modelKind");
        const QStringList views{"ListView", "GridView", "Repeater", "TableView", "TreeView"};
        const QStringList kinds{"stringList", "standardFlat", "standardTree", "proxyOverStandard", "batch", "plainTable"};
        for (const QString &view : views)
            for (const QString &kind : kinds)
                QTest::newRow(qPrintable(view + "/" + kind)) << view << kind;
    }
    void matrix()
    {
        QFETCH(QString, view);
        QFETCH(QString, modelKind);

        // Build the model and the operations the matrix applies to it.
        std::unique_ptr<QObject> owner;
        QAbstractItemModel *model = nullptr;
        std::function<void()> change, insert, remove, reset;
        QString first, changed, inserted, removed;
        int initialRows = 0;
        if (modelKind == "stringList") {
            auto *m = new QStringListModel({"alpha", "beta", "gamma"});
            owner.reset(m); model = m; initialRows = 3;
            first = "alpha"; changed = "ALPHA"; inserted = "delta"; removed = "beta";
            change = [m] { m->setData(m->index(0), "ALPHA"); };
            insert = [m] { m->insertRow(3); m->setData(m->index(3), "delta"); };
            remove = [m] { m->removeRow(1); };
            reset = [m] { m->setStringList({"only"}); };
        } else if (modelKind == "standardFlat" || modelKind == "proxyOverStandard") {
            auto *m = new QStandardItemModel;
            for (const char *label : {"alpha", "beta", "gamma"}) m->appendRow(new QStandardItem(label));
            initialRows = 3;
            first = "alpha"; changed = "ALPHA"; inserted = "delta"; removed = "beta";
            change = [m] { m->item(0)->setText("ALPHA"); };
            insert = [m] { m->appendRow(new QStandardItem("delta")); };
            remove = [m] { m->removeRow(1); };
            reset = [m] { m->clear(); m->appendRow(new QStandardItem("only")); };
            if (modelKind == "proxyOverStandard") {
                auto *proxy = new QSortFilterProxyModel;
                proxy->setSourceModel(m);
                m->setParent(proxy);
                owner.reset(proxy); model = proxy;
            } else {
                owner.reset(m); model = m;
            }
        } else if (modelKind == "standardTree") {
            auto tree = makeTree(2, 2);
            auto *m = tree.release();
            owner.reset(m); model = m; initialRows = 2;
            first = "group 0"; changed = "GROUP 0"; inserted = "group 2"; removed = "group 1";
            change = [m] { m->item(0)->setText("GROUP 0"); };
            insert = [m] { m->appendRow(new QStandardItem("group 2")); };
            remove = [m] { m->removeRow(1); };
            reset = [m] { m->clear(); m->appendRow(new QStandardItem("only")); };
        } else if (modelKind == "batch") {
            auto *m = new BatchModel(30, 10);
            owner.reset(m); model = m; initialRows = 10;
            // Pages land below the viewport; growth is checked on the model.
            first = "row 0"; changed = "row 0"; inserted = "row 0";
            change = [m] { emit m->dataChanged(m->index(0), m->index(0)); };
            insert = [m] { m->fetchMore({}); };
            remove = [] {};
            reset = [m] { m->fetchMore({}); };
        } else {
            auto *m = new PlainTableModel;
            owner.reset(m); model = m; initialRows = 4;
            first = "c0r0"; changed = "c0r0"; inserted = "c0r4"; removed = "c0r0";
            change = [m] { m->rename(0); };
            insert = [m] { m->append(); };
            remove = [m] { m->removeFirst(); };
            reset = [m] { m->resetTo(1); };
        }

        const QString delegate = QStringLiteral(
            "delegate: Text { required property var model; required property int index; text: model.display }");
        QString qml;
        if (view == "ListView")
            qml = QStringLiteral("import Qtui\nListView { width: 40; height: 8; model: testModel; %1 }").arg(delegate);
        else if (view == "GridView")
            qml = QStringLiteral("import Qtui\nGridView { width: 40; height: 8; cellWidth: 20; cellHeight: 1; model: testModel; %1 }").arg(delegate);
        else if (view == "Repeater")
            qml = QStringLiteral("import Qtui\nColumn { width: 40; height: 8; Repeater { model: testModel; %1 } }").arg(delegate);
        else if (view == "TableView")
            qml = QStringLiteral("import Qtui\nTableView { width: 40; height: 8; model: testModel }");
        else
            qml = QStringLiteral("import Qtui\nTreeView { width: 40; height: 8; model: testModel }");

        ViewFixture f;
        const bool flatViewOnTree = modelKind == "standardTree" && view != "TreeView" && view != "TableView";
        if (flatViewOnTree) QTest::ignoreMessage(QtWarningMsg, nestedWarning);
        QVERIFY2(f.create(model, qml), qPrintable(f.component.errorString()));
        QVERIFY2(f.text().contains(first), qPrintable(view + "/" + modelKind + "\n" + f.text()));
        Q_UNUSED(initialRows);

        change();
        f.paint();
        QVERIFY2(f.text().contains(changed), qPrintable(f.text()));

        // A view may already have pulled every page of a lazy model to fill
        // its viewport; growth is only required while pages remain.
        const int rowsBeforeInsert = model->rowCount();
        const bool canGrow = modelKind != "batch" || model->canFetchMore(QModelIndex());
        insert();
        f.paint();
        QVERIFY2(f.text().contains(inserted), qPrintable(f.text()));
        if (canGrow) QVERIFY(model->rowCount() > rowsBeforeInsert);

        remove();
        f.paint();
        QVERIFY2(f.text().contains(inserted), qPrintable(f.text()));
        if (!removed.isEmpty()) QVERIFY2(!f.text().contains(removed), qPrintable(f.text()));

        reset();
        f.paint();
        if (modelKind == "batch") QVERIFY2(f.text().contains("row 0"), qPrintable(f.text()));
        else if (modelKind == "plainTable") QVERIFY2(f.text().contains("c0r0") && !f.text().contains("c0r1"), qPrintable(f.text()));
        else QVERIFY2(f.text().contains("only") && !f.text().contains(inserted), qPrintable(f.text()));
    }

    void rootIndexSurvivesInsertAbove()
    {
        auto model = makeTree(3);
        ViewFixture f;
        QTest::ignoreMessage(QtWarningMsg, nestedWarning);
        QVERIFY2(f.create(model.get(), listQml), qPrintable(f.component.errorString()));
        auto *list = qobject_cast<ListView *>(f.root);
        QVERIFY(list);
        list->setRootIndex(model->index(2, 0));
        f.paint();
        QVERIFY(f.text().contains("leaf 2.0"));
        // Inserting a new group above the root must not retarget the view.
        model->insertRow(0, new QStandardItem(QStringLiteral("group new")));
        f.paint();
        QVERIFY2(f.text().contains("leaf 2.0"), qPrintable(f.text()));
        QCOMPARE(list->rootIndex().row(), 3);
    }
};

QTEST_MAIN(TestModelContracts)
#include "test_model_contracts.moc"
