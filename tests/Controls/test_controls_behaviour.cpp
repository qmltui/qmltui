// Behaviour of the QML-written controls that the reference pages state:
// StackView depth and page ownership, StackLayout following its children,
// TabBar's index surviving declaration order, disabled controls ignoring
// input, Slider snapping clicks to stepSize, ProgressBar style glyphs, and
// the internals of RadioButton and BusyIndicator staying off the public API.
// Note: moc treats a # inside a raw string as a directive, so QML in this file
// uses named colours, never "#rrggbb".
#include <QTest>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <memory>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/Cell.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>

using namespace qmltui;

namespace {
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

std::unique_ptr<QObject> load(QQmlEngine& qml, const char* source, Terminal** terminal) {
    QQmlComponent component(&qml);
    component.setData(QByteArray("import QtQml\nimport QmlTui\nimport QmlTui.Controls\n") + source, QUrl());
    std::unique_ptr<QObject> object(component.create());
    if (!object) qWarning() << component.errorString();
    *terminal = qobject_cast<Terminal*>(object.get());
    return object;
}
} // namespace

class TestControlsBehaviour : public QObject {
    Q_OBJECT
private slots:
    void stackViewDepthAndOwnership() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    id: root
    property Component page: Component { Rectangle { } }
    Item { id: keeper; objectName: "keeper"; visible: false }
    StackView { id: stack; objectName: "stack"; anchors.fill: parent }
    function pushTwo() { stack.push(page); stack.push(page) }
    function pushKeeper() { stack.push(keeper) }
    function popOne() { stack.pop() }
    function clearAll() { stack.clear() }
}
)", &terminal);
        QVERIFY(terminal);
        auto* stack = terminal->findChild<Item*>("stack");
        QVERIFY(stack);
        QCOMPARE(stack->property("depth").toInt(), 0);
        QMetaObject::invokeMethod(terminal, "pushTwo");
        QCOMPARE(stack->property("depth").toInt(), 2);
        QMetaObject::invokeMethod(terminal, "popOne");
        QCOMPARE(stack->property("depth").toInt(), 1);
        QPointer<Item> keeper = terminal->findChild<Item*>("keeper");
        QVERIFY(keeper);
        QMetaObject::invokeMethod(terminal, "pushKeeper");
        QCOMPARE(stack->property("depth").toInt(), 2);
        QVERIFY(keeper->visible());
        QMetaObject::invokeMethod(terminal, "popOne");
        QCOMPARE(stack->property("depth").toInt(), 1);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(keeper, "a page handed in as an Item must survive pop()");
        QVERIFY(!keeper->visible());
        QMetaObject::invokeMethod(terminal, "clearAll");
        QCOMPARE(stack->property("depth").toInt(), 0);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(keeper);
    }

    void stackLayoutCountFollowsChildren() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    id: root
    property Component page: Component { Rectangle { objectName: "late" } }
    StackLayout { id: layout; objectName: "layout"; width: 20; height: 5
        Rectangle { objectName: "first" }
        Rectangle { objectName: "second" }
    }
    function addPage() { page.createObject(layout) }
}
)", &terminal);
        QVERIFY(terminal);
        auto* layout = terminal->findChild<Item*>("layout");
        QVERIFY(layout);
        QCOMPARE(layout->property("count").toInt(), 2);
        QMetaObject::invokeMethod(terminal, "addPage");
        QCOMPARE(layout->property("count").toInt(), 3);
        auto* late = terminal->findChild<Item*>("late");
        QVERIFY(late);
        QVERIFY(!late->visible());              // index 0 is current
        layout->setProperty("currentIndex", 2);
        QVERIFY(late->visible());
        QCOMPARE(late->width(), 20);
        QCOMPARE(late->height(), 5);
        QVERIFY(!terminal->findChild<Item*>("first")->visible());
    }

    void tabBarIndexBeforeTabs() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    TabBar { objectName: "bar"; width: 40; height: 1; currentIndex: 2; tabs: ["A", "B", "C"] }
}
)", &terminal);
        QVERIFY(terminal);
        auto* bar = terminal->findChild<Item*>("bar");
        QVERIFY(bar);
        QCOMPARE(bar->property("currentIndex").toInt(), 2);
        bar->setProperty("tabs", QVariantList{"A", "B"});
        QCOMPARE(bar->property("currentIndex").toInt(), 1);   // clamped to the new range
        bar->setProperty("tabs", QVariantList{});
        bar->setProperty("currentIndex", 5);
        bar->setProperty("tabs", QVariantList{"A", "B", "C"});
        QCOMPARE(bar->property("currentIndex").toInt(), 2);
    }

    void disabledControlsIgnoreInput() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    id: root
    property int clicks: 0
    Button { objectName: "button"; x: 0; y: 0; text: "Go"; enabled: false; onClicked: root.clicks++ }
    Slider { objectName: "slider"; x: 0; y: 4; width: 20; showValue: false; enabled: false; value: 50 }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(40, 10));
        engine->handleMouseInput(0, 2, 1, false);
        engine->handleMouseInput(0, 2, 1, true);
        QCOMPARE(terminal->property("clicks").toInt(), 0);
        auto* slider = terminal->findChild<Item*>("slider");
        engine->handleMouseInput(0, 2, 4, false);
        engine->handleMouseInput(0, 2, 4, true);
        QCOMPARE(slider->property("value").toDouble(), 50.0);
        QVERIFY(!slider->focusable());
    }

    void sliderClickSnapsToStep() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    Slider { objectName: "slider"; x: 0; y: 0; width: 20; showValue: false; from: 0; to: 100; stepSize: 10 }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(40, 10));
        auto* slider = terminal->findChild<Item*>("slider");
        engine->handleMouseInput(0, 4, 0, false);     // 4 of 20 cells = 20%
        engine->handleMouseInput(0, 4, 0, true);
        QCOMPARE(slider->property("value").toDouble(), 20.0);
        engine->handleMouseInput(0, 5, 0, false);     // 25% snaps to a step
        engine->handleMouseInput(0, 5, 0, true);
        QCOMPARE(slider->property("value").toDouble(), 30.0);
        engine->handleMouseInput(0, 19, 0, false);    // never past `to`
        engine->handleMouseInput(0, 19, 0, true);
        QCOMPARE(slider->property("value").toDouble(), 100.0);
    }

    void progressBarStyles() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    Column {
        ProgressBar { width: 10; value: 0.5 }
        ProgressBar { width: 10; value: 0.5; style: "segmented" }
        ProgressBar { width: 10; value: 0.5; style: "pill" }
        ProgressBar { width: 10; value: 0.5; style: "dot" }
    }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(20, 6));
        QTRY_VERIFY(engine->frameSequence() > 0);
        const QString text = frameText(engine);
        QVERIFY2(text.contains(QStringLiteral("█████░░░░░")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("▰▰▰▰▰▱▱▱▱▱")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("▮▮▮▮▮▯▯▯▯▯")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("●●●●●○○○○○")), qPrintable(text));
    }

    void mouseAreaHover() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    MouseArea { objectName: "area"; x: 2; y: 2; width: 4; height: 2; hoverEnabled: true
        property int enters: 0
        property int exits: 0
        onEntered: enters++
        onExited: exits++
        Item { objectName: "inner"; width: 1; height: 1 }
    }
    MouseArea { objectName: "plain"; x: 10; y: 2; width: 4; height: 2 }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(40, 10));
        auto* area = terminal->findChild<Item*>("area");
        auto* plain = terminal->findChild<Item*>("plain");
        QVERIFY(terminal->hasMouseMotionRequests());
        QVERIFY(!area->property("containsMouse").toBool());
        engine->handleMouseInput(3, 2, 2, false, true);   // motion, no button, over the inner child
        QVERIFY(area->property("containsMouse").toBool());
        QCOMPARE(area->property("enters").toInt(), 1);
        engine->handleMouseInput(3, 4, 3, false, true);   // still inside: no second entered()
        QCOMPARE(area->property("enters").toInt(), 1);
        engine->handleMouseInput(3, 11, 2, false, true);  // over the area without hover
        QVERIFY(!area->property("containsMouse").toBool());
        QCOMPARE(area->property("exits").toInt(), 1);
        QVERIFY(!plain->property("containsMouse").toBool());
        engine->handleMouseInput(3, 3, 3, false, true);
        QVERIFY(area->property("containsMouse").toBool());
        area->setProperty("hoverEnabled", false);         // opting out clears the state
        QVERIFY(!area->property("containsMouse").toBool());
        QCOMPARE(area->property("exits").toInt(), 2);
        QVERIFY(!terminal->hasMouseMotionRequests());
    }

    void buttonHover() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    Button { objectName: "plain"; x: 0; y: 0; text: "Go" }
    Button { objectName: "hover"; x: 0; y: 4; text: "Go"; hoverEnabled: true }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(40, 10));
        auto* hover = terminal->findChild<Item*>("hover");
        QVERIFY(terminal->hasMouseMotionRequests());     // the opt-in button asked for motion
        engine->handleMouseInput(3, 2, 5, false, true);
        QVERIFY(hover->property("hovered").toBool());
        engine->handleMouseInput(3, 2, 1, false, true);   // over the button that did not opt in
        QVERIFY(!hover->property("hovered").toBool());
        QVERIFY(!terminal->findChild<Item*>("plain")->property("hovered").toBool());
    }

    // AbstractButton template: custom items are adopted, laid out and sized.
    void buttonCustomItems() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    Button {
        id: filled; objectName: "filled"; x: 0; y: 0; text: "Filled"
        background: Rectangle { objectName: "bg"; color: "lime"; border.style: Border.None }
        contentItem: Text { objectName: "label"; text: filled.text; color: "black"
                            horizontalAlignment: Qt.AlignHCenter; verticalAlignment: Qt.AlignVCenter }
    }
    Button { objectName: "wide"; x: 0; y: 4; width: 20; text: "Go" }
    Button { objectName: "short"; x: 0; y: 8; height: 1; text: "Row" }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(30, 10));
        auto* filled = terminal->findChild<Item*>("filled");
        auto* bg = terminal->findChild<Item*>("bg");
        auto* label = terminal->findChild<Item*>("label");
        QVERIFY(filled && bg && label);
        QCOMPARE(bg->parentItem(), filled);                 // adopted as a visual child
        QVERIFY(filled->childItems().contains(bg));
        QCOMPARE(filled->width(), 6 + 4);                    // label + paddings, unchanged from before
        QCOMPARE(filled->height(), 3);
        QCOMPARE(QSize(bg->width(), bg->height()), QSize(10, 3));
        QCOMPARE(QPoint(label->x(), label->y()), QPoint(2, 1));
        QCOMPARE(QSize(label->width(), label->height()), QSize(6, 1));
        QCOMPARE(bg->z(), -1.0);
        QTRY_VERIFY(engine->frameSequence() > 0);
        const QString text = frameText(engine);
        QVERIFY2(text.contains("Filled"), qPrintable(text));
        // The filled button's own rows carry no default border corner.
        const QStringList rows = text.split('\n');
        QVERIFY2(rows.size() > 2 && !rows[0].left(10).contains(QChar(0x250C))
                 && !rows[2].left(10).contains(QChar(0x2514)), qPrintable(text));
        auto* wide = terminal->findChild<Item*>("wide");
        QCOMPARE(wide->property("availableWidth").toInt(), 16);
        QVERIFY2(text.contains("│         Go       │") || text.contains(" Go "), qPrintable(text));
        auto* shortButton = terminal->findChild<Item*>("short");
        QCOMPARE(shortButton->property("availableHeight").toInt(), 0);
        QVERIFY2(text.contains("Row"), qPrintable(text));      // centred despite no padded room
    }

    void abstractButtonStates() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    id: root
    property int clicks: 0
    property int toggles: 0
    property bool lastState: false
    Button { objectName: "toggle"; x: 0; y: 0; width: 10; height: 3; text: "Pin"; checkable: true
        onClicked: root.clicks++
        onToggled: checked => { root.toggles++; root.lastState = checked } }
    Button { objectName: "styled"; x: 0; y: 4; width: 10; height: 3; text: "Down"; down: true }
    RadioButton { id: a; objectName: "a"; x: 0; y: 8; text: "A"; checked: true }
    ButtonGroup { buttons: [a] }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(30, 12));
        auto* toggle = terminal->findChild<Item*>("toggle");
        engine->handleMouseInput(0, 2, 1, false);
        QVERIFY(toggle->property("pressed").toBool());
        QVERIFY(toggle->property("down").toBool());          // down follows pressed
        engine->handleMouseInput(0, 2, 1, true);
        QVERIFY(!toggle->property("pressed").toBool());
        QVERIFY(toggle->property("checked").toBool());        // checkable: click toggled
        QCOMPARE(terminal->property("toggles").toInt(), 1);
        QVERIFY(terminal->property("lastState").toBool());
        QCOMPARE(terminal->property("clicks").toInt(), 1);   // clicked() follows toggled()
        toggle->setProperty("checked", false);               // assignment emits nothing
        QCOMPARE(terminal->property("toggles").toInt(), 1);
        QMetaObject::invokeMethod(toggle, "toggle");
        QVERIFY(toggle->property("checked").toBool());
        QCOMPARE(terminal->property("toggles").toInt(), 2);
        QCOMPARE(terminal->property("clicks").toInt(), 1);   // toggle() alone does not click
        auto* styled = terminal->findChild<Item*>("styled");
        QVERIFY(styled->property("down").toBool());           // explicit down overrides the binding
        QVERIFY(!styled->property("pressed").toBool());
        auto* a = terminal->findChild<Item*>("a");
        QMetaObject::invokeMethod(a, "toggle");
        QVERIFY(a->property("checked").toBool());             // autoExclusive: stays checked
    }

    // A held mouse button shows as reverse video on the label; focus stays a
    // double border, so the two never look alike.
    void buttonPressFeedback() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    Button { objectName: "go"; x: 0; y: 0; text: "Go" }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(20, 5));
        QTRY_VERIFY(engine->frameSequence() > 0);
        auto reversedAt = [&](int x, int y) {
            const auto frame = engine->currentFrame();
            return frame && (frame->cellAt(x, y).attributes & TextAttribute::Reversed) != TextAttribute::None;
        };
        auto cornerAt = [&](int x, int y) {
            const auto frame = engine->currentFrame();
            return frame ? QString::fromUtf8(frame->cellAt(x, y).character().toByteArray()) : QString();
        };
        QVERIFY(!reversedAt(2, 1));
        QCOMPARE(cornerAt(0, 0), QStringLiteral(u"┌"));      // unfocused: single border
        quint64 seq = engine->frameSequence();
        engine->handleMouseInput(0, 2, 1, false);           // press and hold
        QTRY_VERIFY(engine->frameSequence() > seq);
        QVERIFY2(reversedAt(2, 1), "label should be reversed while down");
        QCOMPARE(cornerAt(0, 0), QStringLiteral(u"╔"));      // the press also focused it
        seq = engine->frameSequence();
        engine->handleMouseInput(0, 2, 1, true);            // release
        QTRY_VERIFY(engine->frameSequence() > seq);
        QVERIFY2(!reversedAt(2, 1), "release must clear the press look");
        QCOMPARE(cornerAt(0, 0), QStringLiteral(u"╔"));      // focus remains, as a double border
    }

    // Focus on a Slider is visible: heavy track, inverted readout.
    void sliderFocusLook() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    Slider { objectName: "a"; x: 0; y: 0; width: 12; value: 50; focus: true }
    Slider { objectName: "b"; x: 0; y: 2; width: 12; value: 50 }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(20, 4));
        QTRY_VERIFY(engine->frameSequence() > 0);
        const QStringList rows = frameText(engine).split('\n');
        QVERIFY(rows.size() > 2);
        QVERIFY2(rows[0].contains(QChar(0x2501)) && !rows[0].contains(QChar(0x2500)), qPrintable(rows[0]));  // heavy track
        QVERIFY2(rows[2].contains(QChar(0x2500)) && !rows[2].contains(QChar(0x2501)), qPrintable(rows[2]));  // light track
        const auto frame = engine->currentFrame();
        QVERIFY(frame);
        const int readout = rows[0].indexOf('[');
        QVERIFY(readout > 0);
        QVERIFY((frame->cellAt(readout, 0).attributes & TextAttribute::Reversed) != TextAttribute::None);
        QVERIFY((frame->cellAt(readout, 2).attributes & TextAttribute::Reversed) == TextAttribute::None);
        terminal->findChild<Item*>("b")->setFocus(true);   // focus moves, so does the look
        QTRY_VERIFY(frameText(engine).split('\n')[2].contains(QChar(0x2501)));
        QVERIFY(frameText(engine).split('\n')[0].contains(QChar(0x2500)));
    }

    void internalsAreNotPublic() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    id: root
    ButtonGroup { id: group; buttons: [a, b] }
    RadioButton { id: a; objectName: "a"; text: "A"; checked: true }
    RadioButton { id: b; objectName: "b"; text: "B"; y: 1 }
    BusyIndicator { id: busy; objectName: "busy"; y: 2; interval: 10 }
    property bool radioHelperHidden: typeof a._uncheckGroup === "undefined"
    property bool busyStateHidden: typeof busy._frameIndex === "undefined"
                                && typeof busy._frames === "undefined"
                                && typeof busy._currentSymbol === "undefined"
    function checkB() { b.toggle() }
}
)", &terminal);
        QVERIFY(terminal);
        QVERIFY(terminal->property("radioHelperHidden").toBool());
        QVERIFY(terminal->property("busyStateHidden").toBool());
        auto* a = terminal->findChild<Item*>("a");
        auto* b = terminal->findChild<Item*>("b");
        QMetaObject::invokeMethod(terminal, "checkB");
        QVERIFY(b->property("checked").toBool());
        QVERIFY(!a->property("checked").toBool());   // exclusivity still works
        auto* busy = terminal->findChild<Item*>("busy");
        const QString first = busy->property("text").toString();
        QTRY_VERIFY(busy->property("text").toString() != first);   // still animates
    }

    void panelBorderSlotsAndAlignment() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    id: root
    Panel {
        id: p1
        objectName: "p1"
        x: 0; y: 0; width: 30; height: 3
        border.style: Border.Rounded
        title: "Speed"
        titleBadge: "[active]"
        titleTrailing: "0.0 km/h"
        footer: "0"
        footerTrailing: "250"
    }
    Panel {
        id: p2
        objectName: "p2"
        x: 0; y: 4; width: 25; height: 3
        border.style: Border.Rounded
        title: "LongTitle"
        titleAlignment: Qt.AlignHCenter
        titleTrailing: "Right"
    }
}
)", &terminal);
        QVERIFY(terminal);
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(30, 8));
        QTRY_VERIFY(engine->frameSequence() > 0);
        const QStringList rows = frameText(engine).split('\n');
        QVERIFY(rows.size() >= 7);

        // p1 top row: rounded corners, title, badge, trailing
        QCOMPARE(rows[0], QString::fromUtf8("╭─ Speed [active]  0.0 km/h ─╮"));
        // p1 middle row: rounded vertical borders
        QCOMPARE(rows[1], QString::fromUtf8("│                            │"));
        // p1 bottom row: rounded corners, footer, footerTrailing (total 30 chars)
        QCOMPARE(rows[2], QString::fromUtf8("╰─ 0 ────────────────── 250 ─╯"));

        // p2 top row: centered title clamped to avoid colliding with trailing text
        // Ideal center for 11 chars in width 25 is x=7, but trailing starts at x=16,
        // so title is clamped to x=5:
        QCOMPARE(rows[4].left(25), QString::fromUtf8("╭──── LongTitle  Right ─╮"));
    }

    void busyIndicatorFramesBehaviour() {
        QQmlEngine qml;
        Terminal* terminal = nullptr;
        auto object = load(qml, R"(
Terminal {
    id: root
    BusyIndicator { id: b1; objectName: "b1"; interval: 10 }
    BusyIndicator { id: b2; objectName: "b2"; frames: ["A", "B", "C"]; running: false }
    BusyIndicator { id: b3; objectName: "b3"; frames: ["X", "Y"]; interval: 10 }
}
)", &terminal);
        QVERIFY(terminal);
        auto* b1 = terminal->findChild<Item*>("b1");
        auto* b2 = terminal->findChild<Item*>("b2");
        auto* b3 = terminal->findChild<Item*>("b3");
        QVERIFY(b1 && b2 && b3);

        // b1 defaults to 10-frame braille spinner and animates
        const QStringList defaultBraille = {
            QStringLiteral(u"⠋"), QStringLiteral(u"⠙"), QStringLiteral(u"⠹"), QStringLiteral(u"⠸"),
            QStringLiteral(u"⠼"), QStringLiteral(u"⠴"), QStringLiteral(u"⠦"), QStringLiteral(u"⠧"),
            QStringLiteral(u"⠇"), QStringLiteral(u"⠏")
        };
        QCOMPARE(b1->property("frames").toStringList(), defaultBraille);
        QTRY_VERIFY(b1->property("text").toString() != QStringLiteral(u"⠋"));

        // b2 paused with custom frames shows initial frame
        QCOMPARE(b2->property("frames").toStringList(), QStringList({QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")}));
        QCOMPARE(b2->property("text").toString(), QStringLiteral("A"));
        b2->setProperty("running", true);
        QTRY_VERIFY(b2->property("text").toString() == QStringLiteral("B") || b2->property("text").toString() == QStringLiteral("C"));

        // b3 custom frames animate
        QCOMPARE(b3->property("frames").toStringList(), QStringList({QStringLiteral("X"), QStringLiteral("Y")}));
        QTRY_VERIFY(b3->property("text").toString() == QStringLiteral("Y"));
    }
};

QTEST_MAIN(TestControlsBehaviour)
#include "test_controls_behaviour.moc"
