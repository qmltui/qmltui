#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <cstdlib>

namespace {
bool writeFile(const QString& path, const QByteArray& data) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}
QByteArray readFile(const QString& path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

// A protocol peer without Runner for lifecycle failures a real scenario cannot
// induce reliably (including malformed results and unresponsive children).
int peer(const QStringList& args) {
    const QString behavior = args.value(2);
    if (behavior == "unsupported") return 0;
    if (behavior == "startup-hang") {
        QThread::sleep(10);
        return 0;
    }
    QSaveFile ack(qEnvironmentVariable("QMLTUI_RUNNER_HANDSHAKE_PATH"));
    const QJsonObject payload{
        {"protocolVersion", behavior == "bad-version" ? 2 : 1},
        {"token",
         behavior == "bad-token" ? "wrong" : qEnvironmentVariable("QMLTUI_RUNNER_HANDSHAKE_TOKEN")},
        {"pid", behavior == "bad-pid" ? qint64(-1) : QCoreApplication::applicationPid()}};
    if (!ack.open(QIODevice::WriteOnly)) return 99;
    ack.write(QJsonDocument(payload).toJson());
    if (!ack.commit()) return 99;
    if (behavior == "hang") {
        QThread::sleep(20);
        return 0;
    }
    if (behavior == "crash") std::abort();
    const QDir artifactDir(qEnvironmentVariable("QMLTUI_RUNNER_ARTIFACT_DIR"));
    const QString result = artifactDir.filePath("result.json");
    if (behavior == "crlf-artifacts") {
        writeFile(artifactDir.filePath("final.txt"), "CRLF ROW\r\n");
        writeFile(artifactDir.filePath("final_styled.txt"),
                  "CRLF ROW\r\n--- styles\r\ny=0 x=0-7 fg=#7FDBCAFF bg=default\r\n");
        writeFile(artifactDir.filePath("scrollback.txt"), "CRLF HISTORY\r\n");
        writeFile(artifactDir.filePath("scrollback_styled.txt"),
                  "CRLF HISTORY\r\n--- styles\r\ny=0 x=0-11 fg=#F1FA8CFF bg=default bold\r\n");
        writeFile(result, QJsonDocument(
                              QJsonObject{{"schemaVersion", 1},
                                          {"mode", "eval"},
                                          {"status", "passed"},
                                          {"exitCode", 0},
                                          {"artifacts", QJsonObject{{"finalFrame", "final.txt"}}}})
                              .toJson());
        return 0;
    }
    if (behavior == "missing") return 0;
    if (behavior == "malformed") {
        writeFile(result, "not json");
        return 0;
    }
    bool valid = true;
    if (behavior == "arguments") {
        valid =
            args.mid(3) == QStringList{"--script", "literal space", "--help", "$value;unchanged"} &&
            !qEnvironmentVariableIsSet("QMLTUI_RUNNER_UNEXPECTED") &&
            qEnvironmentVariable("QMLTUI_RUNNER_COLUMNS") == "80" &&
            qEnvironmentVariable("QMLTUI_RUNNER_ROWS") == "24" &&
            qEnvironmentVariable("QMLTUI_RUNNER_CLOCK") == "real" &&
            qEnvironmentVariable("QMLTUI_RUNNER_UPDATE_SNAPSHOTS") == "0" &&
            qEnvironmentVariable("QMLTUI_TEST_INHERITED") == "preserved";
    } else if (behavior == "live-size") {
        valid = !qEnvironmentVariableIsSet("QMLTUI_RUNNER_COLUMNS") &&
                !qEnvironmentVariableIsSet("QMLTUI_RUNNER_ROWS") &&
                qEnvironmentVariable("QMLTUI_RUNNER_LIVE") == "1";
    }
    const int code = valid ? 0 : 1;
    writeFile(result,
              QJsonDocument(QJsonObject{{"schemaVersion", 1},
                                        {"mode", behavior == "bad-mode" ? "script" : "eval"},
                                        {"status", code == 0 ? "passed" : "failed"},
                                        {"exitCode", behavior == "inconsistent" ? 1 : code}})
                  .toJson());
    return code;
}
QString launcherBinary() {
    return qEnvironmentVariable("QMLTUI_TEST_LAUNCHER",
                                QStringLiteral(QMLTUI_DEFAULT_TEST_LAUNCHER));
}

QString fixtureBinary() {
    return qEnvironmentVariable("QMLTUI_TEST_FIXTURE", QStringLiteral(QMLTUI_DEFAULT_TEST_FIXTURE));
}

QProcessEnvironment withDefaultImportPath(QProcessEnvironment env) {
    const QString importPath = QStringLiteral(QMLTUI_DEFAULT_QML_IMPORT_PATH);
    if (!env.contains("QML_IMPORT_PATH")) env.insert("QML_IMPORT_PATH", importPath);
    if (!env.contains("QML2_IMPORT_PATH")) env.insert("QML2_IMPORT_PATH", importPath);
    return env;
}
} // namespace

class LauncherTest : public QObject {
    Q_OBJECT
    struct Result {
        int code;
        QByteArray out;
        QByteArray err;
    };
    Result run(QStringList args,
               QProcessEnvironment env = QProcessEnvironment::systemEnvironment()) {
        QProcess process;
        process.setProcessEnvironment(withDefaultImportPath(env));
        process.start(launcherBinary(), args);
        if (!process.waitForStarted(5000)) return {-100, {}, process.errorString().toUtf8()};
        if (!process.waitForFinished(20000)) {
            process.kill();
            process.waitForFinished();
            return {-101, {}, "Launcher did not finish"};
        }
        return {process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -102,
                process.readAllStandardOutput(), process.readAllStandardError()};
    }
private slots:
    void invalid_data() {
        QTest::addColumn<QStringList>("options");
        QTest::newRow("missing-mode") << QStringList{};
        QTest::newRow("conflicting-modes") << QStringList{"--eval", "x", "--capture-after-ms", "0"};
        QTest::newRow("columns") << QStringList{"--eval", "x", "--columns", "0"};
        QTest::newRow("clock") << QStringList{"--eval", "x", "--clock", "other"};
        QTest::newRow("capture-budget")
            << QStringList{"--capture-after-ms", "100", "--timeout-ms", "100"};
        QTest::newRow("record-live") << QStringList{"--eval", "x", "--record", "x.cast", "--live"};
        QTest::newRow("record-traversal") << QStringList{"--eval", "x", "--record", "../x.cast"};
        QTest::newRow("empty-eval") << QStringList{"--eval", " "};
        QTest::newRow("missing-script") << QStringList{"--script", "/does-not-exist/test.js"};
        QTest::newRow("bad-sizes") << QStringList{"--sizes", "80x0"};
        QTest::newRow("live-ruler") << QStringList{"--eval", "x", "--live", "--ruler"};
        QTest::newRow("script-sweep")
            << QStringList{"--script", QStringLiteral(QMLTUI_SCENARIOS) + "/success.js", "--sweep"};
    }
    void invalid() {
        QFETCH(QStringList, options);
        options << "--" << QCoreApplication::applicationFilePath() << "--peer" << "unsupported";
        QCOMPARE(run(options).code, 2);
    }
    void protectionAndArguments() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString sentinel = temporary.filePath("keep.txt");
        QVERIFY(writeFile(sentinel, "keep"));
        QCOMPARE(run({"--eval", "x", "--artifacts", temporary.path(), "--", "unused"}).code, 2);
        QCOMPARE(readFile(sentinel), QByteArray("keep"));
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("QMLTUI_RUNNER_UNEXPECTED", "remove");
        environment.insert("QMLTUI_RUNNER_CAPTURE_AFTER_MS", "0");
        environment.insert("QMLTUI_RUNNER_UPDATE_SNAPSHOTS", "1");
        environment.insert("QMLTUI_RUNNER_CLOCK", "virtual");
        environment.insert("QMLTUI_TEST_INHERITED", "preserved");
        const auto result = run({"--eval=x", "--artifacts", temporary.filePath("new directory"),
                                 QCoreApplication::applicationFilePath(), "--peer", "arguments",
                                 "--script", "literal space", "--help", "$value;unchanged"},
                                environment);
        QVERIFY2(result.code == 0, result.err.constData());
        const auto liveResult =
            run({"--eval=x", "--live", "--artifacts", temporary.filePath("live directory"),
                 QCoreApplication::applicationFilePath(), "--peer", "live-size"},
                environment);
        QVERIFY2(liveResult.code == 0, liveResult.err.constData());
    }
    void lifecycle_data() {
        QTest::addColumn<QString>("behavior");
        QTest::addColumn<int>("expected");
        for (const auto& name :
             {"unsupported", "startup-hang", "bad-token", "bad-version", "bad-pid", "bad-mode",
              "missing", "malformed", "inconsistent", "crash", "hang"})
            QTest::newRow(name) << QString::fromLatin1(name) << 2;
        QTest::newRow("fast-success") << QString("success") << 0;
    }
    void lifecycle() {
        QFETCH(QString, behavior);
        QFETCH(int, expected);
        QTemporaryDir temporary;
        const auto result = run({"--eval", "x", "--artifacts", temporary.filePath("results"),
                                 "--startup-timeout-ms", "500", "--timeout-ms", "1", "--",
                                 QCoreApplication::applicationFilePath(), "--peer", behavior});
        QVERIFY2(result.code == expected, result.err.constData());
    }
    void realModes_data() {
        QTest::addColumn<QStringList>("options");
        QTest::addColumn<int>("expected");
        QTest::addColumn<QString>("artifact");
        QTest::newRow("eval") << QStringList{"--eval", "app.expectText('TESTRUNNER FIXTURE')"} << 0
                              << QString("final.txt");
        QTest::newRow("viewport") << QStringList{"--eval",    "app.expectText('SIZE: 100x30')",
                                                 "--columns", "100",
                                                 "--rows",    "30"}
                                  << 0 << QString("final.txt");
        QTest::newRow("capture-zero")
            << QStringList{"--capture-after-ms=0"} << 0 << QString("capture_styled.txt");
        QTest::newRow("capture-delayed")
            << QStringList{"--capture-after-ms", "100"} << 0 << QString("capture.txt");
        QTest::newRow("live") << QStringList{"--eval", "app.expectText('TESTRUNNER FIXTURE')",
                                             "--live"}
                              << 0 << QString("final.txt");
        QTest::newRow("virtual-record")
            << QStringList{"--eval",   "app.advanceTime(100); app.expectText('TESTRUNNER FIXTURE')",
                           "--clock",  "virtual",
                           "--record", "tour.cast"}
            << 0 << QString("tour.cast");
        QTest::newRow("failure") << QStringList{"--eval", "app.fail('intentional')"} << 1
                                 << QString("failure.txt");
        QTest::newRow("module") << QStringList{"--script", QStringLiteral(QMLTUI_SCENARIOS) +
                                                               "/module_main.mjs"}
                                << 0 << QString("module_main.txt");
        QTest::newRow("resource") << QStringList{"--script", ":/qmltui-tests/module_main.mjs"} << 0
                                  << QString("module_main.txt");
    }
    void realModes() {
        QFETCH(QStringList, options);
        QFETCH(int, expected);
        QFETCH(QString, artifact);
        QTemporaryDir temporary;
        const QString directory = temporary.filePath("artifacts");
        options << "--artifacts" << directory << "--" << fixtureBinary();
        const auto result = run(options);
        QVERIFY2(result.code == expected, result.err.constData());
        const QByteArray bytes = readFile(QDir(directory).filePath(artifact));
        QVERIFY(!bytes.isEmpty());
        if (artifact != "tour.cast")
            QVERIFY(bytes.contains("TESTRUNNER FIXTURE"));
        else
            QVERIFY(bytes.contains("\"version\":2"));
        if (options.contains("--live"))
            QVERIFY(result.out.contains('\x1b'));
        else
            QVERIFY(!result.out.contains('\x1b'));
    }
    void launchAndHandshakeFailures() {
        QTemporaryDir temporary;
        QCOMPARE(run({"--eval", "x", "--artifacts", temporary.filePath("missing-app"), "--",
                      temporary.filePath("no-executable")})
                     .code,
                 2);
        QProcess child;
        auto env = withDefaultImportPath(QProcessEnvironment::systemEnvironment());
        for (const auto& key : env.keys()) {
            if (key.startsWith("QMLTUI_RUNNER_")) env.remove(key);
        }
        env.insert("QMLTUI_RUNNER_EVAL", "app.expectText('TESTRUNNER FIXTURE')");
        env.insert("QMLTUI_RUNNER_ARTIFACT_DIR", temporary.filePath("handshake-failure"));
        env.insert("QMLTUI_RUNNER_HANDSHAKE_PATH", temporary.filePath("absent/ack.json"));
        env.insert("QMLTUI_RUNNER_HANDSHAKE_TOKEN", "test-token");
        child.setProcessEnvironment(env);
        child.start(fixtureBinary(), {});
        QVERIFY(child.waitForStarted());
        QVERIFY(child.waitForFinished(5000));
        QCOMPARE(child.exitCode(), 1);
        QVERIFY(readFile(temporary.filePath("handshake-failure/result.json"))
                    .contains("Cannot write Runner activation handshake"));
    }

    void scriptAndSnapshots() {
        QTemporaryDir temporary;
        const QString script = temporary.filePath("a script.js");
        QVERIFY(writeFile(script, "function main(t) { t.expectText('TESTRUNNER FIXTURE'); "
                                  "t.expectSnapshot('baseline.txt'); }"));
        auto result = run({"--script=" + script, "--update-snapshots", "--artifacts",
                           temporary.filePath("recorded"), "--", fixtureBinary()});
        QVERIFY2(result.code == 0, result.err.constData());
        QVERIFY(readFile(temporary.filePath("baseline.txt")).contains("TESTRUNNER FIXTURE"));
        result = run({"--script", script, "--ruler", "--styled", "--artifacts",
                      temporary.filePath("verified"), "--", fixtureBinary()});
        QVERIFY2(result.code == 0, result.err.constData());
        QVERIFY(result.out.contains("--- Styles ---"));
        QVERIFY(result.out.contains("fg=#"));

        const auto crlf =
            run({"--eval=x", "--ruler", "--styled", "--scrollback", "--artifacts",
                 temporary.filePath("crlf"), "--", QCoreApplication::applicationFilePath(),
                 "--peer", "crlf-artifacts"});
        QVERIFY2(crlf.code == 0, crlf.err.constData());
        const QString normalizedCrlfOut =
            QString::fromUtf8(crlf.out).replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
        QVERIFY(!normalizedCrlfOut.contains(QLatin1Char('\r')));
        QVERIFY(normalizedCrlfOut.contains(QStringLiteral("00 \u2502CRLF ROW\n")));
        QVERIFY(normalizedCrlfOut.contains(QStringLiteral("fg=#7FDBCAFF")));
        QVERIFY(normalizedCrlfOut.contains(
            QStringLiteral("--- Scrollback Styles ---\ny=0 x=0-11 fg=#F1FA8CFF bg=default bold")));
    }

    void probeSweepAndCleanArtifacts() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());

        const QString protectedDir = temporary.filePath("non-runner");
        QVERIFY(QDir().mkpath(protectedDir));
        QVERIFY(writeFile(QDir(protectedDir).filePath("keep.txt"), "keep"));
        QCOMPARE(run({"--sweep", "--clean-artifacts", "--artifacts", protectedDir, "--",
                      fixtureBinary()})
                     .code,
                 2);
        QCOMPARE(readFile(QDir(protectedDir).filePath("keep.txt")), QByteArray("keep"));

        const QString probeDir = temporary.filePath("probe");
        auto first = run({"--clock", "virtual", "--sweep", "--idle-ms", "50", "--ruler", "--styled",
                          "--artifacts", probeDir, "--", fixtureBinary()});
        QVERIFY2(first.code == 0, first.err.constData());
        QVERIFY(first.out.contains("FrameStats: frames="));
        QVERIFY(first.out.contains("=== Viewport 60x18 ==="));
        QVERIFY(first.out.contains("=== Viewport 80x24 ==="));
        QVERIFY(first.out.contains("=== Viewport 120x36 ==="));
        QVERIFY(first.out.contains("--- Styles (60x18) ---"));
        QVERIFY(first.out.contains("fg=#"));
        QVERIFY(first.out.contains("00 \xe2\x94\x82"));

        auto second = run({"--clock", "virtual", "--sizes", "70x14,90x20", "--ruler",
                           "--scrollback", "--clean-artifacts", "--artifacts", probeDir, "--eval",
                           "app.typeText('ping'); app.press('Enter');", "--", fixtureBinary(),
                           "--fixture=inline"});
        QVERIFY2(second.code == 0, second.err.constData());
        QVERIFY(second.out.contains("=== Viewport 70x14 ==="));
        QVERIFY(second.out.contains("=== Viewport 90x20 ==="));
        QVERIFY(second.out.contains("=== Inline Scrollback ==="));
        QVERIFY(second.out.contains("ping"));

        // --eval --styled does not synthesize duplicate captures and allows app.finish().
        const QString evalStyledDir = temporary.filePath("eval-styled");
        auto evalStyled = run({"--clock", "virtual", "--ruler", "--styled", "--artifacts",
                               evalStyledDir, "--eval",
                               "function main(app) { app.expectText('TESTRUNNER FIXTURE'); "
                               "app.finish(); }",
                               "--", fixtureBinary()});
        QVERIFY2(evalStyled.code == 0, evalStyled.err.constData());
        QVERIFY(evalStyled.out.contains("--- Styles ---"));
        QVERIFY(!QFileInfo::exists(QDir(evalStyledDir).filePath("capture.txt")));

        // Synthesized probe eval supports const main and Promise-returning main(app).
        const QString asyncProbeDir = temporary.filePath("async-probe");
        auto asyncProbe = run({"--clock", "virtual", "--sizes", "50x12", "--idle-ms", "20",
                               "--ruler", "--artifacts", asyncProbeDir, "--eval",
                               "const main = (app) => Promise.resolve().then(() => {"
                               "  app.expectText('TESTRUNNER FIXTURE');"
                               "});",
                               "--", fixtureBinary()});
        QVERIFY2(asyncProbe.code == 0, asyncProbe.err.constData());
        QVERIFY(asyncProbe.out.contains("=== Viewport 50x12 ==="));

        // Mid-sweep failure ruler uses finalViewport dimensions rather than sizes.last().
        const QString failSweepDir = temporary.filePath("fail-sweep");
        auto failSweep = run({"--clock", "virtual", "--sizes", "45x10,90x24", "--ruler",
                              "--artifacts", failSweepDir, "--eval",
                              "app.expectText('NEVER_MATCHES_SENTINEL');", "--", fixtureBinary()});
        QCOMPARE(failSweep.code, 1);
        QVERIFY(failSweep.out.contains("=== Failure Frame (failure.txt) ==="));
        QVERIFY(failSweep.out.contains("012345678901234567890123456789012345678901234\n"));
        QVERIFY(!failSweep.out.contains("01234567890123456789012345678901234567890123456789"));

        // Inline mode --record suppresses CSI 6 n while keeping item/region clickText() working.
        const QString inlineRecDir = temporary.filePath("inline-rec");
        auto inlineRec = run({"--clock", "virtual", "--columns", "80", "--rows", "24",
                              "--artifacts", inlineRecDir, "--record", "inline.cast", "--script",
                              QStringLiteral(QMLTUI_SCENARIOS "/inline_locators.js"), "--",
                              fixtureBinary(), "--fixture=inline"});
        QVERIFY2(inlineRec.code == 0, inlineRec.err.constData());
        const QByteArray castBytes = readFile(QDir(inlineRecDir).filePath("inline.cast"));
        QVERIFY(castBytes.contains("INLINE CLICKED 2"));
        QVERIFY(!castBytes.contains("\x1b[6n"));
        QVERIFY(!castBytes.contains("\\u001b[6n"));

        // Plain --eval invokes const/let/var main and awaits Promises (never silent exit 0).
        const QString plainConstDir = temporary.filePath("plain-const-main");
        auto plainConst =
            run({"--clock", "virtual", "--artifacts", plainConstDir, "--eval",
                 "const main = (app) => { app.fail('const_main_ran'); };", "--", fixtureBinary()});
        QCOMPARE(plainConst.code, 1);
        QVERIFY(readFile(QDir(plainConstDir).filePath("result.json")).contains("const_main_ran"));

        const QString plainVarDir = temporary.filePath("plain-var-main");
        auto plainVar =
            run({"--clock", "virtual", "--artifacts", plainVarDir, "--eval",
                 "var main = (app) => { app.fail('var_main_ran'); };", "--", fixtureBinary()});
        QCOMPARE(plainVar.code, 1);
        QVERIFY(readFile(QDir(plainVarDir).filePath("result.json")).contains("var_main_ran"));

        const QString plainAsyncDir = temporary.filePath("plain-async-main");
        auto plainAsync = run({"--clock", "virtual", "--artifacts", plainAsyncDir, "--eval",
                               "let main = (app) => Promise.resolve().then(() => {"
                               "  app.expectText('TESTRUNNER FIXTURE');"
                               "});",
                               "--", fixtureBinary()});
        QVERIFY2(plainAsync.code == 0, plainAsync.err.constData());

        const QString plainLiteralDir = temporary.filePath("plain-main-string");
        auto plainLiteral = run({"--clock", "virtual", "--artifacts", plainLiteralDir, "--eval",
                                 "const marker = 'main()'; app.expectText('TESTRUNNER FIXTURE');",
                                 "--", fixtureBinary()});
        QVERIFY2(plainLiteral.code == 0, plainLiteral.err.constData());

        // Missing mode error mentions --scrollback.
        auto missingMode = run({"--", fixtureBinary()});
        QCOMPARE(missingMode.code, 2);
        QVERIFY(missingMode.err.contains("--scrollback"));
    }
};

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().value(1) == "--peer") return peer(app.arguments());
    LauncherTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_runner_launcher.moc"
