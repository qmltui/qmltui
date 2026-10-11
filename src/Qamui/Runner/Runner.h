#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>

#include <QObject>
#include <functional>
#include <QElapsedTimer>
#include <QJSValue>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <memory>
#include <optional>

class QEventLoop;
class QQmlEngine;

namespace qamui {

class RunnerBridge;
class RunnerHardWatchdog;
class Terminal;
class TerminalEngine;
class TextGridBuffer;

class QAMUI_RUNNER_EXPORT Runner final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Runner)

public:
    Runner(QQmlEngine* qmlEngine, Terminal* terminal, QString scriptPath,
           QString artifactDirectory, QSize viewport, int operationTimeoutMs,
           int scenarioTimeoutMs, QString configurationError);
    ~Runner() override;

    static Runner* startFromEnvironment(QQmlEngine* qmlEngine, Terminal* terminal);

    /// Selects the virtual clock for the whole process, as QAMUI_RUNNER_CLOCK=virtual
    /// does. Call it before loading QML; once any Qamui timer or animation
    /// exists the process aborts with a diagnostic instead of mixing timelines.
    static void useVirtualClock();

    /// Marks this process as a scenario run driven by a directly constructed
    /// Runner, as QAMUI_RUNNER_EMBEDDED_SCENARIO=1 does: every Terminal created
    /// afterwards stays quiet (no ANSI output, no frame pacing) unless
    /// liveOutput is true or QAMUI_RUNNER_LIVE is set. Call it before loading
    /// QML, like useVirtualClock(). A Runner constructed without it still
    /// silences its engine, but only from its construction on; the terminal's
    /// startup sequences have been written by then.
    static void useEmbeddedScenario(bool liveOutput = false);

    /// Renders the scenario to the real terminal instead of committing frames
    /// quietly, as QAMUI_RUNNER_LIVE=1 does. Takes effect immediately.
    void setLiveOutput(bool live);

    /// Rewrites snapshot baselines instead of failing on a mismatch, as
    /// QAMUI_RUNNER_UPDATE_SNAPSHOTS=1 does. An explicit value wins over the
    /// environment. Call it before start().
    void setUpdateSnapshots(bool update);

    void start();

    /// Sends one key with optional Shift, Ctrl, Alt, or Meta modifiers and waits
    /// for the resulting frame to be committed.
    Q_INVOKABLE void press(const QString& key, const QStringList& modifiers = {});

    /// Feeds non-empty text through the input decoder — one key event per
    /// character, as a real keyboard would produce — and waits once for the
    /// resulting frame to be committed.
    Q_INVOKABLE void typeText(const QString& text);

    /// Processes Qt events for durationMs without blocking application timers or
    /// rendering. The duration must be between 1 and 30000 milliseconds. Under
    /// the virtual clock this is advanceTime(durationMs).
    Q_INVOKABLE void sleep(int durationMs);

    /// Virtual clock only: advances Qamui time by durationMs in steps of at
    /// most 16 ms (or to the next timer deadline when that is sooner), firing
    /// Qamui timers, animations, and QML Timers, and committing frames for
    /// whatever they invalidated. Nothing scheduled means no frame.
    Q_INVOKABLE void advanceTime(int durationMs);

    /// Milliseconds on the Qamui clock: virtual time under the virtual clock,
    /// wall time since process start otherwise.
    Q_INVOKABLE double now() const;

    /// Waits for a committed frame newer than the current frame.
    Q_INVOKABLE void waitForFrame(int timeoutMs = 2000);

    /// Sends text without waiting for a frame, so a following expectRepaint()
    /// can observe whether the application scheduled the repaint itself.
    Q_INVOKABLE void injectText(const QString& text);

    /// Sends non-empty text as one bracketed paste through the input decoder
    /// and waits for the resulting committed frame. Newlines and tabs inside
    /// the paste arrive untranslated, exactly as a terminal delivers them.
    Q_INVOKABLE void paste(const QString& text);

    /// paste() without the implicit frame wait, for use with expectRepaint().
    Q_INVOKABLE void injectPaste(const QString& text);

    /// press() without the implicit frame wait, for use with expectRepaint().
    Q_INVOKABLE void injectKey(const QString& key, const QStringList& modifiers = {});

    /// Fails unless the application commits a new frame on its own. Unlike
    /// waitForFrame(), this never requests a render, so it verifies that a
    /// change actually invalidated the scene rather than only that the scene
    /// paints correctly when a render is forced.
    Q_INVOKABLE void expectRepaint(int timeoutMs = 2000);

    /// Resizes the test viewport to columns x rows (each 1 through 1000) and
    /// waits until a frame with the new dimensions is committed.
    Q_INVOKABLE void resize(int columns, int rows);

    /// Returns {columns, rows} of the current committed frame.
    Q_INVOKABLE QVariantMap size();

    /// Sequence number of the most recently committed frame. Use it to assert
    /// that an interaction did not cause any repaint at all.
    Q_INVOKABLE int frameCount() const;

    /// Cost of the most recently committed frame and totals for the session:
    /// frame, paintMicros, diffMicros, writeMicros, diffBytes, bytesWritten,
    /// dirtyDuringPaint, frames, paintMicrosAvg, paintMicrosMax, diffMicrosAvg,
    /// bytesTotal, dirtyDuringPaintTotal. bytesTotal sums encoded diff bytes,
    /// so it is meaningful in quiet sessions where nothing reaches a terminal.
    Q_INVOKABLE QVariantMap frameStats() const;

    /// Polls a synchronous JavaScript callback until it returns a truthy value.
    /// The callback receives the current frame text and elapsed milliseconds.
    Q_INVOKABLE void waitForCondition(const QJSValue& condition, int timeoutMs = 2000);

    /// Waits until the committed terminal frame contains text.
    Q_INVOKABLE void waitForText(const QString& text, int timeoutMs = 2000);

    /// Fails unless the next committed terminal frame contains text. A
    /// negative timeout means the configured operation timeout.
    Q_INVOKABLE void expectText(const QString& text, int timeoutMs = -1);

    /// Fails unless the next committed terminal frame does not contain text.
    Q_INVOKABLE void expectNoText(const QString& text, int timeoutMs = -1);

    /// Writes the next committed frame as normalized UTF-8 text beneath the
    /// configured artifact directory.
    Q_INVOKABLE void captureText(const QString& relativePath);

    /// Writes the next committed frame as styled text (see styledText())
    /// beneath the configured artifact directory. Use it to record the baseline
    /// for expectStyled().
    Q_INVOKABLE void captureStyled(const QString& relativePath);

    /// Rows the application has committed to the scrollback (Inline mode).
    /// The count is monotonic and survives history trimming; pass it as the
    /// fromRow of the other scrollback methods to look only at rows a step
    /// appended.
    Q_INVOKABLE int scrollbackRowCount();

    /// Normalized text of the committed scrollback rows from a total-row mark
    /// onwards, without waiting. Empty when nothing has been committed.
    Q_INVOKABLE QString scrollbackText(int fromRow = 0);

    /// Waits until the committed scrollback contains text.
    Q_INVOKABLE void waitForScrollbackText(const QString& text, int timeoutMs = 2000);

    /// Fails unless the scrollback committed by the next frame contains text.
    Q_INVOKABLE void expectScrollbackText(const QString& text, int timeoutMs = -1);

    /// Fails if the scrollback committed by the next frame contains text.
    Q_INVOKABLE void expectNoScrollbackText(const QString& text, int timeoutMs = -1);

    /// Writes the committed scrollback rows from fromRow onwards as normalized
    /// text beneath the configured artifact directory.
    Q_INVOKABLE void captureScrollback(const QString& relativePath, int fromRow = 0);

    /// Like captureScrollback(), in the styledText() format.
    Q_INVOKABLE void captureScrollbackStyled(const QString& relativePath, int fromRow = 0);

    /// Compares the committed scrollback rows from fromRow onwards against a
    /// snapshot resolved relative to the scenario; update mode rewrites it.
    Q_INVOKABLE void expectScrollbackSnapshot(const QString& snapshotRelativePath,
                                              int fromRow = 0, int timeoutMs = -1);

    /// Returns normalized text for the current committed frame without waiting.
    Q_INVOKABLE QString frameText();

    /// Returns the current committed frame as normalized text followed by a
    /// "--- styles" section with one line per run of adjacent cells that share
    /// a non-default foreground, background, or attribute set. The result is
    /// plain text, so snapshot diffs stay readable and platform independent.
    Q_INVOKABLE QString styledText();

    /// Returns the rendered cell at (x, y), including colors and attributes.
    Q_INVOKABLE QVariantMap cell(int x, int y);

    /// Returns every visible single-row occurrence of text in row-major order.
    Q_INVOKABLE QVariantList findText(const QString& text);

    /// Returns normalized text for the sub-grid [x, y, width, height] of the
    /// current committed frame, without waiting. Trailing spaces are trimmed
    /// from each row. Bounds outside the frame fail the scenario.
    Q_INVOKABLE QString regionText(int x, int y, int width, int height);

    /// Returns {x, y, width, height} in terminal cells of the first visible
    /// item with the given objectName, clipped to the viewport. A missing,
    /// hidden, or fully off-screen item fails the scenario.
    Q_INVOKABLE QVariantMap itemRect(const QString& objectName);

    /// Waits until an item with the given objectName is visible inside the
    /// committed viewport.
    Q_INVOKABLE void waitForItem(const QString& objectName, int timeoutMs = 2000);

    /// Compares the region's normalized text against a golden baseline, with
    /// the same artifacts and update mode as expectSnapshot().
    Q_INVOKABLE void expectRegionSnapshot(int x, int y, int width, int height,
                                          const QString& snapshotRelativePath,
                                          int timeoutMs = -1);

    /// Clicks the center of a rendered text match and waits for the resulting frame.
    Q_INVOKABLE void clickText(const QString& text, int occurrence = 0,
                               const QString& button = QStringLiteral("left"),
                               const QStringList& modifiers = {});

    /// Sends a mouse click at (x, y) with optional button ("left", "middle", "right") and modifiers.
    Q_INVOKABLE void click(int x, int y, const QString& button = QStringLiteral("left"), const QStringList& modifiers = {});

    /// Moves the mouse to (x, y) with no button held. The event is delivered
    /// only while the application has hover (any-motion) reporting enabled,
    /// exactly as a real terminal sends it; otherwise it is dropped.
    Q_INVOKABLE void hover(int x, int y, const QStringList& modifiers = {});

    /// Presses a mouse button at (x, y) without releasing it.
    Q_INVOKABLE void mouseDown(int x, int y, const QString& button = QStringLiteral("left"), const QStringList& modifiers = {});

    /// Releases a mouse button at (x, y).
    Q_INVOKABLE void mouseUp(int x, int y, const QString& button = QStringLiteral("left"), const QStringList& modifiers = {});

    /// Sends a double click at (x, y) with optional button and modifiers.
    Q_INVOKABLE void doubleClick(int x, int y, const QString& button = QStringLiteral("left"), const QStringList& modifiers = {});

    /// Drags from (startX, startY) to (endX, endY) with optional button and modifiers.
    Q_INVOKABLE void drag(int startX, int startY, int endX, int endY, const QString& button = QStringLiteral("left"), const QStringList& modifiers = {});

    /// Sends a mouse wheel scroll at (x, y) with deltaX and deltaY.
    Q_INVOKABLE void scroll(int x, int y, int deltaX, int deltaY, const QStringList& modifiers = {});

    /// Waits until a committed frame matches the regex pattern.
    Q_INVOKABLE void waitForRegex(const QString& pattern, int timeoutMs = 2000);

    /// Fails unless the next committed frame matches the regex pattern.
    Q_INVOKABLE void expectRegex(const QString& pattern, int timeoutMs = -1);

    /// Fails unless the specified sub-region contains text.
    Q_INVOKABLE void expectTextInRegion(int x, int y, int width, int height,
                                        const QString& text, int timeoutMs = -1);

    /// Fails unless cell attributes at (x, y) match expectedStyle properties.
    Q_INVOKABLE void expectStyle(int x, int y, const QJSValue& expectedStyle,
                                 int timeoutMs = -1);

    /// Compares the committed frame against a golden baseline text snapshot.
    Q_INVOKABLE void expectSnapshot(const QString& snapshotRelativePath, int timeoutMs = -1);

    /// Compares styledText() against a golden baseline, so colors and
    /// attributes are verified in addition to the characters.
    Q_INVOKABLE void expectStyled(const QString& snapshotRelativePath, int timeoutMs = -1);

    /// Fails the scenario immediately with message.
    Q_INVOKABLE void fail(const QString& message);

    /// Returns the root Terminal QML object for state inspection and automation.
    Q_INVOKABLE QJSValue rootObject();

    /// Marks the scenario successful after the JavaScript file returns normally.
    Q_INVOKABLE void finish();

signals:
    void finished(int exitCode);

private:
    // Internal callbacks reached only through the RunnerBridge facade helper:
    // Promise resolution and rejection in async scenarios, and the structured
    // operation trace.
    friend class RunnerBridge;
    void onPromiseResolved();
    void onPromiseRejected(const QString& error);
    void beginOperation(const QString& name, const QString& stack);
    void endOperation();

    enum class State {
        Starting,
        Running,
        Finishing,
        Passed,
        Failed,
    };

    struct TextMatch {
        int x = 0;
        int y = 0;
        int width = 0;
    };

    struct OperationRecord {
        int index = 0;
        QString name;
        QString status = QStringLiteral("running");
        qint64 startedMs = 0;
        qint64 elapsedMs = 0;
        QString file;
        int line = 0;
        int column = 0;
    };

    bool waitForNextFrame(int timeoutMs, bool requestRender = true);
    void advanceVirtualTime(const char* operation, int durationMs);
    void sendText(const char* operation, const QString& text, bool awaitFrame);
    void sendPaste(const char* operation, const QString& text, bool awaitFrame);
    void sendKey(const char* operation, const QString& key,
                 const QStringList& modifiers, bool awaitFrame);
    bool waitForTextCondition(const QString& text, bool shouldContain, int timeoutMs);
    // Waits for frames and scrollback commits until check() holds; the
    // description names what was awaited in the timeout message.
    bool waitForEngineCondition(const std::function<bool()>& check, int timeoutMs,
                                const QString& description);
    bool ensureScrollbackSession(const char* operation);
    QString scrollbackTextFrom(int fromRow, bool styled, QString* error) const;
    int validateTimeout(int timeoutMs);
    int resolveExpectTimeout(int timeoutMs);
    bool parseMouseArguments(const QString& button, const QStringList& modifiers,
                             int* parsedButton, Qt::KeyboardModifiers* parsedModifiers);
    bool locateItem(const QString& objectName, QRect* outRect, QString* error) const;
    bool ensureRunning(const char* operation);
    QString currentFrameText(QString* error = nullptr) const;
    QString currentStyledFrameText(QString* error = nullptr) const;
    static QString normalizedText(const TextGridBuffer& frame);
    static QString formatStyledText(const TextGridBuffer& frame);
    void compareSnapshot(const QString& snapshotRelativePath, const QString& actualText);
    QList<TextMatch> textMatches(const TextGridBuffer& frame, const QString& text) const;
    bool installJavaScriptFacade();
    void completeCurrentOperation(const QString& status);
    QString resolveSnapshotPath(const QString& path) const;
    static QString unifiedTextDiff(const QString& expectedPath,
                                   const QString& expected,
                                   const QString& actual);
    QString resolveArtifactPath(const QString& relativePath, QString* error) const;
    bool writeTextArtifact(const QString& relativePath, const QString& contents,
                           QString* error = nullptr) const;
    bool writeFinalFrameArtifact(QString* error);
    bool writeResultArtifact(int exitCode, QString* error);
    void failScenario(const QString& message, bool raiseJsError = true);
    void scheduleExit(int exitCode);
    QString formatJsError(const QJSValue& error) const;
    void startHardWatchdog();
    bool stopHardWatchdog();

    QPointer<QQmlEngine> m_qmlEngine;
    QPointer<Terminal> m_terminal;
    QPointer<TerminalEngine> m_terminalEngine;
    QString m_scriptPath;
    QString m_artifactDirectory;
    QSize m_viewport;
    int m_operationTimeoutMs = 2000;
    int m_scenarioTimeoutMs = 10000;
    QString m_configurationError;
    QString m_mode = QStringLiteral("embedded");
    QString m_temporaryScriptPath;
    QString m_message;
    QString m_finalFrameRelativePath;
    QStringList m_capturePaths;
    QStringList m_updatedSnapshots;
    bool m_updateSnapshots = false;
    std::optional<bool> m_updateSnapshotsOverride;
    QVector<OperationRecord> m_operations;
    int m_currentOperation = -1;
    QElapsedTimer m_elapsed;
    QTimer m_watchdog;
    QEventLoop* m_activeLoop = nullptr;
    State m_state = State::Starting;
    QJSValue m_terminalFacade;
    QObject* m_bridge = nullptr;
    bool m_evaluating = false;
    bool m_exitScheduled = false;

    // m_watchdog is a QTimer, so it cannot fire while a scenario blocks inside
    // synchronous JavaScript. This helper thread interrupts the JS engine
    // instead, so a runaway loop still fails with a diagnosis rather than
    // hanging.
    std::unique_ptr<RunnerHardWatchdog> m_hardWatchdog;
};

} // namespace qamui
