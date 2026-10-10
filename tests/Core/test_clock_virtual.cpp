// ClockTimer semantics on the virtual clock: deadline ordering, single-shot
// versus repeating, and mutation from inside a timeout handler.
#include <Qtui/Core/engine/Clock_p.h>

#include <QtTest/QtTest>

using namespace qtui;

class TestClockVirtual : public QObject {
    Q_OBJECT

private slots:
    void virtualTimeIsFrozen();
    void firesInDeadlineOrder();
    void singleShotVersusRepeating();
    void restartDuringDelivery();
    void stopDuringDelivery();
    void reportsNextDeadline();
};

void TestClockVirtual::virtualTimeIsFrozen()
{
    QVERIFY(Clock::isVirtual());
    const qint64 before = Clock::instance()->elapsed();
    QTest::qWait(25);
    QCOMPARE(Clock::instance()->elapsed(), before);
}

void TestClockVirtual::firesInDeadlineOrder()
{
    Clock* clock = Clock::instance();
    QStringList order;
    ClockTimer late;
    late.setSingleShot(true);
    connect(&late, &ClockTimer::timeout, this, [&order] { order << QStringLiteral("late"); });
    ClockTimer early;
    early.setSingleShot(true);
    connect(&early, &ClockTimer::timeout, this, [&order] { order << QStringLiteral("early"); });

    // Registration order (late first) must not beat deadline order.
    late.start(50);
    early.start(20);
    clock->advanceTo(clock->elapsed() + 100);
    QCOMPARE(order, (QStringList{QStringLiteral("early"), QStringLiteral("late")}));
}

void TestClockVirtual::singleShotVersusRepeating()
{
    Clock* clock = Clock::instance();
    int repeats = 0;
    ClockTimer repeating;
    repeating.setInterval(10);
    connect(&repeating, &ClockTimer::timeout, this, [&repeats] { ++repeats; });
    int onceCount = 0;
    ClockTimer once;
    once.setSingleShot(true);
    connect(&once, &ClockTimer::timeout, this, [&onceCount] { ++onceCount; });

    repeating.start();
    once.start(10);
    clock->advanceTo(clock->elapsed() + 35);
    QCOMPARE(repeats, 3);   // 10, 20, 30
    QCOMPARE(onceCount, 1);
    QVERIFY(repeating.isActive());
    QVERIFY(!once.isActive());
    repeating.stop();
}

void TestClockVirtual::restartDuringDelivery()
{
    Clock* clock = Clock::instance();
    int fired = 0;
    ClockTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(10);
    connect(&timer, &ClockTimer::timeout, this, [&fired, &timer] {
        if (++fired == 1)
            timer.start();  // rearm inside the same advance window
    });
    timer.start();
    clock->advanceTo(clock->elapsed() + 25);
    QCOMPARE(fired, 2);
    QVERIFY(!timer.isActive());
}

void TestClockVirtual::stopDuringDelivery()
{
    Clock* clock = Clock::instance();
    int fired = 0;
    ClockTimer timer;
    timer.setInterval(10);
    connect(&timer, &ClockTimer::timeout, this, [&fired, &timer] {
        ++fired;
        timer.stop();  // a handler's stop must beat the automatic repetition
    });
    timer.start();
    clock->advanceTo(clock->elapsed() + 50);
    QCOMPARE(fired, 1);
    QVERIFY(!timer.isActive());
}

void TestClockVirtual::reportsNextDeadline()
{
    Clock* clock = Clock::instance();
    QCOMPARE(clock->nextTimerDeadline(), -1);
    ClockTimer far;
    far.setSingleShot(true);
    far.start(300);
    ClockTimer near;
    near.setSingleShot(true);
    near.start(100);
    QCOMPARE(clock->nextTimerDeadline(), clock->elapsed() + 100);
    clock->advanceTo(clock->elapsed() + 100);
    QCOMPARE(clock->nextTimerDeadline(), clock->elapsed() + 200);
    far.stop();
    QCOMPARE(clock->nextTimerDeadline(), -1);
}

int main(int argc, char* argv[])
{
    // Selected before the QCoreApplication exists, exactly as the Runner's
    // startup hook does it.
    qputenv("QTUI_RUNNER_CLOCK", "virtual");
    QCoreApplication app(argc, argv);
    TestClockVirtual test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_clock_virtual.moc"
