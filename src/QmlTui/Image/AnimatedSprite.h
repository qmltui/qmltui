#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Image/Image.h>
#include <QRect>
#include <QQmlParserStatus>
namespace qmltui { class ClockTimer; }

namespace qmltui {

// Plays a frame-grid animation out of a single sprite sheet, mirroring Qt
// Quick's AnimatedSprite. The sheet is decoded once; each tick moves the
// inherited sourceClipRect to the next frame, so glyph modes re-scale only
// one frame and the pixel protocols upload only frame-sized images. The
// item owns sourceClipRect; a value assigned from outside is overwritten
// on the next frame. Every Image property (fillMode, renderMode, smooth,
// alignment, ...) applies to the current frame.
//
// Frames run from (frameX, frameY) to the right in steps of frameWidth and
// wrap to the sheet's left edge one frameHeight further down when a row runs
// out. When frameWidth or frameHeight is 0 the sheet is taken as one row of
// square frames (both default to the sheet height).
class QMLTUI_IMAGE_EXPORT AnimatedSprite : public Image, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(int frameCount READ frameCount WRITE setFrameCount NOTIFY frameCountChanged)
    Q_PROPERTY(int frameWidth READ frameWidth WRITE setFrameWidth NOTIFY frameWidthChanged)
    Q_PROPERTY(int frameHeight READ frameHeight WRITE setFrameHeight NOTIFY frameHeightChanged)
    Q_PROPERTY(int frameX READ frameX WRITE setFrameX NOTIFY frameXChanged)
    Q_PROPERTY(int frameY READ frameY WRITE setFrameY NOTIFY frameYChanged)
    // Frames per second; when > 0 it takes precedence over frameDuration.
    Q_PROPERTY(qreal frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged)
    // Milliseconds per frame, used when frameRate is not set.
    Q_PROPERTY(int frameDuration READ frameDuration WRITE setFrameDuration NOTIFY frameDurationChanged)
    Q_PROPERTY(int currentFrame READ currentFrame WRITE setCurrentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopsChanged)
    Q_PROPERTY(bool reverse READ reverse WRITE setReverse NOTIFY reverseChanged)
    Q_PROPERTY(FinishBehavior finishBehavior READ finishBehavior WRITE setFinishBehavior NOTIFY finishBehaviorChanged)
    Q_PROPERTY(bool interpolate READ interpolate WRITE setInterpolate NOTIFY interpolateChanged)

public:
    enum Loops {
        Infinite = -1,
    };
    Q_ENUM(Loops)

    enum class FinishBehavior : uint8_t {
        FinishAtInitialFrame,
        FinishAtFinalFrame,
    };
    Q_ENUM(FinishBehavior)

    explicit AnimatedSprite(QObject* parent = nullptr);

    int frameCount() const { return m_frameCount; }
    void setFrameCount(int count);
    int frameWidth() const { return m_frameWidth; }
    void setFrameWidth(int width);
    int frameHeight() const { return m_frameHeight; }
    void setFrameHeight(int height);
    int frameX() const { return m_frameX; }
    void setFrameX(int x);
    int frameY() const { return m_frameY; }
    void setFrameY(int y);
    qreal frameRate() const { return m_frameRate; }
    void setFrameRate(qreal rate);
    int frameDuration() const { return m_frameDuration; }
    void setFrameDuration(int duration);
    int currentFrame() const { return m_currentFrame; }
    void setCurrentFrame(int frame);
    bool running() const { return m_running; }
    void setRunning(bool running);
    bool paused() const { return m_paused; }
    void setPaused(bool paused);
    int loops() const { return m_loops; }
    void setLoops(int loops);
    bool reverse() const { return m_reverse; }
    void setReverse(bool reverse);
    FinishBehavior finishBehavior() const { return m_finishBehavior; }
    void setFinishBehavior(FinishBehavior behavior);
    bool interpolate() const { return m_interpolate; }
    void setInterpolate(bool interpolate);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    // Steps the animation by hand, loops and finishing included; useful while
    // paused or stopped.
    Q_INVOKABLE void advance(int frames = 1);

    void classBegin() override;
    void componentComplete() override;

signals:
    void frameCountChanged();
    void frameWidthChanged();
    void frameHeightChanged();
    void frameXChanged();
    void frameYChanged();
    void frameRateChanged();
    void frameDurationChanged();
    void currentFrameChanged();
    void runningChanged();
    void pausedChanged();
    void loopsChanged();
    void reverseChanged();
    void finishBehaviorChanged();
    void interpolateChanged();
    // Emitted when a finite `loops` count completes; never for Infinite, and
    // not when running is set to false.
    void finished();

private:
    // Frame size after the square-frames fallback; invalid without a sheet.
    QSize resolvedFrameSize() const;
    // The clip rectangle of frame `index` inside the sheet.
    QRect frameRect(int index) const;
    // Points the inherited sourceClipRect at the current frame.
    void applyFrame();
    // One animation step: wraps at the ends, counts loops, finishes.
    void step(int frames);
    // Starts or stops the tick timer to match the current state.
    void updateTimer();
    // The first frame of a run: 0 forward, the last frame in reverse.
    int initialFrame() const;

    // On the QmlTui Clock so frame stepping is deterministic in tests.
    ClockTimer* m_timer = nullptr;
    int m_frameCount = 1;
    int m_frameWidth = 0;
    int m_frameHeight = 0;
    int m_frameX = 0;
    int m_frameY = 0;
    qreal m_frameRate = 0.0;
    int m_frameDuration = -1;
    int m_currentFrame = 0;
    int m_loops = Infinite;
    bool m_running = true;
    bool m_paused = false;
    bool m_reverse = false;
    FinishBehavior m_finishBehavior = FinishBehavior::FinishAtInitialFrame;
    bool m_interpolate = true;
    int m_completedLoops = 0;
    bool m_warnedNoDuration = false;
    bool m_complete = true;
};

} // namespace qmltui
