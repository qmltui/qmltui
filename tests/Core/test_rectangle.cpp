#include <QTest>
#include <QSignalSpy>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QmlTui/Core/Items/Rectangle.h>
#include <QmlTui/Core/Items/Gradient.h>
#include <QmlTui/Core/Items/GradientStop.h>
#include "../TestPaint.h"
#include <memory>

using namespace qmltui;

static TextGridBuffer paintRect(Rectangle& rect, int width = 10, int height = 5) {
    TextGridBuffer buffer;
    buffer.resize(width, height);
    testing::paintItem(rect, buffer, 0, 0);
    return buffer;
}

class TestRectangle : public QObject {
    Q_OBJECT
private slots:
    void testSolidColor();
    void testDefaultColorPaintsAndOccludes();
    void testTransparentColorDoesNotPaint();
    void testVerticalGradient();
    void testHorizontalGradient();
    void testGradientWithBorder();
    void testGradientUpdate();
    void testBorderStyles();
    void testQmlIntegration();
};

// "default" and "transparent" both reach the terminal as its own colour, but
// only "default" is painted, and so only "default" hides what is behind it.
// This is what lets a popup follow the terminal background while still
// occluding the content it covers.
void TestRectangle::testDefaultColorPaintsAndOccludes() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 1));
    for (int x = 0; x < 4; ++x) {
        Cell& cell = buffer.cellAt(x, 0);
        cell.setCharacter("#");
        cell.setBackground(Color("navy"));
    }

    Rectangle rect;
    rect.setWidth(4);
    rect.setHeight(1);
    rect.setColor(Color("default"));
    QVERIFY(!rect.color().isTransparent());
    QCOMPARE(rect.color().argb32(), 0x00000000U);

    testing::paintItem(rect, buffer, 0, 0);
    for (int x = 0; x < 4; ++x) {
        const Cell& cell = buffer.cellAt(x, 0);
        QCOMPARE(cell.character().toByteArray(), QByteArray(" ")); // covered
        QCOMPARE(cell.bgColor, 0x00000000U);                       // SGR 49
    }
}

void TestRectangle::testTransparentColorDoesNotPaint() {
    TextGridBuffer buffer;
    QVERIFY(buffer.resize(4, 1));
    for (int x = 0; x < 4; ++x) {
        Cell& cell = buffer.cellAt(x, 0);
        cell.setCharacter("#");
        cell.setBackground(Color("navy"));
    }

    Rectangle rect;
    rect.setWidth(4);
    rect.setHeight(1);
    rect.setColor(Color("transparent"));
    QVERIFY(rect.color().isTransparent());

    testing::paintItem(rect, buffer, 0, 0);
    for (int x = 0; x < 4; ++x)
        QCOMPARE(buffer.cellAt(x, 0).character().toByteArray(), QByteArray("#"));

    // A Rectangle with no colour set behaves the same way.
    Rectangle unset;
    unset.setWidth(4);
    unset.setHeight(1);
    QVERIFY(unset.color().isTransparent());
    testing::paintItem(unset, buffer, 0, 0);
    for (int x = 0; x < 4; ++x)
        QCOMPARE(buffer.cellAt(x, 0).character().toByteArray(), QByteArray("#"));
}

void TestRectangle::testSolidColor() {
    Rectangle rect;
    rect.setWidth(4);
    rect.setHeight(2);
    rect.setColor(Color("#ff0000"));

    TextGridBuffer buffer = paintRect(rect, 4, 2);
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 4; ++x) {
            QCOMPARE(buffer.cellAt(x, y).bgColor, 0xffff0000u);
            QCOMPARE(buffer.cellAt(x, y).character(), QByteArrayView(" "));
        }
    }
}

void TestRectangle::testVerticalGradient() {
    Rectangle rect;
    rect.setWidth(6);
    rect.setHeight(4);

    auto* gradient = new Gradient(&rect);
    gradient->setOrientation(Gradient::Vertical);
    auto* stop1 = new GradientStop(gradient);
    stop1->setPosition(0.0);
    stop1->setColor(Color("#ff0000"));
    auto* stop2 = new GradientStop(gradient);
    stop2->setPosition(1.0);
    stop2->setColor(Color("#0000ff"));

    gradient->addStop(stop1);
    gradient->addStop(stop2);

    rect.setGradient(gradient);

    TextGridBuffer buffer = paintRect(rect, 6, 4);

    // Top row (y = 0) is red, bottom row (y = 3) is blue
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(5, 0).bgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(0, 3).bgColor, 0xff0000ffu);
    QCOMPARE(buffer.cellAt(5, 3).bgColor, 0xff0000ffu);

    // Intermediate rows interpolate between red and blue
    uint32_t topColor = buffer.cellAt(0, 0).bgColor;
    uint32_t midColor = buffer.cellAt(0, 1).bgColor;
    uint32_t nextColor = buffer.cellAt(0, 2).bgColor;

    QVERIFY(topColor != midColor);
    QVERIFY(midColor != nextColor);
    // Red channel decreases going down
    QVERIFY(((topColor >> 16) & 0xff) > ((midColor >> 16) & 0xff));
    // Blue channel increases going down
    QVERIFY((topColor & 0xff) < (midColor & 0xff));
}

void TestRectangle::testHorizontalGradient() {
    Rectangle rect;
    rect.setWidth(6);
    rect.setHeight(3);

    auto* gradient = new Gradient(&rect);
    gradient->setOrientation(Gradient::Horizontal);
    auto* stop1 = new GradientStop(gradient);
    stop1->setPosition(0.0);
    stop1->setColor(Color("#ff0000"));
    auto* stop2 = new GradientStop(gradient);
    stop2->setPosition(1.0);
    stop2->setColor(Color("#00ff00"));

    gradient->addStop(stop1);
    gradient->addStop(stop2);

    rect.setGradient(gradient);

    TextGridBuffer buffer = paintRect(rect, 6, 3);

    // Left column (x = 0) has red, right column (x = 5) has green
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(0, 2).bgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(5, 0).bgColor, 0xff00ff00u);
    QCOMPARE(buffer.cellAt(5, 2).bgColor, 0xff00ff00u);

    // Horizontal progression across columns
    uint32_t col0 = buffer.cellAt(0, 0).bgColor;
    uint32_t col1 = buffer.cellAt(1, 0).bgColor;
    uint32_t col2 = buffer.cellAt(2, 0).bgColor;

    QVERIFY(col0 != col1);
    QVERIFY(col1 != col2);
    // Red decreases to the right
    QVERIFY(((col0 >> 16) & 0xff) > ((col1 >> 16) & 0xff));
    // Green increases to the right
    QVERIFY(((col0 >> 8) & 0xff) < ((col1 >> 8) & 0xff));
}

void TestRectangle::testGradientWithBorder() {
    Rectangle rect;
    rect.setWidth(6);
    rect.setHeight(4);
    rect.border()->setStyle(Border::Single);
    rect.border()->setColor(Color("#ffffff"));

    auto* gradient = new Gradient(&rect);
    gradient->setOrientation(Gradient::Vertical);
    auto* stop1 = new GradientStop(gradient);
    stop1->setPosition(0.0);
    stop1->setColor(Color("#ff0000"));
    auto* stop2 = new GradientStop(gradient);
    stop2->setPosition(1.0);
    stop2->setColor(Color("#0000ff"));

    gradient->addStop(stop1);
    gradient->addStop(stop2);

    rect.setGradient(gradient);

    TextGridBuffer buffer = paintRect(rect, 6, 4);

    // Corner border cell retains background color from the gradient
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("┌"));
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffffffffu);
}

void TestRectangle::testGradientUpdate() {
    Rectangle rect;
    rect.setWidth(4);
    rect.setHeight(2);

    auto* gradient = new Gradient(&rect);
    auto* stop = new GradientStop(gradient);
    stop->setPosition(0.0);
    stop->setColor(Color("#ff0000"));
    gradient->addStop(stop);

    rect.setGradient(gradient);

    QSignalSpy spy(gradient, &Gradient::updated);
    gradient->setOrientation(Gradient::Horizontal);
    QCOMPARE(spy.count(), 1);

    gradient->setOffset(0.5);
    QCOMPARE(spy.count(), 2);
}

void TestRectangle::testBorderStyles() {
    struct StyleCase {
        Border::Style style;
        QByteArrayView tl, tr, bl, br, hz, vt;
    };

    const StyleCase cases[] = {
        {Border::Single, QByteArrayView("┌"), QByteArrayView("┐"), QByteArrayView("└"),
         QByteArrayView("┘"), QByteArrayView("─"), QByteArrayView("│")},
        {Border::Double, QByteArrayView("╔"), QByteArrayView("╗"), QByteArrayView("╚"),
         QByteArrayView("╝"), QByteArrayView("═"), QByteArrayView("║")},
        {Border::Ascii, QByteArrayView("+"), QByteArrayView("+"), QByteArrayView("+"),
         QByteArrayView("+"), QByteArrayView("-"), QByteArrayView("|")},
        {Border::Rounded, QByteArrayView("╭"), QByteArrayView("╮"), QByteArrayView("╰"),
         QByteArrayView("╯"), QByteArrayView("─"), QByteArrayView("│")},
    };

    for (const auto& c : cases) {
        Rectangle rect;
        rect.setWidth(4);
        rect.setHeight(3);
        rect.border()->setStyle(c.style);
        rect.border()->setColor(Color("#ffffff"));
        QCOMPARE(rect.border()->style(), c.style);

        TextGridBuffer buffer = paintRect(rect, 4, 3);

        // Four corners
        QCOMPARE(buffer.cellAt(0, 0).character(), c.tl);
        QCOMPARE(buffer.cellAt(3, 0).character(), c.tr);
        QCOMPARE(buffer.cellAt(0, 2).character(), c.bl);
        QCOMPARE(buffer.cellAt(3, 2).character(), c.br);

        // Horizontal edges
        QCOMPARE(buffer.cellAt(1, 0).character(), c.hz);
        QCOMPARE(buffer.cellAt(2, 0).character(), c.hz);
        QCOMPARE(buffer.cellAt(1, 2).character(), c.hz);
        QCOMPARE(buffer.cellAt(2, 2).character(), c.hz);

        // Vertical edges
        QCOMPARE(buffer.cellAt(0, 1).character(), c.vt);
        QCOMPARE(buffer.cellAt(3, 1).character(), c.vt);

        // Interior remains blank
        QCOMPARE(buffer.cellAt(1, 1).character(), QByteArrayView(" "));
        QCOMPARE(buffer.cellAt(2, 1).character(), QByteArrayView(" "));
    }

    // None style
    {
        Rectangle rect;
        rect.setWidth(4);
        rect.setHeight(3);
        rect.border()->setStyle(Border::None);
        QCOMPARE(rect.border()->style(), Border::None);
        QCOMPARE(rect.leftPadding(), 0);
    }

    // String overload
    {
        Rectangle rect;
        rect.border()->setStyle(QStringLiteral("rounded"));
        QCOMPARE(rect.border()->style(), Border::Rounded);
    }
}

void TestRectangle::testQmlIntegration() {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QmlTui
        Rectangle {
            width: 8
            height: 4
            border.style: Border.Double
            border.color: "#ffffff"
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#ff0000" }
                GradientStop { position: 1.0; color: "#00ff00" }
            }
        }
    )",
                      QUrl());

    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto* rect = qobject_cast<Rectangle*>(object.get());
    QVERIFY(rect);
    QCOMPARE(rect->border()->style(), Border::Double);
    QVERIFY(rect->gradient());
    QCOMPARE(rect->gradient()->orientation(), Gradient::Horizontal);

    TextGridBuffer buffer = paintRect(*rect, 8, 4);
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xffff0000u);
    QVERIFY(buffer.cellAt(1, 0).bgColor != buffer.cellAt(0, 0).bgColor);
}

QTEST_GUILESS_MAIN(TestRectangle)
#include "test_rectangle.moc"
