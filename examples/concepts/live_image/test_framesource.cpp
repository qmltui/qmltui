#include "framesource.h"

#include <QSignalSpy>
#include <QTest>
#include <QtMath>

class TestFrameSource : public QObject {
    Q_OBJECT

private slots:
    void largeTraceFollowsSineCurves() {
        const QSize size(1200, 800);
        for (int tick : {0, 7}) {
            const QImage frame = FrameSource::paintTrace(tick, size);
            QVERIFY(!frame.hasAlphaChannel());
            int checked = 0;
            for (int x = 10; x < size.width() - 10; x += 17) {
                const double phase = tick * 0.25;
                const double t = x / double(size.width()) * 2 * M_PI;
                const double greenY = size.height() / 2.0
                    - std::sin(t + phase) * 0.9 * (size.height() / 2.0 - 2);
                const double blueY = size.height() / 2.0
                    - std::sin(2 * t + phase + 1.2) * 0.5 * (size.height() / 2.0 - 2);
                // Skip intersections, where the blue stroke covers the green.
                if (std::abs(greenY - blueY) < 4 * (size.height() / 24)) continue;
                QCOMPARE(frame.pixelColor(x, qRound(greenY)), QColor(101, 230, 160));
                QCOMPARE(frame.pixelColor(x, qRound(blueY)), QColor(126, 224, 255));
                ++checked;
            }
            QVERIFY(checked > 10);
        }
    }

    void resizeBurstPaintsLatestSizeWhilePaused() {
        FrameSource source;
        source.setRunning(false);
        QSignalSpy frames(&source, &FrameSource::frameChanged);
        const QImage initial = source.frame();
        for (int edge = 100; edge <= 1000; edge += 100)
            source.setFrameSize(QSize(edge, edge / 2));
        QCOMPARE(frames.count(), 0);
        QCOMPARE(source.frame(), initial);
        QTRY_COMPARE(frames.count(), 1);
        QCOMPARE(source.frame(), FrameSource::paintTrace(0, QSize(1000, 500)));
        QCOMPARE(source.tick(), 0);
        QVERIFY(!source.running());

        // Further resizes still repaint while paused; the animation stays stopped.
        source.setFrameSize(QSize(120, 60));
        QTRY_COMPARE(frames.count(), 2);
        QCOMPARE(source.frame().size(), QSize(120, 60));
        source.setFrameSize(QSize(120, 60));
        QTest::qWait(40);
        QCOMPARE(frames.count(), 2);
        QCOMPARE(source.tick(), 0);
    }
};

QTEST_GUILESS_MAIN(TestFrameSource)
#include "test_framesource.moc"
