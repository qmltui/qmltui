#include <QmlTui/Runner/Runner.h>
#include <QmlTui/Runner/CastRecorder_p.h>
#include <QmlTui/Runner/MemoryTerminalBackend.h>
#include <QmlTui/Core/Primitives/TerminalBackend.h>
#include <QmlTui/Core/Primitives/TerminalCapabilities.h>
#include <QmlTui/Core/Primitives/Color.h>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Engine/Clock_p.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

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
#include <QTextBoundaryFinder>

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

constexpr int minimumTimeoutMs = 1;
constexpr int maximumTimeoutMs = 30000;
// A single wait longer than 30 s is a scenario bug, but a long walkthrough is
// not: the whole-scenario watchdog accepts up to five minutes.
constexpr int maximumScenarioTimeoutMs = 300000;
constexpr int conditionPollIntervalMs = 10;

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

struct ParsedKey {
    int key = Qt::Key_unknown;
    QString text;
};

bool parseKey(const QString& input, ParsedKey* parsed) {
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

    if (trimmed.size() != 1 || trimmed.front().unicode() > 0x7f) return false;

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
                    QString* invalidModifier) {
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

bool parseMouseButton(const QString& name, int* button, QString* error) {
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
    if (error) *error = QStringLiteral("Unknown mouse button: %1").arg(name);
    return false;
}

// Appended to wait-timeout diagnostics: under the virtual clock, application
// time cannot pass on its own, which is the usual reason such a wait expires.
QString pausedClockHint() {
    return Clock::isVirtual()
               ? QStringLiteral(" (the virtual clock is paused; advance it with advanceTime())")
               : QString();
}

QString absoluteOrResourcePath(const QString& path) {
    return path.startsWith(QLatin1String(":")) ? path : QFileInfo(path).absoluteFilePath();
}

bool isSingleGrapheme(const QString& text) {
    if (text.isEmpty()) return false;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    finder.setPosition(0);
    return finder.toNextBoundary() == text.size();
}

// Depth-first search for the first visible item with the given objectName.
// A hidden subtree cannot contribute cells to the frame, so it is skipped.
Item* findVisibleItem(Item* item, const QString& objectName) {
    if (!item || !item->visible()) return nullptr;
    if (item->objectName() == objectName) return item;
    const QList<Item*>& children = item->childItems();
    for (Item* child : children) {
        if (Item* match = findVisibleItem(child, objectName)) return match;
    }
    return nullptr;
}

QString rgbaString(uint32_t rgba) {
    const uint32_t argb = Color::toArgb(rgba);
    return QStringLiteral("#%1%2%3%4")
        .arg((argb >> 16) & 0xffU, 2, 16, QLatin1Char('0'))
        .arg((argb >> 8) & 0xffU, 2, 16, QLatin1Char('0'))
        .arg(argb & 0xffU, 2, 16, QLatin1Char('0'))
        .arg((argb >> 24) & 0xffU, 2, 16, QLatin1Char('0'))
        .toUpper();
}

QString attributeNames(TextAttribute attributes) {
    static const std::pair<TextAttribute, const char*> names[] = {
        {TextAttribute::Bold, "bold"},        {TextAttribute::Dim, "dim"},
        {TextAttribute::Italic, "italic"},    {TextAttribute::Underline, "underline"},
        {TextAttribute::Reversed, "reverse"}, {TextAttribute::Blink, "blink"},
        {TextAttribute::Conceal, "conceal"},  {TextAttribute::Strikethrough, "strikethrough"},
    };
    QStringList result;
    for (const auto& [flag, name] : names) {
        if (static_cast<uint8_t>(attributes) & static_cast<uint8_t>(flag))
            result.append(QLatin1String(name));
    }
    return result.join(QLatin1Char(','));
}

bool parseRgbaString(const QString& text, uint32_t* rgba) {
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
            *rgba = (static_cast<uint32_t>(alpha) << 24) | (static_cast<uint32_t>(red) << 16) |
                    (static_cast<uint32_t>(green) << 8) | static_cast<uint32_t>(blue);
            return true;
        }
    }
    // Named colors resolve the same way QML "color" strings do, so "red" matches
    // a cell painted with color: "red" (an ANSI palette entry, not #FF0000).
    const Color color = Color::fromString(text.trimmed());
    if (!color.isValid()) return false;
    *rgba = color.argb32();
    return true;
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
        if (!evalCode.contains(QLatin1String("main(")) &&
            !evalCode.contains(QLatin1String("main ("))) {
            generatedScript =
                QStringLiteral("function main(terminal) {\n    %1;\n}\n").arg(evalCode).toUtf8();
        } else {
            generatedScript = evalCode.toUtf8();
        }
    } else if (capture) {
        mode = QStringLiteral("capture");
        captureAfterMs = environmentInteger("QMLTUI_RUNNER_CAPTURE_AFTER_MS", 0, 0,
                                            maximumTimeoutMs, &configurationError);
        generatedScript = QByteArrayLiteral("function main(terminal) {\n");
        if (captureAfterMs > 0)
            generatedScript +=
                QStringLiteral("    terminal.sleep(%1);\n").arg(captureAfterMs).toUtf8();
        generatedScript += QByteArrayLiteral("    terminal.captureText('capture.txt');\n"
                                             "    terminal.captureStyled('capture_styled.txt');\n"
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
        const int columns =
            environmentInteger("QMLTUI_RUNNER_COLUMNS", 80, 1, 1000, &configurationError);
        const int rows = environmentInteger("QMLTUI_RUNNER_ROWS", 24, 1, 1000, &configurationError);
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
        // function main(terminal) and may import sibling helper modules.
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
            failScenario(QStringLiteral("Module scenario must export a function main(terminal)"),
                         false);
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
            failScenario(QStringLiteral("Scenario must define a function main(terminal)"), false);
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
                nativeRunner.click(match.x + Math.floor(Math.max(0, match.width - 1) / 2),
                                   match.y,
                                   optionValue("region.clickText", options, "button", "left"),
                                   optionValue("region.clickText", options, "modifiers", []));
            });
        };
        region.click = function(options) {
            composite("region.click", function() {
                rejectUnknownOptions("region.click", options || {}, ["button", "modifiers"]);
                nativeRunner.click(x + Math.floor((width - 1) / 2),
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
    // The facade calls this once a terminal.* call has returned normally. Closing
    // the record here means a later failure — a JavaScript error on a following
    // line, or the scenario watchdog — is no longer attributed to an operation
    // that already succeeded.
    completeCurrentOperation(QStringLiteral("passed"));
    m_currentOperation = -1;
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
        failScenario(QStringLiteral("terminal.%1() is not valid in the current scenario state")
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

void Runner::press(const QString& key, const QStringList& modifiers) {
    sendKey("press", key, modifiers, /*awaitFrame=*/true);
}

void Runner::injectKey(const QString& key, const QStringList& modifiers) {
    sendKey("injectKey", key, modifiers, /*awaitFrame=*/false);
}

void Runner::sendKey(const char* operation, const QString& key, const QStringList& modifiers,
                     bool awaitFrame) {
    if (!ensureRunning(operation)) return;

    ParsedKey parsed;
    if (!parseKey(key, &parsed)) {
        // Any single grapheme a terminal sends as text is a valid key: route
        // it through the input decoder like typed text. Modifier combinations
        // have no byte representation for arbitrary text, so they are invalid.
        const QString trimmed = key.trimmed();
        if (isSingleGrapheme(trimmed)) {
            if (!modifiers.isEmpty()) {
                failScenario(QStringLiteral("Modifiers are not supported with the text key \"%1\"")
                                 .arg(trimmed));
                return;
            }
            m_terminalEngine->feedInput(trimmed.toUtf8());
            if (awaitFrame) (void)waitForNextFrame(m_operationTimeoutMs);
            return;
        }
        failScenario(QStringLiteral("Unknown or unsupported key: %1").arg(key));
        return;
    }
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    QString invalidModifier;
    if (!parseModifiers(modifiers, &parsedModifiers, &invalidModifier)) {
        failScenario(QStringLiteral("Unknown modifier: %1").arg(invalidModifier));
        return;
    }

    if (parsed.key == Qt::Key_Backtab) parsedModifiers |= Qt::ShiftModifier;
    if (parsedModifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
        parsed.text.clear();
    } else if ((parsedModifiers & Qt::ShiftModifier) && parsed.text.size() == 1 &&
               parsed.text.front().isLetter()) {
        parsed.text = parsed.text.toUpper();
    }

    m_terminalEngine->handleKeyInput(parsed.key, parsed.text, parsedModifiers);
    if (awaitFrame) (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::typeText(const QString& text) {
    sendText("typeText", text, /*awaitFrame=*/true);
}

void Runner::injectText(const QString& text) {
    // Deliberately does not wait for a frame: typeText() would request one and
    // consume the repaint that expectRepaint() needs to observe.
    sendText("injectText", text, /*awaitFrame=*/false);
}

void Runner::sendText(const char* operation, const QString& text, bool awaitFrame) {
    if (!ensureRunning(operation)) return;
    if (text.isEmpty()) {
        failScenario(
            QStringLiteral("terminal.%1() requires non-empty text").arg(QLatin1String(operation)));
        return;
    }

    // The bytes take the same path as a real keyboard: the input decoder turns
    // them into one key event per character with the key code a terminal
    // keystroke carries, so a Keys handler matching Qt.Key_W or Qt.Key_1 sees
    // scripted typing exactly as it sees a user typing, and every future
    // decoder change applies to scripts without a parallel table here.
    m_terminalEngine->feedInput(text.toUtf8());
    if (awaitFrame) (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::paste(const QString& text) {
    sendPaste("paste", text, /*awaitFrame=*/true);
}

void Runner::injectPaste(const QString& text) {
    sendPaste("injectPaste", text, /*awaitFrame=*/false);
}

void Runner::sendPaste(const char* operation, const QString& text, bool awaitFrame) {
    if (!ensureRunning(operation)) return;
    if (text.isEmpty()) {
        failScenario(
            QStringLiteral("terminal.%1() requires non-empty text").arg(QLatin1String(operation)));
        return;
    }
    // The decoder frames the bytes exactly as a terminal in bracketed-paste
    // mode delivers them, so newlines and tabs reach TextInput::paste()
    // untranslated instead of acting as Return and Tab keys.
    m_terminalEngine->feedInput(QByteArrayLiteral("\x1b[200~") + text.toUtf8() +
                                QByteArrayLiteral("\x1b[201~"));
    if (awaitFrame) (void)waitForNextFrame(m_operationTimeoutMs);
}

bool Runner::parseMouseArguments(const QString& button, const QStringList& modifiers,
                                 int* parsedButton, Qt::KeyboardModifiers* parsedModifiers) {
    QString error;
    if (!parseMouseButton(button, parsedButton, &error)) {
        failScenario(error);
        return false;
    }
    if (!parseModifiers(modifiers, parsedModifiers, &error)) {
        failScenario(QStringLiteral("Unknown modifier: %1").arg(error));
        return false;
    }
    return true;
}

void Runner::click(int x, int y, const QString& button, const QStringList& modifiers) {
    if (!ensureRunning("click")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::doubleClick(int x, int y, const QString& button, const QStringList& modifiers) {
    if (!ensureRunning("doubleClick")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::hover(int x, int y, const QStringList& modifiers) {
    if (!ensureRunning("hover")) return;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    QString invalidModifier;
    if (!parseModifiers(modifiers, &parsedModifiers, &invalidModifier)) {
        failScenario(QStringLiteral("Unknown modifier: %1").arg(invalidModifier));
        return;
    }
    // A terminal sends motion without a button only under any-motion
    // reporting. When the application never enabled it, the event does not
    // exist; the frame wait still runs so timing matches the delivered case.
    if (m_terminalEngine->mouseMotionReporting()) {
        // SGR button code 3 is "no button" in a motion report.
        m_terminalEngine->handleMouseInput(3, x, y, false, true, parsedModifiers);
    }
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::mouseDown(int x, int y, const QString& button, const QStringList& modifiers) {
    if (!ensureRunning("mouseDown")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, x, y, false, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::mouseUp(int x, int y, const QString& button, const QStringList& modifiers) {
    if (!ensureRunning("mouseUp")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, x, y, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::drag(int startX, int startY, int endX, int endY, const QString& button,
                  const QStringList& modifiers) {
    if (!ensureRunning("drag")) return;
    int b = 0;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseMouseArguments(button, modifiers, &b, &parsedModifiers)) return;
    m_terminalEngine->handleMouseInput(b, startX, startY, false, false, parsedModifiers);
    // Button-held motion is reported cell by cell under mode 1002; walk the
    // path so drag handlers see the same intermediate positions a user
    // produces instead of one jump to the end.
    const int steps = (std::max)(qAbs(endX - startX), qAbs(endY - startY));
    for (int step = 1; step <= steps; ++step) {
        const int x = startX + ((endX - startX) * step) / (std::max)(1, steps);
        const int y = startY + ((endY - startY) * step) / (std::max)(1, steps);
        m_terminalEngine->handleMouseInput(b, x, y, false, true, parsedModifiers);
    }
    m_terminalEngine->handleMouseInput(b, endX, endY, true, false, parsedModifiers);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::scroll(int x, int y, int deltaX, int deltaY, const QStringList& modifiers) {
    if (!ensureRunning("scroll")) return;
    QString error;
    Qt::KeyboardModifiers parsedModifiers = Qt::NoModifier;
    if (!parseModifiers(modifiers, &parsedModifiers, &error)) {
        failScenario(error);
        return;
    }
    m_terminalEngine->handleMouseInput(0, x, y, false, false, parsedModifiers, deltaX, deltaY);
    (void)waitForNextFrame(m_operationTimeoutMs);
}

void Runner::waitForRegex(const QString& pattern, int timeoutMs) {
    if (!ensureRunning("waitForRegex")) return;
    const int validated = validateTimeout(timeoutMs);
    if (!validated) return;

    const QRegularExpression re(pattern);
    if (!re.isValid()) {
        failScenario(
            QStringLiteral("Invalid regex pattern: %1 (%2)").arg(pattern, re.errorString()));
        return;
    }

    QString error;
    const auto matches = [&] {
        const QString frame = currentFrameText(&error);
        return error.isEmpty() && re.match(frame).hasMatch();
    };

    if (matches()) return;

    QDeadlineTimer deadline(validated);
    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested Test wait operations are not supported"));
        return;
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
            }
        });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    timeout.start(static_cast<int>((std::max<qint64>)(1, deadline.remainingTime())));
    m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);

    if (m_state == State::Failed) return;
    if (!matched) {
        failScenario(QStringLiteral("Timed out after %1 ms waiting for regex pattern \"%2\"")
                         .arg(validated)
                         .arg(pattern) +
                     pausedClockHint());
    }
}

void Runner::expectRegex(const QString& pattern, int timeoutMs) {
    if (!ensureRunning("expectRegex") || !waitForNextFrame(resolveExpectTimeout(timeoutMs))) return;
    const QRegularExpression re(pattern);
    if (!re.isValid()) {
        failScenario(
            QStringLiteral("Invalid regex pattern: %1 (%2)").arg(pattern, re.errorString()));
        return;
    }
    QString error;
    const QString frame = currentFrameText(&error);
    if (!error.isEmpty())
        failScenario(error);
    else if (!re.match(frame).hasMatch())
        failScenario(QStringLiteral("Expected frame to match regex \"%1\"").arg(pattern));
}

void Runner::expectTextInRegion(int x, int y, int width, int height, const QString& text,
                                int timeoutMs) {
    if (!ensureRunning("expectTextInRegion") || !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
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
    if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > frame->width() ||
        y + height > frame->height()) {
        failScenario(QStringLiteral("Region bounds [%1,%2,%3,%4] out of frame size %5x%6")
                         .arg(x)
                         .arg(y)
                         .arg(width)
                         .arg(height)
                         .arg(frame->width())
                         .arg(frame->height()));
        return;
    }

    QString regionText;
    for (int r = y; r < y + height; ++r) {
        QString line;
        for (int c = x; c < x + width; ++c) {
            const Cell& cell = frame->cellAt(c, r);
            if (cell.continuation) continue;
            line.append(cell.character().isEmpty() ? QStringLiteral(" ")
                                                   : QString::fromUtf8(cell.character()));
        }
        regionText.append(line);
        regionText.append(QLatin1Char('\n'));
    }

    if (!regionText.contains(text)) {
        failScenario(QStringLiteral("Expected region [%1,%2,%3,%4] to contain \"%5\"")
                         .arg(x)
                         .arg(y)
                         .arg(width)
                         .arg(height)
                         .arg(text));
    }
}

void Runner::expectStyle(int x, int y, const QJSValue& expectedStyle, int timeoutMs) {
    if (!ensureRunning("expectStyle") || !waitForNextFrame(resolveExpectTimeout(timeoutMs))) return;
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
                         .arg(x)
                         .arg(y)
                         .arg(frame->width())
                         .arg(frame->height()));
        return;
    }
    if (!expectedStyle.isObject()) {
        failScenario(
            QStringLiteral("expectStyle requires an object with expected cell attributes"));
        return;
    }

    const Cell& cell = frame->cellAt(x, y);

    if (expectedStyle.hasProperty(QStringLiteral("bold"))) {
        const bool expectedBold = expectedStyle.property(QStringLiteral("bold")).toBool();
        const bool actualBold = (static_cast<uint8_t>(cell.attributes) &
                                 static_cast<uint8_t>(TextAttribute::Bold)) != 0;
        if (expectedBold != actualBold) {
            failScenario(QStringLiteral("Cell at (%1,%2) bold expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedBold)
                             .arg(actualBold));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("dim"))) {
        const bool expectedDim = expectedStyle.property(QStringLiteral("dim")).toBool();
        const bool actualDim =
            (static_cast<uint8_t>(cell.attributes) & static_cast<uint8_t>(TextAttribute::Dim)) != 0;
        if (expectedDim != actualDim) {
            failScenario(QStringLiteral("Cell at (%1,%2) dim expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedDim)
                             .arg(actualDim));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("italic"))) {
        const bool expectedItalic = expectedStyle.property(QStringLiteral("italic")).toBool();
        const bool actualItalic = (static_cast<uint8_t>(cell.attributes) &
                                   static_cast<uint8_t>(TextAttribute::Italic)) != 0;
        if (expectedItalic != actualItalic) {
            failScenario(QStringLiteral("Cell at (%1,%2) italic expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedItalic)
                             .arg(actualItalic));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("underline"))) {
        const bool expectedUnderline = expectedStyle.property(QStringLiteral("underline")).toBool();
        const bool actualUnderline = (static_cast<uint8_t>(cell.attributes) &
                                      static_cast<uint8_t>(TextAttribute::Underline)) != 0;
        if (expectedUnderline != actualUnderline) {
            failScenario(QStringLiteral("Cell at (%1,%2) underline expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedUnderline)
                             .arg(actualUnderline));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("reverse")) ||
        expectedStyle.hasProperty(QStringLiteral("reversed"))) {
        const bool expectedRev = expectedStyle.hasProperty(QStringLiteral("reverse"))
                                     ? expectedStyle.property(QStringLiteral("reverse")).toBool()
                                     : expectedStyle.property(QStringLiteral("reversed")).toBool();
        const bool actualRev = (static_cast<uint8_t>(cell.attributes) &
                                static_cast<uint8_t>(TextAttribute::Reversed)) != 0;
        if (expectedRev != actualRev) {
            failScenario(QStringLiteral("Cell at (%1,%2) reverse expected %3 but got %4")
                             .arg(x)
                             .arg(y)
                             .arg(expectedRev)
                             .arg(actualRev));
            return;
        }
    }
    if (expectedStyle.hasProperty(QStringLiteral("character"))) {
        const QString expectedChar = expectedStyle.property(QStringLiteral("character")).toString();
        const QString actualChar = QString::fromUtf8(cell.character());
        if (expectedChar != actualChar) {
            failScenario(QStringLiteral("Cell at (%1,%2) character expected \"%3\" but got \"%4\"")
                             .arg(x)
                             .arg(y)
                             .arg(expectedChar)
                             .arg(actualChar));
            return;
        }
    }
    const auto compareColor = [&](const QString& property, uint32_t actual) {
        if (!expectedStyle.hasProperty(property)) return true;
        const QString expectedText = expectedStyle.property(property).toString();
        if (expectedText.trimmed() == QLatin1String("default")) {
            if (actual != 0) {
                failScenario(QStringLiteral("Cell at (%1,%2) %3 expected default but got %4")
                                 .arg(x)
                                 .arg(y)
                                 .arg(property, rgbaString(actual)));
                return false;
            }
            return true;
        }
        uint32_t expectedRgba = 0;
        if (!parseRgbaString(expectedText, &expectedRgba)) {
            failScenario(QStringLiteral("Cell style %1 is not a valid color: %2")
                             .arg(property, expectedText));
            return false;
        }
        if (Color::toArgb(expectedRgba) != Color::toArgb(actual)) {
            failScenario(QStringLiteral("Cell at (%1,%2) %3 expected %4 but got %5")
                             .arg(x)
                             .arg(y)
                             .arg(property, expectedText, rgbaString(actual)));
            return false;
        }
        return true;
    };
    if (!compareColor(QStringLiteral("foreground"), cell.fgColor) ||
        !compareColor(QStringLiteral("background"), cell.bgColor)) {
        return;
    }
}

void Runner::expectSnapshot(const QString& snapshotRelativePath, int timeoutMs) {
    if (!ensureRunning("expectSnapshot") || !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString actualText = currentFrameText(&error);
    if (!error.isEmpty()) {
        failScenario(error);
        return;
    }
    compareSnapshot(snapshotRelativePath, actualText);
}

void Runner::expectStyled(const QString& snapshotRelativePath, int timeoutMs) {
    if (!ensureRunning("expectStyled") || !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString actualText = currentStyledFrameText(&error);
    if (!error.isEmpty()) {
        failScenario(error);
        return;
    }
    compareSnapshot(snapshotRelativePath, actualText);
}

void Runner::compareSnapshot(const QString& snapshotRelativePath, const QString& actualText) {
    const QString snapshotPath = resolveSnapshotPath(snapshotRelativePath);
    QFile snapshotFile(snapshotPath);
    const bool baselineExists = snapshotFile.exists();
    QString expectedText;
    if (baselineExists) {
        if (!snapshotFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            failScenario(QStringLiteral("Cannot open snapshot file %1: %2")
                             .arg(snapshotPath, snapshotFile.errorString()));
            return;
        }
        expectedText = QString::fromUtf8(snapshotFile.readAll());
        snapshotFile.close();
        if (actualText == expectedText) return;
    }

    // Derive the name from the whole relative path, not just the file name,
    // so two snapshots called a/frame.txt and b/frame.txt do not overwrite
    // each other's .actual.txt / .diff artifacts. The conventional
    // "snapshots/" prefix is dropped because the artifacts already live
    // under snapshots/.
    QString baseName = snapshotRelativePath;
    if (baseName.startsWith(QLatin1String("snapshots/")))
        baseName = baseName.mid(QStringLiteral("snapshots/").size());
    if (const int dot = baseName.lastIndexOf(QLatin1Char('.')); dot > 0) baseName.truncate(dot);
    static const QRegularExpression sanitizer(QStringLiteral("[^A-Za-z0-9._-]+"));
    baseName.replace(sanitizer, QStringLiteral("_"));
    baseName = baseName.mid(0, 120);
    if (baseName.isEmpty()) baseName = QStringLiteral("snapshot");
    const QString actualArtifactPath = QStringLiteral("snapshots/%1.actual.txt").arg(baseName);
    const QString diffArtifactPath = QStringLiteral("snapshots/%1.diff").arg(baseName);

    if (m_updateSnapshots) {
        // A Qt resource cannot be written back; record the frame where the
        // developer can copy it from and name the file it belongs in.
        if (snapshotPath.startsWith(QLatin1Char(':'))) {
            QString writeError;
            if (writeTextArtifact(actualArtifactPath, actualText, &writeError))
                m_capturePaths.append(actualArtifactPath);
            failScenario(QStringLiteral("Snapshot update mode cannot update resource snapshot %1; "
                                        "the actual frame was written to %2")
                             .arg(snapshotPath, actualArtifactPath));
            return;
        }
        if (!QDir().mkpath(QFileInfo(snapshotPath).absolutePath())) {
            failScenario(
                QStringLiteral("Cannot create snapshot directory for %1").arg(snapshotPath));
            return;
        }
        QSaveFile baseline(snapshotPath);
        const QByteArray bytes = actualText.toUtf8();
        if (!baseline.open(QIODevice::WriteOnly | QIODevice::Text) ||
            baseline.write(bytes) != bytes.size() || !baseline.commit()) {
            failScenario(QStringLiteral("Cannot update snapshot %1: %2")
                             .arg(snapshotPath, baseline.errorString()));
            return;
        }
        m_updatedSnapshots.append(snapshotPath);
        return;
    }

    if (!baselineExists) {
        failScenario(QStringLiteral("Missing snapshot baseline %1; run with "
                                    "QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 to record it")
                         .arg(snapshotPath));
        return;
    }

    QString writeError;
    if (!writeTextArtifact(actualArtifactPath, actualText, &writeError) ||
        !writeTextArtifact(diffArtifactPath,
                           unifiedTextDiff(snapshotPath, expectedText, actualText), &writeError)) {
        failScenario(writeError);
        return;
    }
    m_capturePaths.append(actualArtifactPath);
    m_capturePaths.append(diffArtifactPath);
    failScenario(QStringLiteral("Snapshot mismatch for %1 (actual: %2, diff: %3)")
                     .arg(snapshotPath, actualArtifactPath, diffArtifactPath));
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

    const auto frameMatches = [&] {
        const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
        return frame && frame->width() == columns && frame->height() == rows;
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
        failScenario(QStringLiteral("terminal.waitForCondition() requires a callable condition"));
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
        failScenario(QStringLiteral("terminal.%1() needs a Terminal in Inline mode; "
                                    "scrollback stays pending in AltScreen mode")
                         .arg(QLatin1String(operation)));
        return false;
    }
    if (!m_terminalEngine->scrollbackHistoryEnabled()) {
        failScenario(QStringLiteral("terminal.%1() needs scrollback history, which this "
                                    "session does not record")
                         .arg(QLatin1String(operation)));
        return false;
    }
    return true;
}

QString Runner::scrollbackTextFrom(int fromRow, bool styled, QString* error) const {
    if (!m_terminalEngine) {
        if (error) *error = QStringLiteral("TerminalEngine is no longer available");
        return {};
    }
    if (fromRow < 0) {
        if (error) *error = QStringLiteral("fromRow must not be negative");
        return {};
    }
    const TextGridBuffer rows =
        m_terminalEngine->committedScrollback(static_cast<quint64>(fromRow));
    if (rows.height() <= 0) return {};
    return styled ? formatStyledText(rows) : normalizedText(rows);
}

int Runner::scrollbackRowCount() {
    if (!ensureRunning("scrollbackRowCount") || !ensureScrollbackSession("scrollbackRowCount"))
        return -1;
    return static_cast<int>(m_terminalEngine->committedScrollbackTotal());
}

QString Runner::scrollbackText(int fromRow) {
    if (!ensureRunning("scrollbackText") || !ensureScrollbackSession("scrollbackText")) return {};
    QString error;
    const QString text = scrollbackTextFrom(fromRow, false, &error);
    if (!error.isEmpty()) failScenario(error);
    return text;
}

void Runner::waitForScrollbackText(const QString& text, int timeoutMs) {
    if (!ensureRunning("waitForScrollbackText") ||
        !ensureScrollbackSession("waitForScrollbackText"))
        return;
    if (text.isEmpty()) {
        failScenario(QStringLiteral("Text conditions require a non-empty string"));
        return;
    }
    const int validated = validateTimeout(timeoutMs);
    if (!validated) return;
    const auto contains = [&] {
        QString error;
        const QString rows = scrollbackTextFrom(0, false, &error);
        if (!error.isEmpty()) {
            failScenario(error);
            return false;
        }
        return rows.contains(text);
    };
    (void)waitForEngineCondition(contains, validated,
                                 QStringLiteral("scrollback text \"%1\"").arg(text));
}

void Runner::expectScrollbackText(const QString& text, int timeoutMs) {
    if (!ensureRunning("expectScrollbackText") ||
        !ensureScrollbackSession("expectScrollbackText") ||
        !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(0, false, &error);
    if (!error.isEmpty())
        failScenario(error);
    else if (text.isEmpty() || !rows.contains(text))
        failScenario(QStringLiteral("Expected scrollback to contain \"%1\"").arg(text));
}

void Runner::expectNoScrollbackText(const QString& text, int timeoutMs) {
    if (!ensureRunning("expectNoScrollbackText") ||
        !ensureScrollbackSession("expectNoScrollbackText") ||
        !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(0, false, &error);
    if (!error.isEmpty())
        failScenario(error);
    else if (text.isEmpty() || rows.contains(text))
        failScenario(QStringLiteral("Expected scrollback not to contain \"%1\"").arg(text));
}

void Runner::captureScrollback(const QString& relativePath, int fromRow) {
    if (!ensureRunning("captureScrollback") || !ensureScrollbackSession("captureScrollback") ||
        !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(fromRow, false, &error);
    if (error.isEmpty() && writeTextArtifact(relativePath, rows, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty()) failScenario(error);
}

void Runner::captureScrollbackStyled(const QString& relativePath, int fromRow) {
    if (!ensureRunning("captureScrollbackStyled") ||
        !ensureScrollbackSession("captureScrollbackStyled") ||
        !waitForNextFrame(m_operationTimeoutMs))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(fromRow, true, &error);
    if (error.isEmpty() && writeTextArtifact(relativePath, rows, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty()) failScenario(error);
}

void Runner::expectScrollbackSnapshot(const QString& snapshotRelativePath, int fromRow,
                                      int timeoutMs) {
    if (!ensureRunning("expectScrollbackSnapshot") ||
        !ensureScrollbackSession("expectScrollbackSnapshot") ||
        !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    QString error;
    const QString rows = scrollbackTextFrom(fromRow, false, &error);
    if (!error.isEmpty()) {
        failScenario(error);
        return;
    }
    compareSnapshot(snapshotRelativePath, rows);
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
        failScenario(QStringLiteral("terminal.startRecording() requires QMLTUI_RUNNER_RECORD "
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
        failScenario(QStringLiteral("stopRecording() needs a recording in progress"));
        return;
    }
    closeCast();
}

void Runner::marker(const QString& label) {
    if (!ensureRunning("marker")) return;
    if (!m_cast || !m_cast->isOpen()) {
        failScenario(QStringLiteral("marker() needs a recording in progress"));
        return;
    }
    drainCast();
    m_cast->marker(castSeconds(), label);
}

QString Runner::normalizedText(const TextGridBuffer& frame) {
    QString result;
    for (int row = 0; row < frame.height(); ++row) {
        QString line;
        for (int column = 0; column < frame.width(); ++column) {
            const Cell& cell = frame.cellAt(column, row);
            if (cell.continuation) continue;
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

QString Runner::formatStyledText(const TextGridBuffer& frame) {
    QString result = normalizedText(frame);
    result.append(QStringLiteral("--- styles\n"));
    const auto sameStyle = [](const Cell& lhs, const Cell& rhs) {
        return lhs.fgColor == rhs.fgColor && lhs.bgColor == rhs.bgColor &&
               lhs.attributes == rhs.attributes;
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
            const bool isDefault =
                first.fgColor == 0 && first.bgColor == 0 && first.attributes == TextAttribute::None;
            if (!isDefault) {
                QString line =
                    QStringLiteral("y=%1 x=%2-%3 fg=%4 bg=%5")
                        .arg(row)
                        .arg(column)
                        .arg(end - 1)
                        .arg(first.fgColor ? rgbaString(first.fgColor) : QStringLiteral("default"),
                             first.bgColor ? rgbaString(first.bgColor) : QStringLiteral("default"));
                const QString attributes = attributeNames(first.attributes);
                if (!attributes.isEmpty()) line += QLatin1Char(' ') + attributes;
                result.append(line);
                result.append(QLatin1Char('\n'));
            }
            column = end;
        }
    }
    return result;
}

QList<Runner::TextMatch> Runner::textMatches(const TextGridBuffer& frame,
                                             const QString& text) const {
    QList<TextMatch> matches;
    if (text.isEmpty() || text.contains(QLatin1Char('\n'))) return matches;

    for (int row = 0; row < frame.height(); ++row) {
        QString renderedRow;
        QVector<int> sourceColumns;
        QVector<int> sourceEnds;
        for (int column = 0; column < frame.width(); ++column) {
            const Cell& renderedCell = frame.cellAt(column, row);
            if (renderedCell.continuation) continue;
            const QString character = renderedCell.character().isEmpty()
                                          ? QStringLiteral(" ")
                                          : QString::fromUtf8(renderedCell.character());
            renderedRow.append(character);
            for (qsizetype index = 0; index < character.size(); ++index) {
                sourceColumns.append(column);
                sourceEnds.append(column +
                                  (std::max)(1, static_cast<int>(renderedCell.displayWidth)));
            }
        }

        qsizetype from = 0;
        while (from <= renderedRow.size() - text.size()) {
            const qsizetype matchIndex = renderedRow.indexOf(text, from);
            if (matchIndex < 0) break;
            const qsizetype lastIndex = matchIndex + text.size() - 1;
            if (matchIndex < sourceColumns.size() && lastIndex < sourceEnds.size()) {
                const int x = sourceColumns.at(matchIndex);
                matches.append(TextMatch{x, row, (std::max)(1, sourceEnds.at(lastIndex) - x)});
            }
            from = matchIndex + (std::max<qsizetype>)(1, text.size());
        }
    }
    return matches;
}

QString Runner::resolveSnapshotPath(const QString& path) const {
    if (path.startsWith(QLatin1Char(':')) || QFileInfo(path).isAbsolute())
        return QDir::cleanPath(path);
    return QDir(QFileInfo(m_scriptPath).path()).filePath(path);
}

QString Runner::unifiedTextDiff(const QString& expectedPath, const QString& expected,
                                const QString& actual) {
    const QStringList expectedLines = expected.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    const QStringList actualLines = actual.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    QString diff = QStringLiteral("--- %1\n+++ actual\n@@ -1,%2 +1,%3 @@\n")
                       .arg(expectedPath)
                       .arg(expectedLines.size())
                       .arg(actualLines.size());
    const qsizetype lineCount = (std::max)(expectedLines.size(), actualLines.size());
    for (qsizetype index = 0; index < lineCount; ++index) {
        const bool hasExpected = index < expectedLines.size();
        const bool hasActual = index < actualLines.size();
        if (hasExpected && hasActual && expectedLines.at(index) == actualLines.at(index)) {
            diff += QLatin1Char(' ') + expectedLines.at(index) + QLatin1Char('\n');
            continue;
        }
        if (hasExpected) diff += QLatin1Char('-') + expectedLines.at(index) + QLatin1Char('\n');
        if (hasActual) diff += QLatin1Char('+') + actualLines.at(index) + QLatin1Char('\n');
    }
    return diff;
}

QString Runner::currentFrameText(QString* error) const {
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

QString Runner::currentStyledFrameText(QString* error) const {
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

QString Runner::resolveArtifactPath(const QString& relativePath, QString* error) const {
    const QString cleaned = QDir::cleanPath(relativePath);
    if (relativePath.trimmed().isEmpty() || cleaned == QLatin1String(".") ||
        QFileInfo(cleaned).isAbsolute() || cleaned == QLatin1String("..") ||
        cleaned.startsWith(QLatin1String("../")) || relativePath.contains(QLatin1Char('\n')) ||
        relativePath.contains(QLatin1Char('\r'))) {
        if (error)
            *error = QStringLiteral("Artifact path must be a relative path without newlines: %1")
                         .arg(relativePath);
        return {};
    }

    QString path = QDir(m_artifactDirectory).absoluteFilePath(cleaned);

    // The lexical checks above cannot see through symlinks, so verify the real
    // location: create the parent, then require that its canonical path is the
    // artifact root or below it. Without this a symlinked subdirectory lets a
    // scenario write anywhere on the filesystem while still reporting success.
    const QString parent = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(parent)) {
        if (error)
            *error = QStringLiteral("Cannot create artifact parent directory for %1").arg(path);
        return {};
    }
    const QString canonicalRoot = QFileInfo(m_artifactDirectory).canonicalFilePath();
    const QString canonicalParent = QFileInfo(parent).canonicalFilePath();
    if (canonicalRoot.isEmpty() || canonicalParent.isEmpty() ||
        (canonicalParent != canonicalRoot &&
         !canonicalParent.startsWith(canonicalRoot + QLatin1Char('/')))) {
        if (error)
            *error = QStringLiteral("Artifact path escapes the artifact directory: %1")
                         .arg(relativePath);
        return {};
    }
    if (QFileInfo(path).isSymLink()) {
        if (error)
            *error = QStringLiteral("Artifact path is a symbolic link: %1").arg(relativePath);
        return {};
    }
    return path;
}

bool Runner::writeTextArtifact(const QString& relativePath, const QString& contents,
                               QString* error) const {
    QString localError;
    QString* destinationError = error ? error : &localError;
    const QString path = resolveArtifactPath(relativePath, destinationError);
    if (path.isEmpty()) return false;
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        *destinationError =
            QStringLiteral("Cannot create artifact parent directory for %1").arg(path);
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *destinationError =
            QStringLiteral("Cannot open artifact %1: %2").arg(path, file.errorString());
        return false;
    }
    const QByteArray bytes = contents.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        *destinationError =
            QStringLiteral("Cannot write artifact %1: %2").arg(path, file.errorString());
        return false;
    }
    if (!file.commit()) {
        *destinationError =
            QStringLiteral("Cannot commit artifact %1: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

void Runner::captureText(const QString& relativePath) {
    if (!ensureRunning("captureText") || !waitForNextFrame(m_operationTimeoutMs)) return;
    QString error;
    const QString frame = currentFrameText(&error);
    if (error.isEmpty() && writeTextArtifact(relativePath, frame, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty()) failScenario(error);
}

void Runner::captureStyled(const QString& relativePath) {
    if (!ensureRunning("captureStyled") || !waitForNextFrame(m_operationTimeoutMs)) return;
    QString error;
    const QString frame = currentStyledFrameText(&error);
    if (error.isEmpty() && writeTextArtifact(relativePath, frame, &error))
        m_capturePaths.append(QDir::cleanPath(relativePath));
    if (!error.isEmpty()) failScenario(error);
}

QString Runner::styledText() {
    if (!ensureRunning("styledText")) return {};
    QString error;
    const QString text = currentStyledFrameText(&error);
    if (!error.isEmpty()) failScenario(error);
    return text;
}

QString Runner::frameText() {
    if (!ensureRunning("frameText")) return {};
    QString error;
    const QString text = currentFrameText(&error);
    if (!error.isEmpty()) failScenario(error);
    return text;
}

QVariantMap Runner::cell(int x, int y) {
    if (!ensureRunning("cell") || !m_terminalEngine) return {};
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame || x < 0 || y < 0 || x >= frame->width() || y >= frame->height()) {
        failScenario(
            QStringLiteral("Coordinates (%1,%2) are outside the committed frame").arg(x).arg(y));
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
    attributes.insert(QStringLiteral("blink"), hasAttribute(TextAttribute::Blink));
    attributes.insert(QStringLiteral("conceal"), hasAttribute(TextAttribute::Conceal));
    attributes.insert(QStringLiteral("strikethrough"), hasAttribute(TextAttribute::Strikethrough));

    QVariantMap result;
    result.insert(QStringLiteral("x"), x);
    result.insert(QStringLiteral("y"), y);
    result.insert(QStringLiteral("character"), QString::fromUtf8(renderedCell.character()));
    result.insert(QStringLiteral("continuation"), renderedCell.continuation);
    // "default" mirrors styledText(): the cell uses the terminal's own color.
    result.insert(QStringLiteral("foreground"), renderedCell.fgColor
                                                    ? rgbaString(renderedCell.fgColor)
                                                    : QStringLiteral("default"));
    result.insert(QStringLiteral("background"), renderedCell.bgColor
                                                    ? rgbaString(renderedCell.bgColor)
                                                    : QStringLiteral("default"));
    result.insert(QStringLiteral("attributes"), attributes);
    return result;
}

QVariantList Runner::findText(const QString& text) {
    if (!ensureRunning("findText") || !m_terminalEngine) return {};
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

QString Runner::regionText(int x, int y, int width, int height) {
    if (!ensureRunning("regionText") || !m_terminalEngine) return {};
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame) {
        failScenario(QStringLiteral("No committed frame buffer is available"));
        return {};
    }
    if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > frame->width() ||
        y + height > frame->height()) {
        failScenario(QStringLiteral("Region bounds [%1,%2,%3,%4] out of frame size %5x%6")
                         .arg(x)
                         .arg(y)
                         .arg(width)
                         .arg(height)
                         .arg(frame->width())
                         .arg(frame->height()));
        return {};
    }

    QString result;
    for (int row = y; row < y + height; ++row) {
        QString line;
        for (int column = x; column < x + width; ++column) {
            const Cell& cell = frame->cellAt(column, row);
            if (cell.continuation) continue;
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

bool Runner::locateItem(const QString& objectName, QRect* outRect, QString* error) const {
    if (objectName.trimmed().isEmpty()) {
        *error = QStringLiteral("itemRect() requires a non-empty objectName");
        return false;
    }
    if (!m_terminal || !m_terminalEngine) {
        *error = QStringLiteral("Target Terminal is no longer available");
        return false;
    }
    Item* item = findVisibleItem(m_terminal, objectName);
    if (!item) {
        *error = QStringLiteral("No visible item with objectName \"%1\"").arg(objectName);
        return false;
    }
    if (m_terminalEngine->frameSequence() == 0) {
        *error = QStringLiteral("No committed frame buffer is available");
        return false;
    }
    const int frameWidth = m_terminalEngine->terminalWidth();
    const int frameHeight = m_terminalEngine->terminalHeight();
    if (frameWidth <= 0 || frameHeight <= 0) {
        *error = QStringLiteral("No committed frame buffer is available");
        return false;
    }
    const QPoint origin = item->absolutePosition();
    const QRect clipped =
        QRect(origin, QSize(item->width(), item->height())) & QRect(0, 0, frameWidth, frameHeight);
    if (clipped.isEmpty()) {
        *error = QStringLiteral("Item \"%1\" lies outside the %2x%3 viewport")
                     .arg(objectName)
                     .arg(frameWidth)
                     .arg(frameHeight);
        return false;
    }
    *outRect = clipped;
    return true;
}

QVariantMap Runner::itemRect(const QString& objectName) {
    if (!ensureRunning("itemRect")) return {};
    QRect rect;
    QString error;
    if (!locateItem(objectName, &rect, &error)) {
        failScenario(error);
        return {};
    }
    return {{QStringLiteral("x"), rect.x()},
            {QStringLiteral("y"), rect.y()},
            {QStringLiteral("width"), rect.width()},
            {QStringLiteral("height"), rect.height()}};
}

void Runner::waitForItem(const QString& objectName, int timeoutMs) {
    if (!ensureRunning("waitForItem")) return;
    const int validated = validateTimeout(timeoutMs);
    if (!validated) return;

    QRect rect;
    QString error;
    if (locateItem(objectName, &rect, &error)) return;

    if (m_activeLoop) {
        failScenario(QStringLiteral("Nested wait operations are not supported"));
        return;
    }

    bool matched = false;
    QEventLoop loop;
    QTimer pollTimer;
    pollTimer.setTimerType(Qt::PreciseTimer);
    pollTimer.setInterval(conditionPollIntervalMs);
    QTimer timeout;
    timeout.setSingleShot(true);
    // Visibility is QML state that normally arrives with a frame, but poll as
    // well so a change that failed to invalidate anything is still seen.
    const auto evaluate = [&] {
        QString pollError;
        if (locateItem(objectName, &rect, &pollError)) {
            matched = true;
            loop.quit();
        }
    };
    const QMetaObject::Connection frameConnection = connect(
        m_terminalEngine, &TerminalEngine::frameRendered, &loop, [&](quint64) { evaluate(); });
    connect(&pollTimer, &QTimer::timeout, &loop, evaluate);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_activeLoop = &loop;
    pollTimer.start();
    timeout.start(validated);
    m_terminalEngine->requestRender();
    loop.exec();
    m_activeLoop = nullptr;
    disconnect(frameConnection);

    if (m_state == State::Failed) return;
    if (!matched) {
        failScenario(QStringLiteral("Timed out after %1 ms waiting for item \"%2\" (%3)")
                         .arg(validated)
                         .arg(objectName, error) +
                     pausedClockHint());
    }
}

void Runner::expectRegionSnapshot(int x, int y, int width, int height,
                                  const QString& snapshotRelativePath, int timeoutMs) {
    if (!ensureRunning("expectRegionSnapshot") ||
        !waitForNextFrame(resolveExpectTimeout(timeoutMs)))
        return;
    const QString actualText = regionText(x, y, width, height);
    if (m_state == State::Failed) return;
    compareSnapshot(snapshotRelativePath, actualText);
}

void Runner::clickText(const QString& text, int occurrence, const QString& button,
                       const QStringList& modifiers) {
    if (!ensureRunning("clickText") || !m_terminalEngine) return;
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
                         .arg(text)
                         .arg(occurrence)
                         .arg(matches.size()));
        return;
    }
    const TextMatch& match = matches.at(occurrence);
    const auto offset = m_terminalEngine->renderer()->mouseRowOffset();
    if (!offset) {
        failScenario(QStringLiteral("The inline frame's screen position is not yet known"));
        return;
    }
    const int screenY = match.y + *offset;
    click(match.x + (std::max)(0, match.width - 1) / 2, screenY, button, modifiers);
}

void Runner::hoverText(const QString& text, int occurrence, const QStringList& modifiers) {
    if (!ensureRunning("hoverText") || !m_terminalEngine) return;
    if (text.isEmpty() || text.contains(QLatin1Char('\n'))) {
        failScenario(QStringLiteral("terminal.hoverText() requires non-empty single-row text"));
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
                         .arg(text)
                         .arg(occurrence)
                         .arg(matches.size()));
        return;
    }
    const TextMatch& match = matches.at(occurrence);
    const auto offset = m_terminalEngine->renderer()->mouseRowOffset();
    if (!offset) {
        failScenario(QStringLiteral("The inline frame's screen position is not yet known"));
        return;
    }
    const int screenY = match.y + *offset;
    hover(match.x + (std::max)(0, match.width - 1) / 2, screenY, modifiers);
}

bool Runner::writeFinalFrameArtifact(QString* error) {
    QString frameError;
    const QString frame = currentFrameText(&frameError);
    if (!frameError.isEmpty()) {
        m_finalFrameRelativePath.clear();
        if (error) *error = frameError;
        return false;
    }

    if (m_state == State::Failed)
        m_finalFrameRelativePath = QStringLiteral("failure.txt");
    else if (m_mode == QLatin1String("capture"))
        m_finalFrameRelativePath = QStringLiteral("capture.txt");
    else
        m_finalFrameRelativePath = QStringLiteral("final.txt");
    return writeTextArtifact(m_finalFrameRelativePath, frame, error);
}

bool Runner::writeResultArtifact(int exitCode, QString* error) {
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
                      location.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(location));
        return result;
    };

    QJsonArray operations;
    for (const OperationRecord& operation : std::as_const(m_operations))
        operations.append(operationObject(operation));

    QJsonObject viewport{{QStringLiteral("columns"), m_viewport.width()},
                         {QStringLiteral("rows"), m_viewport.height()}};
    // "viewport" keeps the initial size the scenario was configured with;
    // "finalViewport" records where resize() calls left it.
    QJsonValue finalViewport = QJsonValue(QJsonValue::Null);
    if (m_terminalEngine && m_terminalEngine->frameSequence() > 0) {
        finalViewport = QJsonObject{{QStringLiteral("columns"), m_terminalEngine->terminalWidth()},
                                    {QStringLiteral("rows"), m_terminalEngine->terminalHeight()}};
    }
    QJsonArray captures;
    m_capturePaths.removeDuplicates();
    for (const QString& capture : std::as_const(m_capturePaths))
        captures.append(capture);
    QJsonArray updatedSnapshots;
    m_updatedSnapshots.removeDuplicates();
    for (const QString& snapshot : std::as_const(m_updatedSnapshots))
        updatedSnapshots.append(snapshot);
    QJsonObject artifacts{{QStringLiteral("directory"), m_artifactDirectory},
                          {QStringLiteral("result"), QStringLiteral("result.json")},
                          {QStringLiteral("captures"), captures},
                          {QStringLiteral("updatedSnapshots"), updatedSnapshots}};
    QJsonArray casts;
    for (const QString& cast : std::as_const(m_castPaths))
        casts.append(cast);
    artifacts.insert(QStringLiteral("casts"), casts);
    artifacts.insert(QStringLiteral("finalFrame"), m_finalFrameRelativePath.isEmpty()
                                                       ? QJsonValue(QJsonValue::Null)
                                                       : QJsonValue(m_finalFrameRelativePath));

    QJsonObject root{{QStringLiteral("clock"),
                      Clock::isVirtual() ? QStringLiteral("virtual") : QStringLiteral("real")},
                     {QStringLiteral("schemaVersion"), 1},
                     {QStringLiteral("status"),
                      exitCode == 0 ? QStringLiteral("passed") : QStringLiteral("failed")},
                     {QStringLiteral("exitCode"), exitCode},
                     {QStringLiteral("mode"), m_mode},
                     {QStringLiteral("scenario"), m_scriptPath},
                     {QStringLiteral("message"), m_message},
                     {QStringLiteral("elapsedMs"), m_elapsed.isValid() ? m_elapsed.elapsed() : 0},
                     {QStringLiteral("viewport"), viewport},
                     {QStringLiteral("finalViewport"), finalViewport},
                     {QStringLiteral("operations"), operations},
                     {QStringLiteral("artifacts"), artifacts},
                     {QStringLiteral("frameStats"), QJsonObject::fromVariantMap(frameStats())}};
    if (Clock::isVirtual())
        root.insert(QStringLiteral("virtualTimeMs"), Clock::instance()->elapsed());
    root.insert(QStringLiteral("operation"),
                m_currentOperation >= 0 && m_currentOperation < m_operations.size()
                    ? QJsonValue(operationObject(m_operations.at(m_currentOperation)))
                    : QJsonValue(QJsonValue::Null));

    const QString json = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return writeTextArtifact(QStringLiteral("result.json"), json, error);
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
