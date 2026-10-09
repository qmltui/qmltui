#include <QTest>
#include <QSignalSpy>
#include <QQmlEngine>
#include <QQmlComponent>
#include <Qtui/Core/items/Rectangle.h>
#include <Qtui/Core/items/Gradient.h>
#include <Qtui/Core/items/GradientStop.h>
#include "../TestPaint.h"
#include <memory>

using namespace qtui;

static TextGridBuffer paintRect(Rectangle &rect, int width = 10, int height = 5)
{
    TextGridBuffer buffer;
    buffer.resize(width, height);
    testing::paintItem(rect, buffer, 0, 0);
    return buffer;
}

class TestRectangle : public QObject {
    Q_OBJECT
private slots:
    void testSolidColor();
    void testVerticalGradient();
    void testHorizontalGradient();
    void testGradientWithBorder();
    void testGradientUpdate();
    void testQmlIntegration();
};

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

    auto *gradient = new Gradient(&rect);
    gradient->setOrientation(Gradient::Vertical);
    auto *stop1 = new GradientStop(gradient);
    stop1->setPosition(0.0);
    stop1->setColor(Color("#ff0000"));
    auto *stop2 = new GradientStop(gradient);
    stop2->setPosition(1.0);
    stop2->setColor(Color("#0000ff"));

    gradient->addStop(stop1);
    gradient->addStop(stop2);

    rect.setGradient(gradient);

    TextGridBuffer buffer = paintRect(rect, 6, 4);

    // Top row (y = 0) is red
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(5, 0).bgColor, 0xffff0000u);

    // Bottom row has distinct blue component from gradient
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

    auto *gradient = new Gradient(&rect);
    gradient->setOrientation(Gradient::Horizontal);
    auto *stop1 = new GradientStop(gradient);
    stop1->setPosition(0.0);
    stop1->setColor(Color("#ff0000"));
    auto *stop2 = new GradientStop(gradient);
    stop2->setPosition(1.0);
    stop2->setColor(Color("#00ff00"));

    gradient->addStop(stop1);
    gradient->addStop(stop2);

    rect.setGradient(gradient);

    TextGridBuffer buffer = paintRect(rect, 6, 3);

    // Left column (x = 0) has red
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(0, 2).bgColor, 0xffff0000u);

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
    rect.border()->setStyle("single");
    rect.border()->setColor("#ffffff");

    auto *gradient = new Gradient(&rect);
    gradient->setOrientation(Gradient::Vertical);
    auto *stop1 = new GradientStop(gradient);
    stop1->setPosition(0.0);
    stop1->setColor(Color("#ff0000"));
    auto *stop2 = new GradientStop(gradient);
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

    auto *gradient = new Gradient(&rect);
    auto *stop = new GradientStop(gradient);
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

void TestRectangle::testQmlIntegration() {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import Qtui
        Rectangle {
            width: 8
            height: 4
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#ff0000" }
                GradientStop { position: 1.0; color: "#00ff00" }
            }
        }
    )", QUrl());

    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *rect = qobject_cast<Rectangle *>(object.get());
    QVERIFY(rect);
    QVERIFY(rect->gradient());
    QCOMPARE(rect->gradient()->orientation(), Gradient::Horizontal);

    TextGridBuffer buffer = paintRect(*rect, 8, 4);
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xffff0000u);
    QVERIFY(buffer.cellAt(1, 0).bgColor != buffer.cellAt(0, 0).bgColor);
}

QTEST_GUILESS_MAIN(TestRectangle)
#include "test_rectangle.moc"
