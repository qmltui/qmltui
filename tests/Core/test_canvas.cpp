#include <QTest>
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QSignalSpy>
#include <memory>
#include "Canvas.h"
#include "Terminal.h"
#include "TextGridBuffer.h"
#include "../TestPaint.h"
#include "ColorMapper.h"



using namespace qtui;
using qtui::testing::paintItem;

class TestCanvas : public QObject {
    Q_OBJECT

private slots:
    void testBrailleMode();
    void testHalfBlockMode();
    void testHalfBlockDefaultInk();
    void testShapesOnHalfBlock();
    void testAutoClear();
    void testQmlCanvasHalfBlock();
    void testHalfBlockDemoRendering();
    void testShapesDoNotInvalidateWhilePainting();
};



void TestCanvas::testBrailleMode() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::Braille);
    canvas.setWidth(10);
    canvas.setHeight(5);
    canvas.setColor("red");
    canvas.setAutoClear(false);

    QCOMPARE(canvas.pixelWidth(), 20);
    QCOMPARE(canvas.pixelHeight(), 20);

    // Test individual dot encodings for Unicode Braille block (U+2800..U+28FF)
    struct DotTest {
        int x;
        int y;
        int expectedChar; // Unicode code point
    };

    const DotTest dotTests[] = {
        { 0, 0, 0x2801 }, // Dot 1 (Left row 0)  -> bit 0 (0x01)
        { 0, 1, 0x2802 }, // Dot 2 (Left row 1)  -> bit 1 (0x02)
        { 0, 2, 0x2804 }, // Dot 3 (Left row 2)  -> bit 2 (0x04)
        { 1, 0, 0x2808 }, // Dot 4 (Right row 0) -> bit 3 (0x08)
        { 1, 1, 0x2810 }, // Dot 5 (Right row 1) -> bit 4 (0x10)
        { 1, 2, 0x2820 }, // Dot 6 (Right row 2) -> bit 5 (0x20)
        { 0, 3, 0x2840 }, // Dot 7 (Left row 3)  -> bit 6 (0x40)
        { 1, 3, 0x2880 }, // Dot 8 (Right row 3) -> bit 7 (0x80)
    };

    for (const auto& dt : dotTests) {
        canvas.clear();
        canvas.setPixel(dt.x, dt.y, true);

        TextGridBuffer buf;
        buf.resize(10, 5);
        ClipRect clip{0, 0, 10, 5};
        paintItem(canvas, buf, 0, 0, clip);

        Cell c = buf.cellAt(0, 0);
        QCOMPARE(c.character().toByteArray(), QString(QChar(dt.expectedChar)).toUtf8());
        QCOMPARE(ColorMapper::toArgb(c.fgColor), 0xFFFF0000u);
    }

    // Test horizontal line on Row 2 (dot 3 and dot 6) -> U+2824 (⠤)
    canvas.clear();
    canvas.setPixel(0, 2, true);
    canvas.setPixel(1, 2, true);

    TextGridBuffer lineBuf;
    lineBuf.resize(10, 5);
    ClipRect clip{0, 0, 10, 5};
    paintItem(canvas, lineBuf, 0, 0, clip);

    Cell lineCell = lineBuf.cellAt(0, 0);
    QCOMPARE(lineCell.character().toByteArray(), QString(QChar(0x2824)).toUtf8()); // ⠤
}

void TestCanvas::testHalfBlockMode() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::HalfBlock);
    canvas.setWidth(10);
    canvas.setHeight(5);
    canvas.setAutoClear(false);

    QCOMPARE(canvas.pixelWidth(), 10);
    QCOMPARE(canvas.pixelHeight(), 10);

    // Top pixel red, bottom pixel blue in cell (0, 0)
    canvas.setPixelColor(0, 0, "red");    // top pixel y=0 -> cell (0,0) top
    canvas.setPixelColor(0, 1, "blue");   // bottom pixel y=1 -> cell (0,0) bottom

    TextGridBuffer buffer;
    buffer.resize(10, 5);
    ClipRect clip{0, 0, 10, 5};
    paintItem(canvas, buffer, 0, 0, clip);

    Cell c = buffer.cellAt(0, 0);
    QCOMPARE(c.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀ Upper Half Block
    QCOMPARE(ColorMapper::toArgb(c.fgColor), 0xFFFF0000u); // Red
    QCOMPARE(ColorMapper::toArgb(c.bgColor), 0xFF0000FFu); // Blue
}

void TestCanvas::testHalfBlockDefaultInk() {
    Canvas canvas;
    canvas.setWidth(3); canvas.setHeight(1);
    canvas.setRenderMode(Canvas::RenderMode::HalfBlock);
    canvas.setAutoClear(false);
    canvas.setPixel(0, 0, true);
    canvas.setPixel(1, 1, true);
    canvas.setPixel(2, 0, true); canvas.setPixel(2, 1, true);
    TextGridBuffer buffer;
    buffer.resize(3, 1);
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QStringLiteral("▀").toUtf8());
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QStringLiteral("▄").toUtf8());
    QCOMPARE(buffer.cellAt(2, 0).character().toByteArray(), QStringLiteral("█").toUtf8());
    for (int x = 0; x < 3; ++x) QCOMPARE(buffer.cellAt(x, 0).fgColor, 0u);

    canvas.setPixelColor(1, 0, "red");
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QStringLiteral("▄").toUtf8());
    QCOMPARE(buffer.cellAt(1, 0).fgColor, 0u);
    QCOMPARE(buffer.cellAt(1, 0).bgColor, Color::fromString("red").argb32());

    canvas.setPixel(0, 0, false);
    buffer.clear();
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray(" "));
    canvas.clear();
    buffer.clear();
    paintItem(canvas, buffer, 0, 0);
    for (int x = 0; x < 3; ++x)
        QCOMPARE(buffer.cellAt(x, 0).character().toByteArray(), QByteArray(" "));

    // A transparent fill still clears to holes; only explicitly drawn ink
    // uses the terminal default foreground.
    canvas.setFillColor("transparent");
    canvas.setPixel(0, 0, true);
    canvas.setPixel(0, 0, false);
    buffer.clear();
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray(" "));
    canvas.clear();
    buffer.clear();
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray(" "));

    canvas.setAutoClear(true);
    Point point(&canvas);
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QStringLiteral("▀").toUtf8());
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0u);
}

void TestCanvas::testShapesOnHalfBlock() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::HalfBlock);
    canvas.setWidth(20);
    canvas.setHeight(10);

    // Add a filled yellow CanvasRectangle
    CanvasRectangle* rect = new CanvasRectangle(&canvas);
    rect->setX(0);
    rect->setY(0);
    rect->setWidth(4);
    rect->setHeight(4); // 4 half-block pixels high = 2 character cells high
    rect->setFill(true);
    rect->setColor("yellow");


    TextGridBuffer buffer;
    buffer.resize(20, 10);
    ClipRect clip{0, 0, 20, 10};
    paintItem(canvas, buffer, 0, 0, clip);

    // Cell (0,0): top=yellow, bottom=yellow
    Cell c00 = buffer.cellAt(0, 0);
    QCOMPARE(c00.character().toByteArray(), QByteArray("\xE2\x96\x80"));
    QCOMPARE(ColorMapper::toArgb(c00.fgColor), 0xFFFFFF00u); // Yellow
    QCOMPARE(ColorMapper::toArgb(c00.bgColor), 0xFFFFFF00u); // Yellow

    // Cell (0,1): top=yellow, bottom=yellow
    Cell c01 = buffer.cellAt(0, 1);
    QCOMPARE(c01.character().toByteArray(), QByteArray("\xE2\x96\x80"));
    QCOMPARE(ColorMapper::toArgb(c01.fgColor), 0xFFFFFF00u);
    QCOMPARE(ColorMapper::toArgb(c01.bgColor), 0xFFFFFF00u);
}

void TestCanvas::testAutoClear() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::HalfBlock);
    canvas.setWidth(10);
    canvas.setHeight(5);
    canvas.setAutoClear(false);

    canvas.setPixelColor(2, 2, "green");

    TextGridBuffer buffer;
    buffer.resize(10, 5);
    ClipRect clip{0, 0, 10, 5};
    paintItem(canvas, buffer, 0, 0, clip);

    // Cell (2, 1) has top pixel y=2 green
    Cell c = buffer.cellAt(2, 1);
    QCOMPARE(ColorMapper::toArgb(c.fgColor), 0xFF008000u); // Green
}

void TestCanvas::testQmlCanvasHalfBlock() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/..");

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick
        import Qtui

        Canvas {
            width: 20
            height: 10
            renderMode: Canvas.HalfBlock

            CanvasRectangle {
                x: 0; y: 0; width: 4; height: 4
                fill: true
                color: "red"
            }
        }
    )", QUrl());

    QObject* obj = component.create();
    QVERIFY2(obj != nullptr, qPrintable(component.errorString()));

    Canvas* canvas = qobject_cast<Canvas*>(obj);
    QVERIFY(canvas != nullptr);

    TextGridBuffer buffer;
    buffer.resize(20, 10);
    ClipRect clip{0, 0, 20, 10};
    paintItem(canvas, buffer, 0, 0, clip);

    Cell c00 = buffer.cellAt(0, 0);
    QCOMPARE(c00.character().toByteArray(), QByteArray("\xE2\x96\x80"));
    QCOMPARE(ColorMapper::toArgb(c00.fgColor), 0xFFFF0000u);
    QCOMPARE(ColorMapper::toArgb(c00.bgColor), 0xFFFF0000u);

    delete obj;
}

void TestCanvas::testHalfBlockDemoRendering() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../..");

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(QTUI_SOURCE_DIR "/examples/components/canvas/Main.qml")));

    QObject* obj = component.create();
    QVERIFY2(obj != nullptr, qPrintable(component.errorString()));

    Terminal* term = qobject_cast<Terminal*>(obj);
    QVERIFY(term != nullptr);
    term->setWidth(80);
    term->setHeight(30);

    TextGridBuffer buffer;
    buffer.resize(80, 30);
    paintItem(term, buffer, 0, 0);

    // Count non-space cells or Upper Half Block characters rendered in the buffer
    int canvasCharCount = 0;
    for (int y = 0; y < 30; ++y) {
        QString line;
        for (int x = 0; x < 80; ++x) {
            Cell c = buffer.cellAt(x, y);
            if (c.character().startsWith("\xE2\xA0") || c.character().toByteArray() == "\xE2\x96\x80" || c.character().toByteArray() == "\xE2\x96\x84") {
                canvasCharCount++;
            }
            line += QString::fromUtf8(c.character().toByteArray());
        }
        qDebug().noquote() << QString("Row %1: %2").arg(y, 2).arg(line);
    }

    qDebug() << "Total Canvas graphics characters rendered in frame:" << canvasCharCount;
    QVERIFY(canvasCharCount > 0);

    delete obj;
}

void TestCanvas::testShapesDoNotInvalidateWhilePainting() {
    // Drawing shapes writes the frame being rendered; it must not leave the
    // canvas dirty or ask the engine for another frame.
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/..");

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui

        Canvas {
            width: 20
            height: 10
            renderMode: Canvas.Braille
            color: "cyan"
            Line { x1: 0; y1: 0; x2: 30; y2: 30 }
            Circle { cx: 20; cy: 20; radius: 8 }
        }
    )", QUrl());

    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object != nullptr, qPrintable(component.errorString()));
    Canvas* canvas = qobject_cast<Canvas*>(object.get());
    QVERIFY(canvas != nullptr);

    TextGridBuffer buffer;
    buffer.resize(20, 10);
    paintItem(canvas, buffer, 0, 0);

    QSignalSpy dirtySpy(canvas, &Item::dirty);
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(dirtySpy.count(), 0);
    QVERIFY(!canvas->dirtyFlags().testFlag(Item::DirtyFlag::SelfDirty));
    QCOMPARE(RenderPhase::dirtyDuringPaintCount(), 0);
    QVERIFY(buffer.cellAt(0, 0).character().startsWith("\xE2\xA0"));

    // Pixel writes outside a frame still invalidate.
    canvas->setPixel(1, 1, true);
    QCOMPARE(dirtySpy.count(), 1);
}

QTEST_MAIN(TestCanvas)
#include "test_canvas.moc"

