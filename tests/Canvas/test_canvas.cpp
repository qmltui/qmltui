#include <QTest>
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include "Canvas.h"
#include "Terminal.h"
#include "TextGridBuffer.h"
#include "../TestPaint.h"



using namespace qtui;
using qtui::testing::paintItem;

class TestCanvas : public QObject {
    Q_OBJECT

private slots:
    void testBrailleMode();
    void testHalfBlockMode();
    void testShapesOnHalfBlock();
    void testAutoClear();
    void testQmlCanvasHalfBlock();
    void testHalfBlockDemoRendering();
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
        QCOMPARE(c.character, QString(QChar(dt.expectedChar)).toUtf8());
        QCOMPARE(c.fgColor, 0xFFFF0000);
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
    QCOMPARE(lineCell.character, QString(QChar(0x2824)).toUtf8()); // ⠤
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
    QCOMPARE(c.character, QByteArray("\xE2\x96\x80")); // ▀ Upper Half Block
    QCOMPARE(c.fgColor, 0xFFFF0000); // Red
    QCOMPARE(c.bgColor, 0xFF0000FF); // Blue
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
    QCOMPARE(c00.character, QByteArray("\xE2\x96\x80"));
    QCOMPARE(c00.fgColor, 0xFFFFFF00); // Yellow
    QCOMPARE(c00.bgColor, 0xFFFFFF00); // Yellow

    // Cell (0,1): top=yellow, bottom=yellow
    Cell c01 = buffer.cellAt(0, 1);
    QCOMPARE(c01.character, QByteArray("\xE2\x96\x80"));
    QCOMPARE(c01.fgColor, 0xFFFFFF00);
    QCOMPARE(c01.bgColor, 0xFFFFFF00);
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
    QCOMPARE(c.fgColor, 0xFF008000); // Green
}

void TestCanvas::testQmlCanvasHalfBlock() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/..");

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick
        import Qtui
        import Qtui.Canvas

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
    QCOMPARE(c00.character, QByteArray("\xE2\x96\x80"));
    QCOMPARE(c00.fgColor, 0xFFFF0000);
    QCOMPARE(c00.bgColor, 0xFFFF0000);

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
            if (c.character.startsWith("\xE2\xA0") || c.character == "\xE2\x96\x80" || c.character == "\xE2\x96\x84") {
                canvasCharCount++;
            }
            line += QString::fromUtf8(c.character);
        }
        qDebug().noquote() << QString("Row %1: %2").arg(y, 2).arg(line);
    }

    qDebug() << "Total Canvas graphics characters rendered in frame:" << canvasCharCount;
    QVERIFY(canvasCharCount > 0);

    delete obj;
}

QTEST_MAIN(TestCanvas)
#include "test_canvas.moc"

