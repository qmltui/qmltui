#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QJSValue>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <condition_variable>
#include <mutex>
#include <thread>

class QEventLoop;
class QQmlEngine;

namespace qtui {

class Terminal;
class TerminalEngine;
class TextGridBuffer;

class TestRunner final : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TestRunner)

public:
    TestRunner(QQmlEngine* qmlEngine, Terminal* terminal, QString scriptPath,
               QString artifactDirectory, QSize viewport, int operationTimeoutMs,
               int scenarioTimeoutMs, QString configurationError);
    ~TestRunner() override;

    static TestRunner* startFromEnvironment(QQmlEngine* qmlEngine, Terminal* terminal);

    void start();

    /// Sends one key with optional Shift, Ctrl, Alt, or Meta modifiers and waits
    /// for the resulting frame to be committed.
    Q_INVOKABLE void press(const QString& key, const QStringList& modifiers = {});

    /// Sends non-empty text as one text-input event and waits for the resulting
    /// frame to be committed.
    Q_INVOKABLE void typeText(const QString& text);

    /// Processes Qt events for durationMs without blocking application timers or
    /// rendering. The duration must be between 1 and 30000 milliseconds.
    Q_INVOKABLE void sleep(int durationMs);

    /// Waits for a committed frame newer than the current frame.
    Q_INVOKABLE void waitForFrame(int timeoutMs = 2000);

    /// Sends text without waiting for a frame, so a following expectRepaint()
    /// can observe whether the application scheduled the repaint itself.
    Q_INVOKABLE void injectText(const QString& text);

    /// press() without the implicit frame wait, for use with expectRepaint().
    Q_INVOKABLE void injectKey(const QString& key, const QStringList& modifiers = {});

    /// Fails unless the application commits a new frame on its own. Unlike
    /// waitForFrame(), this never requests a render, so it verifies that a
    /// change actually invalidated the scene rather than only that the scene
    /// paints correctly when a render is forced.
    Q_INVOKABLE void expectRepaint(int timeoutMs = 2000);

    /// Sequence number of the most recently committed frame. Use it to assert
    /// that an interaction did not cause any repaint at all.
    Q_INVOKABLE int frameCount() const;

    /// Polls a synchronous JavaScript callback until it returns a truthy value.
    /// The callback receives the current frame text and elapsed milliseconds.
    Q_INVOKABLE void waitForCondition(const QJSValue& condition, int timeoutMs = 2000);

    /// Waits until the committed terminal frame contains text.
    Q_INVOKABLE void waitForText(const QString& text, int timeoutMs = 2000);

    /// Fails unless the next committed terminal frame contains text.
    Q_INVOKABLE void expectText(const QString& text);

    /// Fails unless the next committed terminal frame does not contain text.
    Q_INVOKABLE void expectNoText(const QString& text);

    /// Writes the next committed frame as normalized UTF-8 text beneath the
    /// configured artifact directory.
    Q_INVOKABLE void captureText(const QString& relativePath);

    /// Writes the next committed frame as an image beneath the configured artifact
    /// directory. This operation requires a QGuiApplication.
    Q_INVOKABLE void captureImage(const QString& relativePath);

    /// Returns normalized text for the current committed frame without waiting.
    Q_INVOKABLE QString frameText();

    /// Returns the rendered cell at (x, y), including colors and attributes.
    Q_INVOKABLE QVariantMap cell(int x, int y);

    /// Returns every visible single-row occurrence of text in row-major order.
    Q_INVOKABLE QVariantList findText(const QString& text);

    /// Clicks the center of a rendered text match and waits for the resulting frame.
    Q_INVOKABLE void clickText(const QString& text, int occurrence = 0,
                               const QString& button = QStringLiteral("left"),
                               const QStringList& modifiers = {});

    /// Sends a mouse click at (x, y) with optional button ("left", "middle", "right") and modifiers.
    Q_INVOKABLE void click(int x, int y, const QString& button = QStringLiteral("left"), const QStringList& modifiers = {});

    /// Sends a double click at (x, y) with optional button and modifiers.
    Q_INVOKABLE void doubleClick(int x, int y, const QString& button = QStringLiteral("left"), const QStringList& modifiers = {});

    /// Drags from (startX, startY) to (endX, endY) with optional button and modifiers.
    Q_INVOKABLE void drag(int startX, int startY, int endX, int endY, const QString& button = QStringLiteral("left"), const QStringList& modifiers = {});

    /// Sends a mouse wheel scroll at (x, y) with deltaX and deltaY.
    Q_INVOKABLE void scroll(int x, int y, int deltaX, int deltaY, const QStringList& modifiers = {});

    /// Waits until a committed frame matches the regex pattern.
    Q_INVOKABLE void waitForRegex(const QString& pattern, int timeoutMs = 2000);

    /// Fails unless the next committed frame matches the regex pattern.
    Q_INVOKABLE void expectRegex(const QString& pattern);

    /// Fails unless the specified sub-region contains text.
    Q_INVOKABLE void expectTextInRegion(int x, int y, int width, int height, const QString& text);

    /// Fails unless cell attributes at (x, y) match expectedStyle properties.
    Q_INVOKABLE void expectStyle(int x, int y, const QJSValue& expectedStyle);

    /// Compares the committed frame against a golden baseline text snapshot.
    Q_INVOKABLE void expectSnapshot(const QString& snapshotRelativePath);

    /// Fails the scenario immediately with message.
    Q_INVOKABLE void fail(const QString& message);

    /// Returns the root Terminal QML object for state inspection and automation.
    Q_INVOKABLE QJSValue rootObject();

    /// Marks the scenario successful after the JavaScript file returns normally.
    Q_INVOKABLE void finish();

    /// Internal callbacks for Promise resolution and rejection in async scenarios.
    Q_INVOKABLE void onPromiseResolved();
    Q_INVOKABLE void onPromiseRejected(const QString& error);

    /// Internal entry point used by the JavaScript facade for structured traces.
    Q_INVOKABLE void beginOperation(const QString& name, const QString& stack);
    Q_INVOKABLE void endOperation();

signals:
    void finished(int exitCode);

private:
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
    void sendKey(const char* operation, const QString& key,
                 const QStringList& modifiers, bool awaitFrame);
    bool waitForTextCondition(const QString& text, bool shouldContain, int timeoutMs);
    int validateTimeout(int timeoutMs);
    bool ensureRunning(const char* operation);
    QString currentFrameText(QString* error = nullptr) const;
    static QString normalizedText(const TextGridBuffer& frame);
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
    void stopHardWatchdog();

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
    QVector<OperationRecord> m_operations;
    int m_currentOperation = -1;
    QElapsedTimer m_elapsed;
    QTimer m_watchdog;
    QEventLoop* m_activeLoop = nullptr;
    State m_state = State::Starting;
    bool m_evaluating = false;
    bool m_exitScheduled = false;

    // m_watchdog is a QTimer, so it cannot fire while a scenario blocks inside
    // synchronous JavaScript. This thread interrupts the JS engine instead, so
    // a runaway loop still fails with a diagnosis rather than hanging.
    std::thread m_hardWatchdog;
    std::mutex m_hardWatchdogMutex;
    std::condition_variable m_hardWatchdogWake;
    bool m_hardWatchdogFinished = false;
    bool m_scenarioInterrupted = false;
};

} // namespace qtui
