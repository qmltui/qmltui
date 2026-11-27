#include "ChildSession.h"

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMetaObject>
#include <QPointer>
#include <QTextStream>
#include <QTimer>
#include <QUuid>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

namespace qmltui::runner_cli {

namespace {

constexpr int kMaxLogLines = 2000;

QJsonObject readJsonObject(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

void terminateProcess(QProcess& process) {
    if (process.state() == QProcess::NotRunning) return;
    process.closeWriteChannel();
    if (process.waitForFinished(500)) return;
    process.terminate();
    if (!process.waitForFinished(1000)) {
        process.kill();
        process.waitForFinished(1000);
    }
}

} // namespace

ChildSession::ChildSession(QObject* parent) : QObject(parent) {
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &ChildSession::drainStdout);
    connect(&m_process, &QProcess::readyReadStandardError, this, &ChildSession::drainStderr);
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                drainStdout();
                drainStderr();
                m_exited = true;
                m_exitCode = code;
                m_exitStatus = status;
                emit childExited(code, status);
            });
}

ChildSession::~ChildSession() {
    if (m_process.state() != QProcess::NotRunning) terminateProcess(m_process);
}

bool ChildSession::isRunning() const {
    return !m_exited && m_process.state() == QProcess::Running;
}

void ChildSession::appendLogLine(const QString& line) {
    m_pendingLogs.append(line);
    m_allLogs.append(line);
    if (m_allLogs.size() > kMaxLogLines)
        m_allLogs.erase(m_allLogs.begin(), m_allLogs.begin() + (m_allLogs.size() - kMaxLogLines));
}

void ChildSession::processStdoutLine(const QByteArray& rawLine) {
    QByteArray line = rawLine;
    if (line.endsWith('\r')) line.chop(1);
    if (!m_ipcPrefix.isEmpty() && line.startsWith(m_ipcPrefix)) {
        const QByteArray payload = line.mid(m_ipcPrefix.size());
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject obj = doc.object();
            if (obj.value(QStringLiteral("event")).toString() == QLatin1String("frame")) {
                m_lastLiveFrameEvent = obj;
                if (obj.contains(QStringLiteral("viewport")))
                    m_viewport = obj.value(QStringLiteral("viewport")).toObject();
                if (obj.contains(QStringLiteral("frameCount")))
                    m_lastFrameCount =
                        obj.value(QStringLiteral("frameCount")).toInt(m_lastFrameCount);
                emit liveFrameReceived(obj);
                return;
            }
            m_incomingMessages.append(obj);
            return;
        }
    }
    if (!line.isEmpty()) appendLogLine(QStringLiteral("[stdout] ") + QString::fromUtf8(line));
}

void ChildSession::drainStdout() {
    m_stdoutBuffer.append(m_process.readAllStandardOutput());
    qsizetype newline = -1;
    while ((newline = m_stdoutBuffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_stdoutBuffer.left(newline);
        m_stdoutBuffer.remove(0, newline + 1);
        processStdoutLine(line);
    }
}

void ChildSession::drainStderr() {
    m_stderrBuffer.append(m_process.readAllStandardError());
    qsizetype newline = -1;
    while ((newline = m_stderrBuffer.indexOf('\n')) >= 0) {
        QByteArray line = m_stderrBuffer.left(newline);
        m_stderrBuffer.remove(0, newline + 1);
        if (line.endsWith('\r')) line.chop(1);
        if (!line.isEmpty()) appendLogLine(QString::fromUtf8(line));
    }
}

bool ChildSession::waitForEventOrReply(int expectedId, int timeoutMs, QJsonObject* outMessage) {
    const auto takeMatch = [&]() -> bool {
        for (qsizetype i = 0; i < m_incomingMessages.size(); ++i) {
            const QJsonObject& msg = m_incomingMessages.at(i);
            const bool matches =
                expectedId < 0
                    ? (msg.value(QStringLiteral("event")).toString() == QLatin1String("ready"))
                    : (msg.contains(QStringLiteral("id")) &&
                       msg.value(QStringLiteral("id")).toInt(-1) == expectedId);
            if (matches) {
                if (outMessage) *outMessage = msg;
                m_incomingMessages.removeAt(i);
                return true;
            }
        }
        return false;
    };

    drainStdout();
    drainStderr();
    if (takeMatch()) return true;
    if (m_exited) return false;

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    bool matched = false;

    const auto check = [&] {
        drainStdout();
        drainStderr();
        if (takeMatch()) {
            matched = true;
            loop.quit();
        } else if (m_exited) {
            loop.quit();
        }
    };

    const auto stdoutConn = connect(&m_process, &QProcess::readyReadStandardOutput, &loop, check);
    const auto stderrConn = connect(&m_process, &QProcess::readyReadStandardError, &loop, check);
    const auto exitConn = connect(this, &ChildSession::childExited, &loop, check);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeout.start((std::max)(1, timeoutMs));
    loop.exec();

    disconnect(stdoutConn);
    disconnect(stderrConn);
    disconnect(exitConn);
    if (!matched) {
        drainStdout();
        drainStderr();
        matched = takeMatch();
    }
    return matched;
}

bool ChildSession::start(const LaunchConfig& config, QJsonObject* readyInfo, QString* error) {
    m_controlDir = std::make_unique<QTemporaryDir>();
    if (!m_controlDir->isValid()) {
        if (error) *error = QStringLiteral("Cannot create Runner handshake directory.");
        return false;
    }

    const QString handshakePath = m_controlDir->filePath(QStringLiteral("activation.json"));
    const QString handshakeToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString sessionToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_ipcPrefix = QByteArrayLiteral("QMLTUI_IPC_") + sessionToken.toUtf8() + ':';
    m_artifactDir = config.artifactDir;
    m_clock = config.clock;
    m_operationTimeoutMs = config.operationTimeoutMs;

    QProcessEnvironment env = config.environment.isEmpty()
                                  ? QProcessEnvironment::systemEnvironment()
                                  : config.environment;
    for (const QString& key : env.keys()) {
        if (key.startsWith(QLatin1String("QMLTUI_RUNNER_"))) env.remove(key);
    }
    const auto setRunnerEnv = [&](const QString& key, const QString& value) {
        env.insert(QStringLiteral("QMLTUI_RUNNER_") + key, value);
    };
    setRunnerEnv(QStringLiteral("INTERACTIVE"), QStringLiteral("1"));
    setRunnerEnv(QStringLiteral("SESSION_TOKEN"), sessionToken);
    setRunnerEnv(QStringLiteral("COLUMNS"), QString::number(config.columns));
    setRunnerEnv(QStringLiteral("ROWS"), QString::number(config.rows));
    setRunnerEnv(QStringLiteral("CLOCK"), config.clock);
    setRunnerEnv(QStringLiteral("OPERATION_TIMEOUT_MS"),
                 QString::number(config.operationTimeoutMs));
    setRunnerEnv(QStringLiteral("ARTIFACT_DIR"), config.artifactDir);
    setRunnerEnv(QStringLiteral("HANDSHAKE_PATH"), handshakePath);
    setRunnerEnv(QStringLiteral("HANDSHAKE_TOKEN"), handshakeToken);
    setRunnerEnv(QStringLiteral("LIVE"), QStringLiteral("0"));
    if (config.liveStream) setRunnerEnv(QStringLiteral("LIVE_STREAM"), QStringLiteral("1"));
    if (!config.recordPath.isEmpty())
        setRunnerEnv(QStringLiteral("RECORD"),
                     QDir(config.artifactDir).filePath(config.recordPath));

    env.insert(QStringLiteral("QT_FORCE_STDERR_LOGGING"), QStringLiteral("1"));
    if (!env.contains(QStringLiteral("QMLTUI_DEBUG_PAINT_DIRTY")))
        env.insert(QStringLiteral("QMLTUI_DEBUG_PAINT_DIRTY"), QStringLiteral("1"));

    m_process.setProcessEnvironment(env);
    if (!config.workingDirectory.isEmpty()) m_process.setWorkingDirectory(config.workingDirectory);

    QDeadlineTimer deadline(config.startupTimeoutMs);
    m_process.start(config.program, config.arguments);
    if (!m_process.waitForStarted(
            static_cast<int>((std::max<qint64>)(1, deadline.remainingTime())))) {
        if (error)
            *error = QStringLiteral("Cannot start application: %1").arg(m_process.errorString());
        return false;
    }
    m_pid = m_process.processId();

    bool handshakeOk = false;
    QJsonObject ready;
    while (!deadline.hasExpired()) {
        if (!handshakeOk && QFileInfo::exists(handshakePath)) {
            const QJsonObject ack = readJsonObject(handshakePath);
            if (ack.value(QStringLiteral("protocolVersion")).toInt(-1) != 1 ||
                ack.value(QStringLiteral("token")).toString() != handshakeToken ||
                ack.value(QStringLiteral("pid")).toInteger(-1) != m_pid) {
                terminateProcess(m_process);
                if (error) *error = QStringLiteral("Invalid Runner activation handshake.");
                return false;
            }
            handshakeOk = true;
        }
        const int slice = static_cast<int>(
            (std::min<qint64>)(50, (std::max<qint64>)(1, deadline.remainingTime())));
        if (waitForEventOrReply(-1, slice, &ready)) break;
        if (m_exited) break;
    }

    if (ready.isEmpty()) {
        drainStderr();
        terminateProcess(m_process);
        QString detail =
            handshakeOk
                ? QStringLiteral(
                      "Application activated Runner but did not emit initial ready frame.")
                : QStringLiteral(
                      "Runner did not activate. Link an interactive-capable QmlTui::Runner or "
                      "increase --startup-timeout-ms.");
        if (!m_allLogs.isEmpty())
            detail += QStringLiteral("\nLogs:\n") + m_allLogs.join(QLatin1Char('\n'));
        if (error) *error = detail;
        return false;
    }

    m_mode = ready.value(QStringLiteral("mode")).toString(QStringLiteral("AltScreen"));
    m_clock = ready.value(QStringLiteral("clock")).toString(config.clock);
    m_viewport = ready.value(QStringLiteral("viewport")).toObject();
    m_initialColumns = m_viewport.value(QStringLiteral("columns")).toInt(config.columns);
    m_initialRows = m_viewport.value(QStringLiteral("rows")).toInt(config.rows);
    m_lastFrame = ready.value(QStringLiteral("frame")).toString();
    m_lastFrameCount = ready.value(QStringLiteral("frameCount")).toInt(0);
    m_lastFrameStats = ready.value(QStringLiteral("frameStats")).toObject();
    if (ready.contains(QStringLiteral("ansi"))) {
        QJsonObject frameEv{{QStringLiteral("event"), QStringLiteral("frame")},
                            {QStringLiteral("frameCount"), m_lastFrameCount},
                            {QStringLiteral("viewport"), m_viewport},
                            {QStringLiteral("ansi"), ready.value(QStringLiteral("ansi"))}};
        if (ready.contains(QStringLiteral("scrollbackAnsi"))) {
            frameEv.insert(QStringLiteral("scrollbackAnsi"),
                           ready.value(QStringLiteral("scrollbackAnsi")));
        }
        m_lastLiveFrameEvent = frameEv;
    }

    QJsonObject info = ready;
    info.insert(QStringLiteral("ok"), true);
    info.insert(QStringLiteral("started"), true);
    info.insert(QStringLiteral("artifacts"), m_artifactDir);
    if (!m_pendingLogs.isEmpty()) {
        QJsonArray logs;
        for (const QString& l : std::as_const(m_pendingLogs))
            logs.append(l);
        info.insert(QStringLiteral("logs"), logs);
        m_pendingLogs.clear();
    }
    if (readyInfo) *readyInfo = info;
    return true;
}

void ChildSession::recordStepIfExportable(const QString& cmd, const QJsonObject& args) {
    if (isExportableCommand(cmd)) m_history.append(RecordedStep{cmd, args});
}

QString ChildSession::exportScenarioJs() const {
    return exportScenarioScript(m_history,
                                ExportMetadata{m_clock, m_initialColumns, m_initialRows});
}

QJsonObject ChildSession::executeCommand(const QString& cmd, const QJsonObject& args,
                                         const QJsonObject& include, int timeoutMs) {
    if (cmd == QLatin1String("logs")) {
        drainStdout();
        drainStderr();
        const bool all = args.value(QStringLiteral("all")).toBool(false);
        QJsonArray arr;
        const QStringList& source = all ? m_allLogs : m_pendingLogs;
        for (const QString& line : source)
            arr.append(line);
        if (!all) m_pendingLogs.clear();
        return QJsonObject{{QStringLiteral("ok"), true}, {QStringLiteral("logs"), arr}};
    }
    if (cmd == QLatin1String("export")) {
        return QJsonObject{{QStringLiteral("ok"), true},
                           {QStringLiteral("script"), exportScenarioJs()},
                           {QStringLiteral("stepCount"), static_cast<int>(m_history.size())}};
    }
    if (cmd == QLatin1String("info")) {
        return QJsonObject{{QStringLiteral("ok"), true},
                           {QStringLiteral("running"), isRunning()},
                           {QStringLiteral("pid"), m_pid},
                           {QStringLiteral("mode"), m_mode},
                           {QStringLiteral("clock"), m_clock},
                           {QStringLiteral("viewport"), m_viewport},
                           {QStringLiteral("artifacts"), m_artifactDir},
                           {QStringLiteral("frameCount"), m_lastFrameCount},
                           {QStringLiteral("frameStats"), m_lastFrameStats},
                           {QStringLiteral("stepCount"), static_cast<int>(m_history.size())}};
    }

    if (m_exited || m_process.state() != QProcess::Running) {
        return QJsonObject{
            {QStringLiteral("ok"), false},
            {QStringLiteral("exited"), true},
            {QStringLiteral("exitCode"), m_exitCode},
            {QStringLiteral("exitStatus"), m_exitStatus == QProcess::NormalExit
                                               ? QStringLiteral("normal")
                                               : QStringLiteral("crash")},
            {QStringLiteral("error"), QStringLiteral("Application is no longer running.")}};
    }

    m_pendingLogs.clear();
    const int id = m_nextRequestId++;
    QJsonObject request;
    request.insert(QStringLiteral("id"), id);
    request.insert(QStringLiteral("cmd"), cmd);
    if (!args.isEmpty()) request.insert(QStringLiteral("args"), args);
    if (!include.isEmpty()) request.insert(QStringLiteral("include"), include);
    if (timeoutMs > 0) request.insert(QStringLiteral("timeoutMs"), timeoutMs);

    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (m_process.write(payload) != payload.size()) {
        return QJsonObject{
            {QStringLiteral("ok"), false},
            {QStringLiteral("error"), QStringLiteral("Failed to write command to application: %1")
                                          .arg(m_process.errorString())}};
    }

    const int waitBudget =
        (timeoutMs > 0 ? timeoutMs : (std::max)(m_operationTimeoutMs + 5000, 30000)) + 5000;
    QJsonObject reply;
    const bool gotReply = waitForEventOrReply(id, waitBudget, &reply);

    if (!gotReply) {
        drainStdout();
        drainStderr();
        QJsonArray logs;
        for (const QString& line : std::as_const(m_pendingLogs))
            logs.append(line);
        m_pendingLogs.clear();

        if (m_exited) {
            const bool cleanExit = (m_exitStatus == QProcess::NormalExit && m_exitCode == 0);
            if (cleanExit) recordStepIfExportable(cmd, args);
            QJsonObject exitReply{{QStringLiteral("ok"), cleanExit},
                                  {QStringLiteral("exited"), true},
                                  {QStringLiteral("exitCode"), m_exitCode},
                                  {QStringLiteral("exitStatus"),
                                   m_exitStatus == QProcess::NormalExit ? QStringLiteral("normal")
                                                                        : QStringLiteral("crash")},
                                  {QStringLiteral("viewport"), m_viewport},
                                  {QStringLiteral("frame"), m_lastFrame},
                                  {QStringLiteral("frameCount"), m_lastFrameCount},
                                  {QStringLiteral("frameStats"), m_lastFrameStats}};
            if (!cleanExit) {
                exitReply.insert(
                    QStringLiteral("error"),
                    m_exitStatus == QProcess::CrashExit
                        ? QStringLiteral("Application crashed during command \"%1\".").arg(cmd)
                        : QStringLiteral("Application exited with code %1 during command \"%2\".")
                              .arg(m_exitCode)
                              .arg(cmd));
            }
            if (!logs.isEmpty()) exitReply.insert(QStringLiteral("logs"), logs);
            return exitReply;
        }
        return QJsonObject{
            {QStringLiteral("ok"), false},
            {QStringLiteral("error"),
             QStringLiteral("Timed out waiting for response to command \"%1\".").arg(cmd)},
            {QStringLiteral("logs"), logs}};
    }

    if (reply.contains(QStringLiteral("viewport")))
        m_viewport = reply.value(QStringLiteral("viewport")).toObject();
    if (reply.contains(QStringLiteral("frame")))
        m_lastFrame = reply.value(QStringLiteral("frame")).toString();
    if (reply.contains(QStringLiteral("frameCount")))
        m_lastFrameCount = reply.value(QStringLiteral("frameCount")).toInt(m_lastFrameCount);
    if (reply.contains(QStringLiteral("frameStats")))
        m_lastFrameStats = reply.value(QStringLiteral("frameStats")).toObject();
    if (cmd == QLatin1String("setLiveStream") && reply.contains(QStringLiteral("ansi"))) {
        QJsonObject frameEv{{QStringLiteral("event"), QStringLiteral("frame")},
                            {QStringLiteral("frameCount"), m_lastFrameCount},
                            {QStringLiteral("viewport"), m_viewport},
                            {QStringLiteral("ansi"), reply.value(QStringLiteral("ansi"))}};
        if (reply.contains(QStringLiteral("scrollbackAnsi"))) {
            frameEv.insert(QStringLiteral("scrollbackAnsi"),
                           reply.value(QStringLiteral("scrollbackAnsi")));
        }
        m_lastLiveFrameEvent = frameEv;
    }

    if (reply.value(QStringLiteral("ok")).toBool() &&
        args.value(QStringLiteral("record")).toBool(true)) {
        recordStepIfExportable(cmd, args);
    }

    if (reply.value(QStringLiteral("exiting")).toBool()) {
        m_process.closeWriteChannel();
        if (!m_exited) m_process.waitForFinished(1500);
        drainStdout();
        drainStderr();
        reply.insert(QStringLiteral("exited"), true);
        reply.insert(QStringLiteral("exitCode"), m_exitCode);
        reply.insert(QStringLiteral("exitStatus"), m_exitStatus == QProcess::NormalExit
                                                       ? QStringLiteral("normal")
                                                       : QStringLiteral("crash"));
    }

    if (!m_pendingLogs.isEmpty()) {
        QJsonArray logs;
        for (const QString& line : std::as_const(m_pendingLogs))
            logs.append(line);
        reply.insert(QStringLiteral("logs"), logs);
        m_pendingLogs.clear();
    }
    return reply;
}

QJsonObject ChildSession::stop() {
    if (m_stopping)
        return QJsonObject{{QStringLiteral("ok"), true}, {QStringLiteral("stopped"), true}};
    m_stopping = true;

    QJsonObject result;
    if (isRunning()) {
        result = executeCommand(QStringLiteral("close"), {}, {}, 5000);
        m_process.closeWriteChannel();
        if (!m_exited) m_process.waitForFinished(1500);
        if (!m_exited) terminateProcess(m_process);
    } else {
        result.insert(QStringLiteral("ok"), true);
    }
    drainStdout();
    drainStderr();
    result.insert(QStringLiteral("stopped"), true);
    result.insert(QStringLiteral("exited"), true);
    result.insert(QStringLiteral("exitCode"), m_exitCode);
    result.insert(QStringLiteral("exitStatus"), m_exitStatus == QProcess::NormalExit
                                                    ? QStringLiteral("normal")
                                                    : QStringLiteral("crash"));
    result.insert(QStringLiteral("artifacts"), m_artifactDir);
    return result;
}

int runInteractiveStdioMode(const ChildSession::LaunchConfig& config) {
    ChildSession session;
    QJsonObject ready;
    QString err;
    if (!session.start(config, &ready, &err)) {
        QTextStream(stderr) << "qmltui-runner: " << err << '\n';
        return 2;
    }

    const auto writeJsonLine = [](const QJsonObject& obj) {
        const QByteArray bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
        std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stdout);
        std::fflush(stdout);
    };

    writeJsonLine(ready);

    QEventLoop loop;
    auto stopped = std::make_shared<std::atomic<bool>>(false);

    QObject::connect(
        &session, &ChildSession::childExited, &loop, [&](int code, QProcess::ExitStatus status) {
            if (stopped->load()) return;
            writeJsonLine(QJsonObject{{QStringLiteral("event"), QStringLiteral("exited")},
                                      {QStringLiteral("exitCode"), code},
                                      {QStringLiteral("exitStatus"),
                                       status == QProcess::NormalExit ? QStringLiteral("normal")
                                                                      : QStringLiteral("crash")}});
            stopped->store(true);
            loop.quit();
        });

    std::thread reader([&session, &loop, stopped, &writeJsonLine] {
        std::string line;
        while (!stopped->load() && std::getline(std::cin, line)) {
            if (stopped->load()) break;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            QByteArray raw(line.data(), static_cast<qsizetype>(line.size()));
            QMetaObject::invokeMethod(
                &session,
                [&session, &loop, stopped, &writeJsonLine, raw = std::move(raw)] {
                    if (stopped->load()) return;
                    const QByteArray trimmed = raw.trimmed();
                    if (trimmed.isEmpty()) return;
                    QJsonParseError parseErr;
                    const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseErr);
                    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
                        writeJsonLine(QJsonObject{
                            {QStringLiteral("ok"), false},
                            {QStringLiteral("error"),
                             QStringLiteral("Invalid JSON: %1").arg(parseErr.errorString())}});
                        return;
                    }
                    const QJsonObject req = doc.object();
                    const QString cmd = req.value(QStringLiteral("cmd")).toString();
                    if (cmd == QLatin1String("close") || cmd == QLatin1String("stop")) {
                        stopped->store(true);
                        QJsonObject res = session.stop();
                        if (req.contains(QStringLiteral("id")))
                            res.insert(QStringLiteral("id"), req.value(QStringLiteral("id")));
                        writeJsonLine(res);
                        loop.quit();
                        return;
                    }
                    QJsonObject res =
                        session.executeCommand(cmd, req.value(QStringLiteral("args")).toObject(),
                                               req.value(QStringLiteral("include")).toObject(),
                                               req.value(QStringLiteral("timeoutMs")).toInt(-1));
                    if (req.contains(QStringLiteral("id")))
                        res.insert(QStringLiteral("id"), req.value(QStringLiteral("id")));
                    writeJsonLine(res);
                    if (res.value(QStringLiteral("exited")).toBool()) {
                        stopped->store(true);
                        loop.quit();
                    }
                },
                Qt::BlockingQueuedConnection);
        }
        if (!stopped->load()) {
            QMetaObject::invokeMethod(
                &session,
                [&session, &loop, stopped] {
                    if (stopped->load()) return;
                    stopped->store(true);
                    (void)session.stop();
                    loop.quit();
                },
                Qt::QueuedConnection);
        }
    });

    loop.exec();
    stopped->store(true);
    if (reader.joinable()) reader.detach();
    return 0;
}

} // namespace qmltui::runner_cli
