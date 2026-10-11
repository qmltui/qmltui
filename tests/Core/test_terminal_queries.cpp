#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <Qtui/Runner/MemoryTerminalBackend.h>
#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/engine/TerminalEngine.h>
#include <Qtui/Core/engine/StreamRenderer.h>

using namespace qtui;

namespace {
MemoryTerminalBackend* backend = nullptr;
QString frameText(TerminalEngine* engine) {
    const auto frame = engine->currentFrame();
    if (!frame) return {};
    QString text;
    for (int y = 0; y < frame->height(); ++y) {
        for (int x = 0; x < frame->width(); ++x)
            if (!frame->cellAt(x, y).continuation)
                text += QString::fromUtf8(frame->cellAt(x, y).character().toByteArray());
        text += '\n';
    }
    return text;
}
class FaultBackend : public TerminalBackend {
public:
    MemoryTerminalBackend memory{QSize(30, 4)};
    bool fail = false;
    bool blocked = false;
    bool enterRawMode() override { return memory.enterRawMode(); }
    bool leaveRawMode() override { return memory.leaveRawMode(); }
    WriteStatus writeSome(const char* data, size_t size, size_t& offset) override {
        if (fail) return WriteStatus::Error;
        if (blocked) return WriteStatus::WouldBlock;
        return memory.writeSome(data, size, offset);
    }
    bool writeAll(std::string& bytes, int timeout) override {
        return !fail && memory.writeAll(bytes, timeout);
    }
    bool supportsAsyncOutput() const override { return true; }
    bool setNonBlockingOutput(bool) override { return true; }
    void setWritableNotifications(bool) override {}
    QSize size() const override { return memory.size(); }
    bool isInteractive() const override { return true; }
    void startInput() override {}
    void stopInput() override {}
};
class CommandItem : public Item {
public:
    explicit CommandItem(Terminal* root) : Item(root), terminal(root) { setWidth(5); setHeight(1); }
    void paint(TextGridBuffer& buffer, int x, int y, const ClipRect&) const override {
        if (!sent) {
            sent = true;
            accepted = terminal->enqueueControlSequence("\033_Gi=42;one\033\\")
                && terminal->enqueueControlSequence("\033_Gi=43;two\033\\");
        }
        buffer.writeText(x, y, QStringLiteral("CELLS"));
    }
    Terminal* terminal;
    mutable bool sent = false;
    mutable bool accepted = false;
};
}

class TestTerminalQueries : public QObject {
    Q_OBJECT
private slots:
    void init() {
        for (const char* name : {"QTUI_SYNC_OUTPUT", "QTUI_KITTY_KEYBOARD", "QTUI_KITTY_GRAPHICS",
                                 "QTUI_QUERY_TIMEOUT_MS", "QTUI_ESCAPE_TIMEOUT_MS", "QTUI_RUNNER_EMBEDDED_SCENARIO",
                                 "QTUI_RUNNER_LIVE",
                                 "TERM_PROGRAM", "TERM", "KITTY_WINDOW_ID", "KONSOLE_VERSION",
                                 "WEZTERM_EXECUTABLE", "GHOSTTY_RESOURCES_DIR"}) {
            saved.insert(name, qgetenv(name));
            qunsetenv(name);
        }
        qputenv("QTUI_QUERY_TIMEOUT_MS", "1000");
        qputenv("TERM_PROGRAM", "kitty");   // a terminal known to parse APC strings
        TerminalBackend::setFactory([](int) {
            auto memory = std::make_unique<MemoryTerminalBackend>(QSize(30, 4));
            backend = memory.get();
            return memory;
        });
    }
    void cleanup() {
        TerminalBackend::setFactory(nullptr);
        for (auto it = saved.cbegin(); it != saved.cend(); ++it) {
            if (it.value().isNull()) qunsetenv(it.key().constData());
            else qputenv(it.key().constData(), it.value());
        }
        saved.clear();
    }
    void repliesUpdateCommittedFrame() {
        QQmlEngine qml;
        QQmlComponent component(&qml);
        component.setData(R"(
pragma ComponentBehavior: Bound
import QtQml
import Qtui
Terminal {
    id: root
    focus: true
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q) Qt.quit()
        event.accepted = true
    }
    Text { text: root.capabilities.kittyKeyboard ? "KEYBOARD" : "LEGACY" }
}
)", QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* terminal = qobject_cast<Terminal*>(object.get());
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        QSignalSpy changed(terminal, &Terminal::capabilitiesChanged);
        QTRY_VERIFY(frameText(engine).contains("LEGACY"));
        const auto& output = backend->output();
        QVERIFY(output.find("\033[?1049h") < output.find("\033[?u"));
        QVERIFY(output.find("\033_Gi=31,") != std::string::npos);
        QVERIFY(output.find("\033[?u") < output.find("LEGACY"));
        QCOMPARE(output.find("\033[?u", output.find("\033[?u") + 1), std::string::npos);
        backend->feedInput("\033[?0u\033_Gi=31;OK\033\\\033[6;20;10t\033[?2026;2$y");
        QVERIFY(!engine->capabilities().kittyKeyboard);
        backend->feedInput("\033[?1;2c");
        QTRY_VERIFY(frameText(engine).contains("KEYBOARD"));
        QCOMPARE(changed.count(), 1);
        QVERIFY(engine->capabilities().kittyGraphics);
        QVERIFY(engine->capabilities().syncOutput);
        QCOMPARE(engine->capabilities().cellPixelSize, QSize(10, 20));
        backend->feedInput("\033[?0u\033[?2026;0$y\033[?1;2c");
        QCOMPARE(changed.count(), 1);
        QVERIFY(engine->capabilities().syncOutput);
        backend->setSize(QSize(40, 5));
        backend->feedInput("\033[6;24;12t");
        QCOMPARE(engine->capabilities().cellPixelSize, QSize(12, 24));
        QCOMPARE(changed.count(), 2);
        // A window size mistaken for a cell size is not believed.
        backend->setSize(QSize(41, 5));
        backend->feedInput("\033[6;1080;1920t");
        QCOMPARE(engine->capabilities().cellPixelSize, QSize(12, 24));
    }
    void syncStatuses_data() {
        QTest::addColumn<int>("status");
        for (int i = 0; i <= 4; ++i) QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void syncStatuses() {
        QFETCH(int, status);
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        backend->feedInput("\033[?2026;" + QByteArray::number(status) + "$y\033[?1c");
        QCOMPARE(engine->capabilities().syncOutput, status >= 1 && status <= 3);
    }
    void overridesAndMalformedReplies() {
        qputenv("QTUI_SYNC_OUTPUT", "off");
        qputenv("QTUI_KITTY_KEYBOARD", "false");
        qputenv("QTUI_KITTY_GRAPHICS", "0");
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        backend->feedInput("\033[?u\033[?2026;5$y\033_Gi=99;OK\033\\\033[6;0;10t");
        backend->feedInput("\033[?0u\033_Gi=31;OK\033\\\033[?2026;1$y\033[?1c");
        QVERIFY(!engine->capabilities().kittyKeyboard);
        QVERIFY(!engine->capabilities().kittyGraphics);
        QVERIFY(!engine->capabilities().syncOutput);
        QVERIFY(!engine->capabilities().cellPixelSize.isValid());
    }
    void timeoutAndStoppedNegotiation() {
        qputenv("QTUI_QUERY_TIMEOUT_MS", "10");
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        QTest::qWait(30);
        backend->feedInput("\033[?0u\033_Gi=31;OK\033\\\033[?1c");
        QVERIFY(!engine->capabilities().kittyKeyboard);
        QVERIFY(!engine->capabilities().kittyGraphics);
        engine->setTerminalOutputEnabled(false);
        backend->clearOutput();
        QVERIFY(!engine->enqueueControlSequence("not written"));
        engine->setTerminalOutputEnabled(true);
        QTRY_VERIFY(backend->output().find("\033[?u") != std::string::npos);
        engine->stop();
        backend->feedInput("\033[?0u\033[?1c");
        QVERIFY(!engine->capabilities().kittyKeyboard);
        QVERIFY(!engine->enqueueControlSequence("not written"));
    }
    void controlsPrecedeCells_data() {
        QTest::addColumn<bool>("stream");
        QTest::newRow("frame") << false;
        QTest::newRow("stream") << true;
    }
    void controlsPrecedeCells() {
        QFETCH(bool, stream);
        Terminal terminal;
        auto* engine = terminal.engine();
        engine->setCapabilities(TerminalCapabilities{}); // deterministic; no probes
        if (stream) engine->setRenderer(new StreamRenderer);
        CommandItem item(&terminal);
        QTRY_VERIFY(engine->frameSequence() > 0);
        QVERIFY(item.accepted);
        const auto& output = backend->output();
        const auto first = output.find("\033_Gi=42;one\033\\");
        const auto second = output.find("\033_Gi=43;two\033\\");
        QVERIFY(first != std::string::npos);
        QVERIFY(first < second);
        QVERIFY(second < output.find("CELLS"));
        QVERIFY(frameText(engine).contains("CELLS"));
        backend->clearOutput();
        auto sequence = engine->frameSequence();
        engine->requestRender();
        QTRY_VERIFY(engine->frameSequence() > sequence);
        QVERIFY(backend->output().find("\033_G") == std::string::npos);
        backend->clearOutput();
        sequence = engine->frameSequence();
        QVERIFY(engine->enqueueControlSequence("\033_Gi=44;only\033\\"));
        QTRY_VERIFY(engine->frameSequence() > sequence);
        QVERIFY(backend->output().find("\033_Gi=44;only\033\\") != std::string::npos);
        QVERIFY(!engine->enqueueControlSequence(QByteArray(TerminalEngine::ControlSequenceQueueLimit + 1, 'x')));
        backend->clearOutput();
        QVERIFY(engine->enqueueControlSequence("\033_Ga=d,d=I,i=44\033\\"));
        engine->stop();
        QVERIFY(backend->output().find("\033_Ga=d,d=I,i=44\033\\") != std::string::npos);
        if (!stream)
            QVERIFY(backend->output().find("\033_Ga=d") < backend->output().find("\033[?1049l"));
    }
    void writeFailureInvalidatesSession_data() {
        QTest::addColumn<bool>("async");
        QTest::newRow("immediate") << false;
        QTest::newRow("queued") << true;
    }
    void writeFailureInvalidatesSession() {
        QFETCH(bool, async);
        FaultBackend* faulty = nullptr;
        TerminalBackend::setFactory([&](int) {
            auto result = std::make_unique<FaultBackend>();
            faulty = result.get();
            return result;
        });
        Terminal terminal;
        auto* engine = terminal.engine();
        engine->setCapabilities(TerminalCapabilities{});
        QTRY_VERIFY(engine->frameSequence() > 0);
        QSignalSpy failed(engine, &TerminalEngine::renderFailed);
        QSignalSpy invalidated(engine, &TerminalEngine::outputSessionInvalidated);
        if (async) faulty->blocked = true;
        else faulty->fail = true;
        const auto frame = engine->frameSequence();
        QVERIFY(engine->enqueueControlSequence("\033_Gi=42;upload\033\\"));
        if (async) {
            QTRY_VERIFY(engine->frameSequence() > frame);
            faulty->fail = true;
            emit faulty->writable();
        }
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(invalidated.count(), 1);
        QVERIFY(engine->takeControlSequences().isEmpty());
        faulty->fail = false;
        faulty->blocked = false;
    }
    void unknownTerminalSkipsGraphicsProbe() {
        qputenv("TERM_PROGRAM", "xterm");
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        const auto& output = backend->output();
        QVERIFY(output.find("\033[?u") != std::string::npos);
        QVERIFY(output.find("\033_G") == std::string::npos);
    }
    void nonInteractiveInputDoesNotProbe() {
        TerminalBackend::setFactory([](int) {
            auto memory = std::make_unique<MemoryTerminalBackend>(QSize(30, 4));
            memory->setInteractive(false);   // a pipe on either end
            backend = memory.get();
            return memory;
        });
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        QVERIFY(backend->output().find("\033[?u") == std::string::npos);
        QVERIFY(backend->output().find("\033[16t") == std::string::npos);
        backend->setSize(QSize(40, 5));
        QTRY_VERIFY(engine->frameSequence() > 1);
        QVERIFY(backend->output().find("\033[16t") == std::string::npos);
    }
    void renegotiationKeepsProvenCapabilities() {
        qputenv("QTUI_QUERY_TIMEOUT_MS", "10");
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        backend->feedInput("\033[?0u\033_Gi=31;OK\033\\\033[6;20;10t\033[?1;2c");
        QVERIFY(engine->capabilities().kittyGraphics);
        QVERIFY(engine->capabilities().kittyKeyboard);
        QSignalSpy changed(engine, &TerminalEngine::capabilitiesChanged);
        backend->clearOutput();
        engine->setRenderer(new StreamRenderer);   // new output session, new negotiation
        QTRY_VERIFY(backend->output().find("\033[?u") != std::string::npos);
        QTest::qWait(40);                          // the terminal never answers this time
        QVERIFY(engine->capabilities().kittyGraphics);
        QVERIFY(engine->capabilities().kittyKeyboard);
        QCOMPARE(engine->capabilities().cellPixelSize, QSize(10, 20));
        QCOMPARE(changed.count(), 0);
    }
    void failedProbeWriteIsReportedOnceAndRetried() {
        FaultBackend* faulty = nullptr;
        TerminalBackend::setFactory([&](int) {
            auto result = std::make_unique<FaultBackend>();
            faulty = result.get();
            result->fail = true;                    // every write fails from the start
            return result;
        });
        Terminal terminal;
        auto* engine = terminal.engine();
        CommandItem item(&terminal);                // a blank frame would write nothing
        QSignalSpy failed(engine, &TerminalEngine::renderFailed);
        QTRY_COMPARE(failed.count(), 1);            // one report for the frame, not two
        QVERIFY(faulty->memory.output().find("\033[?u") == std::string::npos);
        faulty->fail = false;
        engine->requestRender();
        QTRY_VERIFY(engine->frameSequence() > 0);
        QVERIFY(faulty->memory.output().find("\033[?u") != std::string::npos);
        QCOMPARE(failed.count(), 1);
    }
    void disablingOutputSettlesStateBeforeInvalidation() {
        Terminal terminal;
        auto* engine = terminal.engine();
        engine->setCapabilities(TerminalCapabilities{});
        QTRY_VERIFY(engine->frameSequence() > 0);
        bool acceptedDuringDisable = true;
        const auto connection = connect(engine, &TerminalEngine::outputSessionInvalidated, engine, [&] {
            acceptedDuringDisable = engine->enqueueControlSequence("\033_Ga=d\033\\");
        });
        engine->setTerminalOutputEnabled(false);
        disconnect(connection);
        QVERIFY(!acceptedDuringDisable);           // the contract: false when output is off
        QVERIFY(engine->takeControlSequences().isEmpty());
    }
    void backgroundFollowsAlternateScreen() {
        // The alternate screen resets the background on some terminals, so the
        // OSC 11 background must be (re)sent after entering it, not only before.
        QQmlEngine qml;
        QQmlComponent component(&qml);
        component.setData(R"(
import QtQml
import Qtui
Terminal { color: "black"; Text { text: "x" } }
)", QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* terminal = qobject_cast<Terminal*>(object.get());
        auto* engine = terminal->engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        const std::string& out = backend->output();
        const auto alt = out.find("\033[?1049h");
        const auto osc11After = out.find("\033]11;#000000", alt);
        QVERIFY(alt != std::string::npos);
        QVERIFY2(osc11After != std::string::npos,
                 "background not re-sent after entering the alternate screen");
    }
    // Kitty keyboard flag 1: pushed once the terminal has answered the query,
    // on the screen the renderer draws on, and popped before that screen is
    // left. Every pop below is checked against the alternate-screen exit that
    // follows it, because the stacks are per screen.
    void kittyKeyboardPushedAfterReplyAndPoppedOnStop() {
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        const auto& output = backend->output();
        const auto npos = std::string::npos;
        QVERIFY(output.find("\033[?u") != npos);
        QCOMPARE(output.find("\033[>1u"), npos);
        backend->feedInput("\033[?0u");            // zero flags is still a valid reply
        QCOMPARE(output.find("\033[>1u"), npos);     // the window is still open
        backend->feedInput("\033[?1c");
        QVERIFY(engine->capabilities().kittyKeyboard);
        const auto push = output.find("\033[>1u");
        QVERIFY(push != npos);
        QVERIFY(output.find("\033[?1049h") < push);
        QCOMPARE(output.find("\033[>1u", push + 1), npos);
        engine->stop();
        const auto pop = output.find("\033[<u");
        QVERIFY(pop != npos);
        QVERIFY(pop > push);
        QVERIFY(pop < output.find("\033[?1049l", push));
        QCOMPARE(output.find("\033[<u", pop + 1), npos);
    }
    void kittyKeyboardNotPushedWithoutReply_data() {
        QTest::addColumn<QByteArray>("override");
        QTest::newRow("silent terminal") << QByteArray();
        QTest::newRow("disabled by environment") << QByteArray("0");
    }
    void kittyKeyboardNotPushedWithoutReply() {
        QFETCH(QByteArray, override);
        qputenv("QTUI_QUERY_TIMEOUT_MS", "10");
        if (!override.isNull()) qputenv("QTUI_KITTY_KEYBOARD", override);
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        if (override.isNull()) QTest::qWait(30);      // the terminal never answers
        else backend->feedInput("\033[?0u\033[?1c");   // it answers, but the user said no
        QVERIFY(!engine->capabilities().kittyKeyboard);
        const auto& output = backend->output();
        QCOMPARE(output.find("\033[>1u"), std::string::npos);
        engine->stop();
        QCOMPARE(output.find("\033[<u"), std::string::npos);
    }
    void stopDuringNegotiationNeverPushes() {
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        engine->stop();
        backend->feedInput("\033[?0u\033[?1c");         // the reply arrives late
        QVERIFY(!engine->capabilities().kittyKeyboard);
        QCOMPARE(backend->output().find("\033[>1u"), std::string::npos);
        QCOMPARE(backend->output().find("\033[<u"), std::string::npos);
    }
    void kittyKeyboardFollowsOutputSession() {
        Terminal terminal;
        auto* engine = terminal.engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        backend->feedInput("\033[?0u\033[?1c");
        const auto& output = backend->output();
        const auto npos = std::string::npos;
        QVERIFY(output.find("\033[>1u") != npos);
        // Output disabled: popped before the screen is left, nothing after.
        backend->clearOutput();
        engine->setTerminalOutputEnabled(false);
        QVERIFY(output.find("\033[<u") != npos);
        QVERIFY(output.find("\033[<u") < output.find("\033[?1049l"));
        QCOMPARE(output.find("\033[>1u"), npos);
        // Re-enabled: the capability was proven, so the new screen gets its
        // own push right after it is entered, before the query round-trip.
        backend->clearOutput();
        engine->setTerminalOutputEnabled(true);
        QTRY_VERIFY(output.find("\033[>1u") != npos);
        QVERIFY(output.find("\033[?1049h") < output.find("\033[>1u"));
        QCOMPARE(output.find("\033[<u"), npos);
        // Renderer switch: pop on the alternate screen, push on the main one.
        backend->clearOutput();
        engine->setRenderer(new StreamRenderer);
        QTRY_VERIFY(output.find("\033[>1u") != npos);
        QVERIFY(output.find("\033[<u") < output.find("\033[?1049l"));
        QVERIFY(output.find("\033[?1049l") < output.find("\033[>1u"));
        QCOMPARE(output.find("\033[>1u", output.find("\033[>1u") + 1), npos);
        // A programmatic override that withdraws the capability pops at once.
        backend->clearOutput();
        TerminalCapabilities withdrawn = engine->capabilities();
        withdrawn.kittyKeyboard = false;
        engine->setCapabilities(withdrawn);
        QVERIFY(output.find("\033[<u") != npos);
        QCOMPARE(output.find("\033[>1u"), npos);
        backend->clearOutput();
        engine->stop();
        QCOMPARE(output.find("\033[<u"), npos);        // nothing left to pop
    }
    void kittyKeysReachQml() {
        QQmlEngine qml;
        QQmlComponent component(&qml);
        component.setData(R"(
pragma ComponentBehavior: Bound
import QtQml
import Qtui
Terminal {
    id: root
    focus: true
    property int presses: 0
    property string last: "none"
    Keys.onPressed: event => {
        root.presses++
        root.last = "K" + event.key + " M" + event.modifiers + " T" + event.text.length
        event.accepted = true
    }
    Text { text: root.last + " N" + root.presses }
}
)", QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* terminal = qobject_cast<Terminal*>(object.get());
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        QTRY_VERIFY(frameText(engine).contains("none N0"));
        const QString ctrlShift = QString::number(int(Qt::ControlModifier | Qt::ShiftModifier));
        backend->feedInput("\033[97:65:;6u");             // Ctrl+Shift+A
        QTRY_VERIFY(frameText(engine).contains("K65 M" + ctrlShift + " T0 N1"));
        backend->feedInput("\033[9;2u");                  // Shift+Tab keeps the Backtab convention
        QTRY_VERIFY(frameText(engine).contains("K" + QString::number(int(Qt::Key_Backtab))
                                               + " M" + QString::number(int(Qt::ShiftModifier)) + " T0 N2"));
        backend->feedInput("\033[13;5u");                 // Ctrl+Enter: no text
        QTRY_VERIFY(frameText(engine).contains("K" + QString::number(int(Qt::Key_Return))
                                               + " M" + QString::number(int(Qt::ControlModifier)) + " T0 N3"));
        backend->feedInput("\033[27u");                   // Escape, unambiguous and immediate
        QTRY_VERIFY(frameText(engine).contains("K" + QString::number(int(Qt::Key_Escape)) + " M0 T0 N4"));
        backend->feedInput("\033[98;1:2u\033[98;1:3u");   // repeat dispatches, release does not
        QTRY_VERIFY(frameText(engine).contains("K66 M0 T1 N5"));
        QCOMPARE(terminal->property("presses").toInt(), 5);
    }
    void kittyEnterChordsDoNotInsertText() {
        QQmlEngine qml;
        QQmlComponent component(&qml);
        component.setData(R"(
pragma ComponentBehavior: Bound
import QtQml
import Qtui
Terminal {
    id: root
    property int accepts: 0
    Column {
        TextInput { id: input; width: 20; height: 1; focus: true; onAccepted: root.accepts++ }
        Text { text: "LEN" + input.text.length + " ACC" + root.accepts + " [" + input.text + "]" }
    }
}
)", QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* terminal = qobject_cast<Terminal*>(object.get());
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        QTRY_VERIFY(frameText(engine).contains("LEN0 ACC0 []"));
        backend->feedInput("\033[13;5u");                 // Ctrl+Enter
        QTRY_VERIFY(frameText(engine).contains("LEN0 ACC1 []"));
        backend->feedInput("\033[13;2u");                 // Shift+Enter
        QTRY_VERIFY(frameText(engine).contains("LEN0 ACC2 []"));
        backend->feedInput("\033[97:65;2u\033[98u");      // Shift+A (with alternate key), b
        QTRY_VERIFY(frameText(engine).contains("LEN2 ACC2 [Ab]"));
        backend->feedInput("\033[97;5u");                 // Ctrl+A selects all, inserts nothing
        backend->feedInput("\033[99;3u");                 // Alt+C inserts nothing either
        QTRY_VERIFY(frameText(engine).contains("LEN2 ACC2 [Ab]"));
        backend->feedInput("\033[57423u");                // keypad Home: cursor to the start
        backend->feedInput("\033[57399u");                // keypad 0 is typing
        QTRY_VERIFY2(frameText(engine).contains("LEN3 ACC2 [0Ab]"), qPrintable(frameText(engine)));
    }
    void loneEscapeIsDeliveredAfterTheTimeout() {
        qputenv("QTUI_ESCAPE_TIMEOUT_MS", "10");
        QQmlEngine qml;
        QQmlComponent component(&qml);
        component.setData(R"(
pragma ComponentBehavior: Bound
import QtQml
import Qtui
Terminal {
    id: root
    focus: true
    property int escapes: 0
    property int ups: 0
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) root.escapes++
        if (event.key === Qt.Key_Up) root.ups++
        event.accepted = true
    }
    Text { text: "ESC" + root.escapes + " UP" + root.ups }
}
)", QUrl());
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        auto* terminal = qobject_cast<Terminal*>(object.get());
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        QTRY_VERIFY(frameText(engine).contains("ESC0 UP0"));
        backend->feedInput("\033");
        QCOMPARE(terminal->property("escapes").toInt(), 0);   // held back for the window
        QTRY_VERIFY(frameText(engine).contains("ESC1 UP0"));
        // The rest of a split sequence arriving inside the window wins.
        backend->feedInput("\033");
        backend->feedInput("[A");
        QTRY_VERIFY(frameText(engine).contains("ESC1 UP1"));
        QTest::qWait(30);
        QCOMPARE(terminal->property("escapes").toInt(), 1);
        // A second lone ESC after the first was consumed still gets its own timer.
        backend->feedInput("\033");
        QTRY_VERIFY(frameText(engine).contains("ESC2 UP1"));
    }
    void disabledStartupDoesNotProbe() {
        qputenv("QTUI_RUNNER_EMBEDDED_SCENARIO", "1");
        Terminal terminal;
        auto* engine = terminal.engine();
        QVERIFY(engine->testSession());
        QVERIFY(!engine->liveOutput());
        QTRY_VERIFY(engine->frameSequence() > 0);
        QVERIFY(backend->output().empty());
        QVERIFY(!engine->enqueueControlSequence("ignored"));
        QVERIFY(engine->takeControlSequences().isEmpty());
    }
    // The programmatic twin of QTUI_RUNNER_EMBEDDED_SCENARIO / QTUI_RUNNER_LIVE:
    // a test session begun after construction silences output and frame
    // pacing, and live output toggles rendering back on and off.
    void testSessionApiControlsOutput() {
        Terminal terminal;
        auto* engine = terminal.engine();
        QVERIFY(!engine->testSession());
        QVERIFY(engine->terminalOutputEnabled());
        QCOMPARE(engine->maxFrameRate(), 60);

        engine->beginTestSession();
        QVERIFY(engine->testSession());
        QVERIFY(!engine->terminalOutputEnabled());
        QCOMPARE(engine->maxFrameRate(), 0);
        backend->clearOutput();
        engine->requestRender();
        QTRY_VERIFY(engine->frameSequence() > 0);
        const quint64 quietFrame = engine->frameSequence();
        QVERIFY(backend->output().empty());

        engine->setLiveOutput(true);
        QVERIFY(engine->liveOutput());
        QVERIFY(engine->terminalOutputEnabled());
        QTRY_VERIFY(engine->frameSequence() > quietFrame);
        QVERIFY(!backend->output().empty());

        engine->setLiveOutput(false);
        QVERIFY(!engine->terminalOutputEnabled());
        // Live output is only meaningful inside a test session.
        Terminal plain;
        plain.engine()->setLiveOutput(true);
        QVERIFY(plain.engine()->terminalOutputEnabled());
        plain.engine()->setLiveOutput(false);
        QVERIFY(plain.engine()->terminalOutputEnabled());
    }
private:
    QHash<QByteArray, QByteArray> saved;
};

QTEST_MAIN(TestTerminalQueries)
#include "test_terminal_queries.moc"
