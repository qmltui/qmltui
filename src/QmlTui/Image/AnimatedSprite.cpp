#include <QmlTui/Image/AnimatedSprite.h>
#include <QmlTui/Core/Engine/Clock_p.h>

#include <algorithm>
#include <cmath>

namespace qmltui {

AnimatedSprite::AnimatedSprite(QObject* parent) : Image(parent) {
    m_timer = new ClockTimer(this);
    connect(m_timer, &ClockTimer::timeout, this, [this] { step(1); });
    // The sheet decides derived frame sizes and row wrapping, so the clip can
    // only be placed once the picture is in (and again when it is replaced).
    connect(this, &Image::imageChanged, this, &AnimatedSprite::applyFrame);
    // A hidden sprite paints nothing; do not tick for it either.
    connect(this, &Item::visibleChanged, this, &AnimatedSprite::updateTimer);
    updateTimer();
}

QSize AnimatedSprite::resolvedFrameSize() const {
    const QSize sheet = sourceSize();
    if (sheet.isEmpty()) return QSize();
    // Without explicit sizes the sheet is one row of square frames.
    const int h = m_frameHeight > 0 ? m_frameHeight : sheet.height();
    const int w = m_frameWidth > 0 ? m_frameWidth : h;
    return QSize(w, h);
}

QRect AnimatedSprite::frameRect(int index) const {
    const QSize frame = resolvedFrameSize();
    const QSize sheet = sourceSize();
    if (frame.isEmpty() || sheet.isEmpty()) return QRect();
    // Frames run to the right from (frameX, frameY) and wrap to the sheet's
    // left edge one frame further down when the row runs out.
    const int perFirstRow = (std::max)(1, (sheet.width() - m_frameX) / frame.width());
    const int perRow = (std::max)(1, sheet.width() / frame.width());
    int x = m_frameX;
    int y = m_frameY;
    if (index < perFirstRow) {
        x += index * frame.width();
    } else {
        const int rest = index - perFirstRow;
        x = (rest % perRow) * frame.width();
        y += (1 + rest / perRow) * frame.height();
    }
    return QRect(QPoint(x, y), frame);
}

void AnimatedSprite::applyFrame() {
    const QRect rect = frameRect((std::clamp)(m_currentFrame, 0, (std::max)(0, m_frameCount - 1)));
    if (!rect.isNull())
        setSourceClipRect(rect);
}

int AnimatedSprite::initialFrame() const {
    return m_reverse ? (std::max)(0, m_frameCount - 1) : 0;
}

void AnimatedSprite::step(int frames) {
    if (m_frameCount <= 0) return;
    const int direction = m_reverse ? -1 : 1;
    int frame = m_currentFrame;
    for (int i = 0; i < frames; ++i) {
        frame += direction;
        if (frame >= 0 && frame < m_frameCount)
            continue;
        ++m_completedLoops;
        if (m_loops != Infinite && m_completedLoops >= m_loops) {
            frame = (m_finishBehavior == FinishBehavior::FinishAtFinalFrame)
                        ? (m_reverse ? 0 : m_frameCount - 1)
                        : initialFrame();
            setCurrentFrame(frame);
            if (m_running) {
                m_running = false;
                updateTimer();
                emit runningChanged();
            }
            emit finished();
            return;
        }
        frame = m_reverse ? m_frameCount - 1 : 0;
    }
    setCurrentFrame(frame);
}

void AnimatedSprite::classBegin() {
    m_complete = false;
}

void AnimatedSprite::componentComplete() {
    m_complete = true;
    applyFrame();
    updateTimer();
}

void AnimatedSprite::updateTimer() {
    if (!m_complete) return;
    int interval = -1;
    if (m_frameRate > 0.0)
        interval = (std::max)(1, static_cast<int>(std::lround(1000.0 / m_frameRate)));
    else if (m_frameDuration > 0)
        interval = m_frameDuration;
    const bool active = m_running && !m_paused && visible() && m_frameCount > 1 && interval > 0;
    if (active) {
        m_timer->start(interval);
        return;
    }
    m_timer->stop();
    if (m_running && !m_paused && m_frameCount > 1 && interval <= 0 && !m_warnedNoDuration) {
        m_warnedNoDuration = true;
        qWarning("AnimatedSprite: set frameRate or frameDuration, the animation cannot advance without one");
    }
}

void AnimatedSprite::setFrameCount(int count) {
    count = (std::max)(1, count);
    if (m_frameCount == count) return;
    m_frameCount = count;
    emit frameCountChanged();
    if (m_currentFrame >= count)
        setCurrentFrame(count - 1);
    else
        applyFrame();
    updateTimer();
}

void AnimatedSprite::setFrameWidth(int width) {
    width = (std::max)(0, width);
    if (m_frameWidth == width) return;
    m_frameWidth = width;
    emit frameWidthChanged();
    applyFrame();
}

void AnimatedSprite::setFrameHeight(int height) {
    height = (std::max)(0, height);
    if (m_frameHeight == height) return;
    m_frameHeight = height;
    emit frameHeightChanged();
    applyFrame();
}

void AnimatedSprite::setFrameX(int x) {
    x = (std::max)(0, x);
    if (m_frameX == x) return;
    m_frameX = x;
    emit frameXChanged();
    applyFrame();
}

void AnimatedSprite::setFrameY(int y) {
    y = (std::max)(0, y);
    if (m_frameY == y) return;
    m_frameY = y;
    emit frameYChanged();
    applyFrame();
}

void AnimatedSprite::setFrameRate(qreal rate) {
    if (qFuzzyCompare(m_frameRate + 1.0, rate + 1.0)) return;
    m_frameRate = rate;
    if (rate > 0.0) m_warnedNoDuration = false;
    emit frameRateChanged();
    updateTimer();   // restarts the tick with the new pace
}

void AnimatedSprite::setFrameDuration(int duration) {
    if (m_frameDuration == duration) return;
    m_frameDuration = duration;
    if (duration > 0) m_warnedNoDuration = false;
    emit frameDurationChanged();
    updateTimer();
}

void AnimatedSprite::setCurrentFrame(int frame) {
    if (m_frameCount > 0)
        frame = ((frame % m_frameCount) + m_frameCount) % m_frameCount;
    if (m_currentFrame != frame) {
        m_currentFrame = frame;
        emit currentFrameChanged();
    }
    applyFrame();
}

void AnimatedSprite::setRunning(bool running) {
    if (running)
        start();
    else
        stop();
}

void AnimatedSprite::setPaused(bool paused) {
    if (m_paused == paused) return;
    m_paused = paused;
    emit pausedChanged();
    updateTimer();
}

void AnimatedSprite::setLoops(int loops) {
    if (loops < 0)
        loops = Infinite;
    if (m_loops == loops) return;
    m_loops = loops;
    emit loopsChanged();
}

void AnimatedSprite::setReverse(bool reverse) {
    if (m_reverse == reverse) return;
    m_reverse = reverse;
    emit reverseChanged();
}

void AnimatedSprite::setFinishBehavior(FinishBehavior behavior) {
    if (m_finishBehavior == behavior) return;
    m_finishBehavior = behavior;
    emit finishBehaviorChanged();
}

void AnimatedSprite::setInterpolate(bool interpolate) {
    if (m_interpolate == interpolate) return;
    m_interpolate = interpolate;
    setSmooth(interpolate);
    emit interpolateChanged();
}

void AnimatedSprite::start() {
    if (m_running) return;
    m_completedLoops = 0;
    setCurrentFrame(initialFrame());
    m_running = true;
    updateTimer();
    emit runningChanged();
}

void AnimatedSprite::stop() {
    if (!m_running) return;
    m_running = false;
    updateTimer();
    emit runningChanged();
}

void AnimatedSprite::restart() {
    stop();
    start();
}

void AnimatedSprite::pause() {
    setPaused(true);
}

void AnimatedSprite::resume() {
    setPaused(false);
}

void AnimatedSprite::advance(int frames) {
    if (frames > 0)
        step(frames);
}

} // namespace qmltui
