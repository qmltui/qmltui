#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QSocketNotifier>
#include <QTimer>
#include <atomic>
#include <optional>
#include <memory>
#include "../primitives/TerminalDevice.h"
#include "../primitives/InputDecoder.h"
#include "../primitives/TextGridBuffer.h"
#include "Renderer.h"

class TestScrollView;

namespace qtui {

class Terminal;
class Item;
class InputRouter;
class TerminalEngine : public QObject {
    Q_OBJECT

public:
    explicit TerminalEngine(Terminal* rootWindow, QObject* parent = nullptr);
    ~TerminalEngine() override;

    Q_INVOKABLE void requestRender();

    quint64 frameSequence() const { return m_frameSequence; }

    // Accumulated frame cost since the engine was created. Totals sum the
    // per-frame FrameStats fields; bytesTotal sums diffBytes so it is
    // meaningful even when terminal output is disabled.
    struct FrameStatsTotals {
        quint64 frames = 0;
        qint64 paintMicrosTotal = 0;
        qint64 paintMicrosMax = 0;
        qint64 diffMicrosTotal = 0;
        qint64 writeMicrosTotal = 0;
        qint64 bytesTotal = 0;
        qint64 dirtyDuringPaintTotal = 0;
        qint64 skippedForBackpressure = 0;   // renders deferred while output was queued
    };
    FrameStats lastFrameStats() const { return m_lastFrameStats; }
    FrameStatsTotals frameStatsTotals() const { return m_frameStatsTotals; }

    // Upper bound on committed frames per second; 0 disables pacing. Defaults
    // to 60 (QTUI_MAX_FPS), or 0 in test sessions so timing assertions hold.
    void setMaxFrameRate(int framesPerSecond);
    int maxFrameRate() const { return m_maxFrameRate; }

    // Renders are deferred while more than this many bytes wait in the output
    // queue; the next frame is then diffed against the last committed one.
    void setOutputQueueLimit(size_t bytes) { m_outputQueueLimit = bytes; }
    size_t outputQueueLimit() const { return m_outputQueueLimit; }

    std::optional<TextGridBuffer> currentFrame() const;
    void setTestViewportSize(const QSize& size);

    void setRenderer(Renderer* renderer);
    Renderer* renderer() const { return m_renderer; }

    // Detected once from the environment at construction; override for
    // programmatic control (forwarded to the renderer).
    const TerminalCapabilities& capabilities() const { return m_capabilities; }
    void setCapabilities(const TerminalCapabilities& capabilities);
    void setTerminalOutputEnabled(bool enabled);
    bool terminalOutputEnabled() const { return m_terminalOutputEnabled; }
    void stop();
    void setMouseReporting(bool enable);

    int terminalWidth() const;
    int terminalHeight() const;

    void print(const QString& text);
    void append(qtui::Item* item);
    void copyToClipboard(const QString& text);
    static std::string osc52Sequence(const QString& text);
    void setTitle(const QString& title);
    QString title() const { return m_title; }
    static std::string titleSequence(const QString& text);

    void handleKeyInput(int key, const QString& text, int modifiers, bool paste = false);
    void handleMouseInput(int button, int x, int y, bool release, bool motion = false,
                          int modifiers = 0, int wheelDeltaX = 0, int wheelDeltaY = 0);

signals:
    void frameRendered(quint64 sequence);
    void renderFailed(const QString& message);

private slots:
    void readTerminalData();
    void readResizeSignal();
    void renderFrame();

private:
    friend class ::TestScrollView;

    void startRenderer();
    void setMouseMotionReportingEnabled(bool enabled);
    void handleResize();
    void cycleFocus(bool forward, Item* scope = nullptr);
    class Item* hitTestItem(Item* item, int absX, int absY, int parentAbsX, int parentAbsY) const;

    TerminalDevice m_device;
    InputDecoder m_inputDecoder;
    TerminalCapabilities m_capabilities;
    Renderer* m_renderer = nullptr;
    bool m_rendererStarted = false;
    bool m_alternateScreenMessageSession = false;
    bool m_terminalOutputEnabled = true;
    bool m_mouseMotionReportingEnabled = false;
    
    QSocketNotifier* m_notifier = nullptr;
    QPointer<Terminal> m_rootWindow;
    
    std::atomic<bool> m_renderPending{false};
    QSize m_testViewportSize;
    quint64 m_frameSequence = 0;
    FrameStats m_lastFrameStats;
    FrameStatsTotals m_frameStatsTotals;
    bool m_frameStatsLogging = false;
    int m_maxFrameRate = 0;
    int m_frameIntervalMs = 0;
    QTimer m_paceTimer;
    QElapsedTimer m_lastCommit;
    size_t m_outputQueueLimit = 256 * 1024;
    bool m_renderDeferredForOutput = false;
    QString m_title;
    std::unique_ptr<InputRouter> m_router;
};

} // namespace qtui
