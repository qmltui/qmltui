// Views must not schedule frames while nothing changes, and a single state
// change must cost exactly one frame. These tests drive a real Terminal engine
// with a test viewport so the render request path is exercised end to end.
#include <QAbstractListModel>
#include <QCoreApplication>
#include <QObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QElapsedTimer>
#include <algorithm>
#include <QTest>
#include <memory>

#include "Color.h"
#include "GridView.h"
#include "ListView.h"
#include "TableView.h"
#include "Terminal.h"
#include "TerminalEngine.h"
#include "TextGridBuffer.h"
#include "TreeView.h"

using namespace qtui;

namespace {

class LayoutPrompt final : public Item {
public:
    explicit LayoutPrompt(Item *parent) : Item(parent) { setImplicitHeight(1); }
    void setRows(int rows) {
        m_rows = rows;
        markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    }
protected:
    void updateLayout() override { setImplicitHeight(m_rows); }
    void paint(TextGridBuffer &buffer, int x, int y, const ClipRect &) const override {
        for (int row = 0; row < m_rows; ++row)
            buffer.writeText(x, y + row, QStringLiteral("PROMPT %1").arg(row + 1));
    }
private:
    int m_rows = 3;
};

class MutableModel final : public QAbstractListModel {
public:
    explicit MutableModel(const QStringList &rows, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_rows(rows) {}

    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : m_rows.size();
    }

    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || role != Qt::DisplayRole)
            return {};
        return m_rows.value(index.row());
    }

    QHash<int, QByteArray> roleNames() const override {
        return {{Qt::DisplayRole, "display"}};
    }

    void setText(int row, const QString &text) {
        m_rows[row] = text;
        emit dataChanged(index(row), index(row));
    }

private:
    QStringList m_rows;
};

struct Session {
    std::unique_ptr<QObject> root;
    Terminal *terminal = nullptr;
    TerminalEngine *engine = nullptr;
};

Session createSession(QQmlEngine &qmlEngine, const char *qml)
{
    QQmlComponent component(&qmlEngine);
    component.setData(qml, QUrl());
    Session session;
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
        return session;
    }
    session.root.reset(component.create());
    session.terminal = qobject_cast<Terminal *>(session.root.get());
    if (session.terminal) {
        session.engine = session.terminal->engine();
        session.engine->setTestViewportSize(QSize(80, 24));
    }
    return session;
}

QString frameText(TerminalEngine *engine)
{
    const std::optional<TextGridBuffer> frame = engine->currentFrame();
    if (!frame)
        return {};
    QString text;
    for (int y = 0; y < frame->height(); ++y) {
        for (int x = 0; x < frame->width(); ++x) {
            const Cell &cell = frame->cellAt(x, y);
            if (!cell.continuation)
                text += QString::fromUtf8(cell.character().toByteArray());
        }
        text += QLatin1Char('\n');
    }
    return text;
}

} // namespace

// Waits for startup frames, then verifies no frame is committed while idle and
// that the last frame raised no invalidation from paint().
#define QTUI_VERIFY_IDLE(engine) \
    do { \
        QTest::qWait(120); \
        const quint64 settled = (engine)->frameSequence(); \
        QVERIFY(settled > 0); \
        QTest::qWait(150); \
        QCOMPARE((engine)->frameSequence(), settled); \
        QCOMPARE((engine)->lastFrameStats().dirtyDuringPaint, 0); \
    } while (false)

// Runs `mutation` and verifies it costs exactly one frame.
#define QTUI_VERIFY_ONE_FRAME(engine, mutation) \
    do { \
        const quint64 beforeMutation = (engine)->frameSequence(); \
        mutation; \
        QTest::qWait(80); \
        QCOMPARE((engine)->frameSequence(), beforeMutation + 1); \
        QCOMPARE((engine)->lastFrameStats().dirtyDuringPaint, 0); \
    } while (false)

class TestRenderLoop : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void tableViewWithDelegateStaysIdle();
    void tableViewDelegateRebindsOnDataChanged();
    void tableColumnWidthProviderRelayoutsDelegates();
    void streamPromptUsesCompletedLayout();
    void tableViewWithCellFormatStaysIdle();
    void listViewStaysIdle();
    void gridViewStaysIdle();
    void treeViewStaysIdle();
    void canvasWithShapesStaysIdle();
    void framePacingCoalescesBursts();
    void terminalModesAndVisibleChildren();

private:
    QQmlEngine m_qmlEngine;
};

void TestRenderLoop::initTestCase()
{
    // A test session keeps the engine from driving the real terminal; frames
    // still commit and the sequence still advances.
    qputenv("QTUI_INSPECT", "1");
    m_qmlEngine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
}

void TestRenderLoop::terminalModesAndVisibleChildren()
{
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import Qtui
        Terminal {
            mode: Terminal.Inline
            Text { text: "FIRST" }
            Text { objectName: "second"; y: 2; text: "SECOND" }
            Text { objectName: "overlay"; visible: false; text: "OVERLAY"; z: 1 }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    QCOMPARE(session.engine->currentFrame()->height(), 3);
    QVERIFY(frameText(session.engine).contains("FIRST"));
    QVERIFY(frameText(session.engine).contains("SECOND"));
    auto* second = session.terminal->findChild<Item*>("second");
    auto* overlay = session.terminal->findChild<Item*>("overlay");
    QVERIFY(second && overlay);
    QTUI_VERIFY_ONE_FRAME(session.engine, second->setY(4));
    QCOMPARE(session.engine->currentFrame()->height(), 5);
    QTUI_VERIFY_ONE_FRAME(session.engine, second->setVisible(false));
    QCOMPARE(session.engine->currentFrame()->height(), 1);
    QTUI_VERIFY_ONE_FRAME(session.engine, overlay->setVisible(true));
    QVERIFY(frameText(session.engine).contains("OVERLAY"));
    QVERIFY(!frameText(session.engine).contains("FIRST"));
    QTUI_VERIFY_ONE_FRAME(session.engine, session.terminal->setMode(Terminal::Mode::FullScreen));
    QCOMPARE(session.engine->currentFrame()->height(), session.engine->terminalHeight());
    QVERIFY(frameText(session.engine).contains("OVERLAY"));
    QVERIFY(!second->visible());
    QTUI_VERIFY_ONE_FRAME(session.engine, session.terminal->setMode(Terminal::Mode::Inline));
    QCOMPARE(session.engine->currentFrame()->height(), 1);
    QTUI_VERIFY_ONE_FRAME(session.engine, delete overlay);
    QVERIFY(frameText(session.engine).contains("FIRST"));
    QTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::tableViewWithDelegateStaysIdle()
{
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import Qtui
        Terminal {
            TableView {
                objectName: "table"
                anchors.fill: parent
                columns: [ { title: "A", width: 12 }, { title: "B", width: 12 } ]
                model: [ ["a1", "b1"], ["a2", "b2"], ["a3", "b3"], ["a4", "b4"] ]
                delegate: Text {
                    required property string display
                    required property bool current
                    text: display
                    bold: current
                }
            }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("a1")));

    auto *table = session.terminal->findChild<TableView *>(QStringLiteral("table"));
    QVERIFY(table);
    const int delegates = table->findChildren<Item *>().size();
    QTUI_VERIFY_ONE_FRAME(session.engine, table->setSelectedRow(2));
    QCOMPARE(table->findChildren<Item *>().size(), delegates);   // no teardown on selection
    QTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::tableViewDelegateRebindsOnDataChanged()
{
    MutableModel model({QStringLiteral("alpha"), QStringLiteral("beta"), QStringLiteral("gamma")});
    m_qmlEngine.rootContext()->setContextProperty(QStringLiteral("loopModel"), &model);
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import Qtui
        Terminal {
            TableView {
                objectName: "table"
                anchors.fill: parent
                columns: [ { title: "NAME", width: 20 } ]
                model: loopModel
                delegate: Text {
                    required property string display
                    text: display
                }
            }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("beta")));

    auto *table = session.terminal->findChild<TableView *>(QStringLiteral("table"));
    QVERIFY(table);
    const QList<Item *> before = table->findChildren<Item *>();
    QTUI_VERIFY_ONE_FRAME(session.engine, model.setText(1, QStringLiteral("delta")));
    const QString text = frameText(session.engine);
    QVERIFY2(text.contains(QStringLiteral("delta")), qPrintable(text));
    QVERIFY(!text.contains(QStringLiteral("beta")));
    QCOMPARE(table->findChildren<Item *>(), before);   // same delegate instances
    QTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::tableColumnWidthProviderRelayoutsDelegates()
{
    Session session = createSession(m_qmlEngine, R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import Qtui
        Terminal {
            TableView {
                objectName: "table"
                width: 30; height: 5
                columns: [{ title: "A", width: 8 }, { title: "B", width: 8 }]
                model: [["alpha", "beta"]]
                delegate: Text {
                    required property string display
                    text: display
                }
            }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    auto *table = session.terminal->findChild<TableView *>(QStringLiteral("table"));
    QVERIFY(table);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("alpha   beta")));
    QTUI_VERIFY_ONE_FRAME(session.engine,
        table->setColumnWidthProvider(m_qmlEngine.evaluate("(function(column) { return 12; })")));
    const auto frame = session.engine->currentFrame();
    QVERIFY(frame);
    QCOMPARE(frame->cellAt(12, 0).character(), QByteArrayView("B"));
    QCOMPARE(frame->cellAt(12, 2).character(), QByteArrayView("b"));
    QVERIFY(frameText(session.engine).contains(QStringLiteral("alpha       beta")));
}

void TestRenderLoop::streamPromptUsesCompletedLayout()
{
    Terminal terminal;
    terminal.setMode(Terminal::Mode::Inline);
    auto *prompt = new LayoutPrompt(&terminal);
    auto *engine = terminal.engine();
    engine->setTestViewportSize(QSize(30, 8));
    QTRY_COMPARE(engine->frameSequence(), quint64(1));
    QCOMPARE(engine->currentFrame()->height(), 3);
    QVERIFY(frameText(engine).contains(QStringLiteral("PROMPT 3")));
    QTUI_VERIFY_ONE_FRAME(engine, prompt->setRows(5));
    QCOMPARE(engine->currentFrame()->height(), 5);
    QVERIFY(frameText(engine).contains(QStringLiteral("PROMPT 5")));
    QTUI_VERIFY_ONE_FRAME(engine, prompt->setRows(2));
    QCOMPARE(engine->currentFrame()->height(), 2);
    QVERIFY(!frameText(engine).contains(QStringLiteral("PROMPT 3")));
    QTUI_VERIFY_IDLE(engine);

}

void TestRenderLoop::tableViewWithCellFormatStaysIdle()
{
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import Qtui
        Terminal {
            TableView {
                objectName: "table"
                anchors.fill: parent
                alternatingRows: true
                columns: [ { title: "A", width: 12 }, { title: "B", width: 12 } ]
                model: [ ["a1", "b1"], ["a2", "b2"], ["a3", "b3"] ]
                cellFormat: (row, column, value) => ({ text: value, color: column === 1 ? "green" : "white" })
            }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    auto *table = session.terminal->findChild<TableView *>(QStringLiteral("table"));
    QVERIFY(table);
    QTUI_VERIFY_ONE_FRAME(session.engine, table->setSelectedRow(1));
    QTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::listViewStaysIdle()
{
    MutableModel model([] { QStringList rows; for (int i = 0; i < 200; ++i) rows << QStringLiteral("Row %1").arg(i); return rows; }());
    m_qmlEngine.rootContext()->setContextProperty(QStringLiteral("loopModel"), &model);
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import Qtui
        Terminal {
            ListView {
                objectName: "list"
                anchors.fill: parent
                model: loopModel
                delegate: Rectangle {
                    required property int index
                    required property string display
                    height: 1
                    color: ListView.isCurrentItem ? "blue" : "black"
                    Text { text: display }
                }
            }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    auto *list = session.terminal->findChild<ListView *>(QStringLiteral("list"));
    QVERIFY(list);
    QTUI_VERIFY_ONE_FRAME(session.engine, list->setContentY(40));
    QVERIFY(frameText(session.engine).contains(QStringLiteral("Row 45")));
    QTUI_VERIFY_IDLE(session.engine);
    QTUI_VERIFY_ONE_FRAME(session.engine, list->setCurrentIndex(42));
    QTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::gridViewStaysIdle()
{
    MutableModel model([] { QStringList rows; for (int i = 0; i < 30; ++i) rows << QStringLiteral("G%1").arg(i); return rows; }());
    m_qmlEngine.rootContext()->setContextProperty(QStringLiteral("loopModel"), &model);
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import Qtui
        Terminal {
            GridView {
                objectName: "grid"
                anchors.fill: parent
                model: loopModel
                cellWidth: 10
                cellHeight: 2
                delegate: Text {
                    required property string display
                    text: display
                }
            }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("G7")));
    auto *grid = session.terminal->findChild<GridView *>(QStringLiteral("grid"));
    QVERIFY(grid);
    QTUI_VERIFY_ONE_FRAME(session.engine, grid->setCellWidth(16));
    QTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::treeViewStaysIdle()
{
    MutableModel model([] { QStringList rows; for (int i = 0; i < 40; ++i) rows << QStringLiteral("Node %1").arg(i); return rows; }());
    m_qmlEngine.rootContext()->setContextProperty(QStringLiteral("loopModel"), &model);
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import Qtui
        Terminal {
            TreeView {
                objectName: "tree"
                anchors.fill: parent
                model: loopModel
                delegate: Text {
                    required property int depth
                    required property bool hasChildren
                    required property bool expanded
                    required property bool current
                    required property string display
                    required property var modelData
                    text: (current ? "> " : "  ") + display
                }
            }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("Node 3")));
    auto *tree = session.terminal->findChild<TreeView *>(QStringLiteral("tree"));
    QVERIFY(tree);
    QTUI_VERIFY_ONE_FRAME(session.engine, tree->setProperty("currentIndex", 5));
    QTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::canvasWithShapesStaysIdle()
{
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import Qtui
        Terminal {
            Canvas {
                objectName: "canvas"
                anchors.fill: parent
                renderMode: Canvas.Braille
                Line { x1: 0; y1: 0; x2: 100; y2: 60 }
                Circle { cx: 40; cy: 30; radius: 12 }
            }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    auto *canvas = session.terminal->findChild<Item *>(QStringLiteral("canvas"));
    QVERIFY(canvas);
    QTUI_VERIFY_ONE_FRAME(session.engine,
                          canvas->setProperty("color", QVariant::fromValue(Color("red"))));
    QTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::framePacingCoalescesBursts()
{
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import Qtui
        Terminal {
            Text { objectName: "label"; text: "0" }
        }
    )");
    QVERIFY(session.engine);
    QTUI_VERIFY_IDLE(session.engine);
    QCOMPARE(session.engine->maxFrameRate(), 0);   // test sessions are uncapped

    auto *label = session.terminal->findChild<Item *>(QStringLiteral("label"));
    QVERIFY(label);

    // Uncapped: every change in a burst may become its own frame.
    quint64 before = session.engine->frameSequence();
    for (int i = 1; i <= 20; ++i) {
        label->setProperty("text", QString::number(i));
        QTest::qWait(1);
    }
    QTest::qWait(60);
    const quint64 uncapped = session.engine->frameSequence() - before;
    QVERIFY(uncapped >= 10);

    // Capped at 50 fps a ~20 ms burst collapses to a handful of frames while
    // the final state is still rendered.
    session.engine->setMaxFrameRate(50);
    QCOMPARE(session.engine->maxFrameRate(), 50);
    before = session.engine->frameSequence();
    QElapsedTimer burstTimer;
    burstTimer.start();
    for (int i = 21; i <= 40; ++i) {
        label->setProperty("text", QString::number(i));
        QTest::qWait(1);
    }
    // qWait(1) can take a full timer tick (about 16 ms on Windows), so bound
    // the frame count by the time the burst actually took at 50 fps.
    const qint64 burstMs = burstTimer.elapsed();
    QTest::qWait(120);
    const quint64 capped = session.engine->frameSequence() - before;
    const quint64 maxCapped = static_cast<quint64>(burstMs / 20) + 2;
    QVERIFY2(capped >= 1 && capped <= std::max<quint64>(6, maxCapped),
             qPrintable(QStringLiteral("%1 frames in %2 ms").arg(capped).arg(burstMs)));
    QVERIFY(frameText(session.engine).contains(QStringLiteral("40")));
    QTUI_VERIFY_IDLE(session.engine);
    session.engine->setMaxFrameRate(0);
}

QTEST_MAIN(TestRenderLoop)
#include "test_render_loop.moc"
