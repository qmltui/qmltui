// Side-by-side model/view benchmark comparing QmlTui against Qt Quick and Qt
// Widgets over the same QAbstractListModel and viewport capacity (24 visible
// rows, or 4x24 cells for GridView).
//
//   benchmark_view_compare [--rows N] [--steps N] [--json <path>]

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTextStream>
#include <algorithm>
#include <memory>
#include <vector>
#if defined(__GLIBC__)
#include <malloc.h>
#endif

#include <QmlTui/Core/Items/GridView.h>
#include <QmlTui/Core/Items/ListView.h>
#include <QmlTui/Core/Items/Repeater.h>
#include <QmlTui/Core/Items/TableView.h>
#include <QmlTui/Core/Items/TreeView.h>
#include "../TestPaint.h"

#ifdef QMLTUI_BENCHMARK_HAS_QUICK
#include <QGuiApplication>
#include <QImage>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QSGRendererInterface>
#endif

#ifdef QMLTUI_BENCHMARK_HAS_WIDGETS
#include <QApplication>
#include <QFrame>
#include <QHeaderView>
#include <QImage>
#include <QListView>
#include <QScrollBar>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTreeView>
#endif

namespace {

constexpr int kTerminalCols = 80;
constexpr int kTerminalRows = 24;
constexpr int kPixelWidth = 640;
constexpr int kRowHeightPx = 20;
constexpr int kPixelHeight = kTerminalRows * kRowHeightPx; // 480 px = 24 rows

// Flat 1-column model shared across QmlTui, Qt Quick, and Qt Widgets views.
// Counts data() calls to verify that scrolling and mutations fetch visible rows.
class CountModel final : public QAbstractListModel {
public:
    explicit CountModel(int count) : m_count(count) {}

    int rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : m_count;
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid()) return {};
        ++reads;
        if (role != Qt::DisplayRole) return {};
        return QVariant(QStringLiteral("Row %1").arg(index.row() + m_generation));
    }

    QHash<int, QByteArray> roleNames() const override { return {{Qt::DisplayRole, "display"}}; }

    void touch(int row) {
        ++m_generation;
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx, {Qt::DisplayRole});
    }

    mutable qint64 reads = 0;

private:
    int m_count = 0;
    int m_generation = 0;
};

// Per-phase timings and counters for one framework + view + mode combination.
struct BenchmarkRow {
    QString framework;
    QString view;
    QString mode;
    int delegatesCreated = 0;
    double createUs = 0.0;
    double perDelegateUs = 0.0;
    double scrollSyncUs = 0.0;
    double scrollUs = 0.0;
    double changeSyncUs = 0.0;
    double changeUs = 0.0;
    double idleLayoutUs = 0.0;
    double idlePaintUs = 0.0;
    double readsPerScrollStep = 0.0;
};

qint64 allocatedHeapBytes() {
#if defined(__GLIBC__)
    return static_cast<qint64>(mallinfo2().uordblks);
#else
    QFile statm(QStringLiteral("/proc/self/statm"));
    if (!statm.open(QIODevice::ReadOnly)) return -1;
    const QList<QByteArray> fields = statm.readAll().split(' ');
    return fields.size() > 1 ? fields[1].toLongLong() * 4096 : -1;
#endif
}

QString qmlTuiViewSource(const QString& view, const QString& mode) {
    if (mode == QLatin1String("builtin")) {
        if (view == QLatin1String("TableView")) {
            return QStringLiteral("import QmlTui\n"
                                  "TableView { width: 80; height: 24; model: benchmarkModel; "
                                  "columns: [{title: \"Row\", width: 80}] }");
        }
        if (view == QLatin1String("TreeView")) {
            return QStringLiteral("import QmlTui\n"
                                  "TreeView { width: 80; height: 24; model: benchmarkModel }");
        }
    }
    const QString delegate =
        QStringLiteral("Text { required property string display; text: display }");
    QString body;
    if (view == QLatin1String("ListView")) {
        body = QStringLiteral(
            "ListView { width: 80; height: 24; model: benchmarkModel; delegate: %1 }");
    } else if (view == QLatin1String("GridView")) {
        body = QStringLiteral("GridView { width: 80; height: 24; cellWidth: 20; cellHeight: 1; "
                              "model: benchmarkModel; delegate: %1 }");
    } else if (view == QLatin1String("TableView")) {
        body = QStringLiteral("TableView { width: 80; height: 24; model: benchmarkModel; "
                              "columns: [{title: \"Row\", width: 80}]; delegate: %1 }");
    } else {
        body = QStringLiteral(
            "TreeView { width: 80; height: 24; model: benchmarkModel; delegate: %1 }");
    }
    return QStringLiteral("pragma ComponentBehavior: Bound\nimport QmlTui\n") + body.arg(delegate);
}

BenchmarkRow measureQmlTui(QQmlEngine& engine, const QString& view, const QString& mode, int rows,
                           int steps) {
    BenchmarkRow result{QStringLiteral("QmlTui"), view, mode};
    CountModel model(rows);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkModel"), &model);

    QQmlComponent component(&engine);
    component.setData(qmlTuiViewSource(view, mode).toUtf8(), QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << "QmlTui" << view << component.errorString();
        return result;
    }

    qmltui::TextGridBuffer buffer;
    buffer.resize(kTerminalCols, kTerminalRows);

    QElapsedTimer timer;
    timer.start();
    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<qmltui::Item*>(object.get());
    if (!root) return result;
    qmltui::ItemPaintTraversal::renderSubtree(root, buffer, 0, 0);
    result.createUs = timer.nsecsElapsed() / 1000.0;
    result.delegatesCreated = int(root->childItems().size());
    result.perDelegateUs =
        result.delegatesCreated > 0 ? result.createUs / result.delegatesCreated : 0.0;

    const auto scrollStep = [&](int i) {
        if (auto* list = qobject_cast<qmltui::ListView*>(root))
            list->setContentY(i);
        else if (auto* grid = qobject_cast<qmltui::GridView*>(root))
            grid->setContentY(i);
        else if (auto* table = qobject_cast<qmltui::TableView*>(root))
            table->setSelectedRow(kTerminalRows + i);
        else if (auto* tree = qobject_cast<qmltui::TreeView*>(root))
            tree->setCurrentIndex(kTerminalRows + i);
    };

    const int scrollSteps = (std::min)(steps, (std::max)(1, rows - (kTerminalRows + 1)));
    model.reads = 0;
    qint64 scrollSyncNs = 0;
    qint64 scrollTotalNs = 0;
    timer.restart();
    for (int i = 1; i <= scrollSteps; ++i) {
        const qint64 t0 = timer.nsecsElapsed();
        scrollStep(i);
        qmltui::ItemPaintTraversal::layoutSubtree(root);
        const qint64 t1 = timer.nsecsElapsed();
        qmltui::ItemPaintTraversal::paintSubtree(root, buffer, 0, 0);
        const qint64 t2 = timer.nsecsElapsed();
        scrollSyncNs += (t1 - t0);
        scrollTotalNs += (t2 - t0);
    }
    result.scrollSyncUs = scrollSyncNs / 1000.0 / scrollSteps;
    result.scrollUs = scrollTotalNs / 1000.0 / scrollSteps;
    result.readsPerScrollStep = double(model.reads) / scrollSteps;

    qint64 changeSyncNs = 0;
    qint64 changeTotalNs = 0;
    timer.restart();
    for (int i = 0; i < steps; ++i) {
        const qint64 t0 = timer.nsecsElapsed();
        model.touch(scrollSteps + 5 + (i % 10));
        qmltui::ItemPaintTraversal::layoutSubtree(root);
        const qint64 t1 = timer.nsecsElapsed();
        qmltui::ItemPaintTraversal::paintSubtree(root, buffer, 0, 0);
        const qint64 t2 = timer.nsecsElapsed();
        changeSyncNs += (t1 - t0);
        changeTotalNs += (t2 - t0);
    }
    result.changeSyncUs = changeSyncNs / 1000.0 / steps;
    result.changeUs = changeTotalNs / 1000.0 / steps;

    timer.restart();
    for (int i = 0; i < steps; ++i)
        qmltui::ItemPaintTraversal::layoutSubtree(root);
    result.idleLayoutUs = timer.nsecsElapsed() / 1000.0 / steps;

    timer.restart();
    for (int i = 0; i < steps; ++i)
        qmltui::ItemPaintTraversal::paintSubtree(root, buffer, 0, 0);
    result.idlePaintUs = timer.nsecsElapsed() / 1000.0 / steps;

    object.reset();
    engine.collectGarbage();
    return result;
}

double qmlTuiBytesPerDelegate(int count) {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../qml"));
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../QmlTui"));
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
    QQmlComponent component(&engine);
    component.setData("pragma ComponentBehavior: Bound\n"
                      "import QmlTui\n"
                      "Column { width: 80; Repeater { model: benchmarkModel; "
                      "delegate: Text { required property string display; text: display } } }",
                      QUrl());
    if (!component.isReady()) return -1.0;
    {
        CountModel warmModel(1);
        engine.rootContext()->setContextProperty(QStringLiteral("benchmarkModel"), &warmModel);
        std::unique_ptr<QObject> warmObj(component.create());
    }
    engine.collectGarbage();

    CountModel model(count);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkModel"), &model);
    const qint64 before = allocatedHeapBytes();
    std::unique_ptr<QObject> object(component.create());
    const qint64 after = allocatedHeapBytes();
    auto* root = qobject_cast<qmltui::Item*>(object.get());
    const int created = root ? int(root->childItems().size()) : 0;
    return (created > 0 && before > 0 && after >= before) ? double(after - before) / created : 0.0;
}

#ifdef QMLTUI_BENCHMARK_HAS_QUICK

QString qtQuickViewSource(const QString& view, bool reuseItems) {
    const QString reuseProp =
        reuseItems ? QStringLiteral("reuseItems: true; ") : QStringLiteral("reuseItems: false; ");
    if (view == QLatin1String("ListView")) {
        return QStringLiteral("pragma ComponentBehavior: Bound\n"
                              "import QtQuick\n"
                              "ListView {\n"
                              "    width: 640; height: 480; clip: true; cacheBuffer: 40; %1\n"
                              "    model: benchmarkModel\n"
                              "    delegate: Text {\n"
                              "        required property string display\n"
                              "        width: 640; height: 20\n"
                              "        text: display\n"
                              "    }\n"
                              "}\n")
            .arg(reuseProp);
    }
    if (view == QLatin1String("GridView")) {
        return QStringLiteral("pragma ComponentBehavior: Bound\n"
                              "import QtQuick\n"
                              "GridView {\n"
                              "    width: 640; height: 480; cellWidth: 160; cellHeight: 20;\n"
                              "    clip: true; cacheBuffer: 40; %1\n"
                              "    model: benchmarkModel\n"
                              "    delegate: Text {\n"
                              "        required property string display\n"
                              "        width: 160; height: 20\n"
                              "        text: display\n"
                              "    }\n"
                              "}\n")
            .arg(reuseProp);
    }
    if (view == QLatin1String("TableView")) {
        return QStringLiteral("pragma ComponentBehavior: Bound\n"
                              "import QtQuick\n"
                              "TableView {\n"
                              "    width: 640; height: 480; clip: true; %1\n"
                              "    columnWidthProvider: (col) => 640\n"
                              "    rowHeightProvider: (row) => 20\n"
                              "    model: benchmarkModel\n"
                              "    delegate: Text {\n"
                              "        required property string display\n"
                              "        width: 640; height: 20\n"
                              "        text: display\n"
                              "    }\n"
                              "}\n")
            .arg(reuseProp);
    }
    return QStringLiteral("pragma ComponentBehavior: Bound\n"
                          "import QtQuick\n"
                          "TreeView {\n"
                          "    width: 640; height: 480; clip: true; %1\n"
                          "    columnWidthProvider: (col) => 640\n"
                          "    rowHeightProvider: (row) => 20\n"
                          "    model: benchmarkModel\n"
                          "    delegate: Item {\n"
                          "        id: dlg\n"
                          "        required property TreeView treeView\n"
                          "        required property bool isTreeNode\n"
                          "        required property bool expanded\n"
                          "        required property bool hasChildren\n"
                          "        required property int depth\n"
                          "        required property string display\n"
                          "        implicitWidth: 640; implicitHeight: 20\n"
                          "        Text { text: dlg.display }\n"
                          "    }\n"
                          "}\n")
        .arg(reuseProp);
}

int countQuickDelegates(QQuickItem* view) {
    auto* content = view->property("contentItem").value<QQuickItem*>();
    if (!content) return 0;
    int count = 0;
    for (QQuickItem* child : content->childItems()) {
        if (child->isVisible() && child->height() > 0) ++count;
    }
    return count;
}

BenchmarkRow measureQtQuick(QQmlEngine& engine, const QString& view, bool reuseItems, int rows,
                            int steps) {
    const QString mode = reuseItems ? QStringLiteral("reused") : QStringLiteral("default");
    BenchmarkRow result{QStringLiteral("QtQuick"), view, mode};
    CountModel model(rows);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkModel"), &model);

    QImage renderImage(kPixelWidth, kPixelHeight, QImage::Format_ARGB32_Premultiplied);
    QQuickRenderControl renderControl;
    QQuickWindow window(&renderControl);
    window.setGeometry(0, 0, kPixelWidth, kPixelHeight);
    window.setRenderTarget(QQuickRenderTarget::fromPaintDevice(&renderImage));

    const auto renderFrame = [&]() {
        renderControl.polishItems();
        renderControl.sync();
        renderControl.render();
    };

    QQmlComponent component(&engine);
    component.setData(qtQuickViewSource(view, reuseItems).toUtf8(), QUrl());
    if (!component.isReady()) {
        qCritical().noquote() << "QtQuick" << view << component.errorString();
        return result;
    }

    QElapsedTimer timer;
    timer.start();
    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<QQuickItem*>(object.get());
    if (!root) return result;
    root->setParentItem(window.contentItem());
    renderFrame();
    result.createUs = timer.nsecsElapsed() / 1000.0;
    result.delegatesCreated = countQuickDelegates(root);
    result.perDelegateUs =
        result.delegatesCreated > 0 ? result.createUs / result.delegatesCreated : 0.0;

    const int scrollSteps = (std::min)(steps, (std::max)(1, rows - (kTerminalRows + 1)));
    model.reads = 0;
    qint64 scrollSyncNs = 0;
    qint64 scrollTotalNs = 0;
    timer.restart();
    for (int i = 1; i <= scrollSteps; ++i) {
        const qint64 t0 = timer.nsecsElapsed();
        root->setProperty("contentY", double(i * kRowHeightPx));
        renderControl.polishItems();
        renderControl.sync();
        const qint64 t1 = timer.nsecsElapsed();
        renderControl.render();
        const qint64 t2 = timer.nsecsElapsed();
        scrollSyncNs += (t1 - t0);
        scrollTotalNs += (t2 - t0);
    }
    result.scrollSyncUs = scrollSyncNs / 1000.0 / scrollSteps;
    result.scrollUs = scrollTotalNs / 1000.0 / scrollSteps;
    result.readsPerScrollStep = double(model.reads) / scrollSteps;

    qint64 changeSyncNs = 0;
    qint64 changeTotalNs = 0;
    timer.restart();
    for (int i = 0; i < steps; ++i) {
        const qint64 t0 = timer.nsecsElapsed();
        model.touch(scrollSteps + 5 + (i % 10));
        renderControl.polishItems();
        renderControl.sync();
        const qint64 t1 = timer.nsecsElapsed();
        renderControl.render();
        const qint64 t2 = timer.nsecsElapsed();
        changeSyncNs += (t1 - t0);
        changeTotalNs += (t2 - t0);
    }
    result.changeSyncUs = changeSyncNs / 1000.0 / steps;
    result.changeUs = changeTotalNs / 1000.0 / steps;

    timer.restart();
    for (int i = 0; i < steps; ++i)
        renderControl.polishItems();
    result.idleLayoutUs = timer.nsecsElapsed() / 1000.0 / steps;

    auto* content = root->property("contentItem").value<QQuickItem*>();
    timer.restart();
    for (int i = 0; i < steps; ++i) {
        if (content) {
            for (QQuickItem* child : content->childItems())
                child->update();
        }
        renderControl.sync();
        renderControl.render();
    }
    result.idlePaintUs = timer.nsecsElapsed() / 1000.0 / steps;

    object.reset();
    engine.collectGarbage();
    return result;
}

double qtQuickBytesPerDelegate(int count) {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("pragma ComponentBehavior: Bound\n"
                      "import QtQuick\n"
                      "Column { width: 640; Repeater { model: benchmarkModel; "
                      "delegate: Text { required property string display; text: display } } }",
                      QUrl());
    if (!component.isReady()) return -1.0;
    {
        CountModel warmModel(1);
        engine.rootContext()->setContextProperty(QStringLiteral("benchmarkModel"), &warmModel);
        std::unique_ptr<QObject> warmObj(component.create());
    }
    engine.collectGarbage();

    CountModel model(count);
    engine.rootContext()->setContextProperty(QStringLiteral("benchmarkModel"), &model);
    const qint64 before = allocatedHeapBytes();
    std::unique_ptr<QObject> object(component.create());
    const qint64 after = allocatedHeapBytes();
    auto* root = qobject_cast<QQuickItem*>(object.get());
    const int created = root ? int(root->childItems().size()) : 0;
    return (created > 0 && before > 0 && after >= before) ? double(after - before) / created : 0.0;
}

#endif // QMLTUI_BENCHMARK_HAS_QUICK

#ifdef QMLTUI_BENCHMARK_HAS_WIDGETS

// Enforces a fixed 20px row height so Qt Widgets views show the exact same 24
// rows (or 4x24 cells in Grid mode) in a 640x480 viewport as QmlTui at 80x24.
class FixedRowDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        const QSize base = QStyledItemDelegate::sizeHint(option, index);
        return QSize(std::max(base.width(), 160), kRowHeightPx);
    }
};

BenchmarkRow measureQtWidgets(const QString& viewName, int rows, int steps) {
    BenchmarkRow result{QStringLiteral("QtWidgets"), viewName, QStringLiteral("styled")};
    CountModel model(rows);
    FixedRowDelegate delegate;
    QImage renderImage(kPixelWidth, kPixelHeight, QImage::Format_ARGB32_Premultiplied);

    QElapsedTimer timer;
    timer.start();

    std::unique_ptr<QAbstractItemView> view;
    if (viewName == QLatin1String("ListView")) {
        auto* list = new QListView;
        list->setUniformItemSizes(true);
        view.reset(list);
    } else if (viewName == QLatin1String("GridView")) {
        auto* grid = new QListView;
        grid->setViewMode(QListView::IconMode);
        grid->setUniformItemSizes(true);
        grid->setResizeMode(QListView::Adjust);
        grid->setMovement(QListView::Static);
        grid->setGridSize(QSize(160, kRowHeightPx));
        view.reset(grid);
    } else if (viewName == QLatin1String("TableView")) {
        auto* table = new QTableView;
        table->horizontalHeader()->hide();
        table->verticalHeader()->hide();
        table->verticalHeader()->setDefaultSectionSize(kRowHeightPx);
        table->horizontalHeader()->setDefaultSectionSize(kPixelWidth);
        view.reset(table);
    } else {
        auto* tree = new QTreeView;
        tree->setHeaderHidden(true);
        tree->setUniformRowHeights(true);
        tree->setIndentation(0);
        view.reset(tree);
    }

    view->setFrameShape(QFrame::NoFrame);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    view->setItemDelegate(&delegate);
    view->resize(kPixelWidth, kPixelHeight);
    view->setModel(&model);
    view->show();
    view->viewport()->render(&renderImage);

    result.createUs = timer.nsecsElapsed() / 1000.0;
    result.delegatesCreated = 0; // QStyledItemDelegate is a shared flyweight painter.
    result.perDelegateUs = 0.0;

    const int scrollSteps = (std::min)(steps, (std::max)(1, rows - (kTerminalRows + 1)));
    model.reads = 0;
    qint64 scrollSyncNs = 0;
    qint64 scrollTotalNs = 0;
    timer.restart();
    for (int i = 1; i <= scrollSteps; ++i) {
        const qint64 t0 = timer.nsecsElapsed();
        view->verticalScrollBar()->setValue(i * kRowHeightPx);
        const qint64 t1 = timer.nsecsElapsed();
        view->viewport()->render(&renderImage);
        const qint64 t2 = timer.nsecsElapsed();
        scrollSyncNs += (t1 - t0);
        scrollTotalNs += (t2 - t0);
    }
    result.scrollSyncUs = scrollSyncNs / 1000.0 / scrollSteps;
    result.scrollUs = scrollTotalNs / 1000.0 / scrollSteps;
    result.readsPerScrollStep = double(model.reads) / scrollSteps;

    qint64 changeSyncNs = 0;
    qint64 changeTotalNs = 0;
    timer.restart();
    for (int i = 0; i < steps; ++i) {
        const int touchedRow = scrollSteps + 5 + (i % 10);
        const qint64 t0 = timer.nsecsElapsed();
        model.touch(touchedRow);
        const QRect dirtyRect = view->visualRect(model.index(touchedRow, 0));
        const qint64 t1 = timer.nsecsElapsed();
        view->viewport()->render(&renderImage, dirtyRect.topLeft(), QRegion(dirtyRect));
        const qint64 t2 = timer.nsecsElapsed();
        changeSyncNs += (t1 - t0);
        changeTotalNs += (t2 - t0);
    }
    result.changeSyncUs = changeSyncNs / 1000.0 / steps;
    result.changeUs = changeTotalNs / 1000.0 / steps;

    const QModelIndex probeIdx = model.index(scrollSteps, 0);
    int dummyY = 0;
    timer.restart();
    for (int i = 0; i < steps; ++i)
        dummyY += view->visualRect(probeIdx).y();
    result.idleLayoutUs = timer.nsecsElapsed() / 1000.0 / steps;
    if (dummyY == -999999) qWarning() << dummyY;

    timer.restart();
    for (int i = 0; i < steps; ++i)
        view->viewport()->render(&renderImage);
    result.idlePaintUs = timer.nsecsElapsed() / 1000.0 / steps;

    return result;
}

#endif // QMLTUI_BENCHMARK_HAS_WIDGETS

bool writeJson(const QString& path, int rows, int steps, const std::vector<BenchmarkRow>& results,
               double qmlTuiDelegateBytes, double qtQuickDelegateBytes) {
    QJsonArray array;
    for (const BenchmarkRow& r : results) {
        array.append(QJsonObject{
            {QStringLiteral("framework"), r.framework},
            {QStringLiteral("view"), r.view},
            {QStringLiteral("mode"), r.mode},
            {QStringLiteral("delegatesCreated"), r.delegatesCreated},
            {QStringLiteral("createUs"), r.createUs},
            {QStringLiteral("perDelegateUs"), r.perDelegateUs},
            {QStringLiteral("scrollSyncUs"), r.scrollSyncUs},
            {QStringLiteral("scrollUs"), r.scrollUs},
            {QStringLiteral("changeSyncUs"), r.changeSyncUs},
            {QStringLiteral("changeUs"), r.changeUs},
            {QStringLiteral("idleLayoutUs"), r.idleLayoutUs},
            {QStringLiteral("idlePaintUs"), r.idlePaintUs},
            {QStringLiteral("readsPerScrollStep"), r.readsPerScrollStep},
        });
    }
    const QJsonObject root{
        {QStringLiteral("rows"), rows},
        {QStringLiteral("steps"), steps},
        {QStringLiteral("qmlTuiBytesPerDelegate"), qmlTuiDelegateBytes},
        {QStringLiteral("qtQuickBytesPerDelegate"), qtQuickDelegateBytes},
        {QStringLiteral("results"), array},
    };
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCritical().noquote() << "Cannot write" << path;
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));

#ifdef QMLTUI_BENCHMARK_HAS_QUICK
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
#endif

#if defined(QMLTUI_BENCHMARK_HAS_WIDGETS)
    QApplication application(argc, argv);
#elif defined(QMLTUI_BENCHMARK_HAS_QUICK)
    QGuiApplication application(argc, argv);
#else
    QCoreApplication application(argc, argv);
#endif

    qmltui::ListView linkList;
    qmltui::GridView linkGrid;
    qmltui::TableView linkTable;
    qmltui::TreeView linkTree;
    qmltui::Repeater linkRepeater;

    int rows = 100000;
    int steps = 1000;
    QString jsonPath;

    const QStringList args = application.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QLatin1String("--rows") && i + 1 < args.size()) {
            rows = std::max(100, args[++i].toInt());
        } else if (args[i] == QLatin1String("--steps") && i + 1 < args.size()) {
            steps = std::max(1, args[++i].toInt());
        } else if (args[i] == QLatin1String("--json") && i + 1 < args.size()) {
            jsonPath = args[++i];
        } else {
            qCritical().noquote() << "Unknown or incomplete argument:" << args[i];
            return 1;
        }
    }

    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../qml"));
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../QmlTui"));
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

    // Warm up QML type compilation before recording timings.
    (void)measureQmlTui(engine, QStringLiteral("ListView"), QStringLiteral("bound"), 100, 2);
#ifdef QMLTUI_BENCHMARK_HAS_QUICK
    (void)measureQtQuick(engine, QStringLiteral("ListView"), true, 100, 2);
#endif

    std::vector<BenchmarkRow> results;
    for (const QString& view : {QStringLiteral("ListView"), QStringLiteral("GridView"),
                                QStringLiteral("TableView"), QStringLiteral("TreeView")}) {
        results.push_back(measureQmlTui(engine, view, QStringLiteral("bound"), rows, steps));
        if (view == QLatin1String("TableView") || view == QLatin1String("TreeView"))
            results.push_back(measureQmlTui(engine, view, QStringLiteral("builtin"), rows, steps));

#ifdef QMLTUI_BENCHMARK_HAS_QUICK
        if (view == QLatin1String("ListView")) {
            results.push_back(measureQtQuick(engine, view, false, rows, steps));
            results.push_back(measureQtQuick(engine, view, true, rows, steps));
        } else if (view == QLatin1String("GridView")) {
            results.push_back(measureQtQuick(engine, view, false, rows, steps));
        } else {
            results.push_back(measureQtQuick(engine, view, true, rows, steps));
        }
#endif

#ifdef QMLTUI_BENCHMARK_HAS_WIDGETS
        results.push_back(measureQtWidgets(view, rows, steps));
#endif
    }

    const double qmlTuiDelegateBytes = qmlTuiBytesPerDelegate(2000);
#ifdef QMLTUI_BENCHMARK_HAS_QUICK
    const double qtQuickDelegateBytes = qtQuickBytesPerDelegate(2000);
#else
    const double qtQuickDelegateBytes = -1.0;
#endif

    QTextStream out(stdout);
    out << "model rows: " << rows << ", steps: " << steps << ", viewport: 24 rows (4x24 grid)\n";
    out << QStringLiteral("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12\n")
               .arg("framework", -10)
               .arg("view", -10)
               .arg("mode", -8)
               .arg("created", 7)
               .arg("create us", 10)
               .arg("scr sync", 9)
               .arg("scroll us", 10)
               .arg("chg sync", 9)
               .arg("change us", 10)
               .arg("idle lay", 9)
               .arg("idle paint", 10)
               .arg("reads/step", 10);
    bool allValid = true;
    for (const BenchmarkRow& r : results) {
        if (r.createUs <= 0.0 || r.scrollUs <= 0.0) allValid = false;
        out << QStringLiteral("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12\n")
                   .arg(r.framework, -10)
                   .arg(r.view, -10)
                   .arg(r.mode, -8)
                   .arg(r.delegatesCreated, 7)
                   .arg(r.createUs, 10, 'f', 1)
                   .arg(r.scrollSyncUs, 9, 'f', 1)
                   .arg(r.scrollUs, 10, 'f', 1)
                   .arg(r.changeSyncUs, 9, 'f', 1)
                   .arg(r.changeUs, 10, 'f', 1)
                   .arg(r.idleLayoutUs, 9, 'f', 1)
                   .arg(r.idlePaintUs, 10, 'f', 1)
                   .arg(r.readsPerScrollStep, 10, 'f', 1);
    }

    out << "\nAllocated heap bytes per Text delegate (Repeater, 2000 rows):\n";
    out << QStringLiteral("  QmlTui    %1\n").arg(qmlTuiDelegateBytes, 8, 'f', 0);
#ifdef QMLTUI_BENCHMARK_HAS_QUICK
    out << QStringLiteral("  QtQuick   %1\n").arg(qtQuickDelegateBytes, 8, 'f', 0);
#endif
#ifdef QMLTUI_BENCHMARK_HAS_WIDGETS
    out << "  QtWidgets        0 (QStyledItemDelegate shared flyweight)\n";
#endif
    out.flush();

    if (!jsonPath.isEmpty() &&
        !writeJson(jsonPath, rows, steps, results, qmlTuiDelegateBytes, qtQuickDelegateBytes)) {
        return 1;
    }
    return allValid ? 0 : 1;
}
