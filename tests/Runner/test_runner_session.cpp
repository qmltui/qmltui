#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

namespace {

QString launcherBinary() {
    return qEnvironmentVariable("QMLTUI_TEST_LAUNCHER",
                                QStringLiteral(QMLTUI_DEFAULT_TEST_LAUNCHER));
}

QString fixtureBinary() {
    return qEnvironmentVariable("QMLTUI_TEST_FIXTURE", QStringLiteral(QMLTUI_DEFAULT_TEST_FIXTURE));
}

QProcessEnvironment baseEnv(const QString& sessionDir) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString importPath = QStringLiteral(QMLTUI_DEFAULT_QML_IMPORT_PATH);
    if (!env.contains(QStringLiteral("QML_IMPORT_PATH")))
        env.insert(QStringLiteral("QML_IMPORT_PATH"), importPath);
    if (!env.contains(QStringLiteral("QML2_IMPORT_PATH")))
        env.insert(QStringLiteral("QML2_IMPORT_PATH"), importPath);
    if (!sessionDir.isEmpty()) env.insert(QStringLiteral("QMLTUI_RUNNER_SESSION_DIR"), sessionDir);
    return env;
}

QJsonObject parseLastJsonLine(const QByteArray& output) {
    const QList<QByteArray> lines = output.trimmed().split('\n');
    for (qsizetype i = lines.size() - 1; i >= 0; --i) {
        const QJsonDocument doc = QJsonDocument::fromJson(lines.at(i).trimmed());
        if (doc.isObject()) return doc.object();
    }
    return {};
}

bool readJsonLineFromProcess(QProcess& process, QByteArray& buffer, QJsonObject* out,
                             int timeoutMs = 5000) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        const qsizetype nl = buffer.indexOf('\n');
        if (nl >= 0) {
            const QByteArray line = buffer.left(nl).trimmed();
            buffer.remove(0, nl + 1);
            const QJsonDocument doc = QJsonDocument::fromJson(line);
            if (doc.isObject()) {
                if (out) *out = doc.object();
                return true;
            }
            continue;
        }
        if (process.bytesAvailable() > 0 || process.waitForReadyRead(100)) {
            buffer.append(process.readAllStandardOutput());
            continue;
        }
        if (process.state() == QProcess::NotRunning) {
            buffer.append(process.readAllStandardOutput());
            const qsizetype nlAfter = buffer.indexOf('\n');
            if (nlAfter >= 0) {
                const QByteArray line = buffer.left(nlAfter).trimmed();
                buffer.remove(0, nlAfter + 1);
                const QJsonDocument doc = QJsonDocument::fromJson(line);
                if (doc.isObject()) {
                    if (out) *out = doc.object();
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

} // namespace

class RunnerSessionTest : public QObject {
    Q_OBJECT

    struct CmdResult {
        int code = -1;
        QByteArray out;
        QByteArray err;
    };

    CmdResult runLauncher(const QStringList& args, const QString& sessionDir) {
        QProcess proc;
        proc.setProcessEnvironment(baseEnv(sessionDir));
        proc.start(launcherBinary(), args);
        if (!proc.waitForStarted(5000)) return {-100, {}, proc.errorString().toUtf8()};
        if (!proc.waitForFinished(20000)) {
            proc.kill();
            proc.waitForFinished(1000);
            return {-101, proc.readAllStandardOutput(), proc.readAllStandardError()};
        }
        return {proc.exitStatus() == QProcess::NormalExit ? proc.exitCode() : -102,
                proc.readAllStandardOutput(), proc.readAllStandardError()};
    }

private slots:
    void interactiveStdioProtocol() {
        QTemporaryDir artifactParent;
        QVERIFY(artifactParent.isValid());
        const QString artifacts = artifactParent.filePath(QStringLiteral("stdio-artifacts"));

        QProcess proc;
        proc.setProcessEnvironment(baseEnv({}));
        proc.start(launcherBinary(),
                   {QStringLiteral("--interactive"), QStringLiteral("--clock"),
                    QStringLiteral("virtual"), QStringLiteral("--columns"), QStringLiteral("80"),
                    QStringLiteral("--rows"), QStringLiteral("24"), QStringLiteral("--artifacts"),
                    artifacts, QStringLiteral("--"), fixtureBinary()});
        QVERIFY2(proc.waitForStarted(5000), qPrintable(proc.errorString()));

        QByteArray buf;
        QJsonObject ready;
        QVERIFY2(readJsonLineFromProcess(proc, buf, &ready, 8000),
                 qPrintable(QString::fromUtf8(proc.readAllStandardError())));
        QCOMPARE(ready.value(QStringLiteral("event")).toString(), QStringLiteral("ready"));
        QCOMPARE(ready.value(QStringLiteral("protocolVersion")).toInt(), 1);
        QVERIFY(ready.value(QStringLiteral("frame"))
                    .toString()
                    .contains(QStringLiteral("TESTRUNNER FIXTURE")));

        const auto sendCmd = [&](const QJsonObject& req, QJsonObject* reply,
                                 int timeoutMs = 5000) -> bool {
            const QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact) + '\n';
            if (proc.write(payload) != payload.size()) return false;
            return readJsonLineFromProcess(proc, buf, reply, timeoutMs);
        };

        // 1. Mutating key press
        QJsonObject reply;
        QVERIFY(sendCmd(QJsonObject{{QStringLiteral("id"), 1},
                                    {QStringLiteral("cmd"), QStringLiteral("press")},
                                    {QStringLiteral("args"),
                                     QJsonObject{{QStringLiteral("key"), QStringLiteral("k")}}}},
                        &reply));
        QCOMPARE(reply.value(QStringLiteral("id")).toInt(), 1);
        QVERIFY(reply.value(QStringLiteral("ok")).toBool());
        QVERIFY(reply.value(QStringLiteral("frame"))
                    .toString()
                    .contains(QStringLiteral("STATUS: KEY K")));

        // 2. Recoverable command failure must NOT kill the app
        QVERIFY(sendCmd(QJsonObject{{QStringLiteral("id"), 2},
                                    {QStringLiteral("cmd"), QStringLiteral("clickText")},
                                    {QStringLiteral("args"),
                                     QJsonObject{{QStringLiteral("text"),
                                                  QStringLiteral("NONEXISTENT_LABEL")}}}},
                        &reply));
        QCOMPARE(reply.value(QStringLiteral("id")).toInt(), 2);
        QVERIFY(!reply.value(QStringLiteral("ok")).toBool());
        QVERIFY(reply.value(QStringLiteral("error"))
                    .toString()
                    .contains(QStringLiteral("was not found")));

        // 3. Hard JS watchdog interrupts runaway loop and keeps app alive
        QVERIFY(sendCmd(
            QJsonObject{{QStringLiteral("id"), 3},
                        {QStringLiteral("cmd"), QStringLiteral("eval")},
                        {QStringLiteral("timeoutMs"), 300},
                        {QStringLiteral("args"),
                         QJsonObject{{QStringLiteral("code"), QStringLiteral("while (true) {}")}}}},
            &reply, 6000));
        QCOMPARE(reply.value(QStringLiteral("id")).toInt(), 3);
        QVERIFY(!reply.value(QStringLiteral("ok")).toBool());
        QVERIFY(reply.value(QStringLiteral("error"))
                    .toString()
                    .contains(QStringLiteral("interrupted")));

        // 4. App still responds after hard watchdog interruption
        QVERIFY(sendCmd(QJsonObject{{QStringLiteral("id"), 4},
                                    {QStringLiteral("cmd"), QStringLiteral("eval")},
                                    {QStringLiteral("args"),
                                     QJsonObject{{QStringLiteral("code"),
                                                  QStringLiteral("app.size().columns")}}}},
                        &reply));
        QVERIFY(reply.value(QStringLiteral("ok")).toBool());
        QCOMPARE(reply.value(QStringLiteral("result")).toInt(), 80);

        // 5. Clean close writes result.json
        QVERIFY(sendCmd(QJsonObject{{QStringLiteral("id"), 5},
                                    {QStringLiteral("cmd"), QStringLiteral("close")}},
                        &reply));
        QVERIFY(reply.value(QStringLiteral("ok")).toBool());
        QVERIFY(proc.waitForFinished(5000));
        QCOMPARE(proc.exitCode(), 0);

        QFile resultFile(QDir(artifacts).filePath(QStringLiteral("result.json")));
        QVERIFY(resultFile.open(QIODevice::ReadOnly));
        const QJsonObject resultJson = QJsonDocument::fromJson(resultFile.readAll()).object();
        QCOMPARE(resultJson.value(QStringLiteral("mode")).toString(),
                 QStringLiteral("interactive"));
        QCOMPARE(resultJson.value(QStringLiteral("status")).toString(), QStringLiteral("passed"));
    }

#if QMLTUI_RUNNER_HAS_SESSION_DAEMON
    void sessionFullWorkflowAndExport() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sessionDir = temp.filePath(QStringLiteral("sessions"));

        // Start session
        const auto started = runLauncher(
            {QStringLiteral("session"), QStringLiteral("start"), QStringLiteral("--name"),
             QStringLiteral("work"), QStringLiteral("--clock"), QStringLiteral("virtual"),
             QStringLiteral("--columns"), QStringLiteral("80"), QStringLiteral("--rows"),
             QStringLiteral("24"), QStringLiteral("--"), fixtureBinary()},
            sessionDir);
        QCOMPARE(started.code, 0);
        QVERIFY2(started.out.contains("Session: work"), qPrintable(QString::fromUtf8(started.out)));
        QVERIFY(started.out.contains("TESTRUNNER FIXTURE"));
        QVERIFY(started.out.contains("0123456789")); // default ruler ON

        // List shows work as default
        const auto listed = runLauncher(
            {QStringLiteral("session"), QStringLiteral("list"), QStringLiteral("--json")},
            sessionDir);
        QCOMPARE(listed.code, 0);
        const QJsonObject listJson = parseLastJsonLine(listed.out);
        QCOMPARE(listJson.value(QStringLiteral("sessions")).toArray().size(), 1);
        QCOMPARE(listJson.value(QStringLiteral("sessions"))
                     .toArray()
                     .at(0)
                     .toObject()
                     .value(QStringLiteral("id"))
                     .toString(),
                 QStringLiteral("work"));

        // Inspect QML item tree (hidden omitted by default, shown with --include-hidden)
        const auto treeDefault =
            runLauncher({QStringLiteral("session"), QStringLiteral("tree")}, sessionDir);
        QCOMPARE(treeDefault.code, 0);
        QVERIFY(treeDefault.out.contains("#statusText"));
        QVERIFY(!treeDefault.out.contains("#hiddenMarker"));

        const auto treeHidden = runLauncher(
            {QStringLiteral("session"), QStringLiteral("tree"), QStringLiteral("--include-hidden")},
            sessionDir);
        QCOMPARE(treeHidden.code, 0);
        QVERIFY(treeHidden.out.contains("#hiddenMarker"));
        QVERIFY(treeHidden.out.contains("[hidden]"));

        // Press key + virtual clock animation (while root has focus before focusing TextInput)
        QCOMPARE(
            runLauncher({QStringLiteral("session"), QStringLiteral("press"), QStringLiteral("k")},
                        sessionDir)
                .code,
            0);
        QCOMPARE(
            runLauncher({QStringLiteral("session"), QStringLiteral("press"), QStringLiteral("a")},
                        sessionDir)
                .code,
            0);
        const auto advanced = runLauncher(
            {QStringLiteral("session"), QStringLiteral("advance"), QStringLiteral("450")},
            sessionDir);
        QCOMPARE(advanced.code, 0);
        QVERIFY(advanced.out.contains("virtualTime=450ms"));

        const auto barRect = runLauncher({QStringLiteral("session"), QStringLiteral("item-rect"),
                                          QStringLiteral("bar"), QStringLiteral("--json")},
                                         sessionDir);
        QCOMPARE(barRect.code, 0);
        QCOMPARE(parseLastJsonLine(barRect.out)
                     .value(QStringLiteral("result"))
                     .toObject()
                     .value(QStringLiteral("width"))
                     .toInt(),
                 24);

        // Focus TextInput + type text
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("click"),
                              QStringLiteral("5"), QStringLiteral("5")},
                             sessionDir)
                     .code,
                 0);
        const auto typed = runLauncher(
            {QStringLiteral("session"), QStringLiteral("type"), QStringLiteral("hello")},
            sessionDir);
        QCOMPARE(typed.code, 0);
        QVERIFY(typed.out.contains("INPUT: hello"));

        // Tree reflects ACTIVE FOCUS on TextInput
        const auto focusedTree =
            runLauncher({QStringLiteral("session"), QStringLiteral("tree")}, sessionDir);
        QCOMPARE(focusedTree.code, 0);
        QVERIFY(focusedTree.out.contains("TextInput"));
        QVERIFY(focusedTree.out.contains("[ACTIVE FOCUS]"));

        // Recoverable error exits 1 but keeps the session alive
        const auto badClick = runLauncher({QStringLiteral("session"), QStringLiteral("click-text"),
                                           QStringLiteral("DOES_NOT_EXIST")},
                                          sessionDir);
        QCOMPARE(badClick.code, 1);
        QVERIFY(badClick.out.contains("Error:"));
        QVERIFY(badClick.out.contains("INPUT: hello"));

        // Negative delta argument on scroll parses cleanly (not rejected as flag)
        const auto scrolled =
            runLauncher({QStringLiteral("session"), QStringLiteral("scroll"), QStringLiteral("10"),
                         QStringLiteral("5"), QStringLiteral("0"), QStringLiteral("-1")},
                        sessionDir);
        QCOMPARE(scrolled.code, 0);

        // Resize viewport
        const auto resized = runLauncher({QStringLiteral("session"), QStringLiteral("resize"),
                                          QStringLiteral("90"), QStringLiteral("26")},
                                         sessionDir);
        QCOMPARE(resized.code, 0);
        QVERIFY(resized.out.contains("SIZE: 90x26"));

        // Styled frame inspection
        const auto styled = runLauncher(
            {QStringLiteral("session"), QStringLiteral("frame"), QStringLiteral("--styled")},
            sessionDir);
        QCOMPARE(styled.code, 0);
        QVERIFY(styled.out.contains("--- Styles ---"));
        QVERIFY(styled.out.contains("fg=#"));

        // Eval expression returns result
        const auto evalRes =
            runLauncher({QStringLiteral("session"), QStringLiteral("eval"),
                         QStringLiteral("app.expectText(\"INPUT: hello\"); app.size().columns;"),
                         QStringLiteral("--json")},
                        sessionDir);
        QCOMPARE(evalRes.code, 0);
        const QJsonObject evalObj = parseLastJsonLine(evalRes.out);
        QVERIFY(evalObj.value(QStringLiteral("ok")).toBool());

        // Export scenario to .js and verify failed/read-only steps are excluded
        const QString exportedPath = temp.filePath(QStringLiteral("exported_flow.js"));
        const auto exported = runLauncher({QStringLiteral("session"), QStringLiteral("export"),
                                           QStringLiteral("-o"), exportedPath},
                                          sessionDir);
        QCOMPARE(exported.code, 0);
        QFile exportedFile(exportedPath);
        QVERIFY(exportedFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString script = QString::fromUtf8(exportedFile.readAll());
        QVERIFY(script.contains(QStringLiteral("/** @param {Runner} app */")));
        QVERIFY(script.contains(QStringLiteral("app.press(\"k\");")));
        QVERIFY(script.contains(QStringLiteral("app.typeText(\"hello\");")));
        QVERIFY(script.contains(QStringLiteral("app.advanceTime(450);")));
        QVERIFY(script.contains(QStringLiteral("app.resize(90, 26);")));
        QVERIFY(!script.contains(QStringLiteral("DOES_NOT_EXIST")));

        // Stop session
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("stop")}, sessionDir).code,
                 0);

        // Replay the exported .js script via batch qmltui-runner to prove it passes!
        const QString replayArtifacts = temp.filePath(QStringLiteral("replay-artifacts"));
        const auto replay =
            runLauncher({QStringLiteral("--clock"), QStringLiteral("virtual"),
                         QStringLiteral("--script"), exportedPath, QStringLiteral("--artifacts"),
                         replayArtifacts, QStringLiteral("--"), fixtureBinary()},
                        sessionDir);
        QCOMPARE(replay.code, 0);
        QFile finalFrame(QDir(replayArtifacts).filePath(QStringLiteral("final.txt")));
        QVERIFY(finalFrame.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString finalText = QString::fromUtf8(finalFrame.readAll());
        QVERIFY(finalText.contains(QStringLiteral("INPUT: hello")));
        QVERIFY(finalText.contains(QStringLiteral("SIZE: 90x26")));
    }

    void sessionSelfQuit() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sessionDir = temp.filePath(QStringLiteral("sessions"));

        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("start"),
                              QStringLiteral("--name"), QStringLiteral("quitter"),
                              QStringLiteral("--clock"), QStringLiteral("virtual"),
                              QStringLiteral("--"), fixtureBinary()},
                             sessionDir)
                     .code,
                 0);

        // Pressing 'q' triggers Qt.quit() in Fixture.qml.
        const auto quitRes = runLauncher({QStringLiteral("session"), QStringLiteral("press"),
                                          QStringLiteral("q"), QStringLiteral("--json")},
                                         sessionDir);
        QCOMPARE(quitRes.code, 0);
        const QJsonObject obj = parseLastJsonLine(quitRes.out);
        QVERIFY(obj.value(QStringLiteral("ok")).toBool());
        QVERIFY(obj.value(QStringLiteral("exited")).toBool());
        QCOMPARE(obj.value(QStringLiteral("exitCode")).toInt(), 0);
        QCOMPARE(obj.value(QStringLiteral("exitStatus")).toString(), QStringLiteral("normal"));

        // Daemon cleans up automatically after child exits.
        QThread::msleep(100);
        const auto listRes = runLauncher(
            {QStringLiteral("session"), QStringLiteral("list"), QStringLiteral("--json")},
            sessionDir);
        QCOMPARE(listRes.code, 0);
        QCOMPARE(parseLastJsonLine(listRes.out).value(QStringLiteral("sessions")).toArray().size(),
                 0);
    }

    void sessionInlineScrollback() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sessionDir = temp.filePath(QStringLiteral("sessions"));

        const auto started = runLauncher(
            {QStringLiteral("session"), QStringLiteral("start"), QStringLiteral("--name"),
             QStringLiteral("inline"), QStringLiteral("--clock"), QStringLiteral("virtual"),
             QStringLiteral("--columns"), QStringLiteral("80"), QStringLiteral("--rows"),
             QStringLiteral("12"), QStringLiteral("--"), fixtureBinary(),
             QStringLiteral("--fixture=inline")},
            sessionDir);
        QCOMPARE(started.code, 0);
        QVERIFY(started.out.contains("mode=Inline"));

        QCOMPARE(
            runLauncher({QStringLiteral("session"), QStringLiteral("press"), QStringLiteral("F1")},
                        sessionDir)
                .code,
            0);
        QCOMPARE(
            runLauncher({QStringLiteral("session"), QStringLiteral("press"), QStringLiteral("F3")},
                        sessionDir)
                .code,
            0);

        const auto sb = runLauncher(
            {QStringLiteral("session"), QStringLiteral("scrollback"), QStringLiteral("--styled")},
            sessionDir);
        QCOMPARE(sb.code, 0);
        QVERIFY(sb.out.contains("MODEL ROW 0"));
        QVERIFY(sb.out.contains("STYLED ROW"));
        QVERIFY(sb.out.contains("bold"));

        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("stop")}, sessionDir).code,
                 0);
    }

    void sessionRecordingCast() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sessionDir = temp.filePath(QStringLiteral("sessions"));
        const QString artifacts = temp.filePath(QStringLiteral("cast-artifacts"));

        QCOMPARE(runLauncher(
                     {QStringLiteral("session"), QStringLiteral("start"), QStringLiteral("--name"),
                      QStringLiteral("rec"), QStringLiteral("--clock"), QStringLiteral("virtual"),
                      QStringLiteral("--artifacts"), artifacts, QStringLiteral("--record"),
                      QStringLiteral("interactive.cast"), QStringLiteral("--"), fixtureBinary()},
                     sessionDir)
                     .code,
                 0);
        QCOMPARE(runLauncher(
                     {QStringLiteral("session"), QStringLiteral("marker"), QStringLiteral("start")},
                     sessionDir)
                     .code,
                 0);
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("click"),
                              QStringLiteral("5"), QStringLiteral("5")},
                             sessionDir)
                     .code,
                 0);
        QCOMPARE(
            runLauncher({QStringLiteral("session"), QStringLiteral("type"), QStringLiteral("cast")},
                        sessionDir)
                .code,
            0);
        QCOMPARE(runLauncher(
                     {QStringLiteral("session"), QStringLiteral("advance"), QStringLiteral("150")},
                     sessionDir)
                     .code,
                 0);
        QCOMPARE(runLauncher(
                     {QStringLiteral("session"), QStringLiteral("marker"), QStringLiteral("done")},
                     sessionDir)
                     .code,
                 0);
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("stop")}, sessionDir).code,
                 0);

        QFile castFile(QDir(artifacts).filePath(QStringLiteral("interactive.cast")));
        QVERIFY(castFile.exists());
        QVERIFY(castFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QByteArray raw = castFile.readAll();
        const QList<QByteArray> lines = raw.trimmed().split('\n');
        QVERIFY(lines.size() >= 3);
        const QJsonObject header = QJsonDocument::fromJson(lines.first()).object();
        QCOMPARE(header.value(QStringLiteral("version")).toInt(), 2);
        QVERIFY(raw.contains("\"m\",\"start\""));
        QVERIFY(raw.contains("\"m\",\"done\""));
        QVERIFY(raw.contains("TESTRUNNER FIXTURE"));
        QVERIFY(raw.contains("cast"));
    }

    void sessionConcurrentAndIdleTimeout() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sessionDir = temp.filePath(QStringLiteral("sessions"));

        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("start"),
                              QStringLiteral("--name"), QStringLiteral("alpha"),
                              QStringLiteral("--clock"), QStringLiteral("virtual"),
                              QStringLiteral("--"), fixtureBinary()},
                             sessionDir)
                     .code,
                 0);
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("start"),
                              QStringLiteral("--name"), QStringLiteral("beta"),
                              QStringLiteral("--clock"), QStringLiteral("virtual"),
                              QStringLiteral("--"), fixtureBinary()},
                             sessionDir)
                     .code,
                 0);

        // Duplicate name rejected
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("start"),
                              QStringLiteral("--name"), QStringLiteral("alpha"),
                              QStringLiteral("--"), fixtureBinary()},
                             sessionDir)
                     .code,
                 2);

        // Target alpha explicitly while beta is default
        const auto pressAlpha =
            runLauncher({QStringLiteral("session"), QStringLiteral("press"),
                         QStringLiteral("--session"), QStringLiteral("alpha"), QStringLiteral("k")},
                        sessionDir);
        QCOMPARE(pressAlpha.code, 0);
        QVERIFY(pressAlpha.out.contains("STATUS: KEY K"));

        // Beta still has STATUS: READY
        const auto betaFrame = runLauncher({QStringLiteral("session"), QStringLiteral("frame"),
                                            QStringLiteral("--session"), QStringLiteral("beta")},
                                           sessionDir);
        QCOMPARE(betaFrame.code, 0);
        QVERIFY(betaFrame.out.contains("STATUS: READY"));

        // Stop all
        QCOMPARE(runLauncher(
                     {QStringLiteral("session"), QStringLiteral("stop"), QStringLiteral("--all")},
                     sessionDir)
                     .code,
                 0);
        const auto afterStopAll = runLauncher(
            {QStringLiteral("session"), QStringLiteral("list"), QStringLiteral("--json")},
            sessionDir);
        QCOMPARE(
            parseLastJsonLine(afterStopAll.out).value(QStringLiteral("sessions")).toArray().size(),
            0);

        // Idle timeout auto-reaps orphaned daemon
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("start"),
                              QStringLiteral("--name"), QStringLiteral("ephemeral"),
                              QStringLiteral("--idle-timeout-ms"), QStringLiteral("350"),
                              QStringLiteral("--clock"), QStringLiteral("virtual"),
                              QStringLiteral("--"), fixtureBinary()},
                             sessionDir)
                     .code,
                 0);
        QThread::msleep(750);
        const auto afterIdle = runLauncher(
            {QStringLiteral("session"), QStringLiteral("list"), QStringLiteral("--json")},
            sessionDir);
        QCOMPARE(afterIdle.code, 0);
        QCOMPARE(
            parseLastJsonLine(afterIdle.out).value(QStringLiteral("sessions")).toArray().size(), 0);
    }

    void sessionLiveForegroundAndWatch() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sessionDir = temp.filePath(QStringLiteral("sessions"));

        // --live cannot be combined with --json
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("start"),
                              QStringLiteral("--live"), QStringLiteral("--json"),
                              QStringLiteral("--"), fixtureBinary()},
                             sessionDir)
                     .code,
                 2);

        // 1. Start a live foreground session (Terminal 1)
        QProcess liveTerm;
        liveTerm.setProcessEnvironment(baseEnv(sessionDir));
        liveTerm.start(launcherBinary(),
                       {QStringLiteral("session"), QStringLiteral("start"),
                        QStringLiteral("--name"), QStringLiteral("live-demo"),
                        QStringLiteral("--live"), QStringLiteral("--clock"),
                        QStringLiteral("virtual"), QStringLiteral("--columns"),
                        QStringLiteral("70"), QStringLiteral("--rows"), QStringLiteral("24"),
                        QStringLiteral("--"), fixtureBinary()});
        QVERIFY2(liveTerm.waitForStarted(5000), qPrintable(liveTerm.errorString()));

        // Wait until the live daemon registers itself
        QElapsedTimer waitReady;
        waitReady.start();
        bool found = false;
        while (waitReady.elapsed() < 5000) {
            const auto listed = runLauncher(
                {QStringLiteral("session"), QStringLiteral("list"), QStringLiteral("--json")},
                sessionDir);
            if (listed.code == 0 &&
                parseLastJsonLine(listed.out).value(QStringLiteral("sessions")).toArray().size() ==
                    1) {
                found = true;
                break;
            }
            QThread::msleep(50);
        }
        QVERIFY2(found, qPrintable(QString::fromUtf8(liveTerm.readAllStandardError())));

        // Drive the live session from Terminal 2
        const auto pressed = runLauncher({QStringLiteral("session"), QStringLiteral("press"),
                                          QStringLiteral("--session"), QStringLiteral("live-demo"),
                                          QStringLiteral("k")},
                                         sessionDir);
        QCOMPARE(pressed.code, 0);
        QVERIFY(pressed.out.contains("STATUS: KEY K"));

        // Stop from Terminal 2 and verify Terminal 1 rendered both frames and restored the screen
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("stop"),
                              QStringLiteral("--session"), QStringLiteral("live-demo")},
                             sessionDir)
                     .code,
                 0);
        QVERIFY(liveTerm.waitForFinished(5000));
        QCOMPARE(liveTerm.exitCode(), 0);
        const QByteArray liveOut = liveTerm.readAllStandardOutput();
        QVERIFY(liveOut.contains("\033[?1049h"));
        QVERIFY(liveOut.contains("TESTRUNNER FIXTURE"));
        QVERIFY(liveOut.contains("STATUS: KEY K"));
        QVERIFY(liveOut.contains("\033[?25h\033[?1049l"));

        // 2. Attach `session watch` to a normal headless background session
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("start"),
                              QStringLiteral("--name"), QStringLiteral("watched"),
                              QStringLiteral("--clock"), QStringLiteral("virtual"),
                              QStringLiteral("--"), fixtureBinary()},
                             sessionDir)
                     .code,
                 0);

        QProcess watcher;
        watcher.setProcessEnvironment(baseEnv(sessionDir));
        watcher.start(launcherBinary(), {QStringLiteral("session"), QStringLiteral("watch"),
                                         QStringLiteral("--session"), QStringLiteral("watched")});
        QVERIFY2(watcher.waitForStarted(5000), qPrintable(watcher.errorString()));

        QByteArray watchOut;
        QElapsedTimer waitInitial;
        waitInitial.start();
        while (waitInitial.elapsed() < 5000 && !watchOut.contains("TESTRUNNER FIXTURE")) {
            if (watcher.waitForReadyRead(100)) watchOut.append(watcher.readAllStandardOutput());
        }
        QVERIFY2(watchOut.contains("TESTRUNNER FIXTURE"),
                 qPrintable(QString::fromUtf8(watcher.readAllStandardError())));

        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("press"),
                              QStringLiteral("--session"), QStringLiteral("watched"),
                              QStringLiteral("k")},
                             sessionDir)
                     .code,
                 0);
        QCOMPARE(runLauncher({QStringLiteral("session"), QStringLiteral("stop"),
                              QStringLiteral("--session"), QStringLiteral("watched")},
                             sessionDir)
                     .code,
                 0);
        QVERIFY(watcher.waitForFinished(5000));
        QCOMPARE(watcher.exitCode(), 0);
        watchOut.append(watcher.readAllStandardOutput());
        QVERIFY(watchOut.contains("\033[?1049h"));
        QVERIFY(watchOut.contains("STATUS: KEY K"));
        QVERIFY(watchOut.contains("\033[?25h\033[?1049l"));
    }

    void sessionConnectBatchDiffAndClickItem() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sessionDir = temp.filePath(QStringLiteral("sessions"));
        const QString exportedPath = temp.filePath(QStringLiteral("connect_flow.js"));

        // Start via `s` shorthand alias
        bool stoppedRepl = false;
        const auto cleanupRepl = qScopeGuard([&] {
            if (!stoppedRepl) {
                (void)runLauncher({QStringLiteral("s"), QStringLiteral("stop"),
                                   QStringLiteral("--session"), QStringLiteral("repl")},
                                  sessionDir);
            }
        });
        QCOMPARE(
            runLauncher({QStringLiteral("s"), QStringLiteral("start"), QStringLiteral("--name"),
                         QStringLiteral("repl"), QStringLiteral("--clock"),
                         QStringLiteral("virtual"), QStringLiteral("--"), fixtureBinary()},
                        sessionDir)
                .code,
            0);

        // --diff prints only changed rows instead of full 24-row ruled viewport
        const auto diff1 = runLauncher({QStringLiteral("s"), QStringLiteral("press"),
                                        QStringLiteral("k"), QStringLiteral("--diff")},
                                       sessionDir);
        QCOMPARE(diff1.code, 0);
        QVERIFY(diff1.out.contains("=== Frame Diff (1 changed row(s)) ==="));
        QVERIFY(diff1.out.contains("- 03 │ STATUS: READY"));
        QVERIFY(diff1.out.contains("+ 03 │ STATUS: KEY K"));
        QVERIFY(!diff1.out.contains("=== Viewport"));

        // Pressing 'k' again produces no visual diff
        const auto diffNone = runLauncher({QStringLiteral("s"), QStringLiteral("press"),
                                           QStringLiteral("k"), QStringLiteral("--diff")},
                                          sessionDir);
        QCOMPARE(diffNone.code, 0);
        QVERIFY(diffNone.out.contains("=== Frame Diff (no visual changes) ==="));

        // click-item clicks center of probeArea by objectName
        const auto clickedItem =
            runLauncher({QStringLiteral("s"), QStringLiteral("click-item"),
                         QStringLiteral("probeArea"), QStringLiteral("--diff")},
                        sessionDir);
        QCOMPARE(clickedItem.code, 0);
        QVERIFY2(clickedItem.out.contains("MOUSE: UP"),
                 qPrintable(QString::fromUtf8(clickedItem.out)));

        // Piped batch session script with comments, quoted text, mode switch, export, and detach
        QProcess batchProc;
        batchProc.setProcessEnvironment(baseEnv(sessionDir));
        batchProc.start(launcherBinary(),
                        {QStringLiteral("s"), QStringLiteral("batch"), QStringLiteral("repl")});
        QVERIFY(batchProc.waitForStarted(5000));
        const QByteArray scriptInput = QStringLiteral("# focus TextInput and type a quoted phrase\n"
                                                      "mode off\n"
                                                      "click 5 5\n"
                                                      "type \"hi there\"\n"
                                                      "frame --no-ruler\n"
                                                      "export -o \"%1\"\n"
                                                      "detach\n")
                                           .arg(exportedPath)
                                           .toUtf8();
        QCOMPARE(batchProc.write(scriptInput), scriptInput.size());
        batchProc.closeWriteChannel();
        QVERIFY(batchProc.waitForFinished(10000));
        const QByteArray batchOut = batchProc.readAllStandardOutput();
        const QByteArray batchErr = batchProc.readAllStandardError();
        QVERIFY2(batchProc.exitCode() == 0,
                 qPrintable(QStringLiteral("batchProc exited %1; stderr:\n%2\nstdout:\n%3")
                                .arg(batchProc.exitCode())
                                .arg(QString::fromUtf8(batchErr), QString::fromUtf8(batchOut))));
        QVERIFY(batchOut.contains("INPUT: hi there"));

        // Batch stops on first recoverable error with exit code 1, leaving session alive
        QProcess failingBatch;
        failingBatch.setProcessEnvironment(baseEnv(sessionDir));
        failingBatch.start(launcherBinary(),
                           {QStringLiteral("s"), QStringLiteral("batch"), QStringLiteral("repl")});
        QVERIFY(failingBatch.waitForStarted(5000));
        failingBatch.write("click-text \"MISSING_ITEM\"\npress q\n");
        failingBatch.closeWriteChannel();
        QVERIFY(failingBatch.waitForFinished(10000));
        QVERIFY2(failingBatch.exitCode() == 1,
                 qPrintable(QStringLiteral("failingBatch exited %1; stderr:\n%2")
                                .arg(failingBatch.exitCode())
                                .arg(QString::fromUtf8(failingBatch.readAllStandardError()))));

        // Session is still alive after failed batch -> connect and stop cleanly
        QProcess connectProc;
        connectProc.setProcessEnvironment(baseEnv(sessionDir));
        connectProc.start(launcherBinary(), {QStringLiteral("s"), QStringLiteral("connect"),
                                             QStringLiteral("repl"), QStringLiteral("--no-frame")});
        QVERIFY(connectProc.waitForStarted(5000));
        connectProc.write("stats\nstop\n");
        connectProc.closeWriteChannel();
        QVERIFY(connectProc.waitForFinished(10000));
        stoppedRepl = true;
        QVERIFY2(connectProc.exitCode() == 0,
                 qPrintable(QStringLiteral("connectProc exited %1; stderr:\n%2")
                                .arg(connectProc.exitCode())
                                .arg(QString::fromUtf8(connectProc.readAllStandardError()))));

        // Verify exported script includes click-item as app.item(...).click() and replays cleanly
        QFile exportedFile(exportedPath);
        QVERIFY(exportedFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString exportedJs = QString::fromUtf8(exportedFile.readAll());
        QVERIFY(exportedJs.contains(QStringLiteral("app.item(\"probeArea\").click();")));
        QVERIFY(exportedJs.contains(QStringLiteral("app.typeText(\"hi there\");")));

        const QString replayArtifacts = temp.filePath(QStringLiteral("replay-connect"));
        const auto replay =
            runLauncher({QStringLiteral("--clock"), QStringLiteral("virtual"),
                         QStringLiteral("--script"), exportedPath, QStringLiteral("--artifacts"),
                         replayArtifacts, QStringLiteral("--"), fixtureBinary()},
                        sessionDir);
        QCOMPARE(replay.code, 0);
        QFile finalFrame(QDir(replayArtifacts).filePath(QStringLiteral("final.txt")));
        QVERIFY(finalFrame.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString finalText = QString::fromUtf8(finalFrame.readAll());
        QVERIFY(finalText.contains(QStringLiteral("MOUSE: UP")));
        QVERIFY(finalText.contains(QStringLiteral("INPUT: hi there")));
    }

    void sessionConcurrencyAndRobustness() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sessionDir = temp.filePath(QStringLiteral("sessions"));
        bool stoppedAll = false;
        const auto cleanupConc = qScopeGuard([&] {
            if (!stoppedAll) {
                (void)runLauncher(
                    {QStringLiteral("s"), QStringLiteral("stop"), QStringLiteral("--all")},
                    sessionDir);
            }
        });

        // 1. Start with --idle-timeout-ms 0 and --plain (no ruler)
        const auto started =
            runLauncher({QStringLiteral("s"), QStringLiteral("start"), QStringLiteral("--name"),
                         QStringLiteral("conc"), QStringLiteral("--clock"), QStringLiteral("real"),
                         QStringLiteral("--idle-timeout-ms"), QStringLiteral("0"),
                         QStringLiteral("--plain"), QStringLiteral("--"), fixtureBinary()},
                        sessionDir);
        QCOMPARE(started.code, 0);
        QVERIFY(started.out.contains("TESTRUNNER FIXTURE"));
        QVERIFY(!started.out.contains("0123456789"));

        // 2. Concurrent requests serialize cleanly and `list` does not stall
        QProcess sleepProc;
        sleepProc.setProcessEnvironment(baseEnv(sessionDir));
        sleepProc.start(launcherBinary(),
                        {QStringLiteral("s"), QStringLiteral("sleep"), QStringLiteral("600"),
                         QStringLiteral("--no-frame"), QStringLiteral("--json")});
        QVERIFY(sleepProc.waitForStarted(5000));
        QThread::msleep(150);

        const auto listDuringSleep = runLauncher(
            {QStringLiteral("s"), QStringLiteral("list"), QStringLiteral("--json")}, sessionDir);
        QCOMPARE(listDuringSleep.code, 0);
        QCOMPARE(parseLastJsonLine(listDuringSleep.out)
                     .value(QStringLiteral("sessions"))
                     .toArray()
                     .size(),
                 1);

        const auto pressDuringSleep =
            runLauncher({QStringLiteral("s"), QStringLiteral("press"), QStringLiteral("k"),
                         QStringLiteral("--diff"), QStringLiteral("--json")},
                        sessionDir);
        QVERIFY(sleepProc.waitForFinished(5000));
        QCOMPARE(sleepProc.exitCode(), 0);
        QCOMPARE(pressDuringSleep.code, 0);
        const QJsonObject pressObj = parseLastJsonLine(pressDuringSleep.out);
        QVERIFY2(pressObj.value(QStringLiteral("ok")).toBool(),
                 qPrintable(QString::fromUtf8(pressDuringSleep.out)));
        QVERIFY(!pressObj.value(QStringLiteral("frameDiff")).toArray().isEmpty());

        // 3. `eval app.finish()` fails cleanly without wedging subsequent commands
        const auto finishEval = runLauncher(
            {QStringLiteral("s"), QStringLiteral("eval"), QStringLiteral("app.finish()"),
             QStringLiteral("--no-frame"), QStringLiteral("--json")},
            sessionDir);
        QVERIFY2(finishEval.code == 1,
                 qPrintable(QStringLiteral("finishEval exited %1; stdout:\n%2\nstderr:\n%3")
                                .arg(finishEval.code)
                                .arg(QString::fromUtf8(finishEval.out),
                                     QString::fromUtf8(finishEval.err))));
        const QJsonObject finishObj = parseLastJsonLine(finishEval.out);
        QVERIFY(!finishObj.value(QStringLiteral("ok")).toBool());
        QVERIFY(finishObj.value(QStringLiteral("error"))
                    .toString()
                    .contains(QStringLiteral("not supported in interactive sessions")));

        // Follow-up command succeeds immediately
        const auto afterFinish = runLauncher(
            {QStringLiteral("s"), QStringLiteral("frame"), QStringLiteral("--no-ruler")},
            sessionDir);
        QVERIFY2(afterFinish.code == 0,
                 qPrintable(QStringLiteral("afterFinish exited %1; stdout:\n%2\nstderr:\n%3")
                                .arg(afterFinish.code)
                                .arg(QString::fromUtf8(afterFinish.out),
                                     QString::fromUtf8(afterFinish.err))));
        QVERIFY(afterFinish.out.contains("STATUS: KEY K"));

        // 4. Failing `--diff` falls back to full frame instead of "(no visual changes)"
        const auto failDiff =
            runLauncher({QStringLiteral("s"), QStringLiteral("click-text"),
                         QStringLiteral("NO_SUCH_BUTTON"), QStringLiteral("--diff")},
                        sessionDir);
        QCOMPARE(failDiff.code, 1);
        QVERIFY(failDiff.out.contains("Error:"));
        QVERIFY(failDiff.out.contains("=== Viewport"));
        QVERIFY(failDiff.out.contains("STATUS: KEY K"));
        QVERIFY(!failDiff.out.contains("Frame Diff (no visual changes)"));

        // 5. `frame --diff` reports changes since last seen frame (after --no-frame)
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("click"), QStringLiteral("5"),
                              QStringLiteral("5"), QStringLiteral("--no-frame")},
                             sessionDir)
                     .code,
                 0);
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("type"), QStringLiteral("abc"),
                              QStringLiteral("--no-frame")},
                             sessionDir)
                     .code,
                 0);
        const auto frameDiff = runLauncher(
            {QStringLiteral("s"), QStringLiteral("frame"), QStringLiteral("--diff")}, sessionDir);
        QCOMPARE(frameDiff.code, 0);
        QVERIFY2(frameDiff.out.contains("INPUT: abc"),
                 qPrintable(QString::fromUtf8(frameDiff.out)));

        // 6. `tree --json` does not duplicate tree in `result`, and text tree shows focusPolicy
        const auto treeJson = runLauncher(
            {QStringLiteral("s"), QStringLiteral("tree"), QStringLiteral("--json")}, sessionDir);
        QCOMPARE(treeJson.code, 0);
        const QJsonObject treeReply = parseLastJsonLine(treeJson.out);
        QVERIFY(treeReply.contains(QStringLiteral("tree")));
        QVERIFY(!treeReply.contains(QStringLiteral("result")));
        const auto treeText =
            runLauncher({QStringLiteral("s"), QStringLiteral("tree")}, sessionDir);
        QCOMPARE(treeText.code, 0);
        QVERIFY(treeText.out.contains("focusPolicy="));

        // 7. `--json` is honored on CLI usage / invalid-option errors
        const auto badArityJson = runLauncher({QStringLiteral("s"), QStringLiteral("click"),
                                               QStringLiteral("1"), QStringLiteral("--json")},
                                              sessionDir);
        QCOMPARE(badArityJson.code, 2);
        const QJsonObject badArityObj = parseLastJsonLine(badArityJson.out);
        QVERIFY(!badArityObj.isEmpty());
        QCOMPARE(badArityObj.value(QStringLiteral("ok")).toBool(true), false);

        // Irrelevant option rejected cleanly for the verb
        const auto badFlag = runLauncher({QStringLiteral("s"), QStringLiteral("press"),
                                          QStringLiteral("k"), QStringLiteral("--button"),
                                          QStringLiteral("right"), QStringLiteral("--json")},
                                         sessionDir);
        QCOMPARE(badFlag.code, 2);
        QVERIFY(parseLastJsonLine(badFlag.out)
                    .value(QStringLiteral("error"))
                    .toString()
                    .contains(QStringLiteral("--button")));

        // 8. Multi-token eval + --no-record + export header & semicolons
        const auto multiEval = runLauncher(
            {QStringLiteral("s"), QStringLiteral("eval"), QStringLiteral("app.size().columns"),
             QStringLiteral("+"), QStringLiteral("1"), QStringLiteral("--no-record"),
             QStringLiteral("--no-frame"), QStringLiteral("--json")},
            sessionDir);
        QCOMPARE(multiEval.code, 0);
        QCOMPARE(parseLastJsonLine(multiEval.out).value(QStringLiteral("result")).toInt(), 81);

        const auto exported =
            runLauncher({QStringLiteral("s"), QStringLiteral("export")}, sessionDir);
        QCOMPARE(exported.code, 0);
        QVERIFY(
            exported.out.contains("// Recorded from qmltui-runner session (80x24, clock=real)"));
        QVERIFY(!exported.out.contains("app.size().columns"));

        // 9. `batch` rejects invalid `mode` with exit code 2
        QProcess badModeBatch;
        badModeBatch.setProcessEnvironment(baseEnv(sessionDir));
        badModeBatch.start(launcherBinary(),
                           {QStringLiteral("s"), QStringLiteral("batch"), QStringLiteral("conc")});
        QVERIFY(badModeBatch.waitForStarted(5000));
        badModeBatch.write("mode bogus\nstats\n");
        badModeBatch.closeWriteChannel();
        QVERIFY(badModeBatch.waitForFinished(10000));
        QCOMPARE(badModeBatch.exitCode(), 2);

        // 10. `connect` `stop --export` failure reports error without claiming stop, then stops
        // cleanly
        const QString blockerFile = temp.filePath(QStringLiteral("blocker"));
        QFile blocker(blockerFile);
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.write("x");
        blocker.close();
        const QString badExportPath = blockerFile + QStringLiteral("/nested/out.js");

        QProcess connectStop;
        connectStop.setProcessEnvironment(baseEnv(sessionDir));
        connectStop.start(launcherBinary(), {QStringLiteral("s"), QStringLiteral("connect"),
                                             QStringLiteral("conc"), QStringLiteral("--no-frame")});
        QVERIFY(connectStop.waitForStarted(5000));
        connectStop.write(
            QStringLiteral("stop --export \"%1\"\nstop\n").arg(badExportPath).toUtf8());
        connectStop.closeWriteChannel();
        QVERIFY(connectStop.waitForFinished(10000));
        QCOMPARE(connectStop.exitCode(), 0);
        const QByteArray connOut = connectStop.readAllStandardOutput();
        const QByteArray connErr = connectStop.readAllStandardError();
        QVERIFY2(connErr.contains("Error:"), qPrintable(QString::fromUtf8(connErr)));
        QCOMPARE(connOut.count("Stopped session conc"), 1);

        // 11. Inline `scrollback` prints once (no duplicate `Result:`) and supports
        // expect-scrollback
        QCOMPARE(
            runLauncher({QStringLiteral("s"), QStringLiteral("start"), QStringLiteral("--name"),
                         QStringLiteral("in2"), QStringLiteral("--clock"),
                         QStringLiteral("virtual"), QStringLiteral("--columns"),
                         QStringLiteral("80"), QStringLiteral("--rows"), QStringLiteral("12"),
                         QStringLiteral("--"), fixtureBinary(), QStringLiteral("--fixture=inline")},
                        sessionDir)
                .code,
            0);
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("press"), QStringLiteral("F1"),
                              QStringLiteral("--no-frame")},
                             sessionDir)
                     .code,
                 0);
        const auto sbText =
            runLauncher({QStringLiteral("s"), QStringLiteral("scrollback")}, sessionDir);
        QCOMPARE(sbText.code, 0);
        QVERIFY(!sbText.out.contains("Result:"));
        QCOMPARE(sbText.out.count("MODEL ROW 0"), 1);
        const auto sbJson = runLauncher(
            {QStringLiteral("s"), QStringLiteral("scrollback"), QStringLiteral("--json")},
            sessionDir);
        QCOMPARE(sbJson.code, 0);
        const QJsonObject sbObj = parseLastJsonLine(sbJson.out);
        QVERIFY(sbObj.contains(QStringLiteral("scrollback")));
        QVERIFY(!sbObj.contains(QStringLiteral("result")));
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("expect-scrollback"),
                              QStringLiteral("MODEL ROW 0"), QStringLiteral("--no-frame")},
                             sessionDir)
                     .code,
                 0);
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("expect-no-scrollback"),
                              QStringLiteral("MODEL ROW 0"), QStringLiteral("--no-frame")},
                             sessionDir)
                     .code,
                 1);
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("expect-no-scrollback"),
                              QStringLiteral("NOT_IN_SCROLLBACK"), QStringLiteral("--no-frame")},
                             sessionDir)
                     .code,
                 0);
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("stop"),
                              QStringLiteral("--session"), QStringLiteral("in2")},
                             sessionDir)
                     .code,
                 0);

        // 12. Self-quit with --record writes result.json & final.txt so --clean-artifacts works
        const QString selfQuitArt = temp.filePath(QStringLiteral("selfquit-art"));
        QCOMPARE(
            runLauncher({QStringLiteral("s"), QStringLiteral("start"), QStringLiteral("--name"),
                         QStringLiteral("sq"), QStringLiteral("--clock"), QStringLiteral("virtual"),
                         QStringLiteral("--artifacts"), selfQuitArt, QStringLiteral("--record"),
                         QStringLiteral("sq.cast"), QStringLiteral("--"), fixtureBinary()},
                        sessionDir)
                .code,
            0);
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("press"), QStringLiteral("q"),
                              QStringLiteral("--no-frame")},
                             sessionDir)
                     .code,
                 0);
        QVERIFY(QFileInfo::exists(QDir(selfQuitArt).filePath(QStringLiteral("sq.cast"))));
        QVERIFY(QFileInfo::exists(QDir(selfQuitArt).filePath(QStringLiteral("final.txt"))));
        QVERIFY(QFileInfo::exists(QDir(selfQuitArt).filePath(QStringLiteral("result.json"))));
        // Reusing the same directory with --clean-artifacts must succeed
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("start"),
                              QStringLiteral("--name"), QStringLiteral("sq2"),
                              QStringLiteral("--clock"), QStringLiteral("virtual"),
                              QStringLiteral("--clean-artifacts"), QStringLiteral("--artifacts"),
                              selfQuitArt, QStringLiteral("--"), fixtureBinary()},
                             sessionDir)
                     .code,
                 0);
        stoppedAll = true;
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("stop"), QStringLiteral("--all")},
                             sessionDir)
                     .code,
                 0);
    }

    void sessionStartClosesStdoutPipeOnExit() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sessionDir = temp.filePath(QStringLiteral("sessions"));

        // Pipe `session start` stdout into `cmake -E cat -`, which exits only
        // once every writer to the pipe closes it. If the detached daemon
        // inherited stdout, `catProc` would hang for the daemon's lifetime.
        QProcess startProc;
        QProcess catProc;
        bool stoppedDaemon = false;
        const auto cleanup = qScopeGuard([&] {
            if (startProc.state() != QProcess::NotRunning) {
                startProc.kill();
                startProc.waitForFinished(2000);
            }
            if (catProc.state() != QProcess::NotRunning) {
                catProc.kill();
                catProc.waitForFinished(2000);
            }
            if (!stoppedDaemon) {
                (void)runLauncher({QStringLiteral("s"), QStringLiteral("stop"),
                                   QStringLiteral("--session"), QStringLiteral("pipeeof")},
                                  sessionDir);
            }
        });

        startProc.setProcessEnvironment(baseEnv(sessionDir));
        startProc.setStandardOutputProcess(&catProc);

        QByteArray pipedOut;
        QObject::connect(&catProc, &QProcess::readyReadStandardOutput, &catProc,
                         [&] { pipedOut.append(catProc.readAllStandardOutput()); });

        // Start the upstream writer before the downstream reader so Qt closes
        // the parent's copy of the pipe write end before spawning `catProc`
        // (on Windows, starting the sink first lets `catProc` inherit its own
        // stdin write end).
        startProc.start(launcherBinary(),
                        {QStringLiteral("s"), QStringLiteral("start"), QStringLiteral("--name"),
                         QStringLiteral("pipeeof"), QStringLiteral("--clock"),
                         QStringLiteral("virtual"), QStringLiteral("--"), fixtureBinary()});
        QVERIFY2(startProc.waitForStarted(5000), qPrintable(startProc.errorString()));

        catProc.start(QStringLiteral(QMLTUI_CMAKE_COMMAND),
                      {QStringLiteral("-E"), QStringLiteral("cat"), QStringLiteral("-")});
        QVERIFY2(catProc.waitForStarted(5000), qPrintable(catProc.errorString()));

        QElapsedTimer startTimer;
        startTimer.start();
        while (startProc.state() != QProcess::NotRunning && startTimer.elapsed() < 10000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
            QThread::msleep(10);
        }
        QVERIFY2(startProc.state() == QProcess::NotRunning, qPrintable(startProc.errorString()));
        QCOMPARE(startProc.exitStatus(), QProcess::NormalExit);
        QCOMPARE(startProc.exitCode(), 0);
        if (catProc.state() != QProcess::NotRunning) {
            QVERIFY2(
                catProc.waitForFinished(3000),
                "Detached session daemon kept caller stdout pipe open after session start exited");
        }
        QCOMPARE(catProc.exitStatus(), QProcess::NormalExit);
        QCOMPARE(catProc.exitCode(), 0);
        pipedOut.append(catProc.readAllStandardOutput());
        QVERIFY2(pipedOut.contains("Session: pipeeof"), qPrintable(QString::fromUtf8(pipedOut)));
        QVERIFY(pipedOut.contains("TESTRUNNER FIXTURE"));

        stoppedDaemon = true;
        QCOMPARE(runLauncher({QStringLiteral("s"), QStringLiteral("stop"),
                              QStringLiteral("--session"), QStringLiteral("pipeeof")},
                             sessionDir)
                     .code,
                 0);
    }
#endif

    void rawInteractivePipelinedStdin() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString artifacts = temp.filePath(QStringLiteral("raw-pipelined"));

        QProcess child;
        QProcessEnvironment env = baseEnv({});
        env.insert(QStringLiteral("QMLTUI_RUNNER_INTERACTIVE"), QStringLiteral("1"));
        env.insert(QStringLiteral("QMLTUI_RUNNER_CLOCK"), QStringLiteral("real"));
        env.insert(QStringLiteral("QMLTUI_RUNNER_ARTIFACT_DIR"), artifacts);
        child.setProcessEnvironment(env);
        child.start(fixtureBinary(), {});
        QVERIFY2(child.waitForStarted(5000), qPrintable(child.errorString()));

        QByteArray buf;
        QJsonObject ready;
        QVERIFY(readJsonLineFromProcess(child, buf, &ready, 5000));
        QCOMPARE(ready.value(QStringLiteral("event")).toString(), QStringLiteral("ready"));

        // Write three commands back-to-back without waiting for replies so the
        // second arrives while the first is blocked in sleep()'s event loop.
        const QByteArray pipelined = QByteArrayLiteral(
            "{\"id\":1,\"cmd\":\"sleep\",\"args\":{\"durationMs\":250},\"include\":{\"frame\":"
            "false}}\n"
            "{\"id\":2,\"cmd\":\"press\",\"args\":{\"key\":\"k\"},\"include\":{\"frame\":false}}\n"
            "{\"id\":3,\"cmd\":\"close\"}\n");
        QCOMPARE(child.write(pipelined), pipelined.size());
        child.closeWriteChannel();

        QJsonObject r1;
        QJsonObject r2;
        QJsonObject r3;
        QVERIFY(readJsonLineFromProcess(child, buf, &r1, 5000));
        QVERIFY(readJsonLineFromProcess(child, buf, &r2, 5000));
        QVERIFY(readJsonLineFromProcess(child, buf, &r3, 5000));
        QCOMPARE(r1.value(QStringLiteral("id")).toInt(), 1);
        QVERIFY2(r1.value(QStringLiteral("ok")).toBool(),
                 qPrintable(QString::fromUtf8(QJsonDocument(r1).toJson())));
        QCOMPARE(r2.value(QStringLiteral("id")).toInt(), 2);
        QVERIFY2(r2.value(QStringLiteral("ok")).toBool(),
                 qPrintable(QString::fromUtf8(QJsonDocument(r2).toJson())));
        QCOMPARE(r3.value(QStringLiteral("id")).toInt(), 3);
        QVERIFY(r3.value(QStringLiteral("ok")).toBool());
        QVERIFY(r3.value(QStringLiteral("closed")).toBool());
        QVERIFY(
            r3.value(QStringLiteral("frame")).toString().contains(QStringLiteral("STATUS: KEY K")));

        QVERIFY(child.waitForFinished(5000));
        QCOMPARE(child.exitCode(), 0);
    }
};

QTEST_GUILESS_MAIN(RunnerSessionTest)

#include "test_runner_session.moc"
