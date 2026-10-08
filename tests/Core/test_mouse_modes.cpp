// The bytes the engine writes for mouse tracking, from the two inputs that
// decide them: Terminal.mouseReporting (the master switch, automatic per
// mode unless set) and whether any item asked for any-motion reporting
// (MouseArea.hoverEnabled, ToolTip). Motion must never be enabled while the
// switch is off, and leaving motion mode must re-assert button-held motion
// only while the switch is on.
#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <Qtui/TestRunner/MemoryTerminalBackend.h>
#include <Qtui/Core/items/Item.h>
#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/engine/TerminalEngine.h>

using namespace qtui;

namespace {
MemoryTerminalBackend* backend = nullptr;
constexpr const char* kAllOff = "\033[?1000l\033[?1002l\033[?1003l\033[?1006l";
constexpr const char* kClicks = "\033[?1003l\033[?1000h\033[?1002h\033[?1006h";
constexpr const char* kMotion = "\033[?1000h\033[?1002h\033[?1006h\033[?1003h";

std::unique_ptr<QObject> load(QQmlEngine& qml, const char* source, Terminal** terminal) {
    QQmlComponent component(&qml);
    component.setData(QByteArray("import QtQml\nimport Qtui\n") + source, QUrl());
    std::unique_ptr<QObject> object(component.create());
    if (!object) qWarning() << component.errorString();
    *terminal = qobject_cast<Terminal*>(object.get());
    return object;
}
size_t count(const std::string& haystack, const char* needle) {
    size_t n = 0;
    for (size_t pos = haystack.find(needle); pos != std::string::npos; pos = haystack.find(needle, pos + 1)) ++n;
    return n;
}
}

class TestMouseModes : public QObject {
    Q_OBJECT
private slots:
    void init() {
        TerminalBackend::setFactory([](int) {
            auto memory = std::make_unique<MemoryTerminalBackend>(QSize(40, 10));
            backend = memory.get();
            return memory;
        });
    }
    void cleanup() { TerminalBackend::setFactory(nullptr); }

    void fullScreenDefaultsToClicksOnly() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, "Terminal { MouseArea { width: 4; height: 1 } }", &terminal);
        QVERIFY(terminal);
        QVERIFY(terminal->mouseReporting());
        const auto& out = backend->output();
        QVERIFY(out.find(kClicks) != std::string::npos);
        QCOMPARE(out.find("\033[?1003h"), std::string::npos);
    }

    void inlineDefaultsToNoMouse() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, "Terminal { mode: Terminal.Inline; MouseArea { width: 4; height: 1; hoverEnabled: true } }", &terminal);
        QVERIFY(terminal);
        QVERIFY(!terminal->mouseReporting());
        QVERIFY(terminal->hasMouseMotionRequests());
        const auto& out = backend->output();
        QCOMPARE(out.find("\033[?1000h"), std::string::npos);
        QCOMPARE(out.find("\033[?1003h"), std::string::npos);
        QVERIFY(out.find(kAllOff) != std::string::npos);
    }

    void hoverRequestsRespectTheSwitch() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    mouseReporting: false
    MouseArea { objectName: "area"; width: 4; height: 1 }
}
)", &terminal);
        QVERIFY(terminal);
        auto* area = terminal->findChild<Item*>("area");
        const auto& out = backend->output();
        QCOMPARE(out.find("\033[?1000h"), std::string::npos);

        // Asking for hover while the mouse is declined writes nothing that enables it.
        backend->clearOutput();
        area->setProperty("hoverEnabled", true);
        QVERIFY(terminal->hasMouseMotionRequests());
        QCOMPARE(out.find("\033[?1003h"), std::string::npos);
        QCOMPARE(out.find("\033[?1002h"), std::string::npos);

        // Flipping the switch on delivers clicks and the pending motion request together.
        backend->clearOutput();
        terminal->setMouseReporting(true);
        QVERIFY(out.find(kMotion) != std::string::npos);

        // Dropping the last hover request falls back to clicks and drags.
        backend->clearOutput();
        area->setProperty("hoverEnabled", false);
        QVERIFY(out.find(kClicks) != std::string::npos);
        QCOMPARE(out.find("\033[?1003h"), std::string::npos);

        // Switching off turns everything off, and a later hover request stays quiet.
        backend->clearOutput();
        terminal->setMouseReporting(false);
        QVERIFY(out.find(kAllOff) != std::string::npos);
        backend->clearOutput();
        area->setProperty("hoverEnabled", true);
        area->setProperty("hoverEnabled", false);
        QCOMPARE(count(out, "h"), size_t(0));   // no mode enabled at all
    }
};

QTEST_MAIN(TestMouseModes)
#include "test_mouse_modes.moc"
