#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QTimer>
#include <atomic>
#include <optional>
#include <memory>
#include <Qtui/Core/primitives/TerminalDevice.h>
#include <Qtui/Core/primitives/InputDecoder.h>
#include <Qtui/Core/primitives/TextGridBuffer.h>
#include <Qtui/Core/primitives/Color.h>
#include <Qtui/Core/engine/Renderer.h>

class TestScrollView;
class TestTerminalFramework;

namespace qtui {

class Terminal;
class Item;
class InputRouter;
class QTUI_CORE_EXPORT TerminalEngine : public QObject {
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

    // Environment defaults refined by terminal replies; override for
    // programmatic control (forwarded to the renderer).
    const TerminalCapabilities& capabilities() const { return m_capabilities; }
    void setCapabilities(const TerminalCapabilities& capabilities);
    void setTerminalOutputEnabled(bool enabled);
    bool terminalOutputEnabled() const { return m_terminalOutputEnabled; }

    // A scenario driver (qtui::Runner) owns this engine: terminal output stays
    // off unless live output is requested, and frame pacing is disabled so
    // timing assertions measure the framework. The constructor enters the
    // session itself when the environment announces one (QTUI_RUNNER_SCRIPT,
    // _EVAL, _CAPTURE_AFTER_MS or _EMBEDDED_SCENARIO); a Runner constructed
    // after QML loaded calls beginTestSession() from its constructor.
    void beginTestSession();
    bool testSession() const { return m_testSession; }
    // Renders a test session to the real terminal, as QTUI_RUNNER_LIVE=1 does.
    // Outside a test session output is on regardless.
    void setLiveOutput(bool live);
    bool liveOutput() const { return m_liveOutput; }
    // Process-wide request for engines created later, so a directly
    // constructed Runner is quiet from the very first byte: call it before the
    // root Terminal exists. Equivalent to QTUI_RUNNER_EMBEDDED_SCENARIO=1 (and
    // QTUI_RUNNER_LIVE=1 when liveOutput is true); a live request sticks.
    static void requestTestSession(bool liveOutput = false);
    void stop();
    bool enqueueControlSequence(QByteArray bytes);
    QByteArray takeControlSequences();
    // Like enqueueControlSequence, but drained after the cell diff each frame.
    // Protocols that paint at the cursor (sixel) must land on top of the cells,
    // not before them; kitty placements do not need this.
    bool enqueuePostControlSequence(QByteArray bytes);
    QByteArray takePostControlSequences();
    static constexpr qsizetype ControlSequenceQueueLimit = 4 * 1024 * 1024;
    quint64 outputSessionGeneration() const { return m_outputSessionGeneration; }
    void setMouseReporting(bool enable);

    int terminalWidth() const;
    int terminalHeight() const;

    // Accepted output belongs to the session, independent of renderer lifetime.
    // Reject a whole submission on overflow; retain already accepted output.
    bool submitScrollback(const QString& text);
    static constexpr size_t ScrollbackQueueLimit = 1024 * 1024;
    size_t pendingScrollbackBytes() const { return m_scrollback.size(); }
    void copyToClipboard(const QString& text);
    static std::string osc52Sequence(const QString& text);
    void setTitle(const QString& title);
    QString title() const { return m_title; }
    static std::string titleSequence(const QString& text);
    void updateBackgroundColor();
    void reapplyBackgroundColor();
    static std::string osc11Sequence(const Color& color);
    static std::string osc111Sequence();
    static Color parseOsc11Color(QByteArrayView payload);
    bool isBackgroundOsc11Active() const { return m_backgroundOsc11Active; }
    Color appliedBackgroundColor() const { return m_appliedBackgroundColor; }
    Color originalBackgroundColor() const { return m_originalBackgroundColor; }

    void handleKeyInput(int key, const QString& text, int modifiers, bool paste = false);
    // Raw terminal bytes, exactly as a backend delivers them. Runner feeds
    // scripted typing here so it takes the same decoder path as a real keyboard.
    void feedInput(QByteArrayView bytes);
    void handleMouseInput(int button, int x, int y, bool release, bool motion = false,
                          int modifiers = 0, int wheelDeltaX = 0, int wheelDeltaY = 0);
    // Whether a terminal in the current mode would deliver any-motion (hover)
    // events. Runner mirrors this so scripted hover matches what a real
    // terminal can send.
    bool mouseMotionReporting() const { return m_mouseMotionReportingEnabled; }

signals:
    void frameRendered(quint64 sequence);
    void capabilitiesChanged();
    void outputSessionInvalidated(quint64 generation);
    void renderFailed(const QString& message);

private slots:
    void readResizeSignal();
    void handleResume();
    void renderFrame();

private:
    void applyOutputPolicy();
    friend class ::TestScrollView;
    friend class ::TestTerminalFramework;   // reaches renderFrame() and mouse-motion state

    void startRenderer();
    void startQueries();
    void finishQueries();
    bool canQueryTerminal() const;
    void applyCapabilities(const TerminalCapabilities& capabilities);
    // Kitty keyboard flag 1 (disambiguate escape codes), push/pop form. The
    // mode is owned per output session: pushed once the terminal has proven
    // support and the renderer is on its screen, popped before that screen
    // is left. Stacks are per screen, so the order relative to the renderer
    // matters and an entry this session did not push is never popped.
    void updateKittyKeyboard();
    void popKittyKeyboard();
    void scheduleEscapeFlush();
    void handleReply(const InputDecoder::ControlSequence& sequence);
    void invalidateOutputSession();
    void flushControlSequences();
    void setMouseMotionReportingEnabled(bool enabled);
    // Writes the complete mouse-tracking state from the two inputs: the root
    // Terminal's mouseReporting switch and whether any item asked for
    // any-motion (hover) reporting. Motion is only ever enabled under the
    // switch; with reporting off nothing is tracked, whatever was requested.
    void applyMouseModes();
    bool mouseReportingWanted() const;
    void handleResize();
    void cycleFocus(bool forward, Item* scope = nullptr);
    void queryInitialBackgroundColor();
    class Item* hitTestItem(Item* item, int absX, int absY, int parentAbsX, int parentAbsY) const;

    TerminalDevice m_device;
    InputDecoder m_inputDecoder;
    TerminalCapabilities m_capabilities;
    TerminalCapabilities m_pendingCapabilities;
    QTimer m_queryTimer;
    QTimer m_pixelQueryTimer;
    bool m_queriesPending = false;
    bool m_queriesStarted = false;
    bool m_capabilitiesOverridden = false;
    bool m_kittyKeyboardPushed = false;
    // On the Qtui Clock: the lone-Escape flush is a time-semantic behaviour a
    // virtual-clock scenario must be able to step deterministically.
    class ClockTimer* m_escapeTimer = nullptr;
    QByteArray m_controlSequences;
    QByteArray m_postControlSequences;
    quint64 m_outputSessionGeneration = 0;
    Renderer* m_renderer = nullptr;
    bool m_rendererStarted = false;
    bool m_alternateScreenMessageSession = false;
    bool m_terminalOutputEnabled = true;
    bool m_testSession = false;
    bool m_liveOutput = false;
    bool m_maxFrameRateFromEnvironment = false;
    bool m_mouseMotionReportingEnabled = false;
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
    std::string m_scrollback;
    size_t m_scrollbackInFlight = 0;
    bool m_stopped = false;
    bool m_backgroundOsc11Active = false;
    Color m_appliedBackgroundColor;
    bool m_initialBgQueried = false;
    Color m_originalBackgroundColor;
    std::unique_ptr<InputRouter> m_router;
};

} // namespace qtui
