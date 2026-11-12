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

#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Items/GridView.h>
#include <QmlTui/Core/Items/ListView.h>
#include <QmlTui/Core/Items/Popup.h>
#include <QmlTui/Core/Items/Rectangle.h>
#include <QmlTui/Core/Items/ScrollView.h>
#include <QmlTui/Core/Items/TableView.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Items/TextInput.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QmlTui/Core/Items/TreeView.h>
#include "../TestPaint.h"

using namespace qmltui;

namespace {

class LayoutPrompt final : public Item {
public:
    explicit LayoutPrompt(Item* parent) : Item(parent) { setImplicitHeight(1); }
    void setRows(int rows) {
        m_rows = rows;
        markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    }

protected:
    void updateLayout() override { setImplicitHeight(m_rows); }
    void paint(TextGridBuffer& buffer, int x, int y, const ClipRect&) const override {
        for (int row = 0; row < m_rows; ++row)
            buffer.writeText(x, y + row, QStringLiteral("PROMPT %1").arg(row + 1));
    }

private:
    int m_rows = 3;
};

class MutableModel final : public QAbstractListModel {
public:
    explicit MutableModel(const QStringList& rows, QObject* parent = nullptr)
        : QAbstractListModel(parent), m_rows(rows) {}

    int rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : m_rows.size();
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid() || role != Qt::DisplayRole) return {};
        return m_rows.value(index.row());
    }

    QHash<int, QByteArray> roleNames() const override { return {{Qt::DisplayRole, "display"}}; }

    void setText(int row, const QString& text) {
        m_rows[row] = text;
        emit dataChanged(index(row), index(row));
    }

private:
    QStringList m_rows;
};

struct Session {
    std::unique_ptr<QObject> root;
    Terminal* terminal = nullptr;
    TerminalEngine* engine = nullptr;
};

Session createSession(QQmlEngine& qmlEngine, const char* qml) {
    QQmlComponent component(&qmlEngine);
    component.setData(qml, QUrl());
    Session session;
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
        return session;
    }
    session.root.reset(component.create());
    session.terminal = qobject_cast<Terminal*>(session.root.get());
    if (session.terminal) {
        session.engine = session.terminal->engine();
        session.engine->setTestViewportSize(QSize(80, 24));
    }
    return session;
}

QString frameText(TerminalEngine* engine) {
    const std::optional<TextGridBuffer> frame = engine->currentFrame();
    if (!frame) return {};
    QString text;
    for (int y = 0; y < frame->height(); ++y) {
        for (int x = 0; x < frame->width(); ++x) {
            const Cell& cell = frame->cellAt(x, y);
            if (!cell.continuation) text += QString::fromUtf8(cell.character().toByteArray());
        }
        text += QLatin1Char('\n');
    }
    return text;
}

class CountingProbe final : public Item {
public:
    explicit CountingProbe(QObject* parent = nullptr) : Item(parent) {
        setWidth(8);
        setHeight(1);
    }

    mutable int paintCalls = 0;

protected:
    void paint(TextGridBuffer& buffer, int x, int y, const ClipRect& clipRect) const override {
        ++paintCalls;
        if (clipRect.contains(x, y)) buffer.writeText(x, y, QStringLiteral("PROBE"));
    }
};

bool buffersMatchCellForCell(const TextGridBuffer& actual, const TextGridBuffer& expected,
                             QString* mismatchDetail = nullptr) {
    if (actual.width() != expected.width() || actual.height() != expected.height()) {
        if (mismatchDetail) {
            *mismatchDetail = QStringLiteral("size mismatch: %1x%2 vs %3x%4")
                                  .arg(actual.width())
                                  .arg(actual.height())
                                  .arg(expected.width())
                                  .arg(expected.height());
        }
        return false;
    }
    for (int y = 0; y < actual.height(); ++y) {
        for (int x = 0; x < actual.width(); ++x) {
            const Cell& a = actual.cellAt(x, y);
            const Cell& b = expected.cellAt(x, y);
            if (a != b || actual.linkUrl(a.linkId) != expected.linkUrl(b.linkId)) {
                if (mismatchDetail) {
                    *mismatchDetail =
                        QStringLiteral("cell (%1,%2) mismatch: actual='%3' fg=%4 bg=%5 w=%6 "
                                       "cont=%7 vs expected='%8' fg=%9 bg=%10 w=%11 cont=%12")
                            .arg(x)
                            .arg(y)
                            .arg(QString::fromUtf8(a.character().toByteArray()))
                            .arg(a.fgColor, 8, 16, QLatin1Char('0'))
                            .arg(a.bgColor, 8, 16, QLatin1Char('0'))
                            .arg(a.displayWidth)
                            .arg(a.continuation)
                            .arg(QString::fromUtf8(b.character().toByteArray()))
                            .arg(b.fgColor, 8, 16, QLatin1Char('0'))
                            .arg(b.bgColor, 8, 16, QLatin1Char('0'))
                            .arg(b.displayWidth)
                            .arg(b.continuation);
                }
                return false;
            }
        }
    }
    return true;
}

} // namespace

// Waits for startup frames, then verifies no frame is committed while idle and
// that the last frame raised no invalidation from paint().
#define QMLTUI_VERIFY_IDLE(engine)                                                                 \
    do {                                                                                           \
        QTRY_VERIFY_WITH_TIMEOUT((engine)->frameSequence() > 0, 500);                              \
        const quint64 settled = (engine)->frameSequence();                                         \
        QTest::qWait(25);                                                                          \
        QCOMPARE((engine)->frameSequence(), settled);                                              \
        QCOMPARE((engine)->lastFrameStats().dirtyDuringPaint, 0);                                  \
    } while (false)

// Runs `mutation` and verifies it costs exactly one frame.
#define QMLTUI_VERIFY_ONE_FRAME(engine, mutation)                                                  \
    do {                                                                                           \
        const quint64 beforeMutation = (engine)->frameSequence();                                  \
        mutation;                                                                                  \
        QTRY_COMPARE_WITH_TIMEOUT((engine)->frameSequence(), beforeMutation + 1, 500);             \
        QTest::qWait(25);                                                                          \
        QCOMPARE((engine)->frameSequence(), beforeMutation + 1);                                   \
        QCOMPARE((engine)->lastFrameStats().dirtyDuringPaint, 0);                                  \
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
    void partialDamageMatchesFullRender();
    void offscreenCompoundSubtreeCulledInO1();

private:
    QQmlEngine m_qmlEngine;
};

void TestRenderLoop::initTestCase() {
    // A test session keeps the engine from driving the real terminal; frames
    // still commit and the sequence still advances.
    qputenv("QMLTUI_RUNNER_CAPTURE_AFTER_MS", "0");
    m_qmlEngine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
}

void TestRenderLoop::terminalModesAndVisibleChildren() {
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
        Terminal {
            mode: Terminal.Inline
            Text { text: "FIRST" }
            Text { objectName: "second"; y: 2; text: "SECOND" }
            Text { objectName: "overlay"; visible: false; text: "OVERLAY"; z: 1 }
        }
    )");
    QVERIFY(session.engine);
    QMLTUI_VERIFY_IDLE(session.engine);
    QCOMPARE(session.engine->currentFrame()->height(), 3);
    QVERIFY(frameText(session.engine).contains("FIRST"));
    QVERIFY(frameText(session.engine).contains("SECOND"));
    auto* second = session.terminal->findChild<Item*>("second");
    auto* overlay = session.terminal->findChild<Item*>("overlay");
    QVERIFY(second && overlay);
    QMLTUI_VERIFY_ONE_FRAME(session.engine, second->setY(4));
    QCOMPARE(session.engine->currentFrame()->height(), 5);
    QMLTUI_VERIFY_ONE_FRAME(session.engine, second->setVisible(false));
    QCOMPARE(session.engine->currentFrame()->height(), 1);
    QMLTUI_VERIFY_ONE_FRAME(session.engine, overlay->setVisible(true));
    QVERIFY(frameText(session.engine).contains("OVERLAY"));
    QVERIFY(!frameText(session.engine).contains("FIRST"));
    QMLTUI_VERIFY_ONE_FRAME(session.engine, session.terminal->setMode(Terminal::Mode::AltScreen));
    QCOMPARE(session.engine->currentFrame()->height(), session.engine->terminalHeight());
    QVERIFY(frameText(session.engine).contains("OVERLAY"));
    QVERIFY(!second->visible());
    QMLTUI_VERIFY_ONE_FRAME(session.engine, session.terminal->setMode(Terminal::Mode::Inline));
    QCOMPARE(session.engine->currentFrame()->height(), 1);
    QMLTUI_VERIFY_ONE_FRAME(session.engine, delete overlay);
    QVERIFY(frameText(session.engine).contains("FIRST"));
    QMLTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::tableViewWithDelegateStaysIdle() {
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
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
    QMLTUI_VERIFY_IDLE(session.engine);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("a1")));

    auto* table = session.terminal->findChild<TableView*>(QStringLiteral("table"));
    QVERIFY(table);
    const int delegates = table->findChildren<Item*>().size();
    QMLTUI_VERIFY_ONE_FRAME(session.engine, table->setSelectedRow(2));
    QCOMPARE(table->findChildren<Item*>().size(), delegates); // no teardown on selection
    QMLTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::tableViewDelegateRebindsOnDataChanged() {
    MutableModel model({QStringLiteral("alpha"), QStringLiteral("beta"), QStringLiteral("gamma")});
    m_qmlEngine.rootContext()->setContextProperty(QStringLiteral("loopModel"), &model);
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
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
    QMLTUI_VERIFY_IDLE(session.engine);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("beta")));

    auto* table = session.terminal->findChild<TableView*>(QStringLiteral("table"));
    QVERIFY(table);
    const QList<Item*> before = table->findChildren<Item*>();
    QMLTUI_VERIFY_ONE_FRAME(session.engine, model.setText(1, QStringLiteral("delta")));
    const QString text = frameText(session.engine);
    QVERIFY2(text.contains(QStringLiteral("delta")), qPrintable(text));
    QVERIFY(!text.contains(QStringLiteral("beta")));
    QCOMPARE(table->findChildren<Item*>(), before); // same delegate instances
    QMLTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::tableColumnWidthProviderRelayoutsDelegates() {
    Session session = createSession(m_qmlEngine, R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
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
    QMLTUI_VERIFY_IDLE(session.engine);
    auto* table = session.terminal->findChild<TableView*>(QStringLiteral("table"));
    QVERIFY(table);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("alpha   beta")));
    QMLTUI_VERIFY_ONE_FRAME(session.engine, table->setColumnWidthProvider(m_qmlEngine.evaluate(
                                                "(function(column) { return 12; })")));
    const auto frame = session.engine->currentFrame();
    QVERIFY(frame);
    QCOMPARE(frame->cellAt(12, 0).character(), QByteArrayView("B"));
    QCOMPARE(frame->cellAt(12, 2).character(), QByteArrayView("b"));
    QVERIFY(frameText(session.engine).contains(QStringLiteral("alpha       beta")));
}

void TestRenderLoop::streamPromptUsesCompletedLayout() {
    Terminal terminal;
    terminal.setMode(Terminal::Mode::Inline);
    auto* prompt = new LayoutPrompt(&terminal);
    auto* engine = terminal.engine();
    engine->setTestViewportSize(QSize(30, 8));
    QTRY_COMPARE(engine->frameSequence(), quint64(1));
    QCOMPARE(engine->currentFrame()->height(), 3);
    QVERIFY(frameText(engine).contains(QStringLiteral("PROMPT 3")));
    QMLTUI_VERIFY_ONE_FRAME(engine, prompt->setRows(5));
    QCOMPARE(engine->currentFrame()->height(), 5);
    QVERIFY(frameText(engine).contains(QStringLiteral("PROMPT 5")));
    QMLTUI_VERIFY_ONE_FRAME(engine, prompt->setRows(2));
    QCOMPARE(engine->currentFrame()->height(), 2);
    QVERIFY(!frameText(engine).contains(QStringLiteral("PROMPT 3")));
    QMLTUI_VERIFY_IDLE(engine);
}

void TestRenderLoop::tableViewWithCellFormatStaysIdle() {
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
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
    QMLTUI_VERIFY_IDLE(session.engine);
    auto* table = session.terminal->findChild<TableView*>(QStringLiteral("table"));
    QVERIFY(table);
    QMLTUI_VERIFY_ONE_FRAME(session.engine, table->setSelectedRow(1));
    QMLTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::listViewStaysIdle() {
    MutableModel model([] {
        QStringList rows;
        for (int i = 0; i < 200; ++i)
            rows << QStringLiteral("Row %1").arg(i);
        return rows;
    }());
    m_qmlEngine.rootContext()->setContextProperty(QStringLiteral("loopModel"), &model);
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
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
    QMLTUI_VERIFY_IDLE(session.engine);
    auto* list = session.terminal->findChild<ListView*>(QStringLiteral("list"));
    QVERIFY(list);
    QMLTUI_VERIFY_ONE_FRAME(session.engine, list->setContentY(40));
    QVERIFY(frameText(session.engine).contains(QStringLiteral("Row 45")));
    QMLTUI_VERIFY_IDLE(session.engine);
    QMLTUI_VERIFY_ONE_FRAME(session.engine, list->setCurrentIndex(42));
    QMLTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::gridViewStaysIdle() {
    MutableModel model([] {
        QStringList rows;
        for (int i = 0; i < 30; ++i)
            rows << QStringLiteral("G%1").arg(i);
        return rows;
    }());
    m_qmlEngine.rootContext()->setContextProperty(QStringLiteral("loopModel"), &model);
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
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
    QMLTUI_VERIFY_IDLE(session.engine);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("G7")));
    auto* grid = session.terminal->findChild<GridView*>(QStringLiteral("grid"));
    QVERIFY(grid);
    QMLTUI_VERIFY_ONE_FRAME(session.engine, grid->setCellWidth(16));
    QMLTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::treeViewStaysIdle() {
    MutableModel model([] {
        QStringList rows;
        for (int i = 0; i < 40; ++i)
            rows << QStringLiteral("Node %1").arg(i);
        return rows;
    }());
    m_qmlEngine.rootContext()->setContextProperty(QStringLiteral("loopModel"), &model);
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
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
    QMLTUI_VERIFY_IDLE(session.engine);
    QVERIFY(frameText(session.engine).contains(QStringLiteral("Node 3")));
    auto* tree = session.terminal->findChild<TreeView*>(QStringLiteral("tree"));
    QVERIFY(tree);
    QMLTUI_VERIFY_ONE_FRAME(session.engine, tree->setProperty("currentIndex", 5));
    QMLTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::canvasWithShapesStaysIdle() {
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
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
    QMLTUI_VERIFY_IDLE(session.engine);
    auto* canvas = session.terminal->findChild<Item*>(QStringLiteral("canvas"));
    QVERIFY(canvas);
    QMLTUI_VERIFY_ONE_FRAME(session.engine,
                            canvas->setProperty("color", QVariant::fromValue(Color("red"))));
    QMLTUI_VERIFY_IDLE(session.engine);
}

void TestRenderLoop::framePacingCoalescesBursts() {
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
        Terminal {
            Text { objectName: "label"; text: "0" }
        }
    )");
    QVERIFY(session.engine);
    QMLTUI_VERIFY_IDLE(session.engine);
    QCOMPARE(session.engine->maxFrameRate(), 0); // test sessions are uncapped

    auto* label = session.terminal->findChild<Item*>(QStringLiteral("label"));
    QVERIFY(label);

    // Uncapped: every change in a burst may become its own frame.
    quint64 before = session.engine->frameSequence();
    for (int i = 1; i <= 20; ++i) {
        label->setProperty("text", QString::number(i));
        QTest::qWait(1);
    }
    QTRY_VERIFY_WITH_TIMEOUT(session.engine->frameSequence() - before >= 10, 500);
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
    QTRY_VERIFY_WITH_TIMEOUT(frameText(session.engine).contains(QStringLiteral("40")), 500);
    QTest::qWait(25);
    const quint64 capped = session.engine->frameSequence() - before;
    const quint64 maxCapped = static_cast<quint64>(burstMs / 20) + 2;
    QVERIFY2(capped >= 1 && capped <= std::max<quint64>(6, maxCapped),
             qPrintable(QStringLiteral("%1 frames in %2 ms").arg(capped).arg(burstMs)));
    QVERIFY(frameText(session.engine).contains(QStringLiteral("40")));
    QMLTUI_VERIFY_IDLE(session.engine);
    session.engine->setMaxFrameRate(0);
}

void TestRenderLoop::partialDamageMatchesFullRender() {
    Session session = createSession(m_qmlEngine, R"(
        import QtQml
        import QmlTui
        Terminal {
            color: "black"
            Text {
                objectName: "cjkBanner"
                x: 4; y: 1; width: 20; height: 1
                text: "一二三四五六"
                color: "yellow"
            }
            Rectangle {
                objectName: "occluder"
                x: 7; y: 1; width: 4; height: 1; z: 1
                color: "blue"
                Text { anchors.fill: parent; text: "XXXX"; color: "white" }
            }
            TextInput {
                objectName: "editor"
                x: 4; y: 5; width: 20; height: 1
                focus: true
                text: "abc"
            }
            Text {
                objectName: "status"
                x: 40; y: 20; width: 20; height: 1
                text: "idle"
                color: "green"
            }
            ScrollView {
                objectName: "scroller"
                x: 40; y: 2; width: 20; height: 6
                contentHeight: 20
                Column {
                    Repeater {
                        model: 12
                        delegate: Rectangle {
                            required property int index
                            width: 20; height: 1
                            color: index % 2 === 0 ? "navy" : "transparent"
                            Text { text: "Item " + parent.index }
                        }
                    }
                }
            }
            Popup {
                objectName: "modalPopup"
                x: 10; y: 4; width: 24; height: 6
                modal: true
                dimBackground: true
                Rectangle {
                    anchors.fill: parent
                    color: "darkred"
                    border { style: Border.Single; color: "white" }
                    Text { x: 2; y: 2; text: "MODAL" }
                }
            }
        }
    )");
    QVERIFY(session.engine);
    QMLTUI_VERIFY_IDLE(session.engine);

    auto verifyMatchesFreshRender = [&](const char* stepName) {
        const std::optional<TextGridBuffer> committed = session.engine->currentFrame();
        QVERIFY2(committed.has_value(), stepName);
        const std::optional<QPoint> committedCursor = session.engine->hardwareCursorPosition();
        TextGridBuffer fresh;
        fresh.resize(committed->width(), committed->height());
        testing::paintItem(*session.terminal, fresh, 0, 0);
        const std::optional<QPoint> freshCursor = session.engine->hardwareCursorPosition();
        QString detail;
        QVERIFY2(buffersMatchCellForCell(*committed, fresh, &detail),
                 qPrintable(QStringLiteral("%1: %2").arg(QLatin1String(stepName), detail)));
        QCOMPARE(committedCursor, freshCursor);
    };

    verifyMatchesFreshRender("initial");

    auto* status = session.terminal->findChild<Text*>(QStringLiteral("status"));
    auto* occluder = session.terminal->findChild<Rectangle*>(QStringLiteral("occluder"));
    auto* editor = session.terminal->findChild<TextInput*>(QStringLiteral("editor"));
    auto* scroller = session.terminal->findChild<ScrollView*>(QStringLiteral("scroller"));
    auto* modalPopup = session.terminal->findChild<Popup*>(QStringLiteral("modalPopup"));
    QVERIFY(status && occluder && editor && scroller && modalPopup);
    QVERIFY(editor->activeFocus());
    QVERIFY(session.engine->hardwareCursorPosition().has_value());

    // 1. Update status far away from focused TextInput and CJK banner:
    // partial damage must preserve both CJK cells and hardwareCursorPosition.
    QMLTUI_VERIFY_ONE_FRAME(session.engine, status->setText(QStringLiteral("step-1")));
    verifyMatchesFreshRender("distant-status-update");

    // 2. Move occluder by 1 column across 2-column CJK glyph boundaries.
    QMLTUI_VERIFY_ONE_FRAME(session.engine, occluder->setX(6));
    verifyMatchesFreshRender("occluder-shift-left-onto-cjk");

    QMLTUI_VERIFY_ONE_FRAME(session.engine, occluder->setX(8));
    verifyMatchesFreshRender("occluder-shift-right-onto-cjk");

    // 3. Hide occluder so previously split 2-column CJK glyphs underneath uncover cleanly.
    QMLTUI_VERIFY_ONE_FRAME(session.engine, occluder->setVisible(false));
    verifyMatchesFreshRender("occluder-hidden-uncovers-cjk");

    // 4. Re-show and resize occluder.
    QMLTUI_VERIFY_ONE_FRAME(session.engine, {
        occluder->setVisible(true);
        occluder->setWidth(6);
    });
    verifyMatchesFreshRender("occluder-reshown-resized");

    // 5. Scroll ScrollView with rightPadding == 0 (scrollbar at x == width()).
    QMLTUI_VERIFY_ONE_FRAME(session.engine, scroller->setContentY(4));
    verifyMatchesFreshRender("scrollview-scrolled");

    // 6. Edit TextInput text locally.
    QMLTUI_VERIFY_ONE_FRAME(session.engine, editor->setText(QStringLiteral("hello")));
    verifyMatchesFreshRender("editor-text-updated");

    // 7. Open and close modal dimming popup, then do another localized partial update.
    QMLTUI_VERIFY_ONE_FRAME(session.engine, modalPopup->open());
    verifyMatchesFreshRender("modal-popup-open");

    QMLTUI_VERIFY_ONE_FRAME(session.engine, modalPopup->close());
    verifyMatchesFreshRender("modal-popup-closed");

    QMLTUI_VERIFY_ONE_FRAME(session.engine, status->setText(QStringLiteral("after-popup")));
    verifyMatchesFreshRender("partial-after-popup-close");

    // 8. Destroy occluder dynamically and verify its old cells uncover CJK banner cleanly.
    QMLTUI_VERIFY_ONE_FRAME(session.engine, delete occluder);
    verifyMatchesFreshRender("occluder-deleted");
}

void TestRenderLoop::offscreenCompoundSubtreeCulledInO1() {
    Terminal terminal;
    auto* engine = terminal.engine();
    engine->setTestViewportSize(QSize(40, 10));

    auto* scroll = new ScrollView(&terminal);
    scroll->setWidth(20);
    scroll->setHeight(4);
    scroll->setContentHeight(30);

    QList<CountingProbe*> probes;
    for (int i = 0; i < 15; ++i) {
        // Compound unclipped container (`clip() == false`, non-empty children)
        auto* rowWrapper = new Rectangle(scroll);
        rowWrapper->setY(i * 2);
        rowWrapper->setWidth(16);
        rowWrapper->setHeight(1);
        auto* probe = new CountingProbe(rowWrapper);
        probes.append(probe);
    }

    QMLTUI_VERIFY_IDLE(engine);

    for (CountingProbe* probe : probes) {
        probe->paintCalls = 0;
    }

    // Trigger a scroll to y = 10..13 (only rows i = 5 and i = 6 are inside [10, 14)).
    QMLTUI_VERIFY_ONE_FRAME(engine, scroll->setContentY(10));

    for (int i = 0; i < probes.size(); ++i) {
        const bool expectedVisible = (i * 2 >= 10 && i * 2 < 14);
        QCOMPARE(probes[i]->paintCalls, expectedVisible ? 1 : 0);
    }
}

QTEST_MAIN(TestRenderLoop)
#include "test_render_loop.moc"
