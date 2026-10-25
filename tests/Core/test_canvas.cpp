#include <QTest>
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QSignalSpy>
#include <memory>
#include <cmath>
#include <QmlTui/Core/Items/Canvas.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include "../TestPaint.h"
#include <QmlTui/Core/Primitives/Color.h>

using namespace qmltui;
using qmltui::testing::paintItem;

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
    void testHalfBlockGradient();
    void testBrailleGradient();
    void testCanvasAndShapeAttributes();
};

void TestCanvas::testBrailleMode() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::Braille);
    canvas.setWidth(10);
    canvas.setHeight(5);
    canvas.setColor(Color("red"));
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
        {0, 0, 0x2801}, // Dot 1 (Left row 0)  -> bit 0 (0x01)
        {0, 1, 0x2802}, // Dot 2 (Left row 1)  -> bit 1 (0x02)
        {0, 2, 0x2804}, // Dot 3 (Left row 2)  -> bit 2 (0x04)
        {1, 0, 0x2808}, // Dot 4 (Right row 0) -> bit 3 (0x08)
        {1, 1, 0x2810}, // Dot 5 (Right row 1) -> bit 4 (0x10)
        {1, 2, 0x2820}, // Dot 6 (Right row 2) -> bit 5 (0x20)
        {0, 3, 0x2840}, // Dot 7 (Left row 3)  -> bit 6 (0x40)
        {1, 3, 0x2880}, // Dot 8 (Right row 3) -> bit 7 (0x80)
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
        QCOMPARE(Color::toArgb(c.fgColor), 0xFFFF0000u);
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
    canvas.setColor(Color("blue"));
    canvas.setAutoClear(false);

    TextGridBuffer buf;
    buf.resize(4, 1);
    ClipRect clip{0, 0, 4, 1};

    // First paint allocates the pixel storage.
    paintItem(canvas, buf, 0, 0, clip);
    canvas.clear();

    canvas.setPixelColor(0, 0, Color("red"));  // cell 0: explicit ink
    canvas.setPixel(2, 0, true);               // cell 1: canvas color
    canvas.setPixelColor(4, 0, Color("red"));  // cell 2: red, then...
    canvas.setPixelColor(5, 1, Color("lime")); // ...the last dot drawn wins
    canvas.setPixelColor(6, 0, Color("red"));  // cell 3: explicit, then a plain dot
    canvas.setPixel(7, 1, true);               // resets the cell to the canvas color

    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(Color::toArgb(buf.cellAt(0, 0).fgColor), 0xFFFF0000u); // red
    QCOMPARE(Color::toArgb(buf.cellAt(1, 0).fgColor), 0xFF0000FFu); // blue (canvas)
    QCOMPARE(Color::toArgb(buf.cellAt(2, 0).fgColor), 0xFF00FF00u); // lime
    QCOMPARE(Color::toArgb(buf.cellAt(3, 0).fgColor), 0xFF0000FFu); // blue (canvas)
    // Both dots of cell 2 are still set; only the ink changed.
    QCOMPARE(buf.cellAt(2, 0).character().toByteArray(), QString(QChar(0x2801 | 0x2810)).toUtf8());

    // Cells drawn with setPixel() keep following the canvas color; explicit ink does not.
    canvas.setColor(Color("yellow"));
    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(Color::toArgb(buf.cellAt(1, 0).fgColor), 0xFFFFFF00u);
    QCOMPARE(Color::toArgb(buf.cellAt(0, 0).fgColor), 0xFFFF0000u);

    // clear() forgets the ink as well as the dots.
    canvas.clear();
    canvas.setPixel(0, 0, true);
    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(Color::toArgb(buf.cellAt(0, 0).fgColor), 0xFFFFFF00u);
}

void TestCanvas::testBrailleAverageCellColor() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::Braille);
    canvas.setCellColorMode(Canvas::CellColorMode::Average);
    canvas.setWidth(3);
    canvas.setHeight(1);
    canvas.setColor(Color("blue"));
    canvas.setAutoClear(false);

    TextGridBuffer buf;
    buf.resize(3, 1);
    ClipRect clip{0, 0, 3, 1};
    paintItem(canvas, buf, 0, 0, clip);
    canvas.clear();

    canvas.setPixelColor(0, 0, Color("red")); // cell 0: red + lime -> olive-ish mean
    canvas.setPixelColor(1, 0, Color("lime"));
    canvas.setPixelColor(2, 0, Color("red")); // cell 1: red only
    canvas.setPixel(4, 0, true);              // cell 2: canvas blue + red -> purple
    canvas.setPixelColor(5, 0, Color("red"));

    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(Color::toArgb(buf.cellAt(0, 0).fgColor), 0xFF7F7F00u);
    QCOMPARE(Color::toArgb(buf.cellAt(1, 0).fgColor), 0xFFFF0000u);
    QCOMPARE(Color::toArgb(buf.cellAt(2, 0).fgColor), 0xFF7F007Fu);

    // Switching back to LastDot drops the accumulators and restores last-write-wins.
    canvas.setCellColorMode(Canvas::CellColorMode::LastDot);
    canvas.clear();
    canvas.setPixelColor(0, 0, Color("red"));
    canvas.setPixelColor(1, 0, Color("lime"));
    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(Color::toArgb(buf.cellAt(0, 0).fgColor), 0xFF00FF00u);

    // Dots in the terminal default color do not pull the average.
    canvas.setCellColorMode(Canvas::CellColorMode::Average);
    canvas.setColor(Color::Default);
    canvas.clear();
    canvas.setPixel(0, 0, true);
    canvas.setPixelColor(1, 0, Color("red"));
    paintItem(canvas, buf, 0, 0, clip);
    QCOMPARE(Color::toArgb(buf.cellAt(0, 0).fgColor), 0xFFFF0000u);
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
    canvas.setPixelColor(0, 0, Color("red"));  // top pixel y=0 -> cell (0,0) top
    canvas.setPixelColor(0, 1, Color("blue")); // bottom pixel y=1 -> cell (0,0) bottom

    TextGridBuffer buffer;
    buffer.resize(10, 5);
    ClipRect clip{0, 0, 10, 5};
    paintItem(canvas, buffer, 0, 0, clip);

    Cell c = buffer.cellAt(0, 0);
    QCOMPARE(c.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀ Upper Half Block
    QCOMPARE(Color::toArgb(c.fgColor), 0xFFFF0000u);                   // Red
    QCOMPARE(Color::toArgb(c.bgColor), 0xFF0000FFu);                   // Blue
}

void TestCanvas::testHalfBlockDefaultInk() {
    Canvas canvas;
    canvas.setWidth(3);
    canvas.setHeight(1);
    canvas.setRenderMode(Canvas::RenderMode::HalfBlock);
    canvas.setAutoClear(false);
    canvas.setPixel(0, 0, true);
    canvas.setPixel(1, 1, true);
    canvas.setPixel(2, 0, true);
    canvas.setPixel(2, 1, true);
    TextGridBuffer buffer;
    buffer.resize(3, 1);
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QStringLiteral("▀").toUtf8());
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QStringLiteral("▄").toUtf8());
    QCOMPARE(buffer.cellAt(2, 0).character().toByteArray(), QStringLiteral("█").toUtf8());
    for (int x = 0; x < 3; ++x)
        QCOMPARE(buffer.cellAt(x, 0).fgColor, 0u);

    canvas.setPixelColor(1, 0, Color("red"));
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
    canvas.setFillColor(Color::Transparent);
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
    rect->setColor(Color("yellow"));

    TextGridBuffer buffer;
    buffer.resize(20, 10);
    ClipRect clip{0, 0, 20, 10};
    paintItem(canvas, buffer, 0, 0, clip);

    // Cell (0,0): top=yellow, bottom=yellow
    Cell c00 = buffer.cellAt(0, 0);
    QCOMPARE(c00.character().toByteArray(), QByteArray("\xE2\x96\x80"));
    QCOMPARE(Color::toArgb(c00.fgColor), 0xFFFFFF00u); // Yellow
    QCOMPARE(Color::toArgb(c00.bgColor), 0xFFFFFF00u); // Yellow

    // Cell (0,1): top=yellow, bottom=yellow
    Cell c01 = buffer.cellAt(0, 1);
    QCOMPARE(c01.character().toByteArray(), QByteArray("\xE2\x96\x80"));
    QCOMPARE(Color::toArgb(c01.fgColor), 0xFFFFFF00u);
    QCOMPARE(Color::toArgb(c01.bgColor), 0xFFFFFF00u);
}

void TestCanvas::testAutoClear() {
    Canvas canvas;
    canvas.setRenderMode(Canvas::RenderMode::HalfBlock);
    canvas.setWidth(10);
    canvas.setHeight(5);
    canvas.setAutoClear(false);

    canvas.setPixelColor(2, 2, Color("green"));

    TextGridBuffer buffer;
    buffer.resize(10, 5);
    ClipRect clip{0, 0, 10, 5};
    paintItem(canvas, buffer, 0, 0, clip);

    // Frame 1: cell (2, 1) has top pixel y=2 green
    QCOMPARE(Color::toArgb(buffer.cellAt(2, 1).fgColor), 0xFF008000u);

    // Frame 2 with autoClear=false: pixel persists across frames without redrawing
    buffer.clear();
    paintItem(canvas, buffer, 0, 0, clip);
    QCOMPARE(Color::toArgb(buffer.cellAt(2, 1).fgColor), 0xFF008000u);

    // Frame 3 with autoClear=true: canvas clears its pixel buffer before painting
    canvas.setAutoClear(true);
    buffer.clear();
    paintItem(canvas, buffer, 0, 0, clip);
    QCOMPARE(buffer.cellAt(2, 1).character().toByteArray(), QByteArray(" "));
    QCOMPARE(buffer.cellAt(2, 1).fgColor, 0u);
}

void TestCanvas::testQmlCanvasHalfBlock() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/..");

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui

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
    )",
                      QUrl());

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
    QCOMPARE(Color::toArgb(c00.fgColor), 0xFFFF0000u);
    QCOMPARE(Color::toArgb(c00.bgColor), 0xFFFF0000u);

    delete obj;
}

void TestCanvas::testHalfBlockDemoRendering() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/../..");

    QQmlComponent component(
        &engine, QUrl::fromLocalFile(
                     QStringLiteral(QMLTUI_SOURCE_DIR "/examples/gallery/examples/Canvas.qml")));

    QObject* obj = component.create();
    QVERIFY2(obj != nullptr, qPrintable(component.errorString()));

    Item* item = qobject_cast<Item*>(obj);
    QVERIFY(item != nullptr);
    item->setWidth(80);
    item->setHeight(30);

    TextGridBuffer buffer;
    buffer.resize(80, 30);
    paintItem(item, buffer, 0, 0);

    int canvasCharCount = 0;
    for (int y = 0; y < 30; ++y) {
        for (int x = 0; x < 80; ++x) {
            Cell c = buffer.cellAt(x, y);
            if (c.character().startsWith("\xE2\xA0") ||
                c.character().toByteArray() == "\xE2\x96\x80" ||
                c.character().toByteArray() == "\xE2\x96\x84") {
                canvasCharCount++;
            }
        }
    }
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
        import QmlTui

        Canvas {
            width: 20
            height: 10
            renderMode: Canvas.Braille
            color: "cyan"
            Line { x1: 0; y1: 0; x2: 30; y2: 30 }
            Circle { cx: 20; cy: 20; radius: 8 }
        }
    )",
                      QUrl());

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
            QTest::newRow(
                qPrintable(QStringLiteral("r%1-%2").arg(radius).arg(filled ? "filled" : "outline")))
                << radius << filled;
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
        return ch >= 0x2800 && ch <= 0x28ff && ((ch - 0x2800) & (1 << bits[x % 2][y % 4])) != 0;
    };
    for (int y = -radius - 1; y <= radius + 1; ++y) {
        int rowDots = 0;
        for (int x = -radius - 1; x <= radius + 1; ++x) {
            const bool ink = inkAt(x, y);
            const double distance = std::hypot(x, y);
            QCOMPARE(ink, inkAt(-x, y));
            QCOMPARE(ink, inkAt(x, -y));
            QCOMPARE(ink, inkAt(y, x));
            if (distance < radius - 0.5) QCOMPARE(ink, filled);
            if (distance > radius + 0.5) QVERIFY(!ink);
            rowDots += ink;
        }
        if (std::abs(y) <= radius) QVERIFY(rowDots > 0);
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

void TestCanvas::testHalfBlockGradient() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/..");

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui

        Canvas {
            width: 3
            height: 2
            renderMode: Canvas.HalfBlock
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { id: topStop; objectName: "topStop"; position: 0.0; color: "#ff0000" }
                GradientStop { position: 1.0; color: "#0000ff" }
            }

            // Column 0 uses the parent Canvas vertical gradient across 4 subpixels (y=0..3).
            Line {
                x1: 0; y1: 0; x2: 0; y2: 3
            }

            // Column 1-2 filled rectangle has its own horizontal shape gradient.
            CanvasRectangle {
                x: 1; y: 0; width: 2; height: 2
                fill: true
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#00ff00" }
                    GradientStop { position: 1.0; color: "#ffff00" }
                }
            }
        }
    )",
                      QUrl());

    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object != nullptr, qPrintable(component.errorString()));
    Canvas* canvas = qobject_cast<Canvas*>(object.get());
    QVERIFY(canvas != nullptr);

    TextGridBuffer buffer;
    buffer.resize(3, 2);
    paintItem(canvas, buffer, 0, 0);

    // Column 0: vertical gradient across y=0..3 (red -> blue, 2 subpixels per cell).
    const Cell c00 = buffer.cellAt(0, 0);
    const Cell c01 = buffer.cellAt(0, 1);
    QCOMPARE(c00.character().toByteArray(), QStringLiteral("▀").toUtf8());
    QCOMPARE(c01.character().toByteArray(), QStringLiteral("▀").toUtf8());
    QCOMPARE(Color::toArgb(c00.fgColor), 0xFFFF0000u); // y=0 (t=0.0)
    QCOMPARE(Color::toArgb(c00.bgColor), 0xFFAA0055u); // y=1 (t=1/3)
    QCOMPARE(Color::toArgb(c01.fgColor), 0xFF5500AAu); // y=2 (t=2/3)
    QCOMPARE(Color::toArgb(c01.bgColor), 0xFF0000FFu); // y=3 (t=1.0)

    // Columns 1..2, Row 0: shape-local horizontal gradient (#00ff00 -> #ffff00).
    QCOMPARE(Color::toArgb(buffer.cellAt(1, 0).fgColor), 0xFF00FF00u);
    QCOMPARE(Color::toArgb(buffer.cellAt(1, 0).bgColor), 0xFF00FF00u);
    QCOMPARE(Color::toArgb(buffer.cellAt(2, 0).fgColor), 0xFFFFFF00u);
    QCOMPARE(Color::toArgb(buffer.cellAt(2, 0).bgColor), 0xFFFFFF00u);

    // Updating a stop marks the canvas dirty without invalidating during paint.
    QSignalSpy dirtySpy(canvas, &Item::dirty);
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(dirtySpy.count(), 0);
    QCOMPARE(RenderPhase::dirtyDuringPaintCount(), 0);

    auto* topStop = canvas->findChild<GradientStop*>(QStringLiteral("topStop"));
    QVERIFY(topStop != nullptr);
    topStop->setColor(Color("#00ffff"));
    QCOMPARE(dirtySpy.count(), 1);
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(Color::toArgb(buffer.cellAt(0, 0).fgColor), 0xFF00FFFFu);
}

void TestCanvas::testBrailleGradient() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/..");

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui

        Canvas {
            width: 3
            height: 1
            renderMode: Canvas.Braille
            cellColorMode: Canvas.Average
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#ff0000" }
                GradientStop { position: 1.0; color: "#0000ff" }
            }

            // Polyline spanning x=0..5 (3 Braille cells, 2 dots per cell).
            Polygon {
                id: wave
                objectName: "wave"
                closed: false
                points: [{ x: 0, y: 0 }, { x: 5, y: 0 }]
            }
        }
    )",
                      QUrl());

    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object != nullptr, qPrintable(component.errorString()));
    Canvas* canvas = qobject_cast<Canvas*>(object.get());
    QVERIFY(canvas != nullptr);

    TextGridBuffer buffer;
    buffer.resize(3, 1);
    paintItem(canvas, buffer, 0, 0);

    // Each cell averages 2 horizontal subpixels:
    // Cell 0 (x=0,1): red increases blue -> mostly red
    // Cell 1 (x=2,3): middle -> purple (0xFF7F007F)
    // Cell 2 (x=4,5): mostly blue
    const uint32_t fg0 = Color::toArgb(buffer.cellAt(0, 0).fgColor);
    const uint32_t fg1 = Color::toArgb(buffer.cellAt(1, 0).fgColor);
    const uint32_t fg2 = Color::toArgb(buffer.cellAt(2, 0).fgColor);
    QVERIFY(((fg0 >> 16) & 0xFF) > (fg0 & 0xFF));
    QCOMPARE(fg1, 0xFF7F007Fu);
    QVERIFY((fg2 & 0xFF) > ((fg2 >> 16) & 0xFF));

    // Explicit shape color overrides the parent Canvas gradient.
    auto* wave = canvas->findChild<CanvasShape*>(QStringLiteral("wave"));
    QVERIFY(wave != nullptr);
    wave->setColor(Color("#00ff00"));
    buffer.clear();
    paintItem(canvas, buffer, 0, 0);
    for (int x = 0; x < 3; ++x) {
        QCOMPARE(Color::toArgb(buffer.cellAt(x, 0).fgColor), 0xFF00FF00u);
    }

    // Explicit shape gradient overrides explicit shape color.
    auto* shapeGrad = new Gradient(wave);
    shapeGrad->setOrientation(Gradient::Horizontal);
    auto* s0 = new GradientStop(shapeGrad);
    s0->setPosition(0.0);
    s0->setColor(Color("#0000ff"));
    auto* s1 = new GradientStop(shapeGrad);
    s1->setPosition(1.0);
    s1->setColor(Color("#ff0000"));
    shapeGrad->addStop(s0);
    shapeGrad->addStop(s1);
    wave->setGradient(shapeGrad);

    buffer.clear();
    paintItem(canvas, buffer, 0, 0);
    const uint32_t newFg0 = Color::toArgb(buffer.cellAt(0, 0).fgColor);
    const uint32_t newFg2 = Color::toArgb(buffer.cellAt(2, 0).fgColor);
    QVERIFY((newFg0 & 0xFF) > ((newFg0 >> 16) & 0xFF));
    QVERIFY(((newFg2 >> 16) & 0xFF) > (newFg2 & 0xFF));
}

void TestCanvas::testCanvasAndShapeAttributes() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/..");

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui

        Canvas {
            width: 3
            height: 1
            renderMode: Canvas.Braille
            dim: true

            // Cell 0 inherits dim: true from parent Canvas.
            Point {
                x: 0; y: 0
            }

            // Cell 1 overrides inherited dim to false.
            Point {
                objectName: "overridePoint"
                x: 2; y: 0
                dim: false
            }

            // Cell 2 explicitly sets dim: true.
            Point {
                x: 4; y: 0
                dim: true
            }
        }
    )",
                      QUrl());

    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object != nullptr, qPrintable(component.errorString()));
    Canvas* canvas = qobject_cast<Canvas*>(object.get());
    QVERIFY(canvas != nullptr);

    TextGridBuffer buffer;
    buffer.resize(3, 1);
    paintItem(canvas, buffer, 0, 0);

    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Dim);
    QCOMPARE(buffer.cellAt(1, 0).attributes, TextAttribute::None);
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::Dim);

    // Changing dim on the Canvas or shape marks the canvas dirty.
    QSignalSpy dirtySpy(canvas, &Item::dirty);
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(dirtySpy.count(), 0);

    canvas->setDim(false);
    QCOMPARE(dirtySpy.count(), 1);
    buffer.clear();
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::None);
    QCOMPARE(buffer.cellAt(1, 0).attributes, TextAttribute::None);
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::Dim);

    auto* overridePoint = canvas->findChild<CanvasShape*>(QStringLiteral("overridePoint"));
    QVERIFY(overridePoint != nullptr);
    overridePoint->setDim(true);
    QCOMPARE(dirtySpy.count(), 2);
    buffer.clear();
    paintItem(canvas, buffer, 0, 0);
    QCOMPARE(buffer.cellAt(1, 0).attributes, TextAttribute::Dim);
}

QTEST_MAIN(TestCanvas)
#include "test_canvas.moc"
