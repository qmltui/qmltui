#include <QmlTui/Runner/Runner.h>
#include <QmlTui/Runner/CastRecorder_p.h>
#include <QmlTui/Runner/RunnerInteractive_p.h>
#include <QmlTui/Runner/RunnerSnapshot_p.h>

#include <QmlTui/Core/Engine/Clock_p.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMetaObject>
#include <QQmlEngine>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>

namespace qmltui {

namespace {

QStringList stringListArg(const QJsonObject& args, const QString& key) {
    QStringList result;
    const QJsonValue value = args.value(key);
    if (value.isArray()) {
        for (const QJsonValue& entry : value.toArray())
            result.append(entry.toString());
    } else if (value.isString() && !value.toString().trimmed().isEmpty()) {
        for (const QString& part : value.toString().split(QLatin1Char(','), Qt::SkipEmptyParts))
            result.append(part.trimmed());
    }
    return result;
}

bool requireStringArg(const QJsonObject& args, const QString& key, QString* out, QString* error) {
    if (!args.contains(key) || !args.value(key).isString()) {
        *error = QStringLiteral("Missing required string argument \"%1\"").arg(key);
        return false;
    }
    *out = args.value(key).toString();
    return true;
}

bool requireIntArg(const QJsonObject& args, const QString& key, int* out, QString* error) {
    const QJsonValue value = args.value(key);
    if (!value.isDouble()) {
        *error = QStringLiteral("Missing required integer argument \"%1\"").arg(key);
        return false;
    }
    const double number = value.toDouble();
    const int integer = static_cast<int>(number);
    if (static_cast<double>(integer) != number) {
        *error = QStringLiteral("Argument \"%1\" must be an integer").arg(key);
        return false;
    }
    *out = integer;
    return true;
}

int optionalIntArg(const QJsonObject& args, const QString& key, int fallback) {
    const QJsonValue value = args.value(key);
    return value.isDouble() ? value.toInt(fallback) : fallback;
}

QString optionalStringArg(const QJsonObject& args, const QString& key, const QString& fallback) {
    const QJsonValue value = args.value(key);
    return value.isString() ? value.toString() : fallback;
}

QJsonValue jsValueToJson(const QJSValue& value) {
    if (value.isUndefined() || value.isNull()) return QJsonValue::Null;
    if (value.isBool()) return value.toBool();
    if (value.isNumber()) return value.toNumber();
    if (value.isString()) return value.toString();
    const QVariant variant = value.toVariant();
    if (variant.isValid()) return QJsonValue::fromVariant(variant);
    return value.toString();
}

} // namespace

RunnerInteractiveReader::RunnerInteractiveReader(Runner* runner)
    : m_stopped(std::make_shared<std::atomic<bool>>(false)) {
    QPointer<Runner> guarded(runner);
    std::shared_ptr<std::atomic<bool>> stopped = m_stopped;
    m_thread = std::thread([guarded, stopped] {
        std::string line;
        while (!stopped->load(std::memory_order_acquire) && std::getline(std::cin, line)) {
            if (stopped->load(std::memory_order_acquire)) break;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            QByteArray bytes(line.data(), static_cast<qsizetype>(line.size()));
            if (QCoreApplication* app = QCoreApplication::instance()) {
                QMetaObject::invokeMethod(
                    app,
                    [guarded, stopped, bytes = std::move(bytes)] {
                        if (!stopped->load(std::memory_order_acquire) && guarded)
                            guarded->handleInteractiveLine(bytes);
                    },
                    Qt::QueuedConnection);
            }
        }
        if (!stopped->load(std::memory_order_acquire)) {
            if (QCoreApplication* app = QCoreApplication::instance()) {
                QMetaObject::invokeMethod(
                    app,
                    [guarded, stopped] {
                        if (!stopped->load(std::memory_order_acquire) && guarded)
                            guarded->handleInteractiveEof();
                    },
                    Qt::QueuedConnection);
            }
        }
    });
}

RunnerInteractiveReader::~RunnerInteractiveReader() {
    stop(/*detachIfBlocked=*/true);
}

void RunnerInteractiveReader::stop(bool detachIfBlocked) {
    if (m_stopped) m_stopped->store(true, std::memory_order_release);
    if (!m_thread.joinable()) return;
    if (detachIfBlocked)
        m_thread.detach();
    else
        m_thread.join();
}

void Runner::emitInteractiveMessage(const QJsonObject& message) const {
    QByteArray line;
    if (!m_ipcPrefix.isEmpty()) line.append(m_ipcPrefix);
    line.append(QJsonDocument(message).toJson(QJsonDocument::Compact));
    line.append('\n');
    std::fwrite(line.constData(), 1, static_cast<size_t>(line.size()), stdout);
    std::fflush(stdout);
}

QByteArray Runner::currentAnsiFrameBytes() const {
    if (!m_terminalEngine) return {};
    const std::optional<TextGridBuffer> frame = m_terminalEngine->currentFrame();
    if (!frame || frame->width() <= 0 || frame->height() <= 0) return {};
    const std::string ansi = frame->toAnsiString();
    return QByteArray(ansi.data(), static_cast<qsizetype>(ansi.size()));
}

QByteArray Runner::currentScrollbackAnsiBytes() const {
    if (!m_terminal || m_terminal->mode() != Terminal::Mode::Inline || !m_terminalEngine ||
        !m_terminalEngine->scrollbackHistoryEnabled()) {
        return {};
    }
    const quint64 total = m_terminalEngine->committedScrollbackTotal();
    if (total == 0) return {};
    const std::optional<TextGridBuffer> live = m_terminalEngine->currentFrame();
    const int liveHeight = live ? live->height() : 1;
    const int termHeight = (std::max)(1, m_terminalEngine->terminalHeight());
    const quint64 maxRows = static_cast<quint64>((std::max)(1, termHeight - liveHeight));
    const quint64 fromRow = total > maxRows ? (total - maxRows) : 0;
    const TextGridBuffer rows = m_terminalEngine->committedScrollback(fromRow);
    if (rows.height() <= 0) return {};
    const std::string ansi = rows.toAnsiString();
    return QByteArray(ansi.data(), static_cast<qsizetype>(ansi.size()));
}

void Runner::emitLiveFrameEvent() {
    const QByteArray ansi = currentAnsiFrameBytes();
    if (ansi.isEmpty()) return;
    QJsonObject ev;
    ev.insert(QStringLiteral("event"), QStringLiteral("frame"));
    ev.insert(QStringLiteral("frameCount"), frameCount());
    ev.insert(
        QStringLiteral("viewport"),
        QJsonObject{
            {QStringLiteral("columns"), m_terminalEngine ? m_terminalEngine->terminalWidth() : 0},
            {QStringLiteral("rows"), m_terminalEngine ? m_terminalEngine->terminalHeight() : 0}});
    ev.insert(QStringLiteral("ansi"), QString::fromLatin1(ansi.toBase64()));
    const QByteArray sbAnsi = currentScrollbackAnsiBytes();
    if (!sbAnsi.isEmpty())
        ev.insert(QStringLiteral("scrollbackAnsi"), QString::fromLatin1(sbAnsi.toBase64()));
    emitInteractiveMessage(ev);
}

void Runner::startInteractiveSession() {
    QString frameError;
    const QString initialFrame = currentFrameText(&frameError);
    m_lastSentFrame = initialFrame;

    QJsonObject ready;
    ready.insert(QStringLiteral("event"), QStringLiteral("ready"));
    ready.insert(QStringLiteral("protocolVersion"), 1);
    ready.insert(QStringLiteral("pid"), QCoreApplication::applicationPid());
    ready.insert(QStringLiteral("mode"),
                 (m_terminal && m_terminal->mode() == Terminal::Mode::Inline)
                     ? QStringLiteral("Inline")
                     : QStringLiteral("AltScreen"));
    ready.insert(QStringLiteral("clock"),
                 Clock::isVirtual() ? QStringLiteral("virtual") : QStringLiteral("real"));
    ready.insert(
        QStringLiteral("viewport"),
        QJsonObject{
            {QStringLiteral("columns"), m_terminalEngine ? m_terminalEngine->terminalWidth() : 0},
            {QStringLiteral("rows"), m_terminalEngine ? m_terminalEngine->terminalHeight() : 0}});
    ready.insert(QStringLiteral("frame"), initialFrame);
    ready.insert(QStringLiteral("frameCount"), frameCount());
    ready.insert(QStringLiteral("frameStats"), QJsonObject::fromVariantMap(frameStats()));
    if (Clock::isVirtual())
        ready.insert(QStringLiteral("virtualTimeMs"), Clock::instance()->elapsed());
    if (m_liveStream) {
        const QByteArray ansi = currentAnsiFrameBytes();
        if (!ansi.isEmpty())
            ready.insert(QStringLiteral("ansi"), QString::fromLatin1(ansi.toBase64()));
        const QByteArray sbAnsi = currentScrollbackAnsiBytes();
        if (!sbAnsi.isEmpty())
            ready.insert(QStringLiteral("scrollbackAnsi"), QString::fromLatin1(sbAnsi.toBase64()));
    }
    emitInteractiveMessage(ready);

    m_interactiveReader = std::make_unique<RunnerInteractiveReader>(this);
}

void Runner::handleInteractiveEof() {
    if (m_interactiveClosing || m_state != State::Running) return;
    m_interactiveEofPending = true;
    if (!m_inCommand && !m_drainingInteractiveQueue) drainInteractiveQueue();
}

void Runner::handleInteractiveLine(const QByteArray& rawLine) {
    if (m_interactiveClosing || m_state != State::Running) return;
    m_pendingInteractiveLines.append(rawLine);
    if (!m_inCommand && !m_drainingInteractiveQueue) drainInteractiveQueue();
}

void Runner::drainInteractiveQueue() {
    if (m_drainingInteractiveQueue || m_inCommand || m_interactiveClosing ||
        m_state != State::Running) {
        return;
    }
    m_drainingInteractiveQueue = true;
    while (!m_pendingInteractiveLines.isEmpty() && !m_interactiveClosing &&
           m_state == State::Running) {
        const QByteArray nextLine = m_pendingInteractiveLines.takeFirst();
        processInteractiveLine(nextLine);
    }
    m_drainingInteractiveQueue = false;
    if (m_interactiveEofPending && !m_interactiveClosing && m_state == State::Running) {
        m_interactiveEofPending = false;
        m_interactiveClosing = true;
        if (m_interactiveReader) m_interactiveReader->stop(/*detachIfBlocked=*/false);
        m_state = State::Passed;
        m_watchdog.stop();
        stopHardWatchdog();
        scheduleExit(0);
    }
}

void Runner::processInteractiveLine(const QByteArray& rawLine) {
    if (m_interactiveClosing || m_state != State::Running) return;

    const QByteArray trimmed = rawLine.trimmed();
    if (trimmed.isEmpty()) return;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emitInteractiveMessage(QJsonObject{
            {QStringLiteral("id"), QJsonValue::Null},
            {QStringLiteral("ok"), false},
            {QStringLiteral("error"),
             QStringLiteral("Invalid JSON request: %1").arg(parseError.errorString())}});
        return;
    }

    const QJsonObject request = doc.object();
    const QJsonValue id = request.value(QStringLiteral("id"));
    const QString cmd = request.value(QStringLiteral("cmd")).toString().trimmed();
    const QJsonObject args = request.value(QStringLiteral("args")).toObject();
    const QJsonObject include = request.value(QStringLiteral("include")).toObject();

    if (cmd == QLatin1String("ping")) {
        emitInteractiveMessage(
            QJsonObject{{QStringLiteral("id"), id},
                        {QStringLiteral("ok"), true},
                        {QStringLiteral("pid"), QCoreApplication::applicationPid()},
                        {QStringLiteral("frameCount"), frameCount()}});
        return;
    }

    if (cmd == QLatin1String("setLiveStream")) {
        m_liveStream = args.value(QStringLiteral("enabled")).toBool(true);
        QJsonObject reply{
            {QStringLiteral("id"), id},
            {QStringLiteral("ok"), true},
            {QStringLiteral("liveStream"), m_liveStream},
            {QStringLiteral("frameCount"), frameCount()},
            {QStringLiteral("viewport"),
             QJsonObject{{QStringLiteral("columns"),
                          m_terminalEngine ? m_terminalEngine->terminalWidth() : 0},
                         {QStringLiteral("rows"),
                          m_terminalEngine ? m_terminalEngine->terminalHeight() : 0}}}};
        if (m_liveStream) {
            const QByteArray ansi = currentAnsiFrameBytes();
            if (!ansi.isEmpty())
                reply.insert(QStringLiteral("ansi"), QString::fromLatin1(ansi.toBase64()));
            const QByteArray sbAnsi = currentScrollbackAnsiBytes();
            if (!sbAnsi.isEmpty())
                reply.insert(QStringLiteral("scrollbackAnsi"),
                             QString::fromLatin1(sbAnsi.toBase64()));
        }
        emitInteractiveMessage(reply);
        return;
    }

    if (cmd == QLatin1String("close")) {
        m_interactiveClosing = true;
        if (m_cast && m_cast->isOpen()) {
            if (m_terminalEngine) m_terminalEngine->setTerminalOutputEnabled(false);
            closeCast();
        }
        QString frameError;
        const QString finalFrame = currentFrameText(&frameError);
        QJsonObject reply{
            {QStringLiteral("id"), id},
            {QStringLiteral("ok"), true},
            {QStringLiteral("closed"), true},
            {QStringLiteral("frame"), finalFrame},
            {QStringLiteral("frameCount"), frameCount()},
            {QStringLiteral("frameStats"), QJsonObject::fromVariantMap(frameStats())},
            {QStringLiteral("viewport"),
             QJsonObject{{QStringLiteral("columns"),
                          m_terminalEngine ? m_terminalEngine->terminalWidth() : 0},
                         {QStringLiteral("rows"),
                          m_terminalEngine ? m_terminalEngine->terminalHeight() : 0}}}};
        if (Clock::isVirtual())
            reply.insert(QStringLiteral("virtualTimeMs"), Clock::instance()->elapsed());
        emitInteractiveMessage(reply);

        if (m_interactiveReader) m_interactiveReader->stop(/*detachIfBlocked=*/true);
        m_state = State::Passed;
        m_watchdog.stop();
        stopHardWatchdog();
        scheduleExit(0);
        return;
    }

    int commandTimeoutMs = (std::max)(m_operationTimeoutMs + 5000, 30000);
    if (request.contains(QStringLiteral("timeoutMs"))) {
        const int requested = request.value(QStringLiteral("timeoutMs")).toInt(-1);
        if (requested < runner_detail::minimumTimeoutMs ||
            requested > runner_detail::maximumScenarioTimeoutMs) {
            emitInteractiveMessage(QJsonObject{
                {QStringLiteral("id"), id},
                {QStringLiteral("ok"), false},
                {QStringLiteral("error"), QStringLiteral("timeoutMs must be between %1 and %2")
                                              .arg(runner_detail::minimumTimeoutMs)
                                              .arg(runner_detail::maximumScenarioTimeoutMs)}});
            return;
        }
        commandTimeoutMs = requested;
    }

    const int frameCountBefore = frameCount();
    QString frameBeforeError;
    const QString frameBefore = currentFrameText(&frameBeforeError);

    m_inCommand = true;
    m_commandError.clear();
    m_watchdog.start(commandTimeoutMs);

    QJsonValue resultValue = QJsonValue::Undefined;
    QJsonObject computedTree;
    QString argError;

    const auto failArg = [&] {
        if (!argError.isEmpty() && m_commandError.isEmpty()) m_commandError = argError;
    };

    if (cmd == QLatin1String("press") || cmd == QLatin1String("injectKey")) {
        QString key;
        if (requireStringArg(args, QStringLiteral("key"), &key, &argError)) {
            const QStringList modifiers = stringListArg(args, QStringLiteral("modifiers"));
            if (cmd == QLatin1String("press"))
                press(key, modifiers);
            else
                injectKey(key, modifiers);
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("typeText") || cmd == QLatin1String("injectText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError)) {
            if (cmd == QLatin1String("typeText"))
                typeText(text);
            else
                injectText(text);
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("paste") || cmd == QLatin1String("injectPaste")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError)) {
            if (cmd == QLatin1String("paste"))
                paste(text);
            else
                injectPaste(text);
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("click") || cmd == QLatin1String("doubleClick") ||
               cmd == QLatin1String("mouseDown") || cmd == QLatin1String("mouseUp")) {
        int x = 0;
        int y = 0;
        if (requireIntArg(args, QStringLiteral("x"), &x, &argError) &&
            requireIntArg(args, QStringLiteral("y"), &y, &argError)) {
            const QString button =
                optionalStringArg(args, QStringLiteral("button"), QStringLiteral("left"));
            const QStringList modifiers = stringListArg(args, QStringLiteral("modifiers"));
            if (cmd == QLatin1String("click"))
                click(x, y, button, modifiers);
            else if (cmd == QLatin1String("doubleClick"))
                doubleClick(x, y, button, modifiers);
            else if (cmd == QLatin1String("mouseDown"))
                mouseDown(x, y, button, modifiers);
            else
                mouseUp(x, y, button, modifiers);
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("hover")) {
        int x = 0;
        int y = 0;
        if (requireIntArg(args, QStringLiteral("x"), &x, &argError) &&
            requireIntArg(args, QStringLiteral("y"), &y, &argError)) {
            hover(x, y, stringListArg(args, QStringLiteral("modifiers")));
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("drag")) {
        int startX = 0;
        int startY = 0;
        int endX = 0;
        int endY = 0;
        if (requireIntArg(args, QStringLiteral("startX"), &startX, &argError) &&
            requireIntArg(args, QStringLiteral("startY"), &startY, &argError) &&
            requireIntArg(args, QStringLiteral("endX"), &endX, &argError) &&
            requireIntArg(args, QStringLiteral("endY"), &endY, &argError)) {
            const QString button =
                optionalStringArg(args, QStringLiteral("button"), QStringLiteral("left"));
            drag(startX, startY, endX, endY, button,
                 stringListArg(args, QStringLiteral("modifiers")));
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("scroll")) {
        int x = 0;
        int y = 0;
        int dx = 0;
        int dy = 0;
        if (requireIntArg(args, QStringLiteral("x"), &x, &argError) &&
            requireIntArg(args, QStringLiteral("y"), &y, &argError) &&
            requireIntArg(args, QStringLiteral("deltaX"), &dx, &argError) &&
            requireIntArg(args, QStringLiteral("deltaY"), &dy, &argError)) {
            scroll(x, y, dx, dy, stringListArg(args, QStringLiteral("modifiers")));
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("clickText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError)) {
            const int occurrence = optionalIntArg(args, QStringLiteral("occurrence"), 0);
            const QString button =
                optionalStringArg(args, QStringLiteral("button"), QStringLiteral("left"));
            clickText(text, occurrence, button, stringListArg(args, QStringLiteral("modifiers")));
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("hoverText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError)) {
            const int occurrence = optionalIntArg(args, QStringLiteral("occurrence"), 0);
            hoverText(text, occurrence, stringListArg(args, QStringLiteral("modifiers")));
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("resize")) {
        int columns = 0;
        int rows = 0;
        if (requireIntArg(args, QStringLiteral("columns"), &columns, &argError) &&
            requireIntArg(args, QStringLiteral("rows"), &rows, &argError)) {
            resize(columns, rows);
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("advanceTime")) {
        int durationMs = 0;
        if (requireIntArg(args, QStringLiteral("durationMs"), &durationMs, &argError))
            advanceTime(durationMs);
        else
            failArg();
    } else if (cmd == QLatin1String("sleep")) {
        int durationMs = 0;
        if (requireIntArg(args, QStringLiteral("durationMs"), &durationMs, &argError))
            sleep(durationMs);
        else
            failArg();
    } else if (cmd == QLatin1String("waitForFrame")) {
        waitForFrame(optionalIntArg(args, QStringLiteral("timeoutMs"), m_operationTimeoutMs));
    } else if (cmd == QLatin1String("expectRepaint")) {
        expectRepaint(optionalIntArg(args, QStringLiteral("timeoutMs"), m_operationTimeoutMs));
    } else if (cmd == QLatin1String("waitForText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError))
            waitForText(text,
                        optionalIntArg(args, QStringLiteral("timeoutMs"), m_operationTimeoutMs));
        else
            failArg();
    } else if (cmd == QLatin1String("waitForRegex")) {
        QString pattern;
        if (requireStringArg(args, QStringLiteral("pattern"), &pattern, &argError))
            waitForRegex(pattern,
                         optionalIntArg(args, QStringLiteral("timeoutMs"), m_operationTimeoutMs));
        else
            failArg();
    } else if (cmd == QLatin1String("waitForItem")) {
        QString objectName;
        if (requireStringArg(args, QStringLiteral("objectName"), &objectName, &argError))
            waitForItem(objectName,
                        optionalIntArg(args, QStringLiteral("timeoutMs"), m_operationTimeoutMs));
        else
            failArg();
    } else if (cmd == QLatin1String("waitForScrollbackText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError))
            waitForScrollbackText(
                text, optionalIntArg(args, QStringLiteral("timeoutMs"), m_operationTimeoutMs));
        else
            failArg();
    } else if (cmd == QLatin1String("expectText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError))
            expectText(text, optionalIntArg(args, QStringLiteral("timeoutMs"), -1));
        else
            failArg();
    } else if (cmd == QLatin1String("expectNoText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError))
            expectNoText(text, optionalIntArg(args, QStringLiteral("timeoutMs"), -1));
        else
            failArg();
    } else if (cmd == QLatin1String("expectRegex")) {
        QString pattern;
        if (requireStringArg(args, QStringLiteral("pattern"), &pattern, &argError))
            expectRegex(pattern, optionalIntArg(args, QStringLiteral("timeoutMs"), -1));
        else
            failArg();
    } else if (cmd == QLatin1String("expectScrollbackText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError))
            expectScrollbackText(text, optionalIntArg(args, QStringLiteral("timeoutMs"), -1));
        else
            failArg();
    } else if (cmd == QLatin1String("expectNoScrollbackText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError))
            expectNoScrollbackText(text, optionalIntArg(args, QStringLiteral("timeoutMs"), -1));
        else
            failArg();
    } else if (cmd == QLatin1String("frame") || cmd == QLatin1String("styled") ||
               cmd == QLatin1String("frameStats")) {
        // Inspection-only commands; payload assembled below.
    } else if (cmd == QLatin1String("scrollback")) {
        if (ensureScrollbackSession("scrollback")) {
            const int fromRow = optionalIntArg(args, QStringLiteral("fromRow"), 0);
            QString sbError;
            (void)scrollbackTextFrom(fromRow, false, &sbError);
            if (!sbError.isEmpty()) failScenario(sbError, false);
        }
    } else if (cmd == QLatin1String("cell")) {
        int x = 0;
        int y = 0;
        if (requireIntArg(args, QStringLiteral("x"), &x, &argError) &&
            requireIntArg(args, QStringLiteral("y"), &y, &argError)) {
            const QVariantMap map = cell(x, y);
            if (m_commandError.isEmpty()) resultValue = QJsonObject::fromVariantMap(map);
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("findText")) {
        QString text;
        if (requireStringArg(args, QStringLiteral("text"), &text, &argError)) {
            const QVariantList matches = findText(text);
            if (m_commandError.isEmpty()) resultValue = QJsonArray::fromVariantList(matches);
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("regionText")) {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        if (requireIntArg(args, QStringLiteral("x"), &x, &argError) &&
            requireIntArg(args, QStringLiteral("y"), &y, &argError) &&
            requireIntArg(args, QStringLiteral("width"), &w, &argError) &&
            requireIntArg(args, QStringLiteral("height"), &h, &argError)) {
            const QString text = regionText(x, y, w, h);
            if (m_commandError.isEmpty()) resultValue = text;
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("itemRect")) {
        QString objectName;
        if (requireStringArg(args, QStringLiteral("objectName"), &objectName, &argError)) {
            const QVariantMap rect = itemRect(objectName);
            if (m_commandError.isEmpty()) resultValue = QJsonObject::fromVariantMap(rect);
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("clickItem")) {
        QString objectName;
        if (requireStringArg(args, QStringLiteral("objectName"), &objectName, &argError)) {
            QRect rect;
            QString locateError;
            if (!locateItem(objectName, &rect, &locateError)) {
                failScenario(locateError, false);
            } else {
                const QString button =
                    optionalStringArg(args, QStringLiteral("button"), QStringLiteral("left"));
                clickFrame(rect.x() + (std::max)(0, rect.width() - 1) / 2,
                           rect.y() + (std::max)(0, rect.height() - 1) / 2, button,
                           stringListArg(args, QStringLiteral("modifiers")));
            }
        } else {
            failArg();
        }
    } else if (cmd == QLatin1String("itemTree")) {
        const bool includeHidden = args.value(QStringLiteral("includeHidden")).toBool(false);
        const QVariantMap tree = itemTree(includeHidden);
        if (m_commandError.isEmpty()) computedTree = QJsonObject::fromVariantMap(tree);
    } else if (cmd == QLatin1String("size")) {
        const QVariantMap sz = size();
        if (m_commandError.isEmpty()) resultValue = QJsonObject::fromVariantMap(sz);
    } else if (cmd == QLatin1String("now")) {
        resultValue = now();
    } else if (cmd == QLatin1String("frameCount")) {
        resultValue = frameCount();
    } else if (cmd == QLatin1String("startRecording")) {
        QString path;
        if (requireStringArg(args, QStringLiteral("path"), &path, &argError))
            startRecording(path);
        else
            failArg();
    } else if (cmd == QLatin1String("stopRecording")) {
        stopRecording();
    } else if (cmd == QLatin1String("marker")) {
        QString label;
        if (requireStringArg(args, QStringLiteral("label"), &label, &argError))
            marker(label);
        else
            failArg();
    } else if (cmd == QLatin1String("eval")) {
        QString code;
        if (requireStringArg(args, QStringLiteral("code"), &code, &argError)) {
            if (code.trimmed().isEmpty()) {
                failScenario(QStringLiteral("eval requires non-empty code"), false);
            } else if (!m_qmlEngine) {
                failScenario(QStringLiteral("QQmlEngine is no longer available"), false);
            } else {
                m_evaluating = true;
                startHardWatchdog(commandTimeoutMs);
                // Try compiling as a single returnable expression first so
                // queries like `app.frameCount()` return their value directly;
                // fall back to a statement block for multi-statement scripts.
                QJSValue fn = m_qmlEngine->evaluate(
                    QStringLiteral("(function(app) { return (\n%1\n); })").arg(code),
                    QStringLiteral("qmltui-interactive-eval.js"));
                if (fn.isError()) {
                    fn = m_qmlEngine->evaluate(QStringLiteral("(function(app) {\n%1\n})").arg(code),
                                               QStringLiteral("qmltui-interactive-eval.js"));
                }
                QJSValue evalResult;
                if (fn.isError()) {
                    failScenario(formatJsError(fn), false);
                } else {
                    evalResult = fn.call({m_terminalFacade});
                }
                const bool interrupted = stopHardWatchdog();
                m_evaluating = false;
                if (interrupted) {
                    if (m_commandError.isEmpty()) {
                        m_commandError =
                            QStringLiteral("Command did not yield to the event loop within %1 ms; "
                                           "JavaScript execution was interrupted")
                                .arg(commandTimeoutMs);
                    }
                } else if (m_commandError.isEmpty()) {
                    if (evalResult.isError())
                        failScenario(formatJsError(evalResult), false);
                    else if (!evalResult.isUndefined())
                        resultValue = jsValueToJson(evalResult);
                }
            }
        } else {
            failArg();
        }
    } else {
        m_commandError = QStringLiteral("Unknown interactive command: %1").arg(cmd);
    }

    m_watchdog.stop();

    const bool ok = m_commandError.isEmpty();
    const QString errorMessage = m_commandError;
    m_inCommand = false;
    m_commandError.clear();

    QString frameError;
    const QString frameAfter = currentFrameText(&frameError);
    const int frameCountAfter = frameCount();

    const bool includeDiff = include.value(QStringLiteral("diff")).toBool(false);
    // When a command fails while the caller requested a diff instead of a full
    // frame, fall back to the full current frame so the failure is immediately
    // diagnosable instead of showing "(no visual changes)".
    const bool emitDiff = includeDiff && ok;
    const bool includeFrame =
        include.value(QStringLiteral("frame")).toBool(true) || (!ok && includeDiff);
    const bool includeStyled =
        include.value(QStringLiteral("styled")).toBool(false) || cmd == QLatin1String("styled");
    const bool includeScrollback = include.value(QStringLiteral("scrollback")).toBool(false) ||
                                   cmd == QLatin1String("scrollback");
    const bool includeTree =
        include.value(QStringLiteral("tree")).toBool(false) || cmd == QLatin1String("itemTree");
    const bool includeHidden =
        include.value(QStringLiteral("includeHidden"))
            .toBool(args.value(QStringLiteral("includeHidden")).toBool(false));

    QJsonObject reply;
    reply.insert(QStringLiteral("id"), id);
    reply.insert(QStringLiteral("ok"), ok);
    if (!ok) reply.insert(QStringLiteral("error"), errorMessage);
    if (!resultValue.isUndefined()) reply.insert(QStringLiteral("result"), resultValue);

    reply.insert(
        QStringLiteral("viewport"),
        QJsonObject{
            {QStringLiteral("columns"), m_terminalEngine ? m_terminalEngine->terminalWidth() : 0},
            {QStringLiteral("rows"), m_terminalEngine ? m_terminalEngine->terminalHeight() : 0}});
    reply.insert(QStringLiteral("frameCount"), frameCountAfter);
    reply.insert(QStringLiteral("frameDelta"), frameCountAfter - frameCountBefore);
    reply.insert(QStringLiteral("changed"),
                 frameCountAfter != frameCountBefore || frameAfter != frameBefore);
    reply.insert(QStringLiteral("frameStats"), QJsonObject::fromVariantMap(frameStats()));
    reply.insert(QStringLiteral("clock"),
                 Clock::isVirtual() ? QStringLiteral("virtual") : QStringLiteral("real"));
    if (Clock::isVirtual())
        reply.insert(QStringLiteral("virtualTimeMs"), Clock::instance()->elapsed());
    if (m_appQuitRequested) reply.insert(QStringLiteral("exiting"), true);

    if (includeFrame && frameError.isEmpty()) reply.insert(QStringLiteral("frame"), frameAfter);
    if (emitDiff && frameError.isEmpty()) {
        auto splitRows = [](QString text) {
            if (text.endsWith(QLatin1Char('\n'))) text.chop(1);
            return text.isEmpty() ? QStringList{} : text.split(QLatin1Char('\n'));
        };
        const QStringList beforeRows = splitRows(m_lastSentFrame);
        const QStringList afterRows = splitRows(frameAfter);
        const qsizetype count = (std::max)(beforeRows.size(), afterRows.size());
        QJsonArray diffRows;
        for (qsizetype row = 0; row < count; ++row) {
            const QString b = row < beforeRows.size() ? beforeRows.at(row) : QString();
            const QString a = row < afterRows.size() ? afterRows.at(row) : QString();
            if (b != a) {
                diffRows.append(QJsonObject{{QStringLiteral("row"), static_cast<int>(row)},
                                            {QStringLiteral("before"), b},
                                            {QStringLiteral("after"), a}});
            }
        }
        reply.insert(QStringLiteral("frameDiff"), diffRows);
    }
    if (includeStyled) {
        QString styledError;
        const QString styled = currentStyledFrameText(&styledError);
        if (styledError.isEmpty()) reply.insert(QStringLiteral("styled"), styled);
    }
    if (includeScrollback && m_terminal && m_terminal->mode() == Terminal::Mode::Inline &&
        m_terminalEngine && m_terminalEngine->scrollbackHistoryEnabled()) {
        const int fromRow = include.contains(QStringLiteral("fromRow"))
                                ? include.value(QStringLiteral("fromRow")).toInt(0)
                                : optionalIntArg(args, QStringLiteral("fromRow"), 0);
        QString sbError;
        const QString sb = scrollbackTextFrom(fromRow, false, &sbError);
        if (sbError.isEmpty()) {
            reply.insert(QStringLiteral("scrollback"), sb);
            reply.insert(QStringLiteral("scrollbackRowCount"),
                         static_cast<int>(m_terminalEngine->committedScrollbackTotal()));
            if (includeStyled) {
                QString sbStyledError;
                const QString sbStyled = scrollbackTextFrom(fromRow, true, &sbStyledError);
                if (sbStyledError.isEmpty())
                    reply.insert(QStringLiteral("scrollbackStyled"), sbStyled);
            }
        }
    }
    if (includeTree) {
        if (cmd == QLatin1String("itemTree") && !computedTree.isEmpty()) {
            reply.insert(QStringLiteral("tree"), computedTree);
        } else if (m_terminal && m_terminalEngine) {
            const std::optional<TextGridBuffer> current = m_terminalEngine->currentFrame();
            const QRect viewportRect = current ? QRect(0, 0, current->width(), current->height())
                                               : QRect(0, 0, m_terminalEngine->terminalWidth(),
                                                       m_terminalEngine->terminalHeight());
            reply.insert(QStringLiteral("tree"), QJsonObject::fromVariantMap(serializeItemNode(
                                                     m_terminal, viewportRect, includeHidden)));
        }
    }

    if ((includeFrame || emitDiff) && frameError.isEmpty()) m_lastSentFrame = frameAfter;
    emitInteractiveMessage(reply);

    if (m_appQuitRequested) {
        m_interactiveClosing = true;
        finalizeInteractiveArtifacts();
        if (m_interactiveReader) m_interactiveReader->stop(/*detachIfBlocked=*/true);
    }
}

} // namespace qmltui
