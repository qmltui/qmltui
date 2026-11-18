// Real-mode parity: a ClockTimer behaves like the QTimer it wraps, and the
// clock reports advancing wall time.
#include <QmlTui/Core/Engine/Clock_p.h>

#include <QSignalSpy>
#include <QtTest/QtTest>

using namespace qmltui;

class TestClockReal : public QObject {
    Q_OBJECT

private slots:
    void realModeIsDefault();
    void singleShotFiresOnce();
    void repeatingFiresUntilStopped();
    void elapsedAdvances();
};

void TestClockReal::realModeIsDefault() {
    QVERIFY(!Clock::isVirtual());
}

void TestClockReal::singleShotFiresOnce() {
    ClockTimer timer;
    timer.setSingleShot(true);
    QSignalSpy spy(&timer, &ClockTimer::timeout);
    timer.start(10);
    QVERIFY(timer.isActive());
    QTRY_COMPARE(spy.count(), 1);
    QVERIFY(!timer.isActive());
    QTest::qWait(30);
    QCOMPARE(spy.count(), 1);
}

void TestClockReal::repeatingFiresUntilStopped() {
    ClockTimer timer;
    timer.setInterval(10);
    QSignalSpy spy(&timer, &ClockTimer::timeout);
    timer.start();
    QTRY_VERIFY(spy.count() >= 2);
    timer.stop();
    QVERIFY(!timer.isActive());
}

void TestClockReal::elapsedAdvances() {
    const qint64 before = Clock::instance()->elapsed();
    QTest::qWait(20);
    QVERIFY(Clock::instance()->elapsed() >= before + 15);
}

QTEST_GUILESS_MAIN(TestClockReal)

#include "test_clock_real.moc"
