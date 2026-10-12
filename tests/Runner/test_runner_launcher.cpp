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
    QSaveFile ack(qEnvironmentVariable("QAMUI_RUNNER_HANDSHAKE_PATH"));
    const QJsonObject payload{
        {"protocolVersion", behavior == "bad-version" ? 2 : 1},
        {"token",
         behavior == "bad-token" ? "wrong" : qEnvironmentVariable("QAMUI_RUNNER_HANDSHAKE_TOKEN")},
        {"pid", behavior == "bad-pid" ? qint64(-1) : QCoreApplication::applicationPid()}};
    if (!ack.open(QIODevice::WriteOnly)) return 99;
    ack.write(QJsonDocument(payload).toJson());
    if (!ack.commit()) return 99;
    if (behavior == "hang") {
        QThread::sleep(20);
        return 0;
    }
    if (behavior == "crash") std::abort();
    const QString result =
        QDir(qEnvironmentVariable("QAMUI_RUNNER_ARTIFACT_DIR")).filePath("result.json");
    if (behavior == "missing") return 0;
    if (behavior == "malformed") {
        writeFile(result, "not json");
        return 0;
    }
    bool valid = true;
    if (behavior == "arguments") {
        valid =
            args.mid(3) == QStringList{"--script", "literal space", "--help", "$value;unchanged"} &&
            !qEnvironmentVariableIsSet("QAMUI_RUNNER_UNEXPECTED") &&
            qEnvironmentVariable("QAMUI_RUNNER_CLOCK") == "real" &&
            qEnvironmentVariable("QAMUI_RUNNER_UPDATE_SNAPSHOTS") == "0" &&
            qEnvironmentVariable("QAMUI_TEST_INHERITED") == "preserved";
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
        process.setProcessEnvironment(env);
        process.start(qEnvironmentVariable("QAMUI_TEST_LAUNCHER"), args);
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
        environment.insert("QAMUI_RUNNER_UNEXPECTED", "remove");
        environment.insert("QAMUI_RUNNER_CAPTURE_AFTER_MS", "0");
        environment.insert("QAMUI_RUNNER_UPDATE_SNAPSHOTS", "1");
        environment.insert("QAMUI_RUNNER_CLOCK", "virtual");
        environment.insert("QAMUI_TEST_INHERITED", "preserved");
        const auto result = run({"--eval=x", "--artifacts", temporary.filePath("new directory"),
                                 QCoreApplication::applicationFilePath(), "--peer", "arguments",
                                 "--script", "literal space", "--help", "$value;unchanged"},
                                environment);
        QVERIFY2(result.code == 0, result.err.constData());
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
        QTest::newRow("eval") << QStringList{"--eval", "terminal.expectText('TESTRUNNER FIXTURE')"}
                              << 0 << QString("final.txt");
        QTest::newRow("viewport") << QStringList{"--eval",    "terminal.expectText('SIZE: 100x30')",
                                                 "--columns", "100",
                                                 "--rows",    "30"}
                                  << 0 << QString("final.txt");
        QTest::newRow("capture-zero")
            << QStringList{"--capture-after-ms=0"} << 0 << QString("capture_styled.txt");
        QTest::newRow("capture-delayed")
            << QStringList{"--capture-after-ms", "100"} << 0 << QString("capture.txt");
        QTest::newRow("live") << QStringList{"--eval", "terminal.expectText('TESTRUNNER FIXTURE')",
                                             "--live"}
                              << 0 << QString("final.txt");
        QTest::newRow("virtual-record")
            << QStringList{"--eval",
                           "terminal.advanceTime(100); terminal.expectText('TESTRUNNER FIXTURE')",
                           "--clock",
                           "virtual",
                           "--record",
                           "tour.cast"}
            << 0 << QString("tour.cast");
        QTest::newRow("failure") << QStringList{"--eval", "terminal.fail('intentional')"} << 1
                                 << QString("failure.txt");
        QTest::newRow("module") << QStringList{"--script",
                                               QStringLiteral(QAMUI_SCENARIOS) + "/module_main.mjs"}
                                << 0 << QString("module_main.txt");
        QTest::newRow("resource") << QStringList{"--script", ":/qamui-tests/module_main.mjs"} << 0
                                  << QString("module_main.txt");
    }
    void realModes() {
        QFETCH(QStringList, options);
        QFETCH(int, expected);
        QFETCH(QString, artifact);
        QTemporaryDir temporary;
        const QString directory = temporary.filePath("artifacts");
        options << "--artifacts" << directory << "--" << qEnvironmentVariable("QAMUI_TEST_FIXTURE");
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
        auto env = QProcessEnvironment::systemEnvironment();
        for (const auto& key : env.keys()) {
            if (key.startsWith("QAMUI_RUNNER_")) env.remove(key);
        }
        env.insert("QAMUI_RUNNER_EVAL", "terminal.expectText('TESTRUNNER FIXTURE')");
        env.insert("QAMUI_RUNNER_ARTIFACT_DIR", temporary.filePath("handshake-failure"));
        env.insert("QAMUI_RUNNER_HANDSHAKE_PATH", temporary.filePath("absent/ack.json"));
        env.insert("QAMUI_RUNNER_HANDSHAKE_TOKEN", "test-token");
        child.setProcessEnvironment(env);
        child.start(qEnvironmentVariable("QAMUI_TEST_FIXTURE"), {});
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
        auto result =
            run({"--script=" + script, "--update-snapshots", "--artifacts",
                 temporary.filePath("recorded"), "--", qEnvironmentVariable("QAMUI_TEST_FIXTURE")});
        QVERIFY2(result.code == 0, result.err.constData());
        QVERIFY(readFile(temporary.filePath("baseline.txt")).contains("TESTRUNNER FIXTURE"));
        result = run({"--script", script, "--artifacts", temporary.filePath("verified"), "--",
                      qEnvironmentVariable("QAMUI_TEST_FIXTURE")});
        QVERIFY2(result.code == 0, result.err.constData());
    }
};

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().value(1) == "--peer") return peer(app.arguments());
    LauncherTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_runner_launcher.moc"
