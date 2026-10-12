#include <QTest>
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QSignalSpy>
#include <memory>
#include <cmath>
#include <Qamui/Core/items/Canvas.h>
#include <Qamui/Core/items/Terminal.h>
#include <Qamui/Core/primitives/TextGridBuffer.h>
#include "../TestPaint.h"
#include <Qamui/Core/primitives/ColorMapper.h>



using namespace qamui;
using qamui::testing::paintItem;

class TestCanvas : public QObject {
    Q_OBJECT

private slots:
    void testBrailleMode();
    void testBraillePerCellColor();
    void testBrailleAverageCellColor();
    void testHalfBlockMode();
    void testHalfBlockDefaultInk();
    void testShapesOnHalfBlock();
    void testAutoClear();
    void testQmlCanvasHalfBlock();
    void testHalfBlockDemoRendering();
    void testShapesDoNotInvalidateWhilePainting();
    void testRequestPaint();
    void testCircleGeometry_data();
    void testCircleGeometry();
    void testZeroMaskPreservesUnderlyingCell();
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

void TestCanvas::testBraillePerCellColor() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::Braille);
    canvas.setWidth(4);
    canvas.setHeight(1);
    canvas.setColor("blue");
    canvas.setAutoClear(false);

    TextGridBuffer buf;
    buf.resize(4, 1);
    ClipRect clip{0, 0, 4, 1};

    // First paint allocates the pixel storage.
    paintItem(canvas, buf, 0, 0, clip);
    canvas.clear();

    canvas.setPixelColor(0, 0, "red");               // cell 0: explicit ink
    canvas.setPixel(2, 0, true);                      // cell 1: canvas color
    canvas.setPixelColor(4, 0, "red");               // cell 2: red, then...
    canvas.setPixelColor(5, 1, "lime");              // ...the last dot drawn wins
    canvas.setPixelColor(6, 0, "red");               // cell 3: explicit, then a plain dot
    canvas.setPixel(7, 1, true);                      // resets the cell to the canvas color

    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(0, 0).fgColor), 0xFFFF0000u);   // red
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(1, 0).fgColor), 0xFF0000FFu);   // blue (canvas)
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(2, 0).fgColor), 0xFF00FF00u);   // lime
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(3, 0).fgColor), 0xFF0000FFu);   // blue (canvas)
    // Both dots of cell 2 are still set; only the ink changed.
    QCOMPARE(buf.cellAt(2, 0).character().toByteArray(), QString(QChar(0x2801 | 0x2810)).toUtf8());

    // Cells drawn with setPixel() keep following the canvas color; explicit ink does not.
    canvas.setColor("yellow");
    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(1, 0).fgColor), 0xFFFFFF00u);
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(0, 0).fgColor), 0xFFFF0000u);

    // clear() forgets the ink as well as the dots.
    canvas.clear();
    canvas.setPixel(0, 0, true);
    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(0, 0).fgColor), 0xFFFFFF00u);
}

void TestCanvas::testBrailleAverageCellColor() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::Braille);
    canvas.setCellColorMode(Canvas::CellColorMode::Average);
    canvas.setWidth(3);
    canvas.setHeight(1);
    canvas.setColor("blue");
    canvas.setAutoClear(false);

    TextGridBuffer buf;
    buf.resize(3, 1);
    ClipRect clip{0, 0, 3, 1};
    paintItem(canvas, buf, 0, 0, clip);
    canvas.clear();

    canvas.setPixelColor(0, 0, "red");                // cell 0: red + lime -> olive-ish mean
    canvas.setPixelColor(1, 0, "lime");
    canvas.setPixelColor(2, 0, "red");                // cell 1: red only
    canvas.setPixel(4, 0, true);                       // cell 2: canvas blue + red -> purple
    canvas.setPixelColor(5, 0, "red");

    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(0, 0).fgColor), 0xFF7F7F00u);
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(1, 0).fgColor), 0xFFFF0000u);
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(2, 0).fgColor), 0xFF7F007Fu);

    // Switching back to LastDot drops the accumulators and restores last-write-wins.
    canvas.setCellColorMode(Canvas::CellColorMode::LastDot);
    canvas.clear();
    canvas.setPixelColor(0, 0, "red");
    canvas.setPixelColor(1, 0, "lime");
    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(0, 0).fgColor), 0xFF00FF00u);

    // Dots in the terminal default color do not pull the average.
    canvas.setCellColorMode(Canvas::CellColorMode::Average);
    canvas.setColor("default");
    canvas.clear();
    canvas.setPixel(0, 0, true);
    canvas.setPixelColor(1, 0, "red");
    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(ColorMapper::toArgb(buf.cellAt(0, 0).fgColor), 0xFFFF0000u);
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
        import Qamui

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

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(QAMUI_SOURCE_DIR "/examples/components/canvas/Main.qml")));

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
        import Qamui

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

void TestCanvas::testCircleGeometry_data() {
    QTest::addColumn<int>("radius");
    QTest::addColumn<bool>("filled");
    for (int radius : {1, 2, 7, 14, 25}) {
        for (bool filled : {false, true}) {
            QTest::newRow(qPrintable(QStringLiteral("r%1-%2").arg(radius)
                .arg(filled ? "filled" : "outline"))) << radius << filled;
        }
    }
}

void TestCanvas::testCircleGeometry() {
    QFETCH(int, radius);
    QFETCH(bool, filled);
    const int center = radius + 2;
    const int diameter = 2 * center + 1;
    Canvas canvas;
    canvas.setWidth((diameter + 1) / 2);
    canvas.setHeight((diameter + 3) / 4);
    Circle circle(&canvas);
    circle.setCx(center);
    circle.setCy(center);
    circle.setRadius(radius);
    circle.setFill(filled);
    TextGridBuffer buffer;
    buffer.resize(canvas.width(), canvas.height());
    paintItem(canvas, buffer, 0, 0);

    // Inspect rendered braille dots, with a geometric oracle independent of
    // the rasterizer's stepping algorithm.
    const auto inkAt = [&](int dx, int dy) {
        const int x = center + dx;
        const int y = center + dy;
        const auto text = QString::fromUtf8(buffer.cellAt(x / 2, y / 4).character().toByteArray());
        const ushort ch = text.isEmpty() ? 0 : text.at(0).unicode();
        constexpr int bits[2][4] = {{0, 1, 2, 6}, {3, 4, 5, 7}};
        return ch >= 0x2800 && ch <= 0x28ff
            && ((ch - 0x2800) & (1 << bits[x % 2][y % 4])) != 0;
    };
    for (int y = -radius - 1; y <= radius + 1; ++y) {
        int rowDots = 0;
        for (int x = -radius - 1; x <= radius + 1; ++x) {
            const bool ink = inkAt(x, y);
            const double distance = std::hypot(x, y);
            QCOMPARE(ink, inkAt(-x, y));
            QCOMPARE(ink, inkAt(x, -y));
            QCOMPARE(ink, inkAt(y, x));
            if (distance < radius - 0.5)
                QCOMPARE(ink, filled);
            if (distance > radius + 0.5)
                QVERIFY(!ink);
            rowDots += ink;
        }
        if (std::abs(y) <= radius)
            QVERIFY(rowDots > 0);
    }
}

void TestCanvas::testRequestPaint() {
    Canvas canvas;
    QSignalSpy dirtySpy(&canvas, &Item::dirty);
    canvas.requestPaint();
    QCOMPARE(dirtySpy.count(), 1);
    QVERIFY(canvas.dirtyFlags().testFlag(Item::DirtyFlag::SelfDirty));
}

void TestCanvas::testZeroMaskPreservesUnderlyingCell() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::Braille);
    canvas.setWidth(2);
    canvas.setHeight(2);
    canvas.setAutoClear(false);
    // No dots drawn into canvas

    TextGridBuffer buffer;
    buffer.resize(2, 2);
    Cell& cell = buffer.cellAt(0, 0);
    cell.setCharacter("W");
    cell.fgColor = 0xFF556677;
    cell.bgColor = 0xFF8899AA;

    paintItem(canvas, buffer, 0, 0);

    QCOMPARE(cell.character().toByteArray(), QByteArray("W"));
    QCOMPARE(cell.fgColor, 0xFF556677u);
    QCOMPARE(cell.bgColor, 0xFF8899AAu);
}

QTEST_MAIN(TestCanvas)
#include "test_canvas.moc"
