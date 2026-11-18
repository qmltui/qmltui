// The bytes the engine writes for mouse tracking, from the two inputs that
// decide them: Terminal.mouseReporting (the master switch, automatic per
// mode unless set) and whether any item asked for any-motion reporting
// (MouseArea.hoverEnabled, ToolTip). Motion must never be enabled while the
// switch is off, and leaving motion mode must re-assert button-held motion
// only while the switch is on.
#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QmlTui/Runner/MemoryTerminalBackend.h>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>

using namespace qmltui;

namespace {
MemoryTerminalBackend* backend = nullptr;
constexpr const char* kAllOff = "\033[?1000l\033[?1002l\033[?1003l\033[?1006l";
constexpr const char* kClicks = "\033[?1003l\033[?1000h\033[?1002h\033[?1006h";
constexpr const char* kMotion = "\033[?1000h\033[?1002h\033[?1006h\033[?1003h";

std::unique_ptr<QObject> load(QQmlEngine& qml, const char* source, Terminal** terminal) {
    QQmlComponent component(&qml);
    component.setData(QByteArray("import QtQml\nimport QmlTui\n") + source, QUrl());
    std::unique_ptr<QObject> object(component.create());
    if (!object) qWarning() << component.errorString();
    *terminal = qobject_cast<Terminal*>(object.get());
    return object;
}
size_t count(const std::string& haystack, const char* needle) {
    size_t n = 0;
    for (size_t pos = haystack.find(needle); pos != std::string::npos;
         pos = haystack.find(needle, pos + 1))
        ++n;
    return n;
}
} // namespace

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

    void altScreenDefaultsToClicksOnly() {
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
        auto object = load(qml,
                           "Terminal { mode: Terminal.Inline; MouseArea { width: 4; height: 1; "
                           "hoverEnabled: true } }",
                           &terminal);
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
)",
                           &terminal);
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
        QCOMPARE(count(out, "h"), size_t(0)); // no mode enabled at all
    }

    // A Repeater parents its finished delegate after creation, so the area's
    // own parentChanged fired while the delegate was still detached. The
    // registration must follow the ancestor's attach (Item::ancestryChanged),
    // or hover never works inside delegates.
    void hoverRegistersInsideRepeaterDelegates() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    Row {
        Repeater {
            model: 1
            delegate: Rectangle {
                required property int index
                width: 6; height: 1
                MouseArea { objectName: "area"; anchors.fill: parent; hoverEnabled: true }
            }
        }
    }
}
)",
                           &terminal);
        QVERIFY(terminal);
        auto* area = terminal->findChild<Item*>("area");
        QVERIFY(area);
        QVERIFY(terminal->isMouseMotionRequester(area));
        const auto& out = backend->output();
        QVERIFY(out.find(kMotion) != std::string::npos);

        // The router publishing the area as hovered must reach containsMouse.
        terminal->setHoveredItem(area);
        QCOMPARE(area->property("containsMouse").toBool(), true);
        terminal->setHoveredItem(nullptr);
        QCOMPARE(area->property("containsMouse").toBool(), false);

        // Detaching the delegate subtree drops the request again.
        area->parentItem()->setParentItem(nullptr);
        QVERIFY(!terminal->hasMouseMotionRequests());
    }

    // ToolTip binds its terminal the same way and had the same hole.
    void toolTipRegistersInsideRepeaterDelegates() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        QQmlComponent component(&qml);
        component.setData(QByteArray("import QtQml\nimport QmlTui\nimport QmlTui.Controls\n"
                                     R"(
Terminal {
    Row {
        Repeater {
            model: 1
            delegate: Rectangle {
                required property int index
                width: 6; height: 1
                ToolTip.text: "hint"
            }
        }
    }
}
)"),
                          QUrl());
        std::unique_ptr<QObject> object(component.create());
        if (!object) qWarning() << component.errorString();
        terminal = qobject_cast<Terminal*>(object.get());
        QVERIFY(terminal);
        QVERIFY(terminal->hasMouseMotionRequests());
    }
};

QTEST_MAIN(TestMouseModes)
#include "test_mouse_modes.moc"
