#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QStandardItemModel>
#include <memory>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Items/TableView.h>
#include <QmlTui/Core/Items/TreeView.h>
#include <QmlTui/Controls/TabBar.h>
#include <QmlTui/Controls/ToolTip.h>
#include <QmlTui/Core/Items/Popup.h>
#include "../TestPaint.h"

using namespace qmltui;
using qmltui::testing::paintItem;

class TestTerminalColors : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { qputenv("QMLTUI_RUNNER_CAPTURE_AFTER_MS", "0"); }
    void defaultInk_data();
    void defaultInk();
    void explicitColorsSurviveStates();
    void selectionColors();
    void toastAndTooltipOverrides();

private:
    QQmlEngine engine;
    std::unique_ptr<Item> load(const QByteArray& body) {
        QQmlComponent component(&engine);
        component.setData("import QtQml\nimport QmlTui\nimport QmlTui.Controls\n" + body, QUrl());
        if (component.isError()) qWarning().noquote() << component.errorString();
        return std::unique_ptr<Item>(qobject_cast<Item*>(component.create()));
    }
};

void TestTerminalColors::defaultInk_data() {
    QTest::addColumn<QByteArray>("qml");
    QTest::newRow("text") << QByteArray("Text { text: 'Ink' }");
    QTest::newRow("text_wrap") << QByteArray(
        "Text { text: '[Link](url) `code`'; wrapMode: Text.WordWrap; width: 30; height: 2 }");
    QTest::newRow("input") << QByteArray("TextInput { text: 'Ink'; width: 8; height: 1 }");
    QTest::newRow("area") << QByteArray("TextArea { text: 'Ink'; width: 8; height: 1 }");
    QTest::newRow("placeholder") << QByteArray(
        "TextInput { placeholderText: 'Hint'; width: 8; height: 1 }");
    QTest::newRow("button") << QByteArray("Button { text: 'Ink' }");
    QTest::newRow("check") << QByteArray("CheckBox { text: 'Ink'; checked: true }");
    QTest::newRow("radio") << QByteArray("RadioButton { text: 'Ink'; checked: true }");
    QTest::newRow("combo") << QByteArray("ComboBox { model: ['One', 'Two']; width: 16 }");
    QTest::newRow("spin") << QByteArray("SpinBox { value: 50 }");
    QTest::newRow("slider") << QByteArray("Slider { value: 50 }");
    QTest::newRow("progress") << QByteArray("ProgressBar { value: 0.5; showPercentage: true }");
    QTest::newRow("busy") << QByteArray("BusyIndicator { running: false }");
    QTest::newRow("panel") << QByteArray(
        "Panel { width: 12; height: 3; title: Text { text: 'Ink' } }");
    QTest::newRow("border") << QByteArray(
        "Rectangle { border.style: Border.Single; width: 8; height: 3 }");
    QTest::newRow("tabs") << QByteArray("TabBar { tabs: ['One', 'Two']; width: 24 }");
    QTest::newRow("sparkline") << QByteArray("Sparkline { values: [1, 2, 3]; width: 8 }");
    QTest::newRow("canvas") << QByteArray("Canvas { width: 2; height: 1; Point {} }");
}

void TestTerminalColors::defaultInk() {
    QFETCH(QByteArray, qml);
    auto item = load(qml);
    QVERIFY(item);
    QCOMPARE(item->metaObject()->indexOfProperty("palette"), -1);
    TextGridBuffer buffer;
    buffer.resize(40, 8);
    paintItem(item.get(), buffer, 0, 0);
    int glyphs = 0;
    for (int y = 0; y < buffer.height(); ++y) {
        for (int x = 0; x < buffer.width(); ++x) {
            const Cell& cell = buffer.cellAt(x, y);
            if (cell.character().isEmpty() || cell.character() == " ") continue;
            ++glyphs;
            QCOMPARE(cell.fgColor, 0u);
            QCOMPARE(cell.bgColor, 0u);
        }
    }
    QVERIFY(glyphs > 0);
}

void TestTerminalColors::explicitColorsSurviveStates() {
    for (const char* type : {"Button", "CheckBox", "RadioButton", "Slider", "SpinBox"}) {
        auto item = load(QByteArray(type) + " { textColor: '#123456'; width: 20; height: 1 }");
        QVERIFY(item);
        if (item->metaObject()->indexOfProperty("text") >= 0) item->setProperty("text", "Custom");
        item->setEnabled(false);
        TextGridBuffer buffer;
        buffer.resize(24, 3);
        paintItem(item.get(), buffer, 0, 0);
        int glyphs = 0;
        for (int x = 0; x < buffer.width(); ++x) {
            const Cell& cell = buffer.cellAt(x, 0);
            if (cell.character().isEmpty() || cell.character() == " ") continue;
            ++glyphs;
            QCOMPARE(cell.fgColor, 0xff123456u);
            QVERIFY((cell.attributes & TextAttribute::Dim) != TextAttribute::None);
        }
        QVERIFY(glyphs > 0);
    }
    for (const char* type : {"TextInput", "TextArea"}) {
        auto item =
            load(QByteArray(type) +
                 " { text: 'Custom'; color: '#123456'; width: 20; height: 1; enabled: false }");
        QVERIFY(item);
        TextGridBuffer buffer;
        buffer.resize(24, 1);
        paintItem(item.get(), buffer, 0, 0);
        QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xff123456u);
        QVERIFY((buffer.cellAt(0, 0).attributes & TextAttribute::Dim) != TextAttribute::None);
    }
}

void TestTerminalColors::selectionColors() {
    QStandardItemModel model;
    model.appendRow(new QStandardItem("Selected"));
    TableView table;
    table.setWidth(24);
    table.setHeight(5);
    table.setModel(QVariant::fromValue(static_cast<QObject*>(&model)));
    table.setSelectedRow(0);
    TreeView tree;
    tree.setWidth(24);
    tree.setHeight(5);
    tree.setModel(QVariant::fromValue(static_cast<QObject*>(&model)));
    tree.setCurrentIndex(0);
    for (Item* item : {static_cast<Item*>(&table), static_cast<Item*>(&tree)}) {
        TextGridBuffer buffer;
        buffer.resize(24, 5);
        paintItem(item, buffer, 0, 0);
        int selectedGlyphs = 0;
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 24; ++x) {
                const Cell& cell = buffer.cellAt(x, y);
                if (cell.character() == " " &&
                    (cell.attributes & TextAttribute::Reversed) == TextAttribute::None)
                    continue;
                QCOMPARE(cell.fgColor, 0u);
                QCOMPARE(cell.bgColor, 0u);
                if ((cell.attributes & TextAttribute::Reversed) != TextAttribute::None)
                    ++selectedGlyphs;
            }
        QVERIFY(selectedGlyphs > 0);
        item->setProperty("selectionColor", QVariant::fromValue(Color("#ffffff")));
        item->setProperty("selectionBackgroundColor", QVariant::fromValue(Color("#123456")));
        paintItem(item, buffer, 0, 0);
        int customGlyphs = 0;
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 24; ++x) {
                const Cell& cell = buffer.cellAt(x, y);
                if (cell.bgColor != 0xff123456u) continue;
                ++customGlyphs;
                QCOMPARE(cell.fgColor, 0xffffffffu);
                QVERIFY((cell.attributes & TextAttribute::Reversed) == TextAttribute::None);
            }
        QVERIFY(customGlyphs > 0);
    }
}

void TestTerminalColors::toastAndTooltipOverrides() {
    TabBar tabs;
    tabs.setTabs(QStringList{"One", "Two"});
    tabs.setWidth(28);
    tabs.setTextColor(Color("#123456"));
    tabs.setColor(Color("#654321"));
    tabs.setSelectedTextColor(Color("#ffffff"));
    tabs.setSelectionColor(Color("#102030"));
    TextGridBuffer tabBuffer;
    tabBuffer.resize(28, 1);
    paintItem(tabs, tabBuffer, 0, 0);
    QCOMPARE(tabBuffer.cellAt(0, 0).fgColor, 0xffffffffu);
    QCOMPARE(tabBuffer.cellAt(0, 0).bgColor, 0xff102030u);
    QVERIFY((tabBuffer.cellAt(0, 0).attributes & TextAttribute::Reversed) == TextAttribute::None);
    QCOMPARE(tabBuffer.cellAt(10, 0).fgColor, 0xff123456u);
    QCOMPARE(tabBuffer.cellAt(10, 0).bgColor, 0xff654321u);

    TextGridBuffer buffer;
    buffer.resize(28, 4);
    Terminal terminal;
    Item target(&terminal);
    target.setHeight(1);
    controls::ToolTip tip(&target);
    tip.setText("Tip");
    tip.setVisible(true);
    Popup* popup = nullptr;
    for (Item* item : terminal.childItems())
        if (auto* candidate = qobject_cast<Popup*>(item)) popup = candidate;
    QVERIFY(popup);
    paintItem(popup, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(1, 0).fgColor, 0u);
    QCOMPARE(buffer.cellAt(1, 0).bgColor, 0u);
    tip.setTextColor(Color("#123456"));
    tip.setColor(Color("#654321"));
    QCOMPARE(tip.backgroundColor(), Color("#654321"));
    paintItem(popup, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(1, 0).fgColor, 0xff123456u);
    QCOMPARE(buffer.cellAt(1, 0).bgColor, 0xff654321u);

    tip.setBackgroundColor(Color("#112233"));
    QCOMPARE(tip.color(), Color("#112233"));
    tip.border()->setStyle(Border::Rounded);
    tip.border()->setColor(Color("#aabbcc"));
    QCOMPARE(popup->width(), 7);
    QCOMPARE(popup->height(), 3);
    buffer.clear();
    paintItem(popup, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 0).character(), QStringLiteral("╭"));
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffaabbccu);
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xff112233u);
    QCOMPARE(buffer.cellAt(2, 1).character(), QStringLiteral("T"));
    QCOMPARE(buffer.cellAt(2, 1).fgColor, 0xff123456u);
    QCOMPARE(buffer.cellAt(2, 1).bgColor, 0xff112233u);

    auto customTipRoot = load(R"(
        Terminal {
            width: 40; height: 10
            Item {
                x: 2; y: 1; width: 6; height: 1
                ToolTip.visible: true
                ToolTip.timeout: 0
                ToolTip.background: Rectangle {
                    color: "#223344"
                    border {
                        style: Border.Double
                        color: "#556677"
                    }
                }
                ToolTip.contentItem: Text {
                    text: "SLOT"
                    color: "#ddeeff"
                }
            }
        }
    )");
    auto* customTerm = qobject_cast<Terminal*>(customTipRoot.get());
    QVERIFY(customTerm);
    QCOMPARE(customTerm->activePopups().size(), 1);
    Popup* slotPopup = customTerm->activePopups().first();
    QCOMPARE(slotPopup->width(), 8);
    QCOMPARE(slotPopup->height(), 3);
    buffer.clear();
    paintItem(slotPopup, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 0).character(), QStringLiteral("╔"));
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xff556677u);
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xff223344u);
    QCOMPARE(buffer.cellAt(2, 1).character(), QStringLiteral("S"));
    QCOMPARE(buffer.cellAt(2, 1).fgColor, 0xffddeeffu);

    auto qmlColors = load(R"(
        Item {
            property color def: QmlTui.defaultColor
            property color trans: QmlTui.transparent
            property color ansiRed: QmlTui.ansi(1)
            property color ansiOut: QmlTui.ansi(99)
        }
    )");
    QVERIFY(qmlColors);
    QCOMPARE(qmlColors->property("def").value<Color>(), Color(Color::Default));
    QCOMPARE(qmlColors->property("trans").value<Color>(), Color(Color::Transparent));
    QCOMPARE(qmlColors->property("ansiRed").value<Color>(), Color::fromAnsiIndex(1));
    QVERIFY(!qmlColors->property("ansiOut").value<Color>().isValid());
}

QTEST_MAIN(TestTerminalColors)
#include "test_terminal_colors.moc"
