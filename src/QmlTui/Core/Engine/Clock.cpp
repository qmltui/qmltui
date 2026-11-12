#include <QmlTui/Core/Engine/Clock_p.h>

#include <QAnimationDriver>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QVariantAnimation>
#include <algorithm>

namespace qmltui {

namespace {

// One process-wide monotonic origin for real mode.
QElapsedTimer& realOrigin() {
    static QElapsedTimer timer = [] {
        QElapsedTimer origin;
        origin.start();
        return origin;
    }();
    return timer;
}

} // namespace

// Reports the virtual clock to QUnifiedTimer, which drives QML Timer,
// Behavior, Transition, and every other QAbstractAnimationJob consumer.
class VirtualAnimationDriver final : public QAnimationDriver {
public:
    qint64 elapsed() const override { return Clock::instance()->elapsed(); }
};

Clock::Clock() {
    (void)realOrigin();
}

Clock::~Clock() {
    if (m_driverKeepalive) {
        m_driverKeepalive->stop();
    }
    if (m_driver) {
        m_driver->uninstall();
        delete m_driver;
        m_driver = nullptr;
    }
}

Clock* Clock::instance() {
    static Clock* clock = new Clock();
    return clock;
}

bool Clock::isVirtual() {
    return instance()->m_virtual.load(std::memory_order_relaxed);
}

void Clock::setVirtual() {
    Clock* clock = instance();
    if (clock->m_virtual.load(std::memory_order_relaxed)) return;
    // A timer or animation created on the real clock would keep real deadlines
    // forever; mixed timelines are indistinguishable from flaky tests.
    if (clock->m_timersExisted.load(std::memory_order_relaxed)) {
        qFatal("QmlTui: the virtual clock must be selected before any QmlTui timer "
               "or animation exists (set QMLTUI_RUNNER_CLOCK=virtual or call "
               "Runner::useVirtualClock() before loading QML)");
    }
    clock->m_virtual.store(true, std::memory_order_relaxed);
    clock->m_virtualNow = 0;

    // QUnifiedTimer stops an installed driver whenever only pause jobs (a
    // lone QML Timer, for example) remain, and falls back to a wall-clock
    // pause timer. The keepalive is an endless non-pause animation, so the
    // driver runs for the whole process and every delta stays virtual. It is
    // started here, before anything else, so QUnifiedTimer's time origin and
    // driverStartTime are both zero on the virtual timeline. Neither driver
    // nor keepalive ticks on its own: time moves only through advanceTo().
    clock->m_driver = new VirtualAnimationDriver();
    clock->m_driver->install();
    clock->m_driverKeepalive = new QVariantAnimation(clock);
    clock->m_driverKeepalive->setStartValue(0.0);
    clock->m_driverKeepalive->setEndValue(1.0);
    clock->m_driverKeepalive->setDuration(1000000000);
    clock->m_driverKeepalive->setLoopCount(-1);
    clock->m_driverKeepalive->start();
}

qint64 Clock::elapsed() const {
    return m_virtual.load(std::memory_order_relaxed) ? m_virtualNow : realOrigin().elapsed();
}

qint64 Clock::nextTimerDeadline() const {
    qint64 next = -1;
    for (const ClockTimer* timer : m_timers) {
        if (timer->m_active && (next < 0 || timer->m_deadline < next)) next = timer->m_deadline;
    }
    return next;
}

ClockTimer* Clock::takeNextDue(qint64 timeMs) const {
    ClockTimer* due = nullptr;
    for (ClockTimer* timer : m_timers) {
        if (timer->m_active && timer->m_deadline <= timeMs &&
            (!due || timer->m_deadline < due->m_deadline)) {
            due = timer;
        }
    }
    return due;
}

void Clock::advanceTo(qint64 timeMs) {
    Q_ASSERT(m_virtual);
    if (timeMs <= m_virtualNow) return;

    // Fire strictly in deadline order, moving now to each deadline first so a
    // handler that reads elapsed() or restarts a timer sees consistent time.
    while (ClockTimer* due = takeNextDue(timeMs)) {
        m_virtualNow = (std::max)(m_virtualNow, due->m_deadline);
        due->fire();
    }
    m_virtualNow = timeMs;
    if (m_driver) m_driver->advance();
}

ClockTimer::ClockTimer(QObject* parent) : QObject(parent) {
    Clock::instance()->m_timersExisted.store(true, std::memory_order_relaxed);
    if (Clock::isVirtual())
        Clock::instance()->m_timers.append(this);
    else
        connect(&m_timer, &QTimer::timeout, this, &ClockTimer::timeout);
}

ClockTimer::~ClockTimer() {
    if (Clock::isVirtual()) Clock::instance()->m_timers.removeOne(this);
}

void ClockTimer::setInterval(int intervalMs) {
    m_interval = intervalMs;
    m_timer.setInterval(intervalMs);
}

void ClockTimer::setSingleShot(bool singleShot) {
    m_singleShot = singleShot;
    m_timer.setSingleShot(singleShot);
}

bool ClockTimer::isActive() const {
    return Clock::isVirtual() ? m_active : m_timer.isActive();
}

void ClockTimer::start() {
    if (Clock::isVirtual()) {
        m_active = true;
        m_deadline = Clock::instance()->elapsed() + (std::max)(0, m_interval);
    } else {
        m_timer.start();
    }
}

void ClockTimer::start(int intervalMs) {
    setInterval(intervalMs);
    start();
}

void ClockTimer::stop() {
    if (Clock::isVirtual())
        m_active = false;
    else
        m_timer.stop();
}

void ClockTimer::fire() {
    // Reschedule before delivery so a handler that stops or restarts the
    // timer wins over the automatic repetition.
    if (m_singleShot)
        m_active = false;
    else
        m_deadline += (std::max)(1, m_interval);
    emit timeout();
}

} // namespace qmltui

static void qmltuiInitializeClockFromEnvironment() {
    const QByteArray mode = qgetenv("QMLTUI_RUNNER_CLOCK").trimmed().toLower();
    if (mode == "virtual")
        qmltui::Clock::setVirtual();
    else if (!mode.isEmpty() && mode != "real")
        qWarning("QMLTUI_RUNNER_CLOCK must be \"real\" or \"virtual\", not \"%s\"",
                 mode.constData());
}

Q_COREAPP_STARTUP_FUNCTION(qmltuiInitializeClockFromEnvironment)
