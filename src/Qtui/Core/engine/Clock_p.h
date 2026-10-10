#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <QList>
#include <QObject>
#include <QTimer>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtui API. It exists purely as an
// implementation detail and may change from version to version without
// notice, or even be removed.
//

class QVariantAnimation;

namespace qtui {

class ClockTimer;

// Process-wide time source for Qtui's own timers and animations. Real mode
// (the default) delegates to the system clock and adds no idle cost. Virtual
// mode freezes time entirely: it advances only through advanceTo(), which
// fires due ClockTimers in deadline order and steps the installed animation
// driver, so QML Timer and everything else on QUnifiedTimer share the same
// timeline. Main-thread only.
class QTUI_CORE_EXPORT Clock : public QObject {
    Q_OBJECT

public:
    static Clock* instance();
    static bool isVirtual();

    // Selects virtual mode for the whole process. Must run before any Qtui
    // timer or animation exists; QTUI_RUNNER_CLOCK=virtual calls it when the
    // QCoreApplication is constructed. One-way: there is no way back to real
    // time, so a scenario can never observe a mixed timeline.
    static void setVirtual();

    // Milliseconds since process start (real) or since setVirtual() (virtual).
    qint64 elapsed() const;

    // Virtual mode: the earliest pending ClockTimer deadline, or -1.
    qint64 nextTimerDeadline() const;

    // Virtual mode only: moves time forward, fires every ClockTimer whose
    // deadline falls inside the window in deadline order (a handler may
    // restart timers into the same window), then advances the animation
    // driver so QML timers and animations reach the same instant.
    void advanceTo(qint64 timeMs);

private:
    friend class ClockTimer;
    explicit Clock();
    ~Clock() override;

    ClockTimer* takeNextDue(qint64 timeMs) const;

    bool m_virtual = false;
    bool m_timersExisted = false;
    qint64 m_virtualNow = 0;
    QList<ClockTimer*> m_timers;
    class VirtualAnimationDriver* m_driver = nullptr;
    QVariantAnimation* m_driverKeepalive = nullptr;
};

// The subset of QTimer Qtui uses, on the Clock's timeline. In real mode it
// wraps a QTimer, so behaviour and idle cost are unchanged; in virtual mode
// it fires only when advanceTo() crosses its deadline. Main-thread only.
class QTUI_CORE_EXPORT ClockTimer : public QObject {
    Q_OBJECT

public:
    explicit ClockTimer(QObject* parent = nullptr);
    ~ClockTimer() override;

    void setInterval(int intervalMs);
    int interval() const { return m_interval; }
    void setSingleShot(bool singleShot);
    bool isSingleShot() const { return m_singleShot; }
    void setTimerType(Qt::TimerType type) { m_timer.setTimerType(type); }
    bool isActive() const;
    void start();
    void start(int intervalMs);
    void stop();

signals:
    void timeout();

private:
    friend class Clock;
    void fire();

    QTimer m_timer;
    int m_interval = 0;
    qint64 m_deadline = 0;
    bool m_singleShot = false;
    bool m_active = false;
};

} // namespace qtui
