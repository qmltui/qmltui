#include <QtTest/QtTest>
#include <Qamui/Image/AnimatedSprite.h>
#include <QSignalSpy>
#include <Qamui/Core/primitives/TextGridBuffer.h>
#include "../TestPaint.h"

// Four 2x2 frames in one 8x2 row: red, green, blue, yellow.
static QImage rowSheet() {
    QImage sheet(8, 2, QImage::Format_ARGB32);
    const QRgb colors[4] = { qRgb(255, 0, 0), qRgb(0, 255, 0), qRgb(0, 0, 255), qRgb(255, 255, 0) };
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 8; ++x)
            sheet.setPixel(x, y, colors[x / 2]);
    return sheet;
}

static const QRgb rowColors[4] = { 0xFFFF0000u, 0xFF00FF00u, 0xFF0000FFu, 0xFFFFFF00u };

class TestAnimatedSprite : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testFrameRendering();
    void testRowWrapping();
    void testDerivedSquareFrames();
    void testFrameOrigin();
    void testAdvanceWrapsAndReverses();
    void testFiniteLoopsFinish();
    void testFinishAtFinalFrame();
    void testStartRestartsFromInitialFrame();
    void testTimerAdvancesFrames();
};

void TestAnimatedSprite::testInitialState() {
    qamui::AnimatedSprite sprite;
    QCOMPARE(sprite.frameCount(), 1);
    QCOMPARE(sprite.frameWidth(), 0);
    QCOMPARE(sprite.frameHeight(), 0);
    QCOMPARE(sprite.currentFrame(), 0);
    QCOMPARE(sprite.running(), true);
    QCOMPARE(sprite.paused(), false);
    QCOMPARE(sprite.loops(), static_cast<int>(qamui::AnimatedSprite::Infinite));
    QCOMPARE(sprite.reverse(), false);
    QCOMPARE(sprite.finishBehavior(), qamui::AnimatedSprite::FinishBehavior::FinishAtInitialFrame);
}

void TestAnimatedSprite::testFrameRendering() {
    qamui::AnimatedSprite sprite;
    sprite.setRunning(false);
    sprite.setFrameWidth(2);
    sprite.setFrameHeight(2);
    sprite.setFrameCount(4);
    sprite.setImage(rowSheet());
    sprite.setRenderMode(qamui::Image::RenderMode::HalfBlock);
    sprite.setWidth(2);
    sprite.setHeight(1);

    // One frame is one clip: the implicit size follows it.
    QCOMPARE(sprite.implicitWidth(), 2);
    QCOMPARE(sprite.implicitHeight(), 1);

    for (int frame = 0; frame < 4; ++frame) {
        sprite.setCurrentFrame(frame);
        QCOMPARE(sprite.sourceClipRect(), QRect(frame * 2, 0, 2, 2));
        qamui::TextGridBuffer buffer;
        buffer.resize(4, 4);
        qamui::testing::paintItem(sprite, buffer, 0, 0);
        QCOMPARE(buffer.cellAt(0, 0).fgColor, rowColors[frame]);
        QCOMPARE(buffer.cellAt(0, 0).bgColor, rowColors[frame]);
    }
}

void TestAnimatedSprite::testRowWrapping() {
    // A 4x4 sheet of 2x2 frames: two per row, frame 3 sits at (2, 2).
    QImage sheet(4, 4, QImage::Format_ARGB32);
    sheet.fill(QColor(0, 0, 0));

    qamui::AnimatedSprite sprite;
    sprite.setRunning(false);
    sprite.setFrameWidth(2);
    sprite.setFrameHeight(2);
    sprite.setFrameCount(4);
    sprite.setImage(sheet);

    sprite.setCurrentFrame(2);
    QCOMPARE(sprite.sourceClipRect(), QRect(0, 2, 2, 2));
    sprite.setCurrentFrame(3);
    QCOMPARE(sprite.sourceClipRect(), QRect(2, 2, 2, 2));
}

void TestAnimatedSprite::testDerivedSquareFrames() {
    // Without frameWidth/frameHeight the sheet is one row of square frames.
    qamui::AnimatedSprite sprite;
    sprite.setRunning(false);
    sprite.setFrameCount(4);
    sprite.setImage(rowSheet());   // 8x2, so 2x2 frames

    sprite.setCurrentFrame(1);
    QCOMPARE(sprite.sourceClipRect(), QRect(2, 0, 2, 2));
}

void TestAnimatedSprite::testFrameOrigin() {
    // frameX/frameY place the first frame; later rows wrap to the left edge.
    QImage sheet(8, 6, QImage::Format_ARGB32);
    sheet.fill(QColor(0, 0, 0));

    qamui::AnimatedSprite sprite;
    sprite.setRunning(false);
    sprite.setFrameWidth(2);
    sprite.setFrameHeight(2);
    sprite.setFrameCount(6);
    sprite.setFrameX(4);
    sprite.setFrameY(2);
    sprite.setImage(sheet);

    sprite.setCurrentFrame(1);
    QCOMPARE(sprite.sourceClipRect(), QRect(6, 2, 2, 2));
    sprite.setCurrentFrame(2);   // first row ran out
    QCOMPARE(sprite.sourceClipRect(), QRect(0, 4, 2, 2));
    sprite.setCurrentFrame(5);
    QCOMPARE(sprite.sourceClipRect(), QRect(6, 4, 2, 2));
}

void TestAnimatedSprite::testAdvanceWrapsAndReverses() {
    qamui::AnimatedSprite sprite;
    sprite.setRunning(false);
    sprite.setFrameCount(3);
    sprite.setImage(rowSheet());

    sprite.advance();
    QCOMPARE(sprite.currentFrame(), 1);
    sprite.advance(2);
    QCOMPARE(sprite.currentFrame(), 0);   // wrapped past the end

    sprite.setReverse(true);
    sprite.advance();
    QCOMPARE(sprite.currentFrame(), 2);   // wrapped past the start
    sprite.advance();
    QCOMPARE(sprite.currentFrame(), 1);
}

void TestAnimatedSprite::testFiniteLoopsFinish() {
    qamui::AnimatedSprite sprite;
    sprite.setRunning(false);
    sprite.setFrameCount(3);
    sprite.setLoops(2);
    sprite.setImage(rowSheet());
    sprite.setRunning(true);

    QSignalSpy finishedSpy(&sprite, &qamui::AnimatedSprite::finished);
    QSignalSpy runningSpy(&sprite, &qamui::AnimatedSprite::runningChanged);
    for (int i = 0; i < 6; ++i)
        sprite.advance();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(sprite.running(), false);
    QCOMPARE(runningSpy.count(), 1);
    QCOMPARE(sprite.currentFrame(), 0);   // FinishAtInitialFrame

    // Finished for good: stepping further does not run another loop.
    sprite.advance();
    QCOMPARE(finishedSpy.count(), 1);
}

void TestAnimatedSprite::testFinishAtFinalFrame() {
    qamui::AnimatedSprite sprite;
    sprite.setRunning(false);
    sprite.setFrameCount(3);
    sprite.setLoops(1);
    sprite.setFinishBehavior(qamui::AnimatedSprite::FinishBehavior::FinishAtFinalFrame);
    sprite.setImage(rowSheet());

    QSignalSpy finishedSpy(&sprite, &qamui::AnimatedSprite::finished);
    for (int i = 0; i < 3; ++i)
        sprite.advance();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(sprite.currentFrame(), 2);
}

void TestAnimatedSprite::testStartRestartsFromInitialFrame() {
    qamui::AnimatedSprite sprite;
    sprite.setRunning(false);
    sprite.setFrameCount(4);
    sprite.setImage(rowSheet());

    sprite.setCurrentFrame(2);
    sprite.start();
    QCOMPARE(sprite.running(), true);
    QCOMPARE(sprite.currentFrame(), 0);

    sprite.stop();
    sprite.setReverse(true);
    sprite.start();
    QCOMPARE(sprite.currentFrame(), 3);   // reverse runs start at the end
}

void TestAnimatedSprite::testTimerAdvancesFrames() {
    qamui::AnimatedSprite sprite;
    sprite.setFrameCount(4);
    sprite.setFrameDuration(10);
    sprite.setImage(rowSheet());

    QSignalSpy frameSpy(&sprite, &qamui::AnimatedSprite::currentFrameChanged);
    QTRY_VERIFY(frameSpy.count() >= 2);

    // Pausing freezes the clock; the frame stays put.
    sprite.pause();
    const int frozen = sprite.currentFrame();
    QTest::qWait(50);
    QCOMPARE(sprite.currentFrame(), frozen);
    sprite.resume();
    QTRY_VERIFY(sprite.currentFrame() != frozen);
}

QTEST_MAIN(TestAnimatedSprite)
#include "test_animated_sprite.moc"
