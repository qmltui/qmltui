#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QTimer>
#include <atomic>
#include <deque>
#include <optional>
#include <memory>
#include <vector>
#include <QmlTui/Core/Primitives/TerminalDevice.h>
#include <QmlTui/Core/Primitives/InputDecoder.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Engine/Renderer.h>

class TestScrollView;
class TestTerminalFramework;
class TestQtMessageRouting;

namespace qmltui {

class Terminal;
class Item;
class InputRouter;
// Runtime coordinator for a root Terminal item.
//
// Owns the TerminalDevice, InputDecoder, and InputRouter, drives the active
// Renderer (AltScreenRenderer or InlineRenderer), paces frame commits, handles
// terminal capability queries and OSC sequences (title, background color,
// clipboard, progress), and records scrollback history in test sessions.
// Main/engine thread only.
class QMLTUI_CORE_EXPORT TerminalEngine : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TerminalEngine)

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
        qint64 skippedForBackpressure = 0; // renders deferred while output was queued
    };
    FrameStats lastFrameStats() const { return m_lastFrameStats; }
    FrameStatsTotals frameStatsTotals() const { return m_frameStatsTotals; }

    // Upper bound on committed frames per second; 0 disables pacing. Defaults
    // to 60 (QMLTUI_MAX_FPS), or 0 in test sessions so timing assertions hold.
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

    // A scenario driver (qmltui::Runner) owns this engine: terminal output stays
    // off unless live output is requested, and frame pacing is disabled so
    // timing assertions measure the framework. The constructor enters the
    // session itself when the environment announces one (QMLTUI_RUNNER_SCRIPT,
    // _EVAL, _CAPTURE_AFTER_MS or _EMBEDDED_SCENARIO); a Runner constructed
    // after QML loaded calls beginTestSession() from its constructor.
    void beginTestSession();
    bool testSession() const { return m_testSession; }
    // Renders a test session to the real terminal, as QMLTUI_RUNNER_LIVE=1 does.
    // Outside a test session output is on regardless.
    void setLiveOutput(bool live);
    bool liveOutput() const { return m_liveOutput; }
    // Process-wide request for engines created later, so a directly
    // constructed Runner is quiet from the very first byte: call it before the
    // root Terminal exists. Equivalent to QMLTUI_RUNNER_EMBEDDED_SCENARIO=1 (and
    // QMLTUI_RUNNER_LIVE=1 when liveOutput is true); a live request sticks.
    static void requestTestSession(bool liveOutput = false);
    void stop();
    bool enqueueControlSequence(QByteArray bytes);
    QByteArray takeControlSequences();
    // Marks a cell rectangle whose terminal contents were overwritten by
    // pre-diff control sequences so the renderer resets its committed buffer
    // there before computing the frame diff.
    void invalidateCommittedRect(const QRect& rect);
    QList<QRect> takeInvalidatedCommittedRects();
    // Like enqueueControlSequence, but drained after the cell diff each frame.
    // Protocols that paint at the cursor (sixel) must land on top of the cells,
    // not before them; kitty placements do not need this.
    bool enqueuePostControlSequence(QByteArray bytes);
    QByteArray takePostControlSequences();
    // A 2048x2048 RGBA Kitty upload is 16 MiB raw and up to ~22 MiB once
    // compressed, base64-encoded, and framed in 4096-byte APC chunks.
    static constexpr qsizetype ControlSequenceQueueLimit = 32 * 1024 * 1024;
    quint64 outputSessionGeneration() const { return m_outputSessionGeneration; }
    void setMouseReporting(bool enable);

    int terminalWidth() const;
    int terminalHeight() const;

    // Accepted ANSI/UTF-8 bytes belong to the session, independent of renderer
    // lifetime. Whole submissions are rejected on overflow while retaining
    // already-accepted output; QMLTUI_SCROLLBACK_QUEUE_LIMIT overrides the cap.
    bool submitScrollback(const QString& text);
    void setScrollbackQueueLimit(size_t bytes) { m_scrollbackQueueLimit = bytes; }
    size_t scrollbackQueueLimit() const { return m_scrollbackQueueLimit; }
    static constexpr size_t DefaultScrollbackQueueLimit = 1024 * 1024;
    static constexpr size_t ScrollbackQueueLimit = DefaultScrollbackQueueLimit;
    size_t pendingScrollbackBytes() const { return m_scrollback.size(); }
    // Painted rows submitted as one block: the terminal receives them as
    // trimmed ANSI rows and, while history is recorded, the cells are kept so a
    // scenario can assert on them once they are committed.
    bool submitScrollback(const TextGridBuffer& rows);

    // Committed scrollback retained as cells. Off outside test sessions (no
    // memory cost for applications); a Runner session records from the first
    // frame. QMLTUI_SCROLLBACK_HISTORY_ROWS overrides the retained row count.
    // Rows become committed exactly when their bytes are: on the stream render
    // that carries them, or on stop(). AltScreen keeps them pending.
    void setScrollbackHistoryEnabled(bool enabled);
    bool scrollbackHistoryEnabled() const { return m_scrollbackHistoryEnabled; }
    void setScrollbackHistoryLimit(int rows);
    int scrollbackHistoryLimit() const { return m_scrollbackHistoryLimit; }
    static constexpr int DefaultScrollbackHistoryLimit = 5000;
    // Rows committed since the engine was created (monotonic, survives
    // trimming) and how many of them are still retained.
    quint64 committedScrollbackTotal() const { return m_committedScrollbackTotal; }
    int committedScrollbackRowCount() const {
        return static_cast<int>(m_committedScrollbackRows.size());
    }
    size_t pendingScrollbackRowCount() const { return m_pendingScrollbackRows.size(); }
    // Retained rows from a total-row index onwards (clamped to the oldest
    // retained row), padded to the widest row. Empty when nothing qualifies.
    TextGridBuffer committedScrollback(quint64 fromTotalRow = 0) const;
    void copyToClipboard(const QString& text);
    // Empty return means the request is pending; otherwise an error code.
    QString requestClipboardText(int timeoutMs);
    void cancelClipboardRead();
    static std::string osc52Sequence(const QString& text);
    void setTitle(const QString& title);
    QString title() const { return m_title; }
    static std::string titleSequence(const QString& text);
    Q_INVOKABLE void queryColors();
    void updateBackgroundColor();
    void reapplyBackgroundColor();
    static std::string osc11Sequence(const Color& color);
    static std::string osc111Sequence();
    static Color parseOscColorSpec(QByteArrayView value);
    static Color parseOsc10Color(QByteArrayView payload);
    static Color parseOsc11Color(QByteArrayView payload);
    static bool parseOsc4Color(QByteArrayView payload, int& index, Color& color);
    bool isBackgroundOsc11Active() const { return m_backgroundOsc11Active; }
    Color appliedBackgroundColor() const { return m_appliedBackgroundColor; }
    Color originalBackgroundColor() const { return m_originalBackgroundColor; }

    void ringBell();
    void syncTerminalProgress();

    void setHardwareCursorPosition(int x, int y);
    void clearHardwareCursorPosition();
    std::optional<QPoint> hardwareCursorPosition() const { return m_hardwareCursorPosition; }

    // Screen-space damage accumulation for partial-frame rendering.
    void addDamageRect(const QRect& rect);
    void markFullDamage();
    bool hasFullDamage() const { return m_fullDamage; }
    QRect damageRect() const { return m_damageRect; }
    void clearDamage();

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
    void framePainted();
    void frameRendered(quint64 sequence);
    // Emitted before frameRendered() on a frame that committed scrollback rows.
    void scrollbackCommitted(quint64 totalRows);
    void capabilitiesChanged();
    void outputSessionInvalidated(quint64 generation);
    void renderFailed(const QString& message);
    void clipboardReadFinished(const QString& text, const QString& code);

private slots:
    void readResizeSignal();
    void handleResume();
    void renderFrame();

private:
    void applyOutputPolicy();
    void finishClipboardRead(const QString& text, const QString& code);
    class ClockTimer* m_clipboardTimer = nullptr;
    bool m_clipboardReadPending = false;
    friend class ::TestScrollView;
    friend class ::TestTerminalFramework; // reaches renderFrame() and mouse-motion state
    friend class ::TestQtMessageRouting;

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
    using ScrollbackRow = std::vector<Cell>;
    bool enqueueScrollback(std::string&& bytes, std::deque<ScrollbackRow>&& rows);
    void commitScrollbackRows(std::deque<ScrollbackRow>&& rows);
    void trimScrollbackHistory();
    static std::deque<ScrollbackRow> textToScrollbackRows(const QString& text);
    static ScrollbackRow trimmedRow(const TextGridBuffer& buffer, int row);
    void queryInitialBackgroundColor();
    void queryTerminalColors(bool includeAnsiPalette);

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
    // On the QmlTui Clock: the lone-Escape flush is a time-semantic behaviour a
    // virtual-clock scenario must be able to step deterministically.
    class ClockTimer* m_escapeTimer = nullptr;
    QByteArray m_controlSequences;
    QByteArray m_postControlSequences;
    QList<QRect> m_invalidatedCommittedRects;
    quint64 m_outputSessionGeneration = 0;
    Renderer* m_renderer = nullptr;
    std::vector<std::unique_ptr<Renderer>> m_retiredRenderers;
    bool m_renderingFrame = false;
    bool m_rendererStarted = false;
    bool m_alternateScreenMessageSession = false;
    bool m_deferredDefaultStderrSession = false;
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
    size_t m_scrollbackQueueLimit = DefaultScrollbackQueueLimit;
    std::deque<ScrollbackRow> m_pendingScrollbackRows;
    std::deque<ScrollbackRow> m_committedScrollbackRows;
    quint64 m_committedScrollbackTotal = 0;
    int m_scrollbackHistoryLimit = DefaultScrollbackHistoryLimit;
    bool m_scrollbackHistoryEnabled = false;
    std::atomic<bool> m_stopped{false};
    bool m_backgroundOsc11Active = false;
    Color m_appliedBackgroundColor = Color::Invalid;
    bool m_initialBgQueried = false;
    bool m_ansiPaletteQueried = false;
    Color m_originalBackgroundColor = Color::Invalid;
    std::optional<QPoint> m_hardwareCursorPosition;
    QRect m_damageRect;
    bool m_fullDamage = true;
    bool m_terminalProgressActive = false;
    std::unique_ptr<InputRouter> m_router;
};

} // namespace qmltui
