#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#include <QmlTui/Core/Engine/QtMessageRouting.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Primitives/TerminalBackend.h>

#include <atomic>
#include <memory>
#include <vector>

using namespace qmltui;

namespace {

class OrderTrackingBackend : public TerminalBackend {
public:
    explicit OrderTrackingBackend(std::atomic<bool>* leaveRawModeFlag)
        : m_leaveRawModeFlag(leaveRawModeFlag) {}

    bool enterRawMode() override { return true; }
    bool leaveRawMode() override {
        if (m_leaveRawModeFlag) m_leaveRawModeFlag->store(true, std::memory_order_release);
        return true;
    }

    WriteStatus writeSome(const char*, size_t size, size_t& offset) override {
        offset = size;
        return WriteStatus::Done;
    }

    bool writeAll(std::string& pending, int) override {
        pending.clear();
        return true;
    }

    bool supportsAsyncOutput() const override { return true; }
    bool setNonBlockingOutput(bool) override { return true; }
    void setWritableNotifications(bool) override {}
    bool isInteractive() const override { return true; }
    void startInput() override {}
    void stopInput() override {}

    QSize size() const override { return QSize(80, 24); }

private:
    std::atomic<bool>* m_leaveRawModeFlag = nullptr;
};

} // namespace

class TestQtMessageRouting : public QObject {
    Q_OBJECT

private slots:
    void defaultDeferredStderr();
    void immediateInlinePresentation();
    void immediateApplicationDelivery();
    void disabledPresentation();
    void originalContextAndFormatting();
    void bufferOverflow();
    void nestedSessionsAndReplayOrdering();
    void duplicateAndRetiringRejection();
    void concurrentWorkerLoggingAndUnregister();
    void independentExceptions();
    void recursionDetection();
    void unfilteredWarnings();
    void fatalDeliverySubprocess();
    void terminalRestorationPrecedesReplay();
    void reentrantResetFromSinkDoesNotDeadlock();
    void logFileImmediateDeliveryDuringAltScreen();
    void redirectedStderrImmediateDeliveryDuringAltScreen();
};

void TestQtMessageRouting::defaultDeferredStderr() {
    QtMessageSink sink = QtMessageRouting::stderrPresentationSink();
    QVERIFY(static_cast<bool>(sink));

    // Default mode (no explicit registration) operates without crash
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(10, 3));
    terminal.engine()->renderFrame();
    qInfo("default-deferred-stderr-test");
    terminal.engine()->stop();
}

void TestQtMessageRouting::immediateInlinePresentation() {
    QStringList diagMessages;
    QStringList presMessages;

    auto routing =
        QtMessageRouting::install([&](const QtMessage& msg) { diagMessages.append(msg.message); },
                                  [&](const QtMessage& msg) { presMessages.append(msg.message); });
    QVERIFY(routing != nullptr);

    Terminal terminal;
    terminal.setMode(Terminal::Mode::Inline);
    terminal.engine()->setTestViewportSize(QSize(10, 3));
    terminal.engine()->renderFrame();

    qInfo("inline-message-1");
    QCOMPARE(diagMessages.size(), 1);
    QCOMPARE(presMessages.size(), 1);
    QCOMPARE(diagMessages.first(), QStringLiteral("inline-message-1"));
    QCOMPARE(presMessages.first(), QStringLiteral("inline-message-1"));
}

void TestQtMessageRouting::immediateApplicationDelivery() {
    QStringList diagMessages;
    QStringList presMessages;

    auto routing =
        QtMessageRouting::install([&](const QtMessage& msg) { diagMessages.append(msg.message); },
                                  [&](const QtMessage& msg) { presMessages.append(msg.message); });
    QVERIFY(routing != nullptr);

    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(10, 3));
    terminal.engine()->renderFrame();

    qInfo("app-message-deferred");
    // Immediate delivery to diagnostic sink
    QCOMPARE(diagMessages.size(), 1);
    QCOMPARE(diagMessages.first(), QStringLiteral("app-message-deferred"));
    // Deferred delivery for presentation
    QCOMPARE(presMessages.size(), 0);

    terminal.engine()->stop();
    // After session exit, presentation replay occurs
    QCOMPARE(presMessages.size(), 1);
    QCOMPARE(presMessages.first(), QStringLiteral("app-message-deferred"));
}

void TestQtMessageRouting::disabledPresentation() {
    QStringList diagMessages;

    auto routing =
        QtMessageRouting::install([&](const QtMessage& msg) { diagMessages.append(msg.message); }
                                  // Presentation sink omitted
        );
    QVERIFY(routing != nullptr);

    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(10, 3));
    terminal.engine()->renderFrame();

    qInfo("only-diagnostics");
    QCOMPARE(diagMessages.size(), 1);

    terminal.engine()->stop();
    // Still works cleanly on stop
    QCOMPARE(diagMessages.size(), 1);
}

void TestQtMessageRouting::originalContextAndFormatting() {
    QtMessage captured;
    auto routing = QtMessageRouting::install([](const QtMessage&) {},
                                             [&](const QtMessage& msg) { captured = msg; });

    const QDateTime before = QDateTime::currentDateTimeUtc().addSecs(-1);

    {
        Terminal terminal;
        terminal.engine()->setTestViewportSize(QSize(10, 3));
        terminal.engine()->renderFrame();

        qWarning("context-probe-message");
        terminal.engine()->stop();
    }

    const QDateTime after = QDateTime::currentDateTimeUtc().addSecs(1);

    QCOMPARE(captured.message, QStringLiteral("context-probe-message"));
    QCOMPARE(captured.type, QtWarningMsg);
    QVERIFY(captured.timestampUtc >= before);
    QVERIFY(captured.timestampUtc <= after);
    QVERIFY(!captured.formattedMessage().isEmpty());
    QVERIFY(captured.formattedMessage().contains(QStringLiteral("context-probe-message")));
}

void TestQtMessageRouting::bufferOverflow() {
    std::vector<QString> diagDelivered;
    std::vector<QString> presDelivered;

    auto routing = QtMessageRouting::install(
        [&](const QtMessage& msg) { diagDelivered.push_back(msg.message); },
        [&](const QtMessage& msg) { presDelivered.push_back(msg.message); });

    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(10, 3));
    terminal.engine()->renderFrame();

    for (int i = 0; i < 550; ++i) {
        qInfo("msg-%d", i);
    }

    // All 550 must reach diagnostic sink immediately
    QCOMPARE(diagDelivered.size(), 550);

    terminal.engine()->stop();

    // Exactly the latest 500 records must reach presentation
    QCOMPARE(presDelivered.size(), 500);
    // Oldest 50 (msg-0 through msg-49) dropped; msg-50 is first
    QCOMPARE(presDelivered.front(), QStringLiteral("msg-50"));
    QCOMPARE(presDelivered.back(), QStringLiteral("msg-549"));
}

void TestQtMessageRouting::nestedSessionsAndReplayOrdering() {
    QStringList presDelivered;

    auto routing = QtMessageRouting::install(
        [](const QtMessage&) {}, [&](const QtMessage& msg) { presDelivered.append(msg.message); });

    Terminal first;
    Terminal second;
    first.engine()->setTestViewportSize(QSize(10, 3));
    second.engine()->setTestViewportSize(QSize(10, 3));
    first.engine()->renderFrame();
    second.engine()->renderFrame();

    qInfo("nested-1");
    QVERIFY(presDelivered.isEmpty());

    first.engine()->stop();
    QVERIFY(presDelivered.isEmpty());

    second.engine()->stop();
    QCOMPARE(presDelivered.size(), 1);
    QCOMPARE(presDelivered.first(), QStringLiteral("nested-1"));
}

void TestQtMessageRouting::duplicateAndRetiringRejection() {
    auto first = QtMessageRouting::install([](const QtMessage&) {});
    QVERIFY(first != nullptr);

    // Duplicate installation returns nullptr
    auto second = QtMessageRouting::install([](const QtMessage&) {});
    QVERIFY(second == nullptr);

    first->reset();

    // After reset, a new registration succeeds
    auto third = QtMessageRouting::install([](const QtMessage&) {});
    QVERIFY(third != nullptr);
}

void TestQtMessageRouting::concurrentWorkerLoggingAndUnregister() {
    std::atomic<bool> running{true};
    std::atomic<int> loggedCount{0};

    auto routing = QtMessageRouting::install(
        [&](const QtMessage&) { loggedCount.fetch_add(1, std::memory_order_relaxed); });
    QVERIFY(routing != nullptr);

    std::vector<QThread*> threads;
    for (int i = 0; i < 4; ++i) {
        threads.push_back(QThread::create([&]() {
            while (running.load(std::memory_order_relaxed)) {
                qDebug("worker-log-message");
            }
        }));
        threads.back()->start();
    }

    QTest::qWait(20);
    routing->reset();
    running.store(false, std::memory_order_relaxed);

    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }

    QVERIFY(loggedCount.load() > 0);
}

void TestQtMessageRouting::independentExceptions() {
    bool presentationReached = false;

    auto routing = QtMessageRouting::install(
        [](const QtMessage&) { throw std::runtime_error("simulated diagnostic sink crash"); },
        [&](const QtMessage&) { presentationReached = true; });

    qInfo("exception-isolation-probe");
    QVERIFY(presentationReached);
}

void TestQtMessageRouting::recursionDetection() {
    std::atomic<int> outerCount{0};
    auto routing = QtMessageRouting::install([&](const QtMessage& msg) {
        if (msg.message.contains(QLatin1String("outer"))) {
            outerCount.fetch_add(1, std::memory_order_relaxed);
            // Recursive logging within callback
            qWarning("inner-recursive-log");
        }
    });

    qInfo("outer-log");
    QCOMPARE(outerCount.load(), 1);
}

void TestQtMessageRouting::unfilteredWarnings() {
    QStringList diagReceived;
    QStringList presReceived;

    auto routing =
        QtMessageRouting::install([&](const QtMessage& msg) { diagReceived.append(msg.message); },
                                  [&](const QtMessage& msg) { presReceived.append(msg.message); });

    const QString warningText = QStringLiteral("Setting initial properties failed for item");
    qWarning().noquote() << warningText;

    QCOMPARE(diagReceived.size(), 1);
    QCOMPARE(diagReceived.first(), warningText);
    QCOMPARE(presReceived.size(), 1);
    QCOMPARE(presReceived.first(), warningText);
}

void TestQtMessageRouting::fatalDeliverySubprocess() {
#if QT_CONFIG(process)
    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString existingAsan = env.value(QStringLiteral("ASAN_OPTIONS"));
    const QString coredumpOpts = QStringLiteral("disable_coredump=1:unmap_shadow_on_exit=1");
    env.insert(QStringLiteral("ASAN_OPTIONS"),
               existingAsan.isEmpty() ? coredumpOpts
                                      : existingAsan + QLatin1Char(':') + coredumpOpts);
    proc.setProcessEnvironment(env);
    proc.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--fatal-worker")});
    QVERIFY(proc.waitForFinished(15000));

    const QString stdoutOutput = QString::fromUtf8(proc.readAllStandardOutput());
    QVERIFY(stdoutOutput.contains(QStringLiteral("DIAG:intentional-test-fatal")));
    QVERIFY(stdoutOutput.contains(QStringLiteral("PRES:intentional-test-fatal")));
    QCOMPARE(proc.exitStatus(), QProcess::CrashExit);
#else
    QSKIP("QProcess is not available on this platform");
#endif
}

void TestQtMessageRouting::terminalRestorationPrecedesReplay() {
    std::atomic<bool> leaveRawModeCalled{false};
    TerminalBackend::setFactory(
        [&](int) { return std::make_unique<OrderTrackingBackend>(&leaveRawModeCalled); });

    bool rawModeWasDisabledBeforePresentation = false;
    auto routing =
        QtMessageRouting::install([](const QtMessage&) {},
                                  [&](const QtMessage&) {
                                      rawModeWasDisabledBeforePresentation =
                                          leaveRawModeCalled.load(std::memory_order_acquire);
                                  });

    {
        Terminal terminal;
        terminal.engine()->setTestViewportSize(QSize(10, 3));
        terminal.engine()->renderFrame();

        qInfo("replay-ordering-test");
        terminal.engine()->stop();
    }

    TerminalBackend::setFactory(nullptr);
    QVERIFY(rawModeWasDisabledBeforePresentation);
}

void TestQtMessageRouting::reentrantResetFromSinkDoesNotDeadlock() {
    int diagCalls = 0;
    std::unique_ptr<QtMessageRouting> routing;
    routing = QtMessageRouting::install([&](const QtMessage& msg) {
        if (msg.message == QLatin1String("trigger-self-reset")) {
            ++diagCalls;
            routing->reset();
        }
    });
    QVERIFY(routing != nullptr);

    qInfo("trigger-self-reset");
    QCOMPARE(diagCalls, 1);

    // Registration was retired cleanly; subsequent install succeeds immediately
    auto next = QtMessageRouting::install([](const QtMessage&) {});
    QVERIFY(next != nullptr);
}

void TestQtMessageRouting::logFileImmediateDeliveryDuringAltScreen() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString logPath = tempDir.filePath(QStringLiteral("altscreen.log"));
    qputenv("QMLTUI_LOG_FILE", logPath.toUtf8());
    const auto clearEnv = qScopeGuard([] { qunsetenv("QMLTUI_LOG_FILE"); });

    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(10, 3));
    terminal.engine()->renderFrame();

    for (int i = 0; i < 520; ++i) {
        qInfo("live-logfile-entry-%d", i);
    }

    // All 520 messages must already be flushed to disk while AltScreen is still active.
    QFile logFile(logPath);
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString contents = QString::fromUtf8(logFile.readAll());
    QVERIFY(contents.contains(QStringLiteral("live-logfile-entry-0")));
    QVERIFY(contents.contains(QStringLiteral("live-logfile-entry-519")));

    terminal.engine()->stop();
}

void TestQtMessageRouting::redirectedStderrImmediateDeliveryDuringAltScreen() {
    class RedirectedStderrBackend final : public OrderTrackingBackend {
    public:
        explicit RedirectedStderrBackend(bool stderrTty)
            : OrderTrackingBackend(nullptr), m_stderrTty(stderrTty) {}
        bool isStderrInteractive() const override { return m_stderrTty; }

    private:
        bool m_stderrTty = false;
    };

    static QStringList s_capturedStderr;
    s_capturedStderr.clear();
    const QtMessageHandler prevHandler =
        qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& msg) {
            s_capturedStderr.append(msg);
        });
    const auto restoreAll = qScopeGuard([&] {
        TerminalBackend::setFactory(nullptr);
        qInstallMessageHandler(prevHandler);
        s_capturedStderr.clear();
    });

    // When stdout is interactive and stderr is redirected (!isStderrInteractive),
    // default unconfigured logging flushes immediately during AltScreen.
    TerminalBackend::setFactory(
        [](int) { return std::make_unique<RedirectedStderrBackend>(false); });
    {
        Terminal terminal;
        terminal.engine()->setTestViewportSize(QSize(10, 3));
        terminal.engine()->renderFrame();

        qWarning("redirected-stderr-live-probe");
        QVERIFY(s_capturedStderr.contains(QStringLiteral("redirected-stderr-live-probe")));
        terminal.engine()->stop();
    }

    // When stderr shares the interactive terminal (isStderrInteractive == true),
    // default unconfigured logging stays deferred until stop().
    s_capturedStderr.clear();
    TerminalBackend::setFactory(
        [](int) { return std::make_unique<RedirectedStderrBackend>(true); });
    {
        Terminal terminal;
        terminal.engine()->setTestViewportSize(QSize(10, 3));
        terminal.engine()->renderFrame();

        qWarning("tty-stderr-deferred-probe");
        QVERIFY(!s_capturedStderr.contains(QStringLiteral("tty-stderr-deferred-probe")));
        terminal.engine()->stop();
        QVERIFY(s_capturedStderr.contains(QStringLiteral("tty-stderr-deferred-probe")));
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::strcmp(argv[1], "--fatal-worker") == 0) {
        auto routing = qmltui::QtMessageRouting::install(
            [](const qmltui::QtMessage& msg) {
                std::printf("DIAG:%s\n", qPrintable(msg.message));
                std::fflush(stdout);
            },
            [](const qmltui::QtMessage& msg) {
                std::printf("PRES:%s\n", qPrintable(msg.message));
                std::fflush(stdout);
            });
        qFatal("intentional-test-fatal");
        return 0;
    }

    QCoreApplication app(argc, argv);
    TestQtMessageRouting tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_qt_message_routing.moc"
