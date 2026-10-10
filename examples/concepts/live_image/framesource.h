#pragma once

#include <QImage>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

// Shape 1 of a dynamic picture: a C++ object that owns a QImage and redraws
// it on a timer. QML binds `Image.image: feed.frame`; every frameChanged() is
// one repaint of the item, no file and no URL involved.
class FrameSource : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QImage frame READ frame NOTIFY frameChanged)
    Q_PROPERTY(int tick READ tick NOTIFY tickChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    // The pixel size to draw at. Bind it to the showing Image's renderSize so
    // every frame is produced at exactly the resolution the terminal shows:
    // true pixels under kitty or sixel, the glyph grid otherwise. Falls back
    // to 96x48 while invalid.
    Q_PROPERTY(QSize frameSize READ frameSize WRITE setFrameSize NOTIFY frameSizeChanged)

public:
    explicit FrameSource(QObject* parent = nullptr);

    QImage frame() const { return m_frame; }
    int tick() const { return m_tick; }
    bool running() const { return m_timer.isActive(); }
    void setRunning(bool running);
    int interval() const { return m_timer.interval(); }
    void setInterval(int ms);
    QSize frameSize() const { return m_frameSize; }
    void setFrameSize(const QSize& size);

    // A scope trace: two sine waves phase-shifted by `tick`, on a 96x48 pixel
    // canvas. Public and static so the test can check determinism.
    static QImage paintTrace(int tick, const QSize& size = QSize(96, 48));

signals:
    void frameChanged();
    void tickChanged();
    void runningChanged();
    void intervalChanged();
    void frameSizeChanged();

private:
    void advance();

    QSize effectiveSize() const {
        const bool usable = m_frameSize.isValid() && !m_frameSize.isEmpty()
            && m_frameSize.width() <= 4096 && m_frameSize.height() <= 4096;
        return usable ? m_frameSize : QSize(96, 48);
    }

    QTimer m_timer;
    QTimer m_resizeTimer;
    QImage m_frame;
    QSize m_frameSize;
    int m_tick = 0;
};
