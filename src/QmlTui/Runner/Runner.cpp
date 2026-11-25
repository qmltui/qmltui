#include <QmlTui/Runner/Runner.h>
#include <QmlTui/Runner/RunnerSnapshot_p.h>
#include <QmlTui/Runner/CastRecorder_p.h>
#include <QmlTui/Runner/MemoryTerminalBackend.h>
#include <QmlTui/Core/Primitives/TerminalBackend.h>
#include <QmlTui/Core/Primitives/TerminalCapabilities.h>
#include <QmlTui/Core/Primitives/Color.h>

#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Engine/Clock_p.h>
#include <QmlTui/Core/Engine/Renderer.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJSValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSaveFile>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <algorithm>
#include <utility>

namespace qmltui {

// Exposes the runner's internal callbacks — Promise resolution in async
// scenarios and the structured operation trace — to the JavaScript facade
// without making them Q_INVOKABLE on the exported Runner class.
class RunnerBridge final : public QObject {
    Q_OBJECT

public:
    explicit RunnerBridge(Runner* runner) : QObject(runner), m_runner(runner) {}

    Q_INVOKABLE void onPromiseResolved() { m_runner->onPromiseResolved(); }
    Q_INVOKABLE void onPromiseRejected(const QString& error) { m_runner->onPromiseRejected(error); }
    Q_INVOKABLE void beginOperation(const QString& name, const QString& stack) {
        m_runner->beginOperation(name, stack);
    }
    Q_INVOKABLE void endOperation() { m_runner->endOperation(); }

private:
    Runner* const m_runner;
};

// The scenario watchdog is a QTimer, so it cannot fire while synchronous
// JavaScript never yields to the event loop. This thread interrupts the JS
// engine after the deadline instead, so a runaway loop still fails with a
// diagnosis rather than hanging.
class RunnerHardWatchdog final {
public:
    RunnerHardWatchdog(QPointer<QQmlEngine> engine, std::chrono::milliseconds deadline) {
        m_thread = std::thread([this, engine, deadline] {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_wake.wait_for(lock, deadline, [this] { return m_finished; }))
                return; // scenario finished in time
            m_interrupted = true;
            lock.unlock();
            // Documented as thread safe, and intended for exactly this case.
            if (engine) engine->setInterrupted(true);
        });
    }

    ~RunnerHardWatchdog() { stop(); }

    // Signals the thread, joins it, and reports whether it interrupted the
    // JavaScript engine before the scenario finished.
    bool stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_finished = true;
        }
        m_wake.notify_all();
        if (m_thread.joinable()) m_thread.join();
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_interrupted;
    }

private:
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    bool m_finished = false;
    bool m_interrupted = false;
};

namespace {

using runner_detail::conditionPollIntervalMs;
using runner_detail::maximumScenarioTimeoutMs;
using runner_detail::maximumTimeoutMs;
using runner_detail::minimumTimeoutMs;
using runner_detail::pausedClockHint;

int environmentInteger(const char* name, int fallback, int minimum, int maximum, QString* error) {
    const QByteArray value = qgetenv(name);
    if (value.isEmpty()) return fallback;

    bool ok = false;
    const int parsed = value.toInt(&ok);
    if (!ok || parsed < minimum || parsed > maximum) {
        if (error && error->isEmpty()) {
            *error = QStringLiteral("%1 must be an integer between %2 and %3")
                         .arg(QString::fromLatin1(name))
                         .arg(minimum)
                         .arg(maximum);
        }
        return fallback;
    }
    return parsed;
}

QString absoluteOrResourcePath(const QString& path) {
    return path.startsWith(QLatin1String(":")) ? path : QFileInfo(path).absoluteFilePath();
}

struct RunnerAutoHook {
    RunnerAutoHook() {
        Terminal::registerRunnerHook([](QQmlEngine* engine, Terminal* terminal) {
            Runner::startFromEnvironment(engine, terminal);
        });
    }
};
static RunnerAutoHook s_autoHookRegistration;

// The backend that receives a recording session's output. Created by the
// factory below before the Terminal exists; a QPointer because the engine's
// device owns it.
QPointer<MemoryTerminalBackend> s_recordingBackend;

bool environmentFlagEnabled(const char* name) {
    const QByteArray value = qgetenv(name).trimmed().toLower();
    return !value.isEmpty() && value != "0" && value != "false" && value != "off";
}

bool environmentRequestsScenario() {
    return !qgetenv("QMLTUI_RUNNER_SCRIPT").trimmed().isEmpty() ||
           !qgetenv("QMLTUI_RUNNER_EVAL").trimmed().isEmpty() ||
           !qgetenv("QMLTUI_RUNNER_CAPTURE_AFTER_MS").trimmed().isEmpty() ||
           environmentFlagEnabled("QMLTUI_RUNNER_EMBEDDED_SCENARIO");
}

int environmentDimension(const char* name, int fallback) {
    bool ok = false;
    const int value = qgetenv(name).trimmed().toInt(&ok);
    return ok && value >= 1 && value <= 1000 ? value : fallback;
}

} // namespace

// QMLTUI_RUNNER_RECORD routes the terminal bytes into memory so the Runner can
// write them to an asciicast. The engine creates its device when the Terminal
// completes, before any Runner exists, so the factory is installed as soon as
// QCoreApplication is constructed. Recording without a scenario would leave the
// application talking to nobody, so a scenario variable is required too.
static void qmltuiInstallRecordingBackend() {
    if (qgetenv("QMLTUI_RUNNER_RECORD").trimmed().isEmpty() || !environmentRequestsScenario())
        return;
    const int columns = environmentDimension("QMLTUI_RUNNER_COLUMNS", 80);
    const int rows = environmentDimension("QMLTUI_RUNNER_ROWS", 24);
    TerminalBackend::setFactory([columns, rows](int) -> std::unique_ptr<TerminalBackend> {
        auto backend = std::make_unique<MemoryTerminalBackend>(QSize(columns, rows));
        s_recordingBackend = backend.get();
        return backend;
    });
}
Q_COREAPP_STARTUP_FUNCTION(qmltuiInstallRecordingBackend)

Runner* Runner::startFromEnvironment(QQmlEngine* qmlEngine, Terminal* terminal) {
    static QPointer<Runner> s_activeRunner = nullptr;
    if (s_activeRunner) return s_activeRunner.data();

    QString scriptPath = QString::fromUtf8(qgetenv("QMLTUI_RUNNER_SCRIPT")).trimmed();
    const QString evalCode = QString::fromUtf8(qgetenv("QMLTUI_RUNNER_EVAL")).trimmed();
    // Presence counts, not truthiness: QMLTUI_RUNNER_CAPTURE_AFTER_MS=0 is a request to
    // capture the first committed frame.
    const bool capture = !qgetenv("QMLTUI_RUNNER_CAPTURE_AFTER_MS").trimmed().isEmpty();
    const int requestCount =
        (scriptPath.isEmpty() ? 0 : 1) + (evalCode.isEmpty() ? 0 : 1) + (capture ? 1 : 0);
    if (requestCount == 0) return nullptr;

    QString configurationError;
    if (requestCount > 1) {
        configurationError =
            QStringLiteral("QMLTUI_RUNNER_SCRIPT, QMLTUI_RUNNER_EVAL, and "
                           "QMLTUI_RUNNER_CAPTURE_AFTER_MS are mutually exclusive");
    }

    QString mode = QStringLiteral("script");
    QString temporaryScriptPath;
    QByteArray generatedScript;
    int captureAfterMs = 0;
    if (!evalCode.isEmpty()) {
        mode = QStringLiteral("eval");
        generatedScript = QStringLiteral("function main(app) {\n"
                                         "    const __qmltuiOuterMain = main;\n"
                                         "    return (function(app) {\n"
                                         "        {\n"
                                         "%1\n"
                                         "            if (typeof main === \"function\" && main !== "
                                         "__qmltuiOuterMain) {\n"
                                         "                return main(app);\n"
                                         "            }\n"
                                         "        }\n"
                                         "    })(app);\n"
                                         "}\n")
                              .arg(evalCode)
                              .toUtf8();
    } else if (capture) {
        mode = QStringLiteral("capture");
        captureAfterMs = environmentInteger("QMLTUI_RUNNER_CAPTURE_AFTER_MS", 0, 0,
                                            maximumTimeoutMs, &configurationError);
        generatedScript = QByteArrayLiteral("function main(app) {\n");
        if (captureAfterMs > 0)
            generatedScript += QStringLiteral("    app.sleep(%1);\n").arg(captureAfterMs).toUtf8();
        generatedScript += QByteArrayLiteral("    app.captureText('capture.txt');\n"
                                             "    app.captureStyled('capture_styled.txt');\n"
                                             "}\n");
    }

    if (!generatedScript.isEmpty() && configurationError.isEmpty()) {
        temporaryScriptPath = QDir::temp().filePath(
            QStringLiteral("qmltui-%1-%2.js").arg(mode).arg(QCoreApplication::applicationPid()));
        QFile tempFile(temporaryScriptPath);
        if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text) ||
            tempFile.write(generatedScript) != generatedScript.size()) {
            configurationError = QStringLiteral("Cannot create temporary %1 script %2: %3")
                                     .arg(mode, temporaryScriptPath, tempFile.errorString());
        } else {
            tempFile.close();
            scriptPath = temporaryScriptPath;
        }
    }

    QSize viewport;
    const bool hasColumns = !qgetenv("QMLTUI_RUNNER_COLUMNS").trimmed().isEmpty();
    const bool hasRows = !qgetenv("QMLTUI_RUNNER_ROWS").trimmed().isEmpty();
    if (hasColumns || hasRows) {
        const int defaultColumns =
            (terminal && terminal->engine()) ? terminal->engine()->terminalWidth() : 80;
        const int defaultRows =
            (terminal && terminal->engine()) ? terminal->engine()->terminalHeight() : 24;
        const int columns = environmentInteger("QMLTUI_RUNNER_COLUMNS", defaultColumns, 1, 1000,
                                               &configurationError);
        const int rows =
            environmentInteger("QMLTUI_RUNNER_ROWS", defaultRows, 1, 1000, &configurationError);
        viewport = QSize(columns, rows);
    }

    const int scenarioTimeout =
        environmentInteger("QMLTUI_RUNNER_TIMEOUT_MS", 30000, minimumTimeoutMs,
                           maximumScenarioTimeoutMs, &configurationError);
    const int operationTimeout =
        environmentInteger("QMLTUI_RUNNER_OPERATION_TIMEOUT_MS", 2000, minimumTimeoutMs,
                           maximumTimeoutMs, &configurationError);
    // Fail up front instead of letting the watchdog kill the sleep: the capture
    // still needs one frame wait after the delay.
    if (capture && configurationError.isEmpty() &&
        captureAfterMs + operationTimeout > scenarioTimeout) {
        configurationError =
            QStringLiteral("QMLTUI_RUNNER_CAPTURE_AFTER_MS (%1) plus "
                           "QMLTUI_RUNNER_OPERATION_TIMEOUT_MS (%2) must not exceed "
                           "QMLTUI_RUNNER_TIMEOUT_MS (%3)")
                .arg(captureAfterMs)
                .arg(operationTimeout)
                .arg(scenarioTimeout);
    }
    QString artifactDirectory = QString::fromUtf8(qgetenv("QMLTUI_RUNNER_ARTIFACT_DIR")).trimmed();
    if (artifactDirectory.isEmpty()) {
        artifactDirectory = QDir::temp().filePath(
            QStringLiteral("qmltui-runner-%1").arg(QCoreApplication::applicationPid()));
    }

    auto* runner = new Runner(qmlEngine, terminal, absoluteOrResourcePath(scriptPath),
                              QFileInfo(artifactDirectory).absoluteFilePath(), viewport,
                              operationTimeout, scenarioTimeout, configurationError);
    runner->m_mode = mode;
    runner->m_temporaryScriptPath = temporaryScriptPath;
    s_activeRunner = runner;
    QTimer::singleShot(0, runner, &Runner::start);
    return runner;
}

Runner::Runner(QQmlEngine* qmlEngine, Terminal* terminal, QString scriptPath,
               QString artifactDirectory, QSize viewport, int operationTimeoutMs,
               int scenarioTimeoutMs, QString configurationError)
    : QObject(QCoreApplication::instance()), m_qmlEngine(qmlEngine), m_terminal(terminal),
      m_terminalEngine(terminal ? terminal->engine() : nullptr),
      m_scriptPath(std::move(scriptPath)), m_artifactDirectory(std::move(artifactDirectory)),
      m_viewport(viewport), m_operationTimeoutMs(operationTimeoutMs),
      m_scenarioTimeoutMs(scenarioTimeoutMs), m_configurationError(std::move(configurationError)) {
    // Apply the deterministic viewport before the event loop can commit the
    // Terminal's queued physical-size frame. Otherwise a live test runner can
    // paint that frame before overlaying the smaller test viewport.
    if (m_terminalEngine && m_viewport.isValid() && !m_viewport.isEmpty())
        m_terminalEngine->setTestViewportSize(m_viewport);
    // A Runner attached to the engine is a test session even when nothing in
    // the environment announced one: no-op for environment-requested runs and
    // after useEmbeddedScenario(), quiet from here on otherwise.
    if (m_terminalEngine) m_terminalEngine->beginTestSession();
    if (m_terminalEngine && s_recordingBackend) armRecording();

    m_watchdog.setSingleShot(true);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        failScenario(
            QStringLiteral("Scenario exceeded the %1 ms watchdog").arg(m_scenarioTimeoutMs));
    });
}

Runner::~Runner() {
    // A joinable std::thread would terminate the process on destruction.
    (void)stopHardWatchdog();
}

void Runner::useEmbeddedScenario(bool liveOutput) {
    TerminalEngine::requestTestSession(liveOutput);
}

void Runner::setLiveOutput(bool live) {
    if (m_terminalEngine) m_terminalEngine->setLiveOutput(live);
}

void Runner::setUpdateSnapshots(bool update) {
    m_updateSnapshotsOverride = update;
}

void Runner::start() {
    m_elapsed.start();
    // Optional launcher protocol: acknowledge activation before loading the
    // scenario. This is not a claim that initialization or the test passed.
    const QString handshakePath = QString::fromUtf8(qgetenv("QMLTUI_RUNNER_HANDSHAKE_PATH"));
    const QString handshakeToken = QString::fromUtf8(qgetenv("QMLTUI_RUNNER_HANDSHAKE_TOKEN"));
    if (!handshakePath.isEmpty() || !handshakeToken.isEmpty()) {
        QSaveFile handshake(handshakePath);
        const QByteArray payload =
            QJsonDocument(QJsonObject{{QStringLiteral("protocolVersion"), 1},
                                      {QStringLiteral("token"), handshakeToken},
                                      {QStringLiteral("pid"), QCoreApplication::applicationPid()}})
                .toJson(QJsonDocument::Compact);
        if (handshakePath.isEmpty() || handshakeToken.isEmpty() ||
            !handshake.open(QIODevice::WriteOnly) || handshake.write(payload) != payload.size() ||
            !handshake.commit()) {
            m_configurationError = QStringLiteral("Cannot write Runner activation handshake: %1")
                                       .arg(handshake.errorString());
        }
    }
    if (m_updateSnapshotsOverride.has_value()) {
        m_updateSnapshots = *m_updateSnapshotsOverride;
    } else {
        // Inherited from the environment so `QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest`
        // also refreshes baselines of embedded runs.
        m_updateSnapshots = environmentFlagEnabled("QMLTUI_RUNNER_UPDATE_SNAPSHOTS");
    }
    if (!QDir().mkpath(m_artifactDirectory)) {
        m_configurationError =
            QStringLiteral("Cannot create artifact directory %1").arg(m_artifactDirectory);
    }
    if (!m_configurationError.isEmpty()) {
        failScenario(m_configurationError, false);
        return;
    }
    if (!m_qmlEngine || !m_terminal || !m_terminalEngine) {
        failScenario(QStringLiteral("Runner requires a live QQmlEngine and explicit Terminal"),
                     false);
        return;
    }

    const bool moduleScenario = m_scriptPath.endsWith(QLatin1String(".mjs"));
    QByteArray scriptBytes;
    {
        QFile scriptFile(m_scriptPath);
        if (!scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            failScenario(QStringLiteral("Cannot open scenario %1: %2")
                             .arg(m_scriptPath, scriptFile.errorString()),
                         false);
            return;
        }
        if (!moduleScenario) {
            scriptBytes = scriptFile.readAll();
            if (scriptFile.error() != QFileDevice::NoError) {
                failScenario(QStringLiteral("Cannot read scenario %1: %2")
                                 .arg(m_scriptPath, scriptFile.errorString()),
                             false);
                return;
            }
        }
        // A synchronous scenario finishes inside evaluate() below and removes a
        // temporary script; Windows refuses to delete a file that is still open.
    }

    connect(m_qmlEngine, &QObject::destroyed, this, [this] {
        failScenario(QStringLiteral("QQmlEngine was destroyed during the scenario"), false);
    });
    connect(m_terminal, &QObject::destroyed, this, [this] {
        failScenario(QStringLiteral("Target Terminal was destroyed during the scenario"), false);
    });
    connect(m_terminalEngine, &TerminalEngine::renderFailed, this,
            [this](const QString& message) { failScenario(message); });

    m_watchdog.start(m_scenarioTimeoutMs);
    if (!waitForNextFrame(m_operationTimeoutMs)) return;

    m_state = State::Running;
    if (!installJavaScriptFacade()) return;

    m_evaluating = true;
    startHardWatchdog();
    QJSValue mainFunc;
    if (moduleScenario) {
        // An .mjs scenario is an ECMAScript module: it must export
        // function main(app) and may import sibling helper modules.
        const QJSValue moduleNamespace = m_qmlEngine->importModule(m_scriptPath);
        if (moduleNamespace.isError()) {
            stopHardWatchdog();
            m_evaluating = false;
            failScenario(formatJsError(moduleNamespace), false);
            return;
        }
        mainFunc = moduleNamespace.property(QStringLiteral("main"));
        if (!mainFunc.isCallable()) {
            stopHardWatchdog();
            m_evaluating = false;
            failScenario(QStringLiteral("Module scenario must export a function main(app)"), false);
            return;
        }
    } else {
        // A classic script is evaluated inside a function scope, so its
        // top-level declarations stay out of the application's JavaScript
        // globals. The prefix adds no newline and the suffix starts with one,
        // so reported line numbers do not move.
        const QString wrappedSource =
            QStringLiteral("(function() { ") + QString::fromUtf8(scriptBytes) +
            QStringLiteral("\nreturn typeof main === \"function\" ? main : undefined; })()");
        const QJSValue evalResult = m_qmlEngine->evaluate(wrappedSource, m_scriptPath);
        if (evalResult.isError()) {
            stopHardWatchdog();
            m_evaluating = false;
            failScenario(formatJsError(evalResult), false);
            return;
        }
        mainFunc = evalResult;
        if (!mainFunc.isCallable()) {
            stopHardWatchdog();
            m_evaluating = false;
            failScenario(QStringLiteral("Scenario must define a function main(app)"), false);
            return;
        }
    }

    const QJSValue result = mainFunc.call({m_terminalFacade});

    const bool interrupted = stopHardWatchdog();
    m_evaluating = false;

    if (interrupted) {
        failScenario(QStringLiteral("Scenario did not yield to the event loop within %1 ms; "
                                    "JavaScript execution was interrupted")
                         .arg(m_scenarioTimeoutMs),
                     false);
        return;
    }
    if (m_state == State::Failed) return;
    if (result.isError()) {
        failScenario(formatJsError(result), false);
        return;
    }

    if (result.isObject() && result.property(QStringLiteral("then")).isCallable()) {
        QJSValue promiseHandler = m_qmlEngine->evaluate(
            QStringLiteral("(function(promise, bridge) {"
                           "  promise.then(function() { bridge.onPromiseResolved(); })"
                           "         .catch(function(err) { bridge.onPromiseRejected(err ? "
                           "String(err) : 'Promise rejected'); });"
                           "})"));
        promiseHandler.call({result, m_qmlEngine->newQObject(m_bridge)});
        return;
    }

    if (m_state == State::Finishing || m_state == State::Running) {
        m_state = State::Passed;
        m_watchdog.stop();
        scheduleExit(0);
        return;
    }
}

bool Runner::installJavaScriptFacade() {
    if (!m_bridge) m_bridge = new RunnerBridge(this);
    const QJSValue nativeRunner = m_qmlEngine->newQObject(this);
    const QJSValue bridge = m_qmlEngine->newQObject(m_bridge);
    m_qmlEngine->globalObject().setProperty(QStringLiteral("__QmlTuiRunnerNative"), nativeRunner);
    m_qmlEngine->globalObject().setProperty(QStringLiteral("__QmlTuiRunnerBridge"), bridge);
    const QString facadeSource = QStringLiteral(R"JS(
(function(nativeRunner, bridge) {
    const opTimeout = @OPERATION_TIMEOUT@;
    const methodNames = [
        "press", "typeText", "paste", "sleep", "waitForFrame", "waitForCondition",
        "waitForText", "expectText", "expectNoText", "captureText",
        "captureStyled", "frameText", "styledText", "cell", "findText",
        "scrollbackRowCount", "scrollbackText", "waitForScrollbackText",
        "expectScrollbackText", "expectNoScrollbackText", "captureScrollback",
        "captureScrollbackStyled", "expectScrollbackSnapshot",
        "startRecording", "stopRecording", "marker",
        "click", "doubleClick", "drag", "scroll", "hover", "mouseDown", "mouseUp",
        "clickText", "hoverText",
        "waitForRegex",
        "expectRegex", "expectTextInRegion", "expectStyle", "expectSnapshot",
        "expectStyled", "expectRegionSnapshot", "regionText", "itemRect", "waitForItem",
        "expectRepaint", "frameCount", "frameStats",
        "injectText", "injectKey", "injectPaste",
        "resize", "size", "advanceTime", "now",
        "fail", "rootObject", "finish"
    ];
    const isPlainObject = function(value) {
        return value !== null && typeof value === "object" && !Array.isArray(value);
    };
    // Optional parameters may be given positionally or as one trailing options
    // object; an unknown option key fails the scenario so a typo cannot pass
    // silently. Methods without an entry pass their arguments through.
    const signatures = {
        press:        { required: 1, optional: [["modifiers", []]] },
        injectKey:    { required: 1, optional: [["modifiers", []]] },
        hover:        { required: 2, optional: [["modifiers", []]] },
        click:        { required: 2, optional: [["button", "left"], ["modifiers", []]] },
        doubleClick:  { required: 2, optional: [["button", "left"], ["modifiers", []]] },
        mouseDown:    { required: 2, optional: [["button", "left"], ["modifiers", []]] },
        mouseUp:      { required: 2, optional: [["button", "left"], ["modifiers", []]] },
        drag:         { required: 4, optional: [["button", "left"], ["modifiers", []]] },
        scroll:       { required: 4, optional: [["modifiers", []]] },
        clickText:    { required: 1,
                        optional: [["occurrence", 0], ["button", "left"], ["modifiers", []]] },
        hoverText:    { required: 1,
                        optional: [["occurrence", 0], ["modifiers", []]] },
        waitForFrame: { required: 0, optional: [["timeout", 2000]] },
        expectRepaint:{ required: 0, optional: [["timeout", 2000]] },
        waitForText:  { required: 1, optional: [["timeout", 2000]] },
        waitForRegex: { required: 1, optional: [["timeout", 2000]] },
        waitForCondition: { required: 1, optional: [["timeout", 2000]] },
        waitForItem:  { required: 1, optional: [["timeout", 2000]] },
        expectText:   { required: 1, optional: [["timeout", -1]] },
        expectNoText: { required: 1, optional: [["timeout", -1]] },
        expectRegex:  { required: 1, optional: [["timeout", -1]] },
        expectTextInRegion: { required: 5, optional: [["timeout", -1]] },
        expectStyle:  { required: 3, optional: [["timeout", -1]] },
        expectSnapshot: { required: 1, optional: [["timeout", -1]] },
        expectStyled: { required: 1, optional: [["timeout", -1]] },
        expectRegionSnapshot: { required: 5, optional: [["timeout", -1]] },
        scrollbackText: { required: 0, optional: [["fromRow", 0]] },
        waitForScrollbackText: { required: 1, optional: [["timeout", 2000]] },
        expectScrollbackText: { required: 1, optional: [["timeout", -1]] },
        expectNoScrollbackText: { required: 1, optional: [["timeout", -1]] },
        captureScrollback: { required: 1, optional: [["fromRow", 0]] },
        captureScrollbackStyled: { required: 1, optional: [["fromRow", 0]] },
        expectScrollbackSnapshot: { required: 1, optional: [["fromRow", 0], ["timeout", -1]] }
    };
    const rejectUnknownOptions = function(name, options, allowed) {
        for (const key in options) {
            if (allowed.indexOf(key) < 0) {
                nativeRunner.fail("Unknown option \"" + key + "\" for " + name
                                  + "(); allowed: " + allowed.join(", "));
            }
        }
    };
    const buildArguments = function(name, args) {
        const signature = signatures[name];
        if (!signature)
            return args;
        let options = null;
        if (args.length > signature.required && isPlainObject(args[args.length - 1])) {
            options = args[args.length - 1];
            args = args.slice(0, args.length - 1);
            if (args.length > signature.required) {
                nativeRunner.fail(name + "() takes either positional optional arguments "
                                  + "or an options object, not both");
            }
        }
        const call = args.slice(0, signature.required);
        if (options) {
            rejectUnknownOptions(name, options, signature.optional.map(function(entry) {
                return entry[0];
            }));
            signature.optional.forEach(function(entry) {
                call.push(Object.prototype.hasOwnProperty.call(options, entry[0])
                          ? options[entry[0]] : entry[1]);
            });
        } else {
            signature.optional.forEach(function(entry, index) {
                const position = signature.required + index;
                call.push(position < args.length ? args[position] : entry[1]);
            });
        }
        return call;
    };
    const facade = {};
    methodNames.forEach(function(name) {
        facade[name] = function() {
            bridge.beginOperation(name, String(new Error().stack || ""));
            const args = buildArguments(name, Array.prototype.slice.call(arguments));
            const result = nativeRunner[name].apply(nativeRunner, args);
            // Skipped when the call throws, so a failing operation stays open
            // and is the one blamed in the result.
            bridge.endOperation();
            return result;
        };
    });

    // Scoped locators. Each composite records one operation and reaches the
    // native primitives directly, so the trace shows region.waitForText, not
    // the polling underneath it.
    const composite = function(name, body) {
        bridge.beginOperation(name, String(new Error().stack || ""));
        const result = body();
        bridge.endOperation();
        return result;
    };
    const optionValue = function(name, options, key, fallback) {
        if (options === undefined || options === null)
            return fallback;
        if (!isPlainObject(options))
            nativeRunner.fail(name + "() options must be an object");
        return Object.prototype.hasOwnProperty.call(options, key) ? options[key] : fallback;
    };
    const makeRegion = function(x, y, width, height) {
        const bounds = "[" + x + "," + y + "," + width + "," + height + "]";
        const matchesInside = function(text) {
            return nativeRunner.findText(text).filter(function(match) {
                return match.y >= y && match.y < y + height
                    && match.x >= x && match.x + match.width <= x + width;
            });
        };
        const region = { x: x, y: y, width: width, height: height };
        region.text = function() {
            return composite("region.text", function() {
                return nativeRunner.regionText(x, y, width, height);
            });
        };
        region.findText = function(text) {
            return composite("region.findText", function() { return matchesInside(text); });
        };
        region.expectText = function(text, options) {
            composite("region.expectText", function() {
                rejectUnknownOptions("region.expectText", options || {}, ["timeout"]);
                nativeRunner.expectTextInRegion(x, y, width, height, text,
                                                optionValue("region.expectText", options, "timeout", -1));
            });
        };
        region.expectNoText = function(text, options) {
            composite("region.expectNoText", function() {
                rejectUnknownOptions("region.expectNoText", options || {}, ["timeout"]);
                nativeRunner.waitForFrame(
                    optionValue("region.expectNoText", options, "timeout", opTimeout));
                if (nativeRunner.regionText(x, y, width, height).includes(text)) {
                    nativeRunner.fail("Expected region " + bounds
                                      + " not to contain \"" + text + "\"");
                }
            });
        };
        region.waitForText = function(text, options) {
            composite("region.waitForText", function() {
                rejectUnknownOptions("region.waitForText", options || {}, ["timeout"]);
                nativeRunner.waitForCondition(function() {
                    return nativeRunner.regionText(x, y, width, height).includes(text);
                }, optionValue("region.waitForText", options, "timeout", 2000));
            });
        };
        region.expectSnapshot = function(path, options) {
            composite("region.expectSnapshot", function() {
                rejectUnknownOptions("region.expectSnapshot", options || {}, ["timeout"]);
                nativeRunner.expectRegionSnapshot(x, y, width, height, path,
                                                  optionValue("region.expectSnapshot", options, "timeout", -1));
            });
        };
        region.clickText = function(text, options) {
            composite("region.clickText", function() {
                rejectUnknownOptions("region.clickText", options || {},
                                     ["occurrence", "button", "modifiers"]);
                const matches = matchesInside(text);
                const occurrence = optionValue("region.clickText", options, "occurrence", 0);
                if (occurrence < 0 || occurrence >= matches.length) {
                    nativeRunner.fail("Text \"" + text + "\" occurrence " + occurrence
                                      + " was not found in region " + bounds
                                      + " (" + matches.length + " matches)");
                }
                const match = matches[occurrence];
                nativeRunner.clickFrame(match.x + Math.floor(Math.max(0, match.width - 1) / 2),
                                        match.y,
                                        optionValue("region.clickText", options, "button", "left"),
                                        optionValue("region.clickText", options, "modifiers", []));
            });
        };
        region.click = function(options) {
            composite("region.click", function() {
                rejectUnknownOptions("region.click", options || {}, ["button", "modifiers"]);
                nativeRunner.clickFrame(x + Math.floor((width - 1) / 2),
                                        y + Math.floor((height - 1) / 2),
                                        optionValue("region.click", options, "button", "left"),
                                        optionValue("region.click", options, "modifiers", []));
            });
        };
        return region;
    };
    facade.region = function(x, y, width, height) {
        return makeRegion(x, y, width, height);
    };
    facade.item = function(objectName) {
        const rect = facade.itemRect(objectName);
        const region = makeRegion(rect.x, rect.y, rect.width, rect.height);
        region.objectName = objectName;
        return region;
    };
    delete this.__QmlTuiRunnerNative;
    delete this.__QmlTuiRunnerBridge;
    return facade;
})(__QmlTuiRunnerNative, __QmlTuiRunnerBridge);
)JS");
    m_terminalFacade = m_qmlEngine->evaluate(
        QString(facadeSource)
            .replace(QLatin1String("@OPERATION_TIMEOUT@"), QString::number(m_operationTimeoutMs)),
        QStringLiteral("qmltui-runner-facade.js"));
    if (m_terminalFacade.isError()) {
        failScenario(QStringLiteral("Cannot install Terminal JavaScript facade: %1")
                         .arg(formatJsError(m_terminalFacade)),
                     false);
        return false;
    }
    return true;
}

void Runner::beginOperation(const QString& name, const QString& stack) {
    if (m_state != State::Running && m_state != State::Finishing) return;
    completeCurrentOperation(QStringLiteral("passed"));

    OperationRecord record;
    record.index = m_operations.size() + 1;
    record.name = name;
    record.startedMs = m_elapsed.isValid() ? m_elapsed.elapsed() : 0;

    static const QRegularExpression locationPattern(
        QStringLiteral("(?:at\\s+)?([^\\n]+?):(\\d+)(?::(\\d+))?"));
    QRegularExpressionMatchIterator matches = locationPattern.globalMatch(stack);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        const QString file = match.captured(1).trimmed().section(QLatin1Char('@'), -1);
        if (file.contains(QLatin1String("qmltui-runner-facade.js"))) continue;
        record.file = file;
        record.line = match.captured(2).toInt();
        record.column = match.captured(3).toInt();
        break;
    }

    m_operations.append(record);
    m_currentOperation = m_operations.size() - 1;
}

void Runner::startHardWatchdog() {
    if (m_hardWatchdog || !m_qmlEngine) return;
    // Give the event-loop watchdog a chance to report first: it produces a much
    // better message ("Timed out after N ms waiting for ...") and only fails to
    // fire when JavaScript never yields.
    m_hardWatchdog = std::make_unique<RunnerHardWatchdog>(
        m_qmlEngine, std::chrono::milliseconds(m_scenarioTimeoutMs + 1000));
}

bool Runner::stopHardWatchdog() {
    bool interrupted = false;
    if (m_hardWatchdog) {
        interrupted = m_hardWatchdog->stop();
        m_hardWatchdog.reset();
    }
    if (m_qmlEngine && m_qmlEngine->isInterrupted()) m_qmlEngine->setInterrupted(false);
    return interrupted;
}

void Runner::endOperation() {
    // The facade calls this once an app.* call has returned normally. Closing
    // the record here means a later failure — a JavaScript error on a following
    // line, or the scenario watchdog — is no longer attributed to an operation
    // that already succeeded.
    completeCurrentOperation(QStringLiteral("passed"));
    m_currentOperation = -1;
    if (!m_activeLoop && m_state == State::Running) {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
}

void Runner::completeCurrentOperation(const QString& status) {
    if (m_currentOperation < 0 || m_currentOperation >= m_operations.size()) return;
    OperationRecord& record = m_operations[m_currentOperation];
    if (record.status == QLatin1String("running")) {
        record.status = status;
        record.elapsedMs = (std::max<qint64>)(0, m_elapsed.elapsed() - record.startedMs);
    }
}

bool Runner::ensureRunning(const char* operation) {
    if (m_state == State::Running) return true;
    if (m_state != State::Failed) {
        failScenario(QStringLiteral("app.%1() is not valid in the current scenario state")
                         .arg(QString::fromLatin1(operation)));
    }
    return false;
}

int Runner::validateTimeout(int timeoutMs) {
    if (timeoutMs >= minimumTimeoutMs && timeoutMs <= maximumTimeoutMs) return timeoutMs;
    failScenario(QStringLiteral("Timeout must be between %1 and %2 ms")
                     .arg(minimumTimeoutMs)
                     .arg(maximumTimeoutMs));
    return 0;
}

int Runner::resolveExpectTimeout(int timeoutMs) {
    // expect* assertions default to the configured operation timeout; an
    // explicit timeout overrides it for one call.
    if (timeoutMs < 0) return m_operationTimeoutMs;
    return validateTimeout(timeoutMs);
}

bool Runner::waitForNextFrame(int timeoutMs, bool requestRender) {
    if (!m_terminalEngine) return false;
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested wait operations are not supported"));
        return false;
    }
    if (timeoutMs < minimumTimeoutMs || timeoutMs > maximumTimeoutMs) {
        failScenario(QStringLiteral("Timeout must be between %1 and %2 ms")
                         .arg(minimumTimeoutMs)
                         .arg(maximumTimeoutMs));
        return false;
    }

    const quint64 baseline = m_terminalEngine->frameSequence();
    bool received = false;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    const QMetaObject::Connection frameConnection =
        connect(m_terminalEngine, &TerminalEngine::frameRendered, &loop, [&](quint64 sequence) {
            if (sequence > baseline) {
                received = true;
                loop.quit();
            }
        });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    timeout.start(timeoutMs);
    if (requestRender) m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);

    if (m_state == State::Failed) return false;
    if (!received) {
        failScenario((requestRender
                          ? QStringLiteral("Timed out after %1 ms waiting for a committed frame")
                                .arg(timeoutMs)
                          : QStringLiteral("Timed out after %1 ms waiting for the application to "
                                           "schedule a repaint")
                                .arg(timeoutMs)) +
                     pausedClockHint());
        return false;
    }
    return true;
}

bool Runner::waitForTextCondition(const QString& text, bool shouldContain, int timeoutMs) {
    if (text.isEmpty()) {
        failScenario(QStringLiteral("Text conditions require a non-empty string"));
        return false;
    }
    QString error;
    const auto matches = [&] {
        const QString frame = currentFrameText(&error);
        return error.isEmpty() && (frame.contains(text) == shouldContain);
    };
    if (matches()) return true;
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
    const QMetaObject::Connection frameConnection =
        connect(m_terminalEngine, &TerminalEngine::frameRendered, &loop, [&](quint64) {
            if (matches()) {
                matched = true;
                loop.quit();
            } else if (!error.isEmpty()) {
                loop.quit();
            }
        });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    timeout.start(static_cast<int>((std::max<qint64>)(1, deadline.remainingTime())));
    m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);

    if (m_state == State::Failed) return false;
    if (!error.isEmpty()) {
        failScenario(error);
        return false;
    }
    if (!matched) {
        failScenario(QStringLiteral("Timed out after %1 ms waiting for text \"%2\"")
                         .arg(timeoutMs)
                         .arg(text) +
                     pausedClockHint());
        return false;
    }
    return true;
}

void Runner::sleep(int durationMs) {
    if (!ensureRunning("sleep")) return;
    // Under the virtual clock a sleep means "let this much application time
    // pass", so existing scenarios run instantly without edits.
    if (Clock::isVirtual()) {
        advanceVirtualTime("sleep", durationMs);
        return;
    }
    const int validated = validateTimeout(durationMs);
    if (!validated) return;
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

void Runner::advanceTime(int durationMs) {
    if (!ensureRunning("advanceTime")) return;
    if (!Clock::isVirtual()) {
        failScenario(QStringLiteral(
            "advanceTime() requires the virtual clock; run with QMLTUI_RUNNER_CLOCK=virtual "
            "or qmltui_add_scenario(... CLOCK VIRTUAL)"));
        return;
    }
    advanceVirtualTime("advanceTime", durationMs);
}

void Runner::advanceVirtualTime(const char* operation, int durationMs) {
    Q_UNUSED(operation);
    const int validated = validateTimeout(durationMs);
    if (!validated) return;
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested wait operations are not supported"));
        return;
    }

    Clock* clock = Clock::instance();
    const qint64 target = clock->elapsed() + validated;
    while (clock->elapsed() < target) {
        // Step at most 16 ms — the animation tick — or to the next QmlTui timer
        // deadline when that is sooner, so timers fire at their exact time.
        qint64 next = (std::min)(clock->elapsed() + 16, target);
        const qint64 deadline = clock->nextTimerDeadline();
        if (deadline > clock->elapsed() && deadline < next) next = deadline;
        clock->advanceTo(next);
        // Timer handlers queue renders and deferred deletions; deliver them
        // now. A step that invalidated nothing commits no frame.
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        if (m_state != State::Running) return;
    }
}

double Runner::now() const {
    return static_cast<double>(Clock::instance()->elapsed());
}

void Runner::useVirtualClock() {
    Clock::setVirtual();
}

void Runner::waitForFrame(int timeoutMs) {
    if (!ensureRunning("waitForFrame")) return;
    const int validated = validateTimeout(timeoutMs);
    if (validated) (void)waitForNextFrame(validated);
}

void Runner::expectRepaint(int timeoutMs) {
    if (!ensureRunning("expectRepaint")) return;
    const int validated = validateTimeout(timeoutMs);
    if (!validated) return;
    // Deliberately no draining of an in-flight render here: after an inject*
    // call the pending render *is* the repaint the application scheduled, and
    // consuming it would make this assertion vacuous.
    (void)waitForNextFrame(validated, /*requestRender=*/false);
}

void Runner::resize(int columns, int rows) {
    if (!ensureRunning("resize")) return;
    if (columns < 1 || columns > 1000 || rows < 1 || rows > 1000) {
        failScenario(
            QStringLiteral("resize() requires columns and rows between 1 and 1000, got %1x%2")
                .arg(columns)
                .arg(rows));
        return;
    }

    const bool viewportChanged =
        m_terminalEngine->terminalWidth() != columns || m_terminalEngine->terminalHeight() != rows;
    const quint64 sequenceBefore = m_terminalEngine->frameSequence();
    const auto frameMatches = [&] {
        if (viewportChanged && m_terminalEngine->frameSequence() <= sequenceBefore) return false;
        const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
        if (!frame || frame->width() != columns) return false;
        if (m_terminal && m_terminal->mode() == Terminal::Mode::Inline) {
            const int maxInlineHeight = (std::max)(1, rows - 1);
            return m_terminalEngine->terminalWidth() == columns &&
                   m_terminalEngine->terminalHeight() == rows && frame->height() <= maxInlineHeight;
        }
        return frame->height() == rows;
    };

    if (m_cast && m_cast->isOpen()) {
        drainCast();
        m_cast->resize(castSeconds(), columns, rows);
    }
    m_terminalEngine->setTestViewportSize(QSize(columns, rows));
    // The next committed frame is normally already at the new size, but a
    // render requested before the resize may still land first; keep waiting
    // until the dimensions match, bounded by the operation timeout.
    QDeadlineTimer deadline(m_operationTimeoutMs);
    while (!frameMatches()) {
        if (deadline.hasExpired()) {
            const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
            failScenario(
                QStringLiteral("Timed out after %1 ms waiting for a %2x%3 frame (still %4x%5)")
                    .arg(m_operationTimeoutMs)
                    .arg(columns)
                    .arg(rows)
                    .arg(frame ? frame->width() : 0)
                    .arg(frame ? frame->height() : 0));
            return;
        }
        const int remaining = static_cast<int>((std::max<qint64>)(1, deadline.remainingTime()));
        if (!waitForNextFrame(remaining)) return;
    }
}

QVariantMap Runner::size() {
    if (!ensureRunning("size") || !m_terminalEngine) return {};
    if (m_terminalEngine->frameSequence() == 0) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return {};
    }
    const int width = m_terminalEngine->terminalWidth();
    const int height = m_terminalEngine->terminalHeight();
    if (width <= 0 || height <= 0) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return {};
    }
    return {{QStringLiteral("columns"), width}, {QStringLiteral("rows"), height}};
}

int Runner::frameCount() const {
    return m_terminalEngine ? static_cast<int>(m_terminalEngine->frameSequence()) : -1;
}

QVariantMap Runner::frameStats() const {
    QVariantMap stats;
    if (!m_terminalEngine) return stats;
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

void Runner::waitForCondition(const QJSValue& condition, int timeoutMs) {
    if (!ensureRunning("waitForCondition")) return;
    if (!condition.isCallable()) {
        failScenario(QStringLiteral("app.waitForCondition() requires a callable condition"));
        return;
    }
    const int validated = validateTimeout(timeoutMs);
    if (!validated) return;
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

        const QJSValue result =
            condition.call({QJSValue(frameText), QJSValue(static_cast<double>(elapsed.elapsed()))});
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

    if (m_state == State::Failed || matched) return;
    failScenario(QStringLiteral("Timed out after %1 ms waiting for condition").arg(validated) +
                 pausedClockHint());
}

void Runner::waitForText(const QString& text, int timeoutMs) {
    if (!ensureRunning("waitForText")) return;
    const int validated = validateTimeout(timeoutMs);
    if (validated) (void)waitForTextCondition(text, true, validated);
}

void Runner::expectText(const QString& text, int timeoutMs) {
    if (!ensureRunning("expectText") || !waitForNextFrame(resolveExpectTimeout(timeoutMs))) return;
    QString error;
    const QString frame = currentFrameText(&error);
    if (!error.isEmpty())
        failScenario(error);
    else if (text.isEmpty() || !frame.contains(text))
        failScenario(QStringLiteral("Expected frame to contain \"%1\"").arg(text));
}

void Runner::expectNoText(const QString& text, int timeoutMs) {
    if (!ensureRunning("expectNoText") || !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString frame = currentFrameText(&error);
    if (!error.isEmpty())
        failScenario(error);
    else if (text.isEmpty() || frame.contains(text))
        failScenario(QStringLiteral("Expected frame not to contain \"%1\"").arg(text));
}

bool Runner::waitForEngineCondition(const std::function<bool()>& check, int timeoutMs,
                                    const QString& description) {
    if (check()) return true;
    if (m_state == State::Failed) return false;

    QDeadlineTimer deadline(timeoutMs);
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested Test wait operations are not supported"));
        return false;
    }

    bool matched = false;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    const auto evaluate = [&] {
        if (check()) {
            matched = true;
            loop.quit();
        } else if (m_state == State::Failed) {
            loop.quit();
        }
    };
    const QMetaObject::Connection frameConnection = connect(
        m_terminalEngine, &TerminalEngine::frameRendered, &loop, [&](quint64) { evaluate(); });
    const QMetaObject::Connection scrollbackConnection =
        connect(m_terminalEngine, &TerminalEngine::scrollbackCommitted, &loop,
                [&](quint64) { evaluate(); });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    timeout.start(static_cast<int>((std::max<qint64>)(1, deadline.remainingTime())));
    m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);
    disconnect(scrollbackConnection);

    if (m_state == State::Failed) return false;
    if (!matched) {
        failScenario(
            QStringLiteral("Timed out after %1 ms waiting for %2").arg(timeoutMs).arg(description) +
            pausedClockHint());
        return false;
    }
    return true;
}

bool Runner::ensureScrollbackSession(const char* operation) {
    if (!m_terminalEngine || !m_terminal) {
        failScenario(QStringLiteral("TerminalEngine is no longer available"));
        return false;
    }
    if (m_terminal->mode() != Terminal::Mode::Inline) {
        failScenario(QStringLiteral("app.%1() needs a Terminal in Inline mode; "
                                    "scrollback stays pending in AltScreen mode")
                         .arg(QLatin1String(operation)));
        return false;
    }
    if (!m_terminalEngine->scrollbackHistoryEnabled()) {
        failScenario(QStringLiteral("app.%1() needs scrollback history, which this "
                                    "session does not record")
                         .arg(QLatin1String(operation)));
        return false;
    }
    return true;
}

void Runner::armRecording() {
    // The recorded terminal is an in-memory one with fixed capabilities: 24-bit
    // colour, no synchronized-output bracketing, no kitty protocols. Fixing them
    // also stops the engine from probing, so no query bytes land in the cast.
    TerminalCapabilities capabilities = m_terminalEngine->capabilities();
    capabilities.colorDepth = ColorDepth::TrueColor;
    capabilities.syncOutput = false;
    capabilities.kittyKeyboard = false;
    capabilities.kittyGraphics = false;
    capabilities.sixelGraphics = false;
    capabilities.mayProbeGraphics = false;
    capabilities.isTty = true;
    m_terminalEngine->setCapabilities(capabilities);
    m_terminalEngine->setLiveOutput(true);
    connect(m_terminalEngine, &TerminalEngine::frameRendered, this,
            [this](quint64) { drainCast(); });

    const QString requested = QString::fromUtf8(qgetenv("QMLTUI_RUNNER_RECORD")).trimmed();
    QString error;
    if (!requested.isEmpty() && !openCast(requested, &error) && m_configurationError.isEmpty())
        m_configurationError = error;
}

bool Runner::openCast(const QString& requestedPath, QString* error) {
    QString path = requestedPath;
    if (QDir::isAbsolutePath(path)) {
        QDir().mkpath(QFileInfo(path).path());
    } else {
        path = resolveArtifactPath(requestedPath, error);
        if (path.isEmpty()) return false;
    }
    if (!m_cast) m_cast = std::make_unique<CastRecorder>();
    if (m_cast->isOpen()) closeCast();
    const QString title = QFileInfo(path).completeBaseName();
    if (!m_cast->open(path, m_terminalEngine->terminalWidth(), m_terminalEngine->terminalHeight(),
                      title, error))
        return false;
    m_castStartMs = Clock::instance()->elapsed();
    const QString relative = QDir(m_artifactDirectory).relativeFilePath(path);
    m_castPaths.append(relative.startsWith(QLatin1String("..")) ? path : QDir::cleanPath(relative));
    return true;
}

void Runner::closeCast() {
    if (!m_cast) return;
    drainCast();
    m_cast->close();
}

void Runner::drainCast() {
    if (!s_recordingBackend) return;
    if (!m_cast || !m_cast->isOpen()) {
        s_recordingBackend->clearOutput();
        return;
    }
    const std::string& bytes = s_recordingBackend->output();
    if (bytes.empty()) return;
    m_cast->output(castSeconds(), bytes);
    s_recordingBackend->clearOutput();
}

double Runner::castSeconds() const {
    return static_cast<double>(Clock::instance()->elapsed() - m_castStartMs) / 1000.0;
}

void Runner::startRecording(const QString& relativePath) {
    if (!ensureRunning("startRecording")) return;
    if (!s_recordingBackend || !m_terminalEngine) {
        failScenario(QStringLiteral("app.startRecording() requires QMLTUI_RUNNER_RECORD "
                                    "(a RECORD scenario) so the terminal bytes are captured"));
        return;
    }
    QString pathError;
    if (resolveArtifactPath(relativePath, &pathError).isEmpty()) {
        failScenario(pathError);
        return;
    }
    if (m_cast && m_cast->isOpen()) closeCast();
    // Bytes emitted between recordings belong to neither cast; the new one
    // opens with a full repaint instead.
    s_recordingBackend->clearOutput();
    QString error;
    if (!openCast(relativePath, &error)) {
        failScenario(error);
        return;
    }
    if (m_terminalEngine->renderer()) m_terminalEngine->renderer()->discardCommittedState();
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::stopRecording() {
    if (!ensureRunning("stopRecording")) return;
    if (!m_cast || !m_cast->isOpen()) {
        qWarning().noquote() << QStringLiteral(
            "[Test] WARNING: stopRecording() ignored because no recording is in progress");
        return;
    }
    closeCast();
}

void Runner::marker(const QString& label) {
    if (!ensureRunning("marker")) return;
    if (!m_cast || !m_cast->isOpen()) {
        qWarning().noquote()
            << QStringLiteral(
                   "[Test] WARNING: marker(\"%1\") ignored because no recording is in progress")
                   .arg(label);
        return;
    }
    drainCast();
    m_cast->marker(castSeconds(), label);
}

void Runner::fail(const QString& message) {
    if (!ensureRunning("fail")) return;
    failScenario(message.isEmpty() ? QStringLiteral("Scenario failed") : message);
}

QJSValue Runner::rootObject() {
    if (!m_qmlEngine || !m_terminal) return QJSValue();
    return m_qmlEngine->newQObject(m_terminal);
}

void Runner::finish() {
    if (!ensureRunning("finish")) return;
    m_state = State::Finishing;
    if (!m_evaluating) {
        m_state = State::Passed;
        m_watchdog.stop();
        stopHardWatchdog();
        scheduleExit(0);
    }
}

void Runner::onPromiseResolved() {
    if (m_state == State::Running || m_state == State::Finishing) {
        m_state = State::Passed;
        m_watchdog.stop();
        stopHardWatchdog();
        scheduleExit(0);
    }
}

void Runner::onPromiseRejected(const QString& error) {
    if (m_state != State::Failed && m_state != State::Passed) {
        failScenario(error.isEmpty() ? QStringLiteral("Promise rejected") : error, false);
    }
}

void Runner::failScenario(const QString& message, bool raiseJsError) {
    if (m_state == State::Failed || m_state == State::Passed) return;
    m_state = State::Failed;
    m_watchdog.stop();
    // A script can catch the failure and keep running. Evaluation owns the
    // hard watchdog until JavaScript has actually returned.
    if (!m_evaluating) stopHardWatchdog();
    m_message = message;
    completeCurrentOperation(QStringLiteral("failed"));
    if (m_activeLoop) m_activeLoop->quit();
    if (raiseJsError && m_evaluating && m_qmlEngine) m_qmlEngine->throwError(message);
    scheduleExit(1);
}

void Runner::scheduleExit(int exitCode) {
    if (m_exitScheduled) return;
    m_exitScheduled = true;
    completeCurrentOperation(exitCode == 0 ? QStringLiteral("passed") : QStringLiteral("failed"));
    if (m_cast && m_cast->isOpen()) {
        // Leave the recorded terminal the way a real session would: cursor
        // shown, mouse reporting off, styles reset.
        if (m_terminalEngine) m_terminalEngine->setTerminalOutputEnabled(false);
        closeCast();
    }

    QString artifactError;
    if (!writeFinalFrameArtifact(&artifactError) && exitCode == 0) {
        exitCode = 1;
        m_state = State::Failed;
        m_message = artifactError;
    }
    if (!writeResultArtifact(exitCode, &artifactError)) {
        exitCode = 1;
        m_state = State::Failed;
        if (m_message.isEmpty()) m_message = artifactError;
    }

    QString summary =
        exitCode == 0
            ? QStringLiteral("[Test] PASSED in %1 ms; artifacts: %2")
                  .arg(m_elapsed.isValid() ? m_elapsed.elapsed() : 0)
                  .arg(m_artifactDirectory)
            : QStringLiteral("[Test] FAILED: %1; artifacts: %2")
                  .arg(m_message.isEmpty() ? QStringLiteral("Scenario failed") : m_message,
                       m_artifactDirectory);
    // A rewritten baseline must never be silent: name every file that changed.
    if (!m_updatedSnapshots.isEmpty()) {
        summary += QStringLiteral("; updated snapshots: %1")
                       .arg(m_updatedSnapshots.join(QStringLiteral(", ")));
    }
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

QString Runner::formatJsError(const QJSValue& error) const {
    return QStringLiteral("JavaScript error at %1:%2: %3\n%4")
        .arg(error.property(QStringLiteral("fileName")).toString())
        .arg(error.property(QStringLiteral("lineNumber")).toInt())
        .arg(error.toString(), error.property(QStringLiteral("stack")).toString());
}

} // namespace qmltui

#include "Runner.moc"
