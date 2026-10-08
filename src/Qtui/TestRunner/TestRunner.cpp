#include <Qtui/TestRunner/TestRunner.h>
#include <Qtui/Core/primitives/ColorMapper.h>

#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/engine/TerminalEngine.h>
#include <Qtui/Core/primitives/TextGridBuffer.h>

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJSValue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>

#include <utility>

namespace qtui {

namespace {

constexpr int minimumTimeoutMs = 1;
constexpr int maximumTimeoutMs = 30000;
constexpr int conditionPollIntervalMs = 10;

int environmentInteger(const char* name, int fallback, int minimum, int maximum,
                       QString* error)
{
    const QByteArray value = qgetenv(name);
    if (value.isEmpty())
        return fallback;

    bool ok = false;
    const int parsed = value.toInt(&ok);
    if (!ok || parsed < minimum || parsed > maximum) {
        if (error && error->isEmpty()) {
            *error = QStringLiteral("%1 must be an integer between %2 and %3")
                         .arg(QString::fromLatin1(name)).arg(minimum).arg(maximum);
        }
        return fallback;
    }
    return parsed;
}

struct ParsedKey {
    int key = Qt::Key_unknown;
    QString text;
};

bool parseKey(const QString& input, ParsedKey* parsed)
{
    static const QHash<QString, int> namedKeys = {
        {QStringLiteral("ENTER"), Qt::Key_Return},
        {QStringLiteral("RETURN"), Qt::Key_Return},
        {QStringLiteral("ESC"), Qt::Key_Escape},
        {QStringLiteral("ESCAPE"), Qt::Key_Escape},
        {QStringLiteral("TAB"), Qt::Key_Tab},
        {QStringLiteral("BACKTAB"), Qt::Key_Backtab},
        {QStringLiteral("BACKSPACE"), Qt::Key_Backspace},
        {QStringLiteral("SPACE"), Qt::Key_Space},
        {QStringLiteral("UP"), Qt::Key_Up},
        {QStringLiteral("DOWN"), Qt::Key_Down},
        {QStringLiteral("LEFT"), Qt::Key_Left},
        {QStringLiteral("RIGHT"), Qt::Key_Right},
        {QStringLiteral("HOME"), Qt::Key_Home},
        {QStringLiteral("END"), Qt::Key_End},
        {QStringLiteral("INSERT"), Qt::Key_Insert},
        {QStringLiteral("DELETE"), Qt::Key_Delete},
        {QStringLiteral("PAGEUP"), Qt::Key_PageUp},
        {QStringLiteral("PAGEDOWN"), Qt::Key_PageDown},
    };

    const QString trimmed = input.trimmed();
    const QString upper = trimmed.toUpper();
    if (const auto it = namedKeys.constFind(upper); it != namedKeys.cend()) {
        parsed->key = it.value();
        if (parsed->key == Qt::Key_Space)
            parsed->text = QStringLiteral(" ");
        else if (parsed->key == Qt::Key_Return)
            parsed->text = QStringLiteral("\n");
        return true;
    }

    if (upper.startsWith(QLatin1Char('F')) && upper.size() > 1) {
        bool ok = false;
        const int number = upper.sliced(1).toInt(&ok);
        if (ok && number >= 1 && number <= 35) {
            parsed->key = Qt::Key_F1 + number - 1;
            return true;
        }
        return false;
    }

    if (trimmed.size() != 1 || trimmed.front().unicode() > 0x7f)
        return false;

    const QChar character = trimmed.front();
    parsed->text = trimmed;
    if (character.isLetter())
        parsed->key = Qt::Key_A + character.toUpper().unicode() - 'A';
    else if (character.isDigit())
        parsed->key = Qt::Key_0 + character.unicode() - '0';
    else
        parsed->key = character.unicode();
    return true;
}

bool parseModifiers(const QStringList& names, Qt::KeyboardModifiers* modifiers,
                    QString* invalidModifier)
{
    for (const QString& name : names) {
        const QString upper = name.trimmed().toUpper();
        if (upper == QLatin1String("SHIFT"))
            *modifiers |= Qt::ShiftModifier;
        else if (upper == QLatin1String("CTRL") || upper == QLatin1String("CONTROL"))
            *modifiers |= Qt::ControlModifier;
        else if (upper == QLatin1String("ALT"))
            *modifiers |= Qt::AltModifier;
        else if (upper == QLatin1String("META"))
            *modifiers |= Qt::MetaModifier;
        else {
            *invalidModifier = name;
            return false;
        }
    }
    return true;
}

bool parseMouseButton(const QString& name, int* button, QString* error)
{
    const QString upper = name.trimmed().toUpper();
    if (upper == QLatin1String("LEFT") || upper.isEmpty()) {
        *button = 0;
        return true;
    }
    if (upper == QLatin1String("MIDDLE")) {
        *button = 1;
        return true;
    }
    if (upper == QLatin1String("RIGHT")) {
        *button = 2;
        return true;
    }
    if (error)
        *error = QStringLiteral("Unknown mouse button: %1").arg(name);
    return false;
}

QString absoluteOrResourcePath(const QString& path)
{
    return path.startsWith(QLatin1String(":")) ? path : QFileInfo(path).absoluteFilePath();
}

QString rgbaString(uint32_t rgba)
{
    const uint32_t argb = ColorMapper::toArgb(rgba);
    return QStringLiteral("#%1%2%3%4")
        .arg((argb >> 16) & 0xffU, 2, 16, QLatin1Char('0'))
        .arg((argb >> 8) & 0xffU, 2, 16, QLatin1Char('0'))
        .arg(argb & 0xffU, 2, 16, QLatin1Char('0'))
        .arg((argb >> 24) & 0xffU, 2, 16, QLatin1Char('0'))
        .toUpper();
}

QString attributeNames(TextAttribute attributes)
{
    static const std::pair<TextAttribute, const char*> names[] = {
        {TextAttribute::Bold, "bold"},
        {TextAttribute::Dim, "dim"},
        {TextAttribute::Italic, "italic"},
        {TextAttribute::Underline, "underline"},
        {TextAttribute::Reversed, "reverse"},
        {TextAttribute::Blink, "blink"},
        {TextAttribute::Conceal, "conceal"},
        {TextAttribute::Strikethrough, "strikethrough"},
    };
    QStringList result;
    for (const auto& [flag, name] : names) {
        if (static_cast<uint8_t>(attributes) & static_cast<uint8_t>(flag))
            result.append(QLatin1String(name));
    }
    return result.join(QLatin1Char(','));
}

bool parseRgbaString(const QString& text, uint32_t* rgba)
{
    static const QRegularExpression rgbaPattern(QStringLiteral("^#([0-9A-Fa-f]{8})$"));
    const QRegularExpressionMatch match = rgbaPattern.match(text.trimmed());
    if (match.hasMatch()) {
        bool ok = false;
        const uint32_t encoded = match.captured(1).toUInt(&ok, 16);
        if (ok) {
            const int red = (encoded >> 24) & 0xff;
            const int green = (encoded >> 16) & 0xff;
            const int blue = (encoded >> 8) & 0xff;
            const int alpha = encoded & 0xff;
            *rgba = (static_cast<uint32_t>(alpha) << 24) | (static_cast<uint32_t>(red) << 16)
                | (static_cast<uint32_t>(green) << 8) | static_cast<uint32_t>(blue);
            return true;
        }
    }
    // Named colors resolve the same way QML "color" strings do, so "red" matches
    // a cell painted with color: "red" (an ANSI palette entry, not #FF0000).
    const Color color = Color::fromString(text.trimmed());
    if (!color.isValid())
        return false;
    *rgba = color.argb32();
    return true;
}

struct TestRunnerAutoHook {
    TestRunnerAutoHook() {
        Terminal::registerTestRunnerHook([](QQmlEngine* engine, Terminal* terminal) {
            TestRunner::startFromEnvironment(engine, terminal);
        });
    }
};
static TestRunnerAutoHook s_autoHookRegistration;

} // namespace

TestRunner* TestRunner::startFromEnvironment(QQmlEngine* qmlEngine, Terminal* terminal)
{
    static QPointer<TestRunner> s_activeRunner = nullptr;
    if (s_activeRunner)
        return s_activeRunner.data();

    QString scriptPath = QString::fromUtf8(qgetenv("QTUI_SCRIPT")).trimmed();
    const QString evalCode = QString::fromUtf8(qgetenv("QTUI_EVAL")).trimmed();
    // Presence counts, not truthiness: QTUI_CAPTURE_AFTER_MS=0 is a request to
    // capture the first committed frame.
    const bool capture = !qgetenv("QTUI_CAPTURE_AFTER_MS").trimmed().isEmpty();
    const int requestCount = (scriptPath.isEmpty() ? 0 : 1)
        + (evalCode.isEmpty() ? 0 : 1) + (capture ? 1 : 0);
    if (requestCount == 0)
        return nullptr;

    QString configurationError;
    if (requestCount > 1) {
        configurationError = QStringLiteral(
            "QTUI_SCRIPT, QTUI_EVAL, and QTUI_CAPTURE_AFTER_MS are mutually exclusive");
    }

    QString mode = QStringLiteral("script");
    QString temporaryScriptPath;
    QByteArray generatedScript;
    int captureAfterMs = 0;
    if (!evalCode.isEmpty()) {
        mode = QStringLiteral("eval");
        if (!evalCode.contains(QLatin1String("main(")) && !evalCode.contains(QLatin1String("main ("))) {
            generatedScript = QStringLiteral("function main(terminal) {\n    %1;\n}\n").arg(evalCode).toUtf8();
        } else {
            generatedScript = evalCode.toUtf8();
        }
    } else if (capture) {
        mode = QStringLiteral("capture");
        captureAfterMs = environmentInteger("QTUI_CAPTURE_AFTER_MS", 0, 0, maximumTimeoutMs,
                                            &configurationError);
        generatedScript = QByteArrayLiteral("function main(terminal) {\n");
        if (captureAfterMs > 0)
            generatedScript += QStringLiteral("    terminal.sleep(%1);\n").arg(captureAfterMs).toUtf8();
        generatedScript += QByteArrayLiteral("    terminal.captureText('capture.txt');\n"
                                             "    terminal.captureStyled('capture_styled.txt');\n"
                                             "}\n");
    }

    if (!generatedScript.isEmpty() && configurationError.isEmpty()) {
        temporaryScriptPath = QDir::temp().filePath(
            QStringLiteral("qtui-%1-%2.js")
                .arg(mode).arg(QCoreApplication::applicationPid()));
        QFile tempFile(temporaryScriptPath);
        if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)
            || tempFile.write(generatedScript) != generatedScript.size()) {
            configurationError = QStringLiteral("Cannot create temporary %1 script %2: %3")
                .arg(mode, temporaryScriptPath, tempFile.errorString());
        } else {
            tempFile.close();
            scriptPath = temporaryScriptPath;
        }
    }

    QSize viewport;
    const bool hasColumns = !qgetenv("QTUI_TEST_COLUMNS").trimmed().isEmpty();
    const bool hasRows = !qgetenv("QTUI_TEST_ROWS").trimmed().isEmpty();
    if (hasColumns || hasRows) {
        const int columns = environmentInteger("QTUI_TEST_COLUMNS", 80, 1, 1000,
                                               &configurationError);
        const int rows = environmentInteger("QTUI_TEST_ROWS", 24, 1, 1000,
                                            &configurationError);
        viewport = QSize(columns, rows);
    }

    const int scenarioTimeout = environmentInteger("QTUI_TEST_TIMEOUT_MS", 10000,
                                                    minimumTimeoutMs, maximumTimeoutMs,
                                                    &configurationError);
    const int operationTimeout = environmentInteger("QTUI_TEST_OPERATION_TIMEOUT_MS", 2000,
                                                     minimumTimeoutMs, maximumTimeoutMs,
                                                     &configurationError);
    // Fail up front instead of letting the watchdog kill the sleep: the capture
    // still needs one frame wait after the delay.
    if (capture && configurationError.isEmpty()
        && captureAfterMs + operationTimeout > scenarioTimeout) {
        configurationError = QStringLiteral(
            "QTUI_CAPTURE_AFTER_MS (%1) plus QTUI_TEST_OPERATION_TIMEOUT_MS (%2) must not exceed "
            "QTUI_TEST_TIMEOUT_MS (%3)").arg(captureAfterMs).arg(operationTimeout).arg(scenarioTimeout);
    }
    QString artifactDirectory = QString::fromUtf8(qgetenv("QTUI_TEST_ARTIFACT_DIR")).trimmed();
    if (artifactDirectory.isEmpty()) {
        artifactDirectory = QDir::temp().filePath(
            QStringLiteral("qtui-test-%1").arg(QCoreApplication::applicationPid()));
    }

    auto* runner = new TestRunner(qmlEngine, terminal, absoluteOrResourcePath(scriptPath),
                                  QFileInfo(artifactDirectory).absoluteFilePath(),
                                  viewport, operationTimeout, scenarioTimeout,
                                  configurationError);
    runner->m_mode = mode;
    runner->m_temporaryScriptPath = temporaryScriptPath;
    s_activeRunner = runner;
    QTimer::singleShot(0, runner, &TestRunner::start);
    return runner;
}

TestRunner::TestRunner(QQmlEngine* qmlEngine, Terminal* terminal, QString scriptPath,
                       QString artifactDirectory, QSize viewport, int operationTimeoutMs,
                       int scenarioTimeoutMs, QString configurationError)
    : QObject(QCoreApplication::instance())
    , m_qmlEngine(qmlEngine)
    , m_terminal(terminal)
    , m_terminalEngine(terminal ? terminal->engine() : nullptr)
    , m_scriptPath(std::move(scriptPath))
    , m_artifactDirectory(std::move(artifactDirectory))
    , m_viewport(viewport)
    , m_operationTimeoutMs(operationTimeoutMs)
    , m_scenarioTimeoutMs(scenarioTimeoutMs)
    , m_configurationError(std::move(configurationError))
{
    // Apply the deterministic viewport before the event loop can commit the
    // Terminal's queued physical-size frame. Otherwise a live test runner can
    // paint that frame before overlaying the smaller test viewport.
    if (m_terminalEngine && m_viewport.isValid() && !m_viewport.isEmpty())
        m_terminalEngine->setTestViewportSize(m_viewport);

    m_watchdog.setSingleShot(true);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        failScenario(QStringLiteral("Scenario exceeded the %1 ms watchdog")
                         .arg(m_scenarioTimeoutMs));
    });
}

TestRunner::~TestRunner()
{
    // A joinable std::thread would terminate the process on destruction.
    stopHardWatchdog();
}

void TestRunner::start()
{
    m_elapsed.start();
    if (!QDir().mkpath(m_artifactDirectory)) {
        m_configurationError = QStringLiteral("Cannot create artifact directory %1")
            .arg(m_artifactDirectory);
    }
    if (!m_configurationError.isEmpty()) {
        failScenario(m_configurationError, false);
        return;
    }
    if (!m_qmlEngine || !m_terminal || !m_terminalEngine) {
        failScenario(QStringLiteral("TestRunner requires a live QQmlEngine and explicit Terminal"),
                     false);
        return;
    }

    QFile scriptFile(m_scriptPath);
    if (!scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        failScenario(QStringLiteral("Cannot open scenario %1: %2")
                         .arg(m_scriptPath, scriptFile.errorString()), false);
        return;
    }
    const QByteArray scriptBytes = scriptFile.readAll();
    if (scriptFile.error() != QFileDevice::NoError) {
        failScenario(QStringLiteral("Cannot read scenario %1: %2")
                         .arg(m_scriptPath, scriptFile.errorString()), false);
        return;
    }
    // A synchronous scenario finishes inside evaluate() below and removes a
    // temporary script; Windows refuses to delete a file that is still open.
    scriptFile.close();

    connect(m_qmlEngine, &QObject::destroyed, this, [this] {
        failScenario(QStringLiteral("QQmlEngine was destroyed during the scenario"), false);
    });
    connect(m_terminal, &QObject::destroyed, this, [this] {
        failScenario(QStringLiteral("Target Terminal was destroyed during the scenario"), false);
    });
    connect(m_terminalEngine, &TerminalEngine::renderFailed, this,
            [this](const QString& message) { failScenario(message); });

    m_watchdog.start(m_scenarioTimeoutMs);
    if (!waitForNextFrame(m_operationTimeoutMs))
        return;

    m_state = State::Running;
    if (!installJavaScriptFacade())
        return;

    m_evaluating = true;
    startHardWatchdog();
    const QJSValue evalResult = m_qmlEngine->evaluate(QString::fromUtf8(scriptBytes), m_scriptPath);
    if (evalResult.isError()) {
        stopHardWatchdog();
        m_evaluating = false;
        failScenario(formatJsError(evalResult), false);
        return;
    }

    QJSValue mainFunc = m_qmlEngine->globalObject().property(QStringLiteral("main"));
    if (!mainFunc.isCallable()) {
        stopHardWatchdog();
        m_evaluating = false;
        failScenario(QStringLiteral("Scenario must define a function main(terminal)"), false);
        return;
    }

    const QJSValue result = mainFunc.call({ m_terminalFacade });
    m_qmlEngine->globalObject().deleteProperty(QStringLiteral("main"));

    const bool interrupted = [this] {
        std::lock_guard<std::mutex> lock(m_hardWatchdogMutex);
        return m_scenarioInterrupted;
    }();
    stopHardWatchdog();
    m_evaluating = false;

    if (interrupted) {
        failScenario(QStringLiteral("Scenario did not yield to the event loop within %1 ms; "
                                    "JavaScript execution was interrupted")
                         .arg(m_scenarioTimeoutMs),
                     false);
        return;
    }
    if (m_state == State::Failed)
        return;
    if (result.isError()) {
        failScenario(formatJsError(result), false);
        return;
    }

    if (result.isObject() && result.property(QStringLiteral("then")).isCallable()) {
        QJSValue promiseHandler = m_qmlEngine->evaluate(QStringLiteral(
            "(function(promise, nativeRunner) {"
            "  promise.then(function() { nativeRunner.onPromiseResolved(); })"
            "         .catch(function(err) { nativeRunner.onPromiseRejected(err ? String(err) : 'Promise rejected'); });"
            "})"
        ));
        promiseHandler.call({result, m_qmlEngine->newQObject(this)});
        return;
    }

    if (m_state == State::Finishing || m_state == State::Running) {
        m_state = State::Passed;
        m_watchdog.stop();
        scheduleExit(0);
        return;
    }
}

bool TestRunner::installJavaScriptFacade()
{
    const QJSValue nativeRunner = m_qmlEngine->newQObject(this);
    m_qmlEngine->globalObject().setProperty(QStringLiteral("__QtuiTestNative"), nativeRunner);
    const QString facadeSource = QStringLiteral(R"JS(
(function(nativeRunner) {
    const methodNames = [
        "press", "typeText", "sleep", "waitForFrame", "waitForCondition",
        "waitForText", "expectText", "expectNoText", "captureText",
        "captureStyled", "frameText", "styledText", "cell", "findText", "clickText",
        "click", "doubleClick", "drag", "scroll", "waitForRegex",
        "expectRegex", "expectTextInRegion", "expectStyle", "expectSnapshot",
        "expectStyled",
        "expectRepaint", "frameCount", "frameStats", "injectText", "injectKey",
        "fail", "rootObject", "finish"
    ];
    const facade = {};
    methodNames.forEach(function(name) {
        facade[name] = function() {
            nativeRunner.beginOperation(name, String(new Error().stack || ""));
            const result = nativeRunner[name].apply(nativeRunner, arguments);
            // Skipped when the call throws, so a failing operation stays open
            // and is the one blamed in the result.
            nativeRunner.endOperation();
            return result;
        };
    });
    delete this.__QtuiTestNative;
    return facade;
})(__QtuiTestNative);
)JS");
    m_terminalFacade = m_qmlEngine->evaluate(facadeSource,
                                             QStringLiteral("qtui-test-facade.js"));
    if (m_terminalFacade.isError()) {
        failScenario(QStringLiteral("Cannot install Terminal JavaScript facade: %1")
                         .arg(formatJsError(m_terminalFacade)), false);
        return false;
    }
    return true;
}

void TestRunner::beginOperation(const QString& name, const QString& stack)
{
    if (m_state != State::Running && m_state != State::Finishing)
        return;
    completeCurrentOperation(QStringLiteral("passed"));

    OperationRecord record;
    record.index = m_operations.size() + 1;
    record.name = name;
    record.startedMs = m_elapsed.isValid() ? m_elapsed.elapsed() : 0;

    const QRegularExpression locationPattern(
        QStringLiteral("(?:at\\s+)?([^\\n]+?):(\\d+)(?::(\\d+))?"));
    QRegularExpressionMatchIterator matches = locationPattern.globalMatch(stack);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        const QString file = match.captured(1).trimmed().section(QLatin1Char('@'), -1);
        if (file.contains(QLatin1String("qtui-test-facade.js")))
            continue;
        record.file = file;
        record.line = match.captured(2).toInt();
        record.column = match.captured(3).toInt();
        break;
    }

    m_operations.append(record);
    m_currentOperation = m_operations.size() - 1;
}

void TestRunner::startHardWatchdog()
{
    if (m_hardWatchdog.joinable() || !m_qmlEngine)
        return;
    // Give the event-loop watchdog a chance to report first: it produces a much
    // better message ("Timed out after N ms waiting for ...") and only fails to
    // fire when JavaScript never yields.
    const auto deadline = std::chrono::milliseconds(m_scenarioTimeoutMs + 1000);
    QPointer<QQmlEngine> engine = m_qmlEngine;
    m_hardWatchdog = std::thread([this, engine, deadline] {
        std::unique_lock<std::mutex> lock(m_hardWatchdogMutex);
        if (m_hardWatchdogWake.wait_for(lock, deadline,
                                        [this] { return m_hardWatchdogFinished; })) {
            return; // scenario finished in time
        }
        m_scenarioInterrupted = true;
        lock.unlock();
        // Documented as thread safe, and intended for exactly this case.
        if (engine)
            engine->setInterrupted(true);
    });
}

void TestRunner::stopHardWatchdog()
{
    {
        std::lock_guard<std::mutex> lock(m_hardWatchdogMutex);
        m_hardWatchdogFinished = true;
    }
    m_hardWatchdogWake.notify_all();
    if (m_hardWatchdog.joinable())
        m_hardWatchdog.join();
    if (m_qmlEngine && m_qmlEngine->isInterrupted())
        m_qmlEngine->setInterrupted(false);
}

void TestRunner::endOperation()
{
    // The facade calls this once a terminal.* call has returned normally. Closing
    // the record here means a later failure — a JavaScript error on a following
    // line, or the scenario watchdog — is no longer attributed to an operation
    // that already succeeded.
    completeCurrentOperation(QStringLiteral("passed"));
    m_currentOperation = -1;
}

void TestRunner::completeCurrentOperation(const QString& status)
{
    if (m_currentOperation < 0 || m_currentOperation >= m_operations.size())
        return;
    OperationRecord& record = m_operations[m_currentOperation];
    if (record.status == QLatin1String("running")) {
        record.status = status;
        record.elapsedMs = qMax<qint64>(0, m_elapsed.elapsed() - record.startedMs);
    }
}

bool TestRunner::ensureRunning(const char* operation)
{
    if (m_state == State::Running)
        return true;
    if (m_state != State::Failed) {
        failScenario(QStringLiteral("terminal.%1() is not valid in the current scenario state")
                         .arg(QString::fromLatin1(operation)));
    }
    return false;
}

int TestRunner::validateTimeout(int timeoutMs)
{
    if (timeoutMs >= minimumTimeoutMs && timeoutMs <= maximumTimeoutMs)
        return timeoutMs;
    failScenario(QStringLiteral("Timeout must be between %1 and %2 ms")
                     .arg(minimumTimeoutMs).arg(maximumTimeoutMs));
    return 0;
}

bool TestRunner::waitForNextFrame(int timeoutMs, bool requestRender)
{
    if (!m_terminalEngine)
        return false;
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested wait operations are not supported"));
        return false;
    }
    if (timeoutMs < minimumTimeoutMs || timeoutMs > maximumTimeoutMs) {
        failScenario(QStringLiteral("Timeout must be between %1 and %2 ms")
                         .arg(minimumTimeoutMs).arg(maximumTimeoutMs));
        return false;
    }

    const quint64 baseline = m_terminalEngine->frameSequence();
    bool received = false;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    const QMetaObject::Connection frameConnection = connect(
        m_terminalEngine, &TerminalEngine::frameRendered, &loop,
        [&](quint64 sequence) {
            if (sequence > baseline) {
                received = true;
                loop.quit();
            }
        });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    timeout.start(timeoutMs);
    if (requestRender)
        m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);

    if (m_state == State::Failed)
        return false;
    if (!received) {
        failScenario(requestRender
                         ? QStringLiteral("Timed out after %1 ms waiting for a committed frame")
                               .arg(timeoutMs)
                         : QStringLiteral("Timed out after %1 ms waiting for the application to "
                                          "schedule a repaint")
                               .arg(timeoutMs));
        return false;
    }
    return true;
}

bool TestRunner::waitForTextCondition(const QString& text, bool shouldContain, int timeoutMs)
{
    if (text.isEmpty()) {
        failScenario(QStringLiteral("Text conditions require a non-empty string"));
        return false;
    }
    QString error;
    const auto matches = [&] {
        const QString frame = currentFrameText(&error);
        return error.isEmpty() && (frame.contains(text) == shouldContain);
    };
    if (matches())
        return true;
    if (!error.isEmpty()) {
        failScenario(error);
        return false;
    }

    QDeadlineTimer deadline(timeoutMs);
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested Test wait operations are not supported"));
        return false;
    }

    bool matched = false;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    const QMetaObject::Connection frameConnection = connect(
        m_terminalEngine, &TerminalEngine::frameRendered, &loop,
        [&](quint64) {
            if (matches()) {
                matched = true;
                loop.quit();
            } else if (!error.isEmpty()) {
                loop.quit();
            }
        });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    timeout.start(static_cast<int>(qMax<qint64>(1, deadline.remainingTime())));
    m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);

    if (m_state == State::Failed)
        return false;
    if (!error.isEmpty()) {
        failScenario(error);
        return false;
    }
    if (!matched) {
        failScenario(QStringLiteral("Timed out after %1 ms waiting for text \"%2\"")
                         .arg(timeoutMs).arg(text));
        return false;
    }
    return true;
}

void TestRunner::press(const QString& key, const QStringList& modifiers)
{
    sendKey("press", key, modifiers, /*awaitFrame=*/true);
}

void TestRunner::injectKey(const QString& key, const QStringList& modifiers)
{
    sendKey("injectKey", key, modifiers, /*awaitFrame=*/false);
}

void TestRunner::sendKey(const char* operation, const QString& key,
                         const QStringList& modifiers, bool awaitFrame)
{
    if (!ensureRunning(operation))
        return;

    ParsedKey parsed;
    if (!parseKey(key, &parsed)) {
        failScenario(QStringLiteral("Unknown or unsupported key: %1").arg(key));
        return;
    }
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    QString invalidModifier;
    if (!parseModifiers(modifiers, &parsedModifiers, &invalidModifier)) {
        failScenario(QStringLiteral("Unknown modifier: %1").arg(invalidModifier));
        return;
    }

    if (parsed.key == Qt::Key_Backtab)
        parsedModifiers |= Qt::ShiftModifier;
    if (parsedModifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
        parsed.text.clear();
    } else if ((parsedModifiers & Qt::ShiftModifier) && parsed.text.size() == 1
               && parsed.text.front().isLetter()) {
        parsed.text = parsed.text.toUpper();
    }

    m_terminalEngine->handleKeyInput(parsed.key, parsed.text, parsedModifiers);
    if (awaitFrame)
        (void)waitForNextFrame(m_operationTimeoutMs);
}

void TestRunner::typeText(const QString& text)
{
    sendText("typeText", text, /*awaitFrame=*/true);
}

void TestRunner::injectText(const QString& text)
{
    // Deliberately does not wait for a frame: typeText() would request one and
    // consume the repaint that expectRepaint() needs to observe.
    sendText("injectText", text, /*awaitFrame=*/false);
}

void TestRunner::sendText(const char* operation, const QString& text, bool awaitFrame)
{
    if (!ensureRunning(operation))
        return;
    if (text.isEmpty()) {
        failScenario(QStringLiteral("terminal.%1() requires non-empty text")
                         .arg(QLatin1String(operation)));
        return;
    }

    // The bytes take the same path as a real keyboard: the input decoder turns
    // them into one key event per character with the key code a terminal
    // keystroke carries, so a Keys handler matching Qt.Key_W or Qt.Key_1 sees
    // scripted typing exactly as it sees a user typing, and every future
    // decoder change applies to scripts without a parallel table here.
    m_terminalEngine->feedInput(text.toUtf8());
    if (awaitFrame)
        (void)waitForNextFrame(m_operationTimeoutMs);
}

void TestRunner::click(int x, int y, const QString& button, const QStringList& modifiers)
{
    if (!ensureRunning("click"))
        return;
    int b = 0;
    QString error;
    if (!parseMouseButton(button, &b, &error)) {
        failScenario(error);
        return;
    }
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseModifiers(modifiers, &parsedModifiers, &error)) {
        failScenario(error);
        return;
    }
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void TestRunner::doubleClick(int x, int y, const QString& button, const QStringList& modifiers)
{
    if (!ensureRunning("doubleClick"))
        return;
    int b = 0;
    QString error;
    if (!parseMouseButton(button, &b, &error)) {
        failScenario(error);
        return;
    }
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseModifiers(modifiers, &parsedModifiers, &error)) {
        failScenario(error);
        return;
    }
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void TestRunner::drag(int startX, int startY, int endX, int endY, const QString& button, const QStringList& modifiers)
{
    if (!ensureRunning("drag"))
        return;
    int b = 0;
    QString error;
    if (!parseMouseButton(button, &b, &error)) {
        failScenario(error);
        return;
    }
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseModifiers(modifiers, &parsedModifiers, &error)) {
        failScenario(error);
        return;
    }
    m_terminalEngine->handleMouseInput(b, startX, startY, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, endX, endY, false, true, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, endX, endY, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void TestRunner::scroll(int x, int y, int deltaX, int deltaY, const QStringList& modifiers)
{
    if (!ensureRunning("scroll"))
        return;
    QString error;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseModifiers(modifiers, &parsedModifiers, &error)) {
        failScenario(error);
        return;
    }
    m_terminalEngine->handleMouseInput(0, x, y, false, false, parsedModifiers, deltaX, deltaY);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void TestRunner::waitForRegex(const QString& pattern, int timeoutMs)
{
    if (!ensureRunning("waitForRegex"))
        return;
    const int validated = validateTimeout(timeoutMs);
    if (!validated)
        return;

    const QRegularExpression re(pattern);
    if (!re.isValid()) {
        failScenario(QStringLiteral("Invalid regex pattern: %1 (%2)").arg(pattern, re.errorString()));
        return;
    }

    QString error;
    const auto matches = [&] {
        const QString frame = currentFrameText(&error);
        return error.isEmpty() && re.match(frame).hasMatch();
    };

    if (matches())
        return;

    QDeadlineTimer deadline(validated);
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested Test wait operations are not supported"));
        return;
    }

    bool matched = false;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    const QMetaObject::Connection frameConnection = connect(
        m_terminalEngine, &TerminalEngine::frameRendered, &loop,
        [&](quint64) {
            if (matches()) {
                matched = true;
                loop.quit();
            }
        });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    timeout.start(static_cast<int>(qMax<qint64>(1, deadline.remainingTime())));
    m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);

    if (m_state == State::Failed)
        return;
    if (!matched) {
        failScenario(QStringLiteral("Timed out after %1 ms waiting for regex pattern \"%2\"")
                         .arg(validated).arg(pattern));
    }
}

void TestRunner::expectRegex(const QString& pattern)
{
    if (!ensureRunning("expectRegex") || !waitForNextFrame(m_operationTimeoutMs))
        return;
    const QRegularExpression re(pattern);
    if (!re.isValid()) {
        failScenario(QStringLiteral("Invalid regex pattern: %1 (%2)").arg(pattern, re.errorString()));
        return;
    }
    QString error;
    const QString frame = currentFrameText(&error);
    if (!error.isEmpty())
        failScenario(error);
    else if (!re.match(frame).hasMatch())
        failScenario(QStringLiteral("Expected frame to match regex \"%1\"").arg(pattern));
}

void TestRunner::expectTextInRegion(int x, int y, int width, int height, const QString& text)
{
    if (!ensureRunning("expectTextInRegion") || !waitForNextFrame(m_operationTimeoutMs))
        return;
    if (!m_terminalEngine) {
        failScenario(QStringLiteral("TerminalEngine is no longer available"));
        return;
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return;
    }
    if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > frame->width() || y + height > frame->height()) {
        failScenario(QStringLiteral("Region bounds [%1,%2,%3,%4] out of frame size %5x%6")
                         .arg(x).arg(y).arg(width).arg(height).arg(frame->width()).arg(frame->height()));
        return;
    }

    QString regionText;
    for (int r = y; r < y + height; ++r) {
        QString line;
        for (int c = x; c < x + width; ++c) {
            const Cell& cell = frame->cellAt(c, r);
            if (cell.continuation) continue;
            line.append(cell.character().isEmpty() ? QStringLiteral(" ") : QString::fromUtf8(cell.character()));
        }
        regionText.append(line);
        regionText.append(QLatin1Char('\n'));
    }

    if (!regionText.contains(text)) {
        failScenario(QStringLiteral("Expected region [%1,%2,%3,%4] to contain \"%5\"")
                         .arg(x).arg(y).arg(width).arg(height).arg(text));
    }
}

void TestRunner::expectStyle(int x, int y, const QJSValue& expectedStyle)
{
    if (!ensureRunning("expectStyle") || !waitForNextFrame(m_operationTimeoutMs))
        return;
    if (!m_terminalEngine) {
        failScenario(QStringLiteral("TerminalEngine is no longer available"));
        return;
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return;
    }
    if (x < 0 || y < 0 || x >= frame->width() || y >= frame->height()) {
        failScenario(QStringLiteral("Coordinates (%1,%2) out of frame bounds %3x%4")
                         .arg(x).arg(y).arg(frame->width()).arg(frame->height()));
        return;
    }
    if (!expectedStyle.isObject()) {
        failScenario(QStringLiteral("expectStyle requires an object with expected cell attributes"));
        return;
    }

    const Cell& cell = frame->cellAt(x, y);

    if (expectedStyle.hasProperty(QStringLiteral("bold"))) {
        const bool expectedBold = expectedStyle.property(QStringLiteral("bold")).toBool();
        const bool actualBold = (static_cast<uint8_t>(cell.attributes) & static_cast<uint8_t>(TextAttribute::Bold)) != 0;
        if (expectedBold != actualBold) {
            failScenario(QStringLiteral("Cell at (%1,%2) bold expected %3 but got %4").arg(x).arg(y).arg(expectedBold).arg(actualBold));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("dim"))) {
        const bool expectedDim = expectedStyle.property(QStringLiteral("dim")).toBool();
        const bool actualDim = (static_cast<uint8_t>(cell.attributes) & static_cast<uint8_t>(TextAttribute::Dim)) != 0;
        if (expectedDim != actualDim) {
            failScenario(QStringLiteral("Cell at (%1,%2) dim expected %3 but got %4").arg(x).arg(y).arg(expectedDim).arg(actualDim));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("italic"))) {
        const bool expectedItalic = expectedStyle.property(QStringLiteral("italic")).toBool();
        const bool actualItalic = (static_cast<uint8_t>(cell.attributes) & static_cast<uint8_t>(TextAttribute::Italic)) != 0;
        if (expectedItalic != actualItalic) {
            failScenario(QStringLiteral("Cell at (%1,%2) italic expected %3 but got %4").arg(x).arg(y).arg(expectedItalic).arg(actualItalic));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("underline"))) {
        const bool expectedUnderline = expectedStyle.property(QStringLiteral("underline")).toBool();
        const bool actualUnderline = (static_cast<uint8_t>(cell.attributes) & static_cast<uint8_t>(TextAttribute::Underline)) != 0;
        if (expectedUnderline != actualUnderline) {
            failScenario(QStringLiteral("Cell at (%1,%2) underline expected %3 but got %4").arg(x).arg(y).arg(expectedUnderline).arg(actualUnderline));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("reverse")) || expectedStyle.hasProperty(QStringLiteral("reversed"))) {
        const bool expectedRev = expectedStyle.hasProperty(QStringLiteral("reverse")) ?
            expectedStyle.property(QStringLiteral("reverse")).toBool() : expectedStyle.property(QStringLiteral("reversed")).toBool();
        const bool actualRev = (static_cast<uint8_t>(cell.attributes) & static_cast<uint8_t>(TextAttribute::Reversed)) != 0;
        if (expectedRev != actualRev) {
            failScenario(QStringLiteral("Cell at (%1,%2) reverse expected %3 but got %4").arg(x).arg(y).arg(expectedRev).arg(actualRev));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("character"))) {
        const QString expectedChar = expectedStyle.property(QStringLiteral("character")).toString();
        const QString actualChar = QString::fromUtf8(cell.character());
        if (expectedChar != actualChar) {
            failScenario(QStringLiteral("Cell at (%1,%2) character expected \"%3\" but got \"%4\"").arg(x).arg(y).arg(expectedChar).arg(actualChar));
            return;
        }
    }
    const auto compareColor = [&](const QString& property, uint32_t actual) {
        if (!expectedStyle.hasProperty(property))
            return true;
        const QString expectedText = expectedStyle.property(property).toString();
        uint32_t expectedRgba = 0;
        if (!parseRgbaString(expectedText, &expectedRgba)) {
            failScenario(QStringLiteral("Cell style %1 is not a valid color: %2")
                             .arg(property, expectedText));
            return false;
        }
        if (ColorMapper::toArgb(expectedRgba) != ColorMapper::toArgb(actual)) {
            failScenario(QStringLiteral("Cell at (%1,%2) %3 expected %4 but got %5")
                             .arg(x).arg(y).arg(property, expectedText, rgbaString(actual)));
            return false;
        }
        return true;
    };
    if (!compareColor(QStringLiteral("foreground"), cell.fgColor)
        || !compareColor(QStringLiteral("background"), cell.bgColor)) {
        return;
    }
}

void TestRunner::expectSnapshot(const QString& snapshotRelativePath)
{
    if (!ensureRunning("expectSnapshot") || !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString actualText = currentFrameText(&error);
    if (!error.isEmpty()) {
        failScenario(error);
        return;
    }
    compareSnapshot(snapshotRelativePath, actualText);
}

void TestRunner::expectStyled(const QString& snapshotRelativePath)
{
    if (!ensureRunning("expectStyled") || !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString actualText = currentStyledFrameText(&error);
    if (!error.isEmpty()) {
        failScenario(error);
        return;
    }
    compareSnapshot(snapshotRelativePath, actualText);
}

void TestRunner::compareSnapshot(const QString& snapshotRelativePath, const QString& actualText)
{
    const QString snapshotPath = resolveSnapshotPath(snapshotRelativePath);
    QFile snapshotFile(snapshotPath);
    if (!snapshotFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        failScenario(QStringLiteral("Cannot open snapshot file %1: %2")
                         .arg(snapshotPath, snapshotFile.errorString()));
        return;
    }
    const QString expectedText = QString::fromUtf8(snapshotFile.readAll());
    snapshotFile.close();

    if (actualText != expectedText) {
        // Derive the name from the whole relative path, not just the file name,
        // so two snapshots called a/frame.txt and b/frame.txt do not overwrite
        // each other's .actual.txt / .diff artifacts. The conventional
        // "snapshots/" prefix is dropped because the artifacts already live
        // under snapshots/.
        QString baseName = snapshotRelativePath;
        if (baseName.startsWith(QLatin1String("snapshots/")))
            baseName = baseName.mid(QStringLiteral("snapshots/").size());
        if (const int dot = baseName.lastIndexOf(QLatin1Char('.')); dot > 0)
            baseName.truncate(dot);
        baseName.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")),
                         QStringLiteral("_"));
        baseName = baseName.mid(0, 120);
        if (baseName.isEmpty())
            baseName = QStringLiteral("snapshot");
        const QString actualArtifactPath = QStringLiteral("snapshots/%1.actual.txt").arg(baseName);
        const QString diffArtifactPath = QStringLiteral("snapshots/%1.diff").arg(baseName);
        QString writeError;
        if (!writeTextArtifact(actualArtifactPath, actualText, &writeError)
            || !writeTextArtifact(diffArtifactPath,
                                  unifiedTextDiff(snapshotPath, expectedText, actualText),
                                  &writeError)) {
            failScenario(writeError);
            return;
        }
        m_capturePaths.append(actualArtifactPath);
        m_capturePaths.append(diffArtifactPath);
        failScenario(QStringLiteral(
            "Snapshot mismatch for %1 (actual: %2, diff: %3)")
                         .arg(snapshotPath, actualArtifactPath, diffArtifactPath));
    }
}

void TestRunner::sleep(int durationMs)
{
    if (!ensureRunning("sleep"))
        return;
    const int validated = validateTimeout(durationMs);
    if (!validated)
        return;
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested Test wait operations are not supported"));
        return;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    timer.start(validated);
    loop.exec();
    m_activeLoop = nullptr;
}

void TestRunner::waitForFrame(int timeoutMs)
{
    if (!ensureRunning("waitForFrame"))
        return;
    const int validated = validateTimeout(timeoutMs);
    if (validated)
        (void)waitForNextFrame(validated);
}

void TestRunner::expectRepaint(int timeoutMs)
{
    if (!ensureRunning("expectRepaint"))
        return;
    const int validated = validateTimeout(timeoutMs);
    if (!validated)
        return;
    // Deliberately no draining of an in-flight render here: after an inject*
    // call the pending render *is* the repaint the application scheduled, and
    // consuming it would make this assertion vacuous.
    (void)waitForNextFrame(validated, /*requestRender=*/false);
}

int TestRunner::frameCount() const
{
    return m_terminalEngine ? static_cast<int>(m_terminalEngine->frameSequence()) : -1;
}

QVariantMap TestRunner::frameStats() const
{
    QVariantMap stats;
    if (!m_terminalEngine)
        return stats;
    const FrameStats last = m_terminalEngine->lastFrameStats();
    const TerminalEngine::FrameStatsTotals totals = m_terminalEngine->frameStatsTotals();
    const qint64 frames = static_cast<qint64>(totals.frames);
    stats.insert(QStringLiteral("frame"), static_cast<qint64>(last.sequence));
    stats.insert(QStringLiteral("paintMicros"), last.paintMicros);
    stats.insert(QStringLiteral("diffMicros"), last.diffMicros);
    stats.insert(QStringLiteral("writeMicros"), last.writeMicros);
    stats.insert(QStringLiteral("diffBytes"), last.diffBytes);
    stats.insert(QStringLiteral("bytesWritten"), last.bytesWritten);
    stats.insert(QStringLiteral("dirtyDuringPaint"), last.dirtyDuringPaint);
    stats.insert(QStringLiteral("frames"), frames);
    stats.insert(QStringLiteral("paintMicrosAvg"),
                 frames > 0 ? totals.paintMicrosTotal / frames : qint64(0));
    stats.insert(QStringLiteral("paintMicrosMax"), totals.paintMicrosMax);
    stats.insert(QStringLiteral("diffMicrosAvg"),
                 frames > 0 ? totals.diffMicrosTotal / frames : qint64(0));
    stats.insert(QStringLiteral("bytesTotal"), totals.bytesTotal);
    stats.insert(QStringLiteral("dirtyDuringPaintTotal"), totals.dirtyDuringPaintTotal);
    stats.insert(QStringLiteral("queuedBytes"), last.queuedBytes);
    stats.insert(QStringLiteral("skippedForBackpressure"), totals.skippedForBackpressure);
    return stats;
}

void TestRunner::waitForCondition(const QJSValue& condition, int timeoutMs)
{
    if (!ensureRunning("waitForCondition"))
        return;
    if (!condition.isCallable()) {
        failScenario(QStringLiteral("terminal.waitForCondition() requires a callable condition"));
        return;
    }
    const int validated = validateTimeout(timeoutMs);
    if (!validated)
        return;
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested wait operations are not supported"));
        return;
    }

    QElapsedTimer elapsed;
    elapsed.start();
    QEventLoop loop;
    QTimer pollTimer;
    pollTimer.setTimerType(Qt::PreciseTimer);
    pollTimer.setInterval(conditionPollIntervalMs);
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    bool matched = false;

    const auto evaluateCondition = [&] {
        QString error;
        const QString frameText = currentFrameText(&error);
        if (!error.isEmpty()) {
            failScenario(error);
            return;
        }

        const QJSValue result = condition.call(
            {QJSValue(frameText), QJSValue(static_cast<double>(elapsed.elapsed()))});
        if (result.isError()) {
            failScenario(formatJsError(result));
            return;
        }
        if (result.toBool()) {
            matched = true;
            loop.quit();
        }
    };

    connect(&pollTimer, &QTimer::timeout, &loop, evaluateCondition);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    evaluateCondition();
    if (!matched && m_state == State::Running) {
        pollTimer.start();
        timeoutTimer.start(validated);
        m_terminalEngine->requestRender();
        loop.exec();
    }
    m_activeLoop = nullptr;

    if (m_state == State::Failed || matched)
        return;
    failScenario(QStringLiteral("Timed out after %1 ms waiting for condition").arg(validated));
}

void TestRunner::waitForText(const QString& text, int timeoutMs)
{
    if (!ensureRunning("waitForText"))
        return;
    const int validated = validateTimeout(timeoutMs);
    if (validated)
        (void)waitForTextCondition(text, true, validated);
}

void TestRunner::expectText(const QString& text)
{
    if (!ensureRunning("expectText") || !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString frame = currentFrameText(&error);
    if (!error.isEmpty())
        failScenario(error);
    else if (text.isEmpty() || !frame.contains(text))
        failScenario(QStringLiteral("Expected frame to contain \"%1\"").arg(text));
}

void TestRunner::expectNoText(const QString& text)
{
    if (!ensureRunning("expectNoText") || !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString frame = currentFrameText(&error);
    if (!error.isEmpty())
        failScenario(error);
    else if (text.isEmpty() || frame.contains(text))
        failScenario(QStringLiteral("Expected frame not to contain \"%1\"").arg(text));
}

QString TestRunner::normalizedText(const TextGridBuffer& frame)
{
    QString result;
    for (int row = 0; row < frame.height(); ++row) {
        QString line;
        for (int column = 0; column < frame.width(); ++column) {
            const Cell& cell = frame.cellAt(column, row);
            if (cell.continuation)
                continue;
            line.append(cell.character().isEmpty() ? QStringLiteral(" ")
                                                  : QString::fromUtf8(cell.character()));
        }
        while (line.endsWith(QLatin1Char(' ')))
            line.chop(1);
        result.append(line);
        result.append(QLatin1Char('\n'));
    }
    return result;
}

QString TestRunner::formatStyledText(const TextGridBuffer& frame)
{
    QString result = normalizedText(frame);
    result.append(QStringLiteral("--- styles\n"));
    const auto sameStyle = [](const Cell& lhs, const Cell& rhs) {
        return lhs.fgColor == rhs.fgColor && lhs.bgColor == rhs.bgColor
            && lhs.attributes == rhs.attributes;
    };
    for (int row = 0; row < frame.height(); ++row) {
        int column = 0;
        while (column < frame.width()) {
            const Cell& first = frame.cellAt(column, row);
            int end = column + 1;
            while (end < frame.width() && sameStyle(first, frame.cellAt(end, row)))
                ++end;
            // Runs that use the terminal defaults everywhere carry no information
            // and would bury the interesting lines on themes without a background.
            const bool isDefault = first.fgColor == 0 && first.bgColor == 0
                && first.attributes == TextAttribute::None;
            if (!isDefault) {
                QString line = QStringLiteral("y=%1 x=%2-%3 fg=%4 bg=%5")
                    .arg(row).arg(column).arg(end - 1)
                    .arg(first.fgColor ? rgbaString(first.fgColor) : QStringLiteral("default"),
                         first.bgColor ? rgbaString(first.bgColor) : QStringLiteral("default"));
                const QString attributes = attributeNames(first.attributes);
                if (!attributes.isEmpty())
                    line += QLatin1Char(' ') + attributes;
                result.append(line);
                result.append(QLatin1Char('\n'));
            }
            column = end;
        }
    }
    return result;
}

QList<TestRunner::TextMatch> TestRunner::textMatches(const TextGridBuffer& frame,
                                                     const QString& text) const
{
    QList<TextMatch> matches;
    if (text.isEmpty() || text.contains(QLatin1Char('\n')))
        return matches;

    for (int row = 0; row < frame.height(); ++row) {
        QString renderedRow;
        QVector<int> sourceColumns;
        QVector<int> sourceEnds;
        for (int column = 0; column < frame.width(); ++column) {
            const Cell& renderedCell = frame.cellAt(column, row);
            if (renderedCell.continuation)
                continue;
            const QString character = renderedCell.character().isEmpty()
                ? QStringLiteral(" ") : QString::fromUtf8(renderedCell.character());
            renderedRow.append(character);
            for (qsizetype index = 0; index < character.size(); ++index) {
                sourceColumns.append(column);
                sourceEnds.append(column + qMax(1, static_cast<int>(renderedCell.displayWidth)));
            }
        }

        qsizetype from = 0;
        while (from <= renderedRow.size() - text.size()) {
            const qsizetype matchIndex = renderedRow.indexOf(text, from);
            if (matchIndex < 0)
                break;
            const qsizetype lastIndex = matchIndex + text.size() - 1;
            if (matchIndex < sourceColumns.size() && lastIndex < sourceEnds.size()) {
                const int x = sourceColumns.at(matchIndex);
                matches.append(TextMatch{x, row, qMax(1, sourceEnds.at(lastIndex) - x)});
            }
            from = matchIndex + qMax<qsizetype>(1, text.size());
        }
    }
    return matches;
}

QString TestRunner::resolveSnapshotPath(const QString& path) const
{
    if (path.startsWith(QLatin1Char(':')) || QFileInfo(path).isAbsolute())
        return QDir::cleanPath(path);
    return QDir(QFileInfo(m_scriptPath).path()).filePath(path);
}

QString TestRunner::unifiedTextDiff(const QString& expectedPath,
                                    const QString& expected,
                                    const QString& actual)
{
    const QStringList expectedLines = expected.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    const QStringList actualLines = actual.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    QString diff = QStringLiteral("--- %1\n+++ actual\n@@ -1,%2 +1,%3 @@\n")
        .arg(expectedPath).arg(expectedLines.size()).arg(actualLines.size());
    const qsizetype lineCount = qMax(expectedLines.size(), actualLines.size());
    for (qsizetype index = 0; index < lineCount; ++index) {
        const bool hasExpected = index < expectedLines.size();
        const bool hasActual = index < actualLines.size();
        if (hasExpected && hasActual && expectedLines.at(index) == actualLines.at(index)) {
            diff += QLatin1Char(' ') + expectedLines.at(index) + QLatin1Char('\n');
            continue;
        }
        if (hasExpected)
            diff += QLatin1Char('-') + expectedLines.at(index) + QLatin1Char('\n');
        if (hasActual)
            diff += QLatin1Char('+') + actualLines.at(index) + QLatin1Char('\n');
    }
    return diff;
}

QString TestRunner::currentFrameText(QString* error) const
{
    if (!m_terminalEngine) {
        if (error) *error = QStringLiteral("TerminalEngine is no longer available");
        return {};
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame || frame->width() <= 0 || frame->height() <= 0) {
        if (error) *error = QStringLiteral("No committed frame buffer is available");
        return {};
    }
    return normalizedText(*frame);
}

QString TestRunner::currentStyledFrameText(QString* error) const
{
    if (!m_terminalEngine) {
        if (error) *error = QStringLiteral("TerminalEngine is no longer available");
        return {};
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame || frame->width() <= 0 || frame->height() <= 0) {
        if (error) *error = QStringLiteral("No committed frame buffer is available");
        return {};
    }
    return formatStyledText(*frame);
}

QString TestRunner::resolveArtifactPath(const QString& relativePath, QString* error) const
{
    const QString cleaned = QDir::cleanPath(relativePath);
    if (relativePath.trimmed().isEmpty() || cleaned == QLatin1String(".")
        || QFileInfo(cleaned).isAbsolute() || cleaned == QLatin1String("..")
        || cleaned.startsWith(QLatin1String("../"))
        || relativePath.contains(QLatin1Char('\n')) || relativePath.contains(QLatin1Char('\r'))) {
        *error = QStringLiteral("Artifact path must be a relative path without newlines: %1")
                     .arg(relativePath);
        return {};
    }

    const QString path = QDir(m_artifactDirectory).absoluteFilePath(cleaned);

    // The lexical checks above cannot see through symlinks, so verify the real
    // location: create the parent, then require that its canonical path is the
    // artifact root or below it. Without this a symlinked subdirectory lets a
    // scenario write anywhere on the filesystem while still reporting success.
    const QString parent = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(parent)) {
        *error = QStringLiteral("Cannot create artifact parent directory for %1").arg(path);
        return {};
    }
    const QString canonicalRoot = QFileInfo(m_artifactDirectory).canonicalFilePath();
    const QString canonicalParent = QFileInfo(parent).canonicalFilePath();
    if (canonicalRoot.isEmpty() || canonicalParent.isEmpty()
        || (canonicalParent != canonicalRoot
            && !canonicalParent.startsWith(canonicalRoot + QLatin1Char('/')))) {
        *error = QStringLiteral("Artifact path escapes the artifact directory: %1")
                     .arg(relativePath);
        return {};
    }
    if (QFileInfo(path).isSymLink()) {
        *error = QStringLiteral("Artifact path is a symbolic link: %1").arg(relativePath);
        return {};
    }
    return path;
}

bool TestRunner::writeTextArtifact(const QString& relativePath, const QString& contents,
                                   QString* error) const
{
    QString localError;
    QString* destinationError = error ? error : &localError;
    const QString path = resolveArtifactPath(relativePath, destinationError);
    if (path.isEmpty())
        return false;
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        *destinationError = QStringLiteral("Cannot create artifact parent directory for %1").arg(path);
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *destinationError = QStringLiteral("Cannot open artifact %1: %2")
                                .arg(path, file.errorString());
        return false;
    }
    const QByteArray bytes = contents.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        *destinationError = QStringLiteral("Cannot write artifact %1: %2")
                                .arg(path, file.errorString());
        return false;
    }
    if (!file.commit()) {
        *destinationError = QStringLiteral("Cannot commit artifact %1: %2")
                                .arg(path, file.errorString());
        return false;
    }
    return true;
}

void TestRunner::captureText(const QString& relativePath)
{
    if (!ensureRunning("captureText") || !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString frame = currentFrameText(&error);
    if (error.isEmpty() && writeTextArtifact(relativePath, frame, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty())
        failScenario(error);
}

void TestRunner::captureStyled(const QString& relativePath)
{
    if (!ensureRunning("captureStyled") || !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString frame = currentStyledFrameText(&error);
    if (error.isEmpty() && writeTextArtifact(relativePath, frame, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty())
        failScenario(error);
}

QString TestRunner::styledText()
{
    if (!ensureRunning("styledText"))
        return {};
    QString error;
    const QString text = currentStyledFrameText(&error);
    if (!error.isEmpty())
        failScenario(error);
    return text;
}

QString TestRunner::frameText()
{
    if (!ensureRunning("frameText"))
        return {};
    QString error;
    const QString text = currentFrameText(&error);
    if (!error.isEmpty())
        failScenario(error);
    return text;
}

QVariantMap TestRunner::cell(int x, int y)
{
    if (!ensureRunning("cell") || !m_terminalEngine)
        return {};
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame || x < 0 || y < 0 || x >= frame->width() || y >= frame->height()) {
        failScenario(QStringLiteral("Coordinates (%1,%2) are outside the committed frame")
                         .arg(x).arg(y));
        return {};
    }

    const Cell& renderedCell = frame->cellAt(x, y);
    const auto hasAttribute = [&](TextAttribute attribute) {
        return static_cast<uint8_t>(renderedCell.attributes & attribute) != 0;
    };
    QVariantMap attributes;
    attributes.insert(QStringLiteral("bold"), hasAttribute(TextAttribute::Bold));
    attributes.insert(QStringLiteral("dim"), hasAttribute(TextAttribute::Dim));
    attributes.insert(QStringLiteral("italic"), hasAttribute(TextAttribute::Italic));
    attributes.insert(QStringLiteral("underline"), hasAttribute(TextAttribute::Underline));
    attributes.insert(QStringLiteral("reversed"), hasAttribute(TextAttribute::Reversed));

    QVariantMap result;
    result.insert(QStringLiteral("x"), x);
    result.insert(QStringLiteral("y"), y);
    result.insert(QStringLiteral("character"), QString::fromUtf8(renderedCell.character()));
    result.insert(QStringLiteral("continuation"), renderedCell.continuation);
    result.insert(QStringLiteral("foreground"), rgbaString(renderedCell.fgColor));
    result.insert(QStringLiteral("background"), rgbaString(renderedCell.bgColor));
    result.insert(QStringLiteral("attributes"), attributes);
    return result;
}

QVariantList TestRunner::findText(const QString& text)
{
    if (!ensureRunning("findText") || !m_terminalEngine)
        return {};
    if (text.isEmpty() || text.contains(QLatin1Char('\n'))) {
        failScenario(QStringLiteral("terminal.findText() requires non-empty single-row text"));
        return {};
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return {};
    }

    QVariantList result;
    for (const TextMatch& match : textMatches(*frame, text)) {
        result.append(QVariantMap{{QStringLiteral("x"), match.x},
                                  {QStringLiteral("y"), match.y},
                                  {QStringLiteral("width"), match.width}});
    }
    return result;
}

void TestRunner::clickText(const QString& text, int occurrence, const QString& button,
                           const QStringList& modifiers)
{
    if (!ensureRunning("clickText") || !m_terminalEngine)
        return;
    if (text.isEmpty() || text.contains(QLatin1Char('\n'))) {
        failScenario(QStringLiteral("terminal.clickText() requires non-empty single-row text"));
        return;
    }
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return;
    }
    const QList<TextMatch> matches = textMatches(*frame, text);
    if (occurrence < 0 || occurrence >= matches.size()) {
        failScenario(QStringLiteral("Text \"%1\" occurrence %2 was not found (%3 matches)")
                         .arg(text).arg(occurrence).arg(matches.size()));
        return;
    }
    const TextMatch& match = matches.at(occurrence);
    click(match.x + qMax(0, match.width - 1) / 2, match.y, button, modifiers);
}

bool TestRunner::writeFinalFrameArtifact(QString* error)
{
    QString frameError;
    const QString frame = currentFrameText(&frameError);
    if (!frameError.isEmpty()) {
        m_finalFrameRelativePath.clear();
        return true;
    }

    if (m_state == State::Failed)
        m_finalFrameRelativePath = QStringLiteral("failure.txt");
    else if (m_mode == QLatin1String("capture"))
        m_finalFrameRelativePath = QStringLiteral("capture.txt");
    else
        m_finalFrameRelativePath = QStringLiteral("final.txt");
    return writeTextArtifact(m_finalFrameRelativePath, frame, error);
}

bool TestRunner::writeResultArtifact(int exitCode, QString* error)
{
    const auto operationObject = [](const OperationRecord& operation) {
        QJsonObject location;
        if (!operation.file.isEmpty()) {
            location.insert(QStringLiteral("file"), operation.file);
            location.insert(QStringLiteral("line"), operation.line);
            location.insert(QStringLiteral("column"), operation.column);
        }
        QJsonObject result{{QStringLiteral("index"), operation.index},
                           {QStringLiteral("name"), operation.name},
                           {QStringLiteral("status"), operation.status},
                           {QStringLiteral("elapsedMs"), operation.elapsedMs}};
        result.insert(QStringLiteral("location"),
                      location.isEmpty() ? QJsonValue(QJsonValue::Null)
                                         : QJsonValue(location));
        return result;
    };

    QJsonArray operations;
    for (const OperationRecord& operation : std::as_const(m_operations))
        operations.append(operationObject(operation));

    QJsonObject viewport{{QStringLiteral("columns"), m_viewport.width()},
                         {QStringLiteral("rows"), m_viewport.height()}};
    QJsonArray captures;
    m_capturePaths.removeDuplicates();
    for (const QString& capture : std::as_const(m_capturePaths))
        captures.append(capture);
    QJsonObject artifacts{{QStringLiteral("directory"), m_artifactDirectory},
                          {QStringLiteral("result"), QStringLiteral("result.json")},
                          {QStringLiteral("captures"), captures}};
    artifacts.insert(QStringLiteral("finalFrame"),
                     m_finalFrameRelativePath.isEmpty()
                         ? QJsonValue(QJsonValue::Null)
                         : QJsonValue(m_finalFrameRelativePath));

    QJsonObject root{{QStringLiteral("schemaVersion"), 1},
                     {QStringLiteral("status"), exitCode == 0
                          ? QStringLiteral("passed") : QStringLiteral("failed")},
                     {QStringLiteral("exitCode"), exitCode},
                     {QStringLiteral("mode"), m_mode},
                     {QStringLiteral("scenario"), m_scriptPath},
                     {QStringLiteral("message"), m_message},
                     {QStringLiteral("elapsedMs"), m_elapsed.isValid() ? m_elapsed.elapsed() : 0},
                     {QStringLiteral("viewport"), viewport},
                     {QStringLiteral("operations"), operations},
                     {QStringLiteral("artifacts"), artifacts},
                     {QStringLiteral("frameStats"), QJsonObject::fromVariantMap(frameStats())}};
    root.insert(QStringLiteral("operation"),
                m_currentOperation >= 0 && m_currentOperation < m_operations.size()
                    ? QJsonValue(operationObject(m_operations.at(m_currentOperation)))
                    : QJsonValue(QJsonValue::Null));

    const QString json = QString::fromUtf8(
        QJsonDocument(root).toJson(QJsonDocument::Indented));
    return writeTextArtifact(QStringLiteral("result.json"), json, error);
}

void TestRunner::fail(const QString& message)
{
    if (!ensureRunning("fail"))
        return;
    failScenario(message.isEmpty() ? QStringLiteral("Scenario failed") : message);
}

QJSValue TestRunner::rootObject()
{
    if (!m_qmlEngine || !m_terminal)
        return QJSValue();
    return m_qmlEngine->newQObject(m_terminal);
}

void TestRunner::finish()
{
    if (!ensureRunning("finish"))
        return;
    m_state = State::Finishing;
    if (!m_evaluating) {
        m_state = State::Passed;
        m_watchdog.stop();
        scheduleExit(0);
    }
}

void TestRunner::onPromiseResolved()
{
    if (m_state == State::Running || m_state == State::Finishing) {
        m_state = State::Passed;
        m_watchdog.stop();
        scheduleExit(0);
    }
}

void TestRunner::onPromiseRejected(const QString& error)
{
    if (m_state != State::Failed && m_state != State::Passed) {
        failScenario(error.isEmpty() ? QStringLiteral("Promise rejected") : error, false);
    }
}

void TestRunner::failScenario(const QString& message, bool raiseJsError)
{
    if (m_state == State::Failed || m_state == State::Passed)
        return;
    m_state = State::Failed;
    m_watchdog.stop();
    m_message = message;
    completeCurrentOperation(QStringLiteral("failed"));
    if (m_activeLoop)
        m_activeLoop->quit();
    if (raiseJsError && m_evaluating && m_qmlEngine)
        m_qmlEngine->throwError(message);
    scheduleExit(1);
}

void TestRunner::scheduleExit(int exitCode)
{
    if (m_exitScheduled)
        return;
    m_exitScheduled = true;
    completeCurrentOperation(exitCode == 0 ? QStringLiteral("passed")
                                           : QStringLiteral("failed"));

    QString artifactError;
    if (!writeFinalFrameArtifact(&artifactError) && exitCode == 0) {
        exitCode = 1;
        m_state = State::Failed;
        m_message = artifactError;
    }
    if (!writeResultArtifact(exitCode, &artifactError)) {
        exitCode = 1;
        m_state = State::Failed;
        if (m_message.isEmpty())
            m_message = artifactError;
    }

    const QString summary = exitCode == 0
        ? QStringLiteral("[Test] PASSED in %1 ms; artifacts: %2")
              .arg(m_elapsed.isValid() ? m_elapsed.elapsed() : 0)
              .arg(m_artifactDirectory)
        : QStringLiteral("[Test] FAILED: %1; artifacts: %2")
              .arg(m_message.isEmpty() ? QStringLiteral("Scenario failed") : m_message,
                   m_artifactDirectory);
    if (exitCode == 0)
        qInfo().noquote() << summary;
    else
        qCritical().noquote() << summary;

    if (!m_temporaryScriptPath.isEmpty()) {
        QFile::remove(m_temporaryScriptPath);
        m_temporaryScriptPath.clear();
    }
    emit finished(exitCode);
    QTimer::singleShot(0, qApp, [exitCode] { QCoreApplication::exit(exitCode); });
}

QString TestRunner::formatJsError(const QJSValue& error) const
{
    return QStringLiteral("JavaScript error at %1:%2: %3\n%4")
        .arg(error.property(QStringLiteral("fileName")).toString())
        .arg(error.property(QStringLiteral("lineNumber")).toInt())
        .arg(error.toString(), error.property(QStringLiteral("stack")).toString());
}

} // namespace qtui
