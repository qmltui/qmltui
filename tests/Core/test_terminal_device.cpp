// Non-blocking terminal output and synchronized-output frame bracketing,
// exercised against pipes and real pseudo-terminal pairs.
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <QScopeGuard>
#include <string>
#include <thread>
#include <termios.h>
#include <unistd.h>

#include <Qtui/Core/primitives/Color.h>
#include <Qtui/Core/engine/FrameRenderer.h>
#include <Qtui/Core/engine/StreamRenderer.h>
#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/primitives/TerminalCapabilities.h>
#include <Qtui/Core/primitives/TerminalDevice.h>
#include <Qtui/Core/engine/TerminalEngine.h>
#include <Qtui/Core/items/Scrollback.h>

using namespace qtui;

namespace {

struct Pipe {
    int readEnd = -1;
    int writeEnd = -1;
    bool open()
    {
        int fds[2];
        if (::pipe(fds) != 0)
            return false;
        readEnd = fds[0];
        writeEnd = fds[1];
#ifdef F_SETPIPE_SZ
        ::fcntl(writeEnd, F_SETPIPE_SZ, 4096);   // keep the kernel buffer small
#endif
        return true;
    }
    bool openPty()
    {
        readEnd = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (readEnd < 0 || ::grantpt(readEnd) != 0 || ::unlockpt(readEnd) != 0)
            return false;
        const char *name = ::ptsname(readEnd);
        if (!name) return false;
        writeEnd = ::open(name, O_RDWR | O_NOCTTY);
        if (writeEnd < 0) return false;
        termios mode;
        if (::tcgetattr(writeEnd, &mode) != 0) return false;
        ::cfmakeraw(&mode);
        return ::tcsetattr(writeEnd, TCSANOW, &mode) == 0;
    }
    ~Pipe()
    {
        if (readEnd >= 0) ::close(readEnd);
        if (writeEnd >= 0) ::close(writeEnd);
    }
};

std::string payloadOf(size_t size)
{
    std::string payload(size, 'x');
    for (size_t i = 0; i < size; ++i)
        payload[i] = static_cast<char>('a' + i % 26);
    return payload;
}

// Reads whatever is available without blocking.
std::string readAvailable(int fd)
{
    const int flags = ::fcntl(fd, F_GETFL);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    std::string data;
    char buffer[65536];
    for (;;) {
        const ssize_t count = ::read(fd, buffer, sizeof buffer);
        if (count <= 0)
            break;
        data.append(buffer, static_cast<size_t>(count));
    }
    ::fcntl(fd, F_SETFL, flags);
    return data;
}

} // namespace

class TestTerminalDevice : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testBlockingWriteUnchanged();
    void testAsyncWriteQueuesAndDrainsFromEventLoop();
    void testFlushBlockingCompletesQueue();
    void testSyncOutputBracketsFrame();
    void testDisableTimeoutPreservesQueue_data();
    void testDisableTimeoutPreservesQueue();
    void testFinishTimeoutRestoresFlags_data();
    void testFinishTimeoutRestoresFlags();
    void testFlushDeadlineWithSlowReader();
    void testStreamPrintLaysOutPrompt();
    void testPtyShutdownRestoresRawMode();
    void testDestructorUsesBoundedShutdown();
    void testScrollbackAcrossModes();
    void testOsc11BackgroundOutput();
    void testResizeRepaint();
};

void TestTerminalDevice::initTestCase()
{
    qputenv("QTUI_CAPTURE_AFTER_MS", "0");   // a test-session variable keeps Terminal off stdout
}

void TestTerminalDevice::testBlockingWriteUnchanged()
{
    Pipe pipe;
    QVERIFY(pipe.open());
    TerminalDevice device(pipe.writeEnd);
    QVERIFY(!device.asyncOutput());
    QVERIFY(device.write("hello"));
    QCOMPARE(device.pendingBytes(), size_t(0));
    QCOMPARE(readAvailable(pipe.readEnd), std::string("hello"));
}

void TestTerminalDevice::testAsyncWriteQueuesAndDrainsFromEventLoop()
{
    Pipe pipe;
    QVERIFY(pipe.open());
    TerminalDevice device(pipe.writeEnd);
    device.setAsyncOutput(true);
    QVERIFY(device.asyncOutput());
    int drained = 0;
    device.setDrainedCallback([&] { ++drained; });

    const std::string payload = payloadOf(1 << 20);
    QElapsedTimer timer;
    timer.start();
    QVERIFY(device.write(payload));
    QVERIFY2(timer.elapsed() < 500, "write() must not block on a full pipe");
    QVERIFY(device.pendingBytes() > 0);
    QVERIFY(device.write("tail"));            // ordering is preserved behind the queue
    QCOMPARE(drained, 0);

    std::string received;
    QElapsedTimer guard;
    guard.start();
    while (received.size() < payload.size() + 4 && guard.elapsed() < 5000) {
        received += readAvailable(pipe.readEnd);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    QCOMPARE(received.size(), payload.size() + 4);
    QVERIFY(received.compare(0, payload.size(), payload) == 0);
    QCOMPARE(received.substr(payload.size()), std::string("tail"));
    QTRY_COMPARE(device.pendingBytes(), size_t(0));
    QCOMPARE(drained, 1);

    device.setAsyncOutput(false);
    QVERIFY(!device.asyncOutput());
    QVERIFY((::fcntl(pipe.writeEnd, F_GETFL) & O_NONBLOCK) == 0);   // flags restored
}

void TestTerminalDevice::testFlushBlockingCompletesQueue()
{
    Pipe pipe;
    QVERIFY(pipe.open());
    TerminalDevice device(pipe.writeEnd);
    device.setAsyncOutput(true);
    const std::string payload = payloadOf(512 * 1024);
    QVERIFY(device.write(payload));
    QVERIFY(device.pendingBytes() > 0);

    std::string received;
    std::thread reader([&] {
        char buffer[65536];
        while (received.size() < payload.size()) {
            const ssize_t count = ::read(pipe.readEnd, buffer, sizeof buffer);
            if (count <= 0)
                break;
            received.append(buffer, static_cast<size_t>(count));
        }
    });
    QVERIFY(device.flushBlocking(5000));
    QCOMPARE(device.pendingBytes(), size_t(0));
    reader.join();
    QVERIFY(received == payload);
}

void TestTerminalDevice::testDisableTimeoutPreservesQueue_data()
{
    QTest::addColumn<bool>("pty");
    QTest::newRow("pipe") << false;
    QTest::newRow("pty") << true;
}

void TestTerminalDevice::testDisableTimeoutPreservesQueue()
{
    QFETCH(bool, pty);
    Pipe pipe;
    QVERIFY(pty ? pipe.openPty() : pipe.open());
    TerminalDevice device(pipe.writeEnd);
    const int originalFlags = ::fcntl(pipe.writeEnd, F_GETFL);
    QVERIFY(device.setAsyncOutput(true));
    const std::string payload = payloadOf(512 * 1024);
    QVERIFY(device.write(payload));
    const size_t pending = device.pendingBytes();
    QVERIFY(pending > 0);
    QVERIFY(!device.setAsyncOutput(false, 10));
    QVERIFY(device.asyncOutput());
    QVERIFY(device.pendingBytes() > 0);
    QVERIFY(::fcntl(pipe.writeEnd, F_GETFL) & O_NONBLOCK);
    QVERIFY(device.write("tail"));
    std::string received;
    QElapsedTimer guard;
    guard.start();
    while (device.pendingBytes() && guard.elapsed() < 5000) {
        received += readAvailable(pipe.readEnd);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    received += readAvailable(pipe.readEnd);
    QCOMPARE(received, payload + "tail");
    QVERIFY(device.setAsyncOutput(false, 10));
    QCOMPARE(::fcntl(pipe.writeEnd, F_GETFL), originalFlags);
    QVERIFY(device.write("blocking"));
    QCOMPARE(readAvailable(pipe.readEnd), std::string("blocking"));
    QVERIFY(device.finishOutput(100));
    QVERIFY(readAvailable(pipe.readEnd).ends_with("\033[?25h\033[0m"));
}

void TestTerminalDevice::testFinishTimeoutRestoresFlags_data()
{
    testDisableTimeoutPreservesQueue_data();
}

void TestTerminalDevice::testFinishTimeoutRestoresFlags()
{
    QFETCH(bool, pty);
    Pipe pipe;
    QVERIFY(pty ? pipe.openPty() : pipe.open());
    const int originalFlags = ::fcntl(pipe.writeEnd, F_GETFL);
    QElapsedTimer timer;
    {
        TerminalDevice device(pipe.writeEnd);
        QVERIFY(device.setAsyncOutput(true));
        QVERIFY(device.write(payloadOf(512 * 1024)));
        QVERIFY(device.pendingBytes() > 0);
        // Also handle a descriptor whose owner restored blocking mode early.
        QVERIFY(::fcntl(pipe.writeEnd, F_SETFL, originalFlags) == 0);
        timer.start();
        QVERIFY(!device.finishOutput(20));
        QVERIFY(timer.elapsed() < 1000);
        QCOMPARE(device.pendingBytes(), size_t(0));
        QVERIFY(!device.asyncOutput());
        QCOMPARE(::fcntl(pipe.writeEnd, F_GETFL), originalFlags);
        QVERIFY(!device.write("must not block"));
        QVERIFY(!device.finishOutput(20));
    }
    QVERIFY(timer.elapsed() < 1000); // no second blocking write in the destructor
}

void TestTerminalDevice::testFlushDeadlineWithSlowReader()
{
    Pipe pipe;
    QVERIFY(pipe.open());
    TerminalDevice device(pipe.writeEnd);
    QVERIFY(device.setAsyncOutput(true));
    QVERIFY(device.write(payloadOf(1 << 20)));
    std::jthread reader([&](std::stop_token stop) {
        const int flags = ::fcntl(pipe.readEnd, F_GETFL);
        ::fcntl(pipe.readEnd, F_SETFL, flags | O_NONBLOCK);
        char buffer[4096];
        while (!stop.stop_requested()) {
            const ssize_t count = ::read(pipe.readEnd, buffer, sizeof buffer);
            if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    QElapsedTimer timer;
    timer.start();
    const bool flushed = device.flushBlocking(40);
    const qint64 elapsed = timer.elapsed();
    reader.request_stop();
    reader.join();
    // Explicitly finish to avoid spending the default shutdown deadline here.
    (void)device.finishOutput(0);
    QVERIFY(!flushed);
    QVERIFY2(elapsed < 500, "the timeout must apply to the entire flush, not each poll");
}

void TestTerminalDevice::testStreamPrintLaysOutPrompt()
{
    class Prompt final : public Item {
    public:
        explicit Prompt(Item *parent) : Item(parent) { setImplicitHeight(1); }
    protected:
        void updateLayout() override { setImplicitHeight(3); }
        void paint(TextGridBuffer &buffer, int x, int y, const ClipRect &) const override {
            buffer.writeText(x, y + 2, QStringLiteral("THIRD ROW"));
        }
    };
    Pipe pipe;
    QVERIFY(pipe.open());
    TerminalDevice device(pipe.writeEnd);
    Terminal terminal;
    terminal.setMode(Terminal::Mode::Inline);
    new Prompt(&terminal);
    StreamRenderer renderer;
    renderer.resize(30, 8);
    renderer.start(&terminal, device);
    QVERIFY(renderer.renderFrame(&terminal, device, "message\r\n"));
    const std::string output = readAvailable(pipe.readEnd);
    QVERIFY(output.find("message\r\n") != std::string::npos);
    QVERIFY(output.find("THIRD ROW") != std::string::npos);
    QCOMPARE(renderer.currentBuffer()->height(), 3);
}

void TestTerminalDevice::testPtyShutdownRestoresRawMode()
{
    Pipe pty;
    QVERIFY(pty.openPty());
    termios original;
    QVERIFY(::tcgetattr(pty.writeEnd, &original) == 0);
    original.c_lflag |= ECHO | ICANON | ISIG;
    QVERIFY(::tcsetattr(pty.writeEnd, TCSANOW, &original) == 0);
    const int savedInput = ::dup(STDIN_FILENO);
    QVERIFY(savedInput >= 0);
    const auto restoreInput = qScopeGuard([&] {
        ::dup2(savedInput, STDIN_FILENO);
        ::close(savedInput);
    });
    QVERIFY(::dup2(pty.writeEnd, STDIN_FILENO) >= 0);
    TerminalDevice device(pty.writeEnd);
    QVERIFY(device.setAsyncOutput(true));
    QVERIFY(device.enableRawMode());
    QVERIFY(device.write(payloadOf(512 * 1024)));
    QVERIFY(!device.finishOutput(20));
    QVERIFY(device.disableRawMode());
    termios restored;
    QVERIFY(::tcgetattr(pty.writeEnd, &restored) == 0);
    QCOMPARE(restored.c_lflag, original.c_lflag);
    QCOMPARE(restored.c_iflag, original.c_iflag);
    QCOMPARE(restored.c_oflag, original.c_oflag);
    QCOMPARE(restored.c_cflag, original.c_cflag);
}

void TestTerminalDevice::testDestructorUsesBoundedShutdown()
{
    Pipe pty;
    QVERIFY(pty.openPty());
    const int originalFlags = ::fcntl(pty.writeEnd, F_GETFL);
    QElapsedTimer timer;
    {
        TerminalDevice device(pty.writeEnd);
        QVERIFY(device.setAsyncOutput(true));
        QVERIFY(device.write(payloadOf(512 * 1024)));
        QVERIFY(device.pendingBytes() > 0);
        timer.start();
    }
    QVERIFY2(timer.elapsed() < 4000, "destruction must honor the 2 s default flush deadline");
    QCOMPARE(::fcntl(pty.writeEnd, F_GETFL), originalFlags);
}

void TestTerminalDevice::testSyncOutputBracketsFrame()
{
    Terminal terminal;
    terminal.setColor(Color("#123456"));
    terminal.engine()->setTestViewportSize(QSize(8, 1));

    for (const bool synchronize : {true, false}) {
        Pipe pipe;
        QVERIFY(pipe.open());
        TerminalDevice device(pipe.writeEnd);
        FrameRenderer renderer;
        renderer.setOutputEnabled(true);
        TerminalCapabilities capabilities;
        capabilities.syncOutput = synchronize;
        renderer.setCapabilities(capabilities);
        renderer.resize(8, 1);
        renderer.start(&terminal, device);
        (void)readAvailable(pipe.readEnd);   // discard the clear-screen prologue

        QVERIFY(renderer.renderFrame(&terminal, device));
        const std::string frame = readAvailable(pipe.readEnd);
        QVERIFY(!frame.empty());
        QCOMPARE(frame.starts_with("\033[?2026h"), synchronize);
        QCOMPARE(frame.ends_with("\033[?2026l"), synchronize);
        QVERIFY(frame.find("48;2;18;52;86") != std::string::npos);

        // An unchanged frame writes nothing, not even the bracketing.
        QVERIFY(renderer.renderFrame(&terminal, device));
        QVERIFY(readAvailable(pipe.readEnd).empty());
        QCOMPARE(renderer.lastFrameStats().diffBytes, qint64(0));
    }
}

void TestTerminalDevice::testScrollbackAcrossModes()
{
    Pipe pipe;
    QVERIFY(pipe.open());
    const int savedOutput = ::dup(STDOUT_FILENO);
    QVERIFY(savedOutput >= 0);
    const auto restoreOutput = qScopeGuard([&] {
        ::dup2(savedOutput, STDOUT_FILENO);
        ::close(savedOutput);
    });
    QVERIFY(::dup2(pipe.writeEnd, STDOUT_FILENO) >= 0);
    Terminal terminal;
    auto* engine = terminal.engine();
    engine->setTestViewportSize(QSize(30, 8));
    engine->setTerminalOutputEnabled(true);
    auto commit = [&] { return QMetaObject::invokeMethod(engine, "renderFrame", Qt::DirectConnection); };
    QVERIFY(commit());
    QVERIFY(readAvailable(pipe.readEnd).find("\033[?1049h") != std::string::npos);

    {
        Scrollback history(&terminal);
        QVERIFY(history.print(QString::fromUtf8("first café\n")));
        QVERIFY(history.print(QStringLiteral("second")));
        QSignalSpy rejected(&history, &Scrollback::outputRejected);
        QTest::ignoreMessage(QtWarningMsg, "Scrollback queue limit (1 MiB) exceeded; submission rejected");
        QVERIFY(!history.print(QString(TerminalEngine::ScrollbackQueueLimit, 'x')));
        QCOMPARE(rejected.count(), 1);
    } // Accepted bytes belong to the session, not the output object.
    QVERIFY(commit());
    QVERIFY(readAvailable(pipe.readEnd).find("first") == std::string::npos);
    QVERIFY(engine->pendingScrollbackBytes() > 0);

    terminal.setMode(Terminal::Mode::Inline);
    QVERIFY(commit());
    const std::string resumed = readAvailable(pipe.readEnd);
    const auto exit = resumed.find("\033[?1049l");
    const auto first = resumed.find("first café\r\n");
    const auto second = resumed.find("second\r\n");
    QVERIFY(exit != std::string::npos);
    QVERIFY(first != std::string::npos);
    QVERIFY(second != std::string::npos);
    QVERIFY(exit < first && first < second);
    QCOMPARE(engine->pendingScrollbackBytes(), size_t(0));
    QCOMPARE(terminal.engine(), engine);
    QVERIFY(commit());
    QVERIFY(readAvailable(pipe.readEnd).find("first") == std::string::npos);

    terminal.setMode(Terminal::Mode::FullScreen);
    QVERIFY(commit());
    (void)readAvailable(pipe.readEnd);
    Scrollback history(&terminal);
    QVERIFY(history.print(QStringLiteral("shutdown output")));
    engine->stop();
    const std::string stopped = readAvailable(pipe.readEnd);
    QVERIFY(stopped.find("\033[?1049l") < stopped.find("shutdown output\r\n"));
    QVERIFY(stopped.find("shutdown output\r\n") != std::string::npos);
    engine->stop();
    QVERIFY(readAvailable(pipe.readEnd).empty());
}

void TestTerminalDevice::testOsc11BackgroundOutput()
{
    Pipe pipe;
    QVERIFY(pipe.open());
    const int savedOutput = ::dup(STDOUT_FILENO);
    QVERIFY(savedOutput >= 0);
    const auto restoreOutput = qScopeGuard([&] {
        ::dup2(savedOutput, STDOUT_FILENO);
        ::close(savedOutput);
    });
    QVERIFY(::dup2(pipe.writeEnd, STDOUT_FILENO) >= 0);

    Terminal terminal;
    terminal.setColor(Color("#123456"));
    auto* engine = terminal.engine();
    engine->setTestViewportSize(QSize(20, 5));
    engine->setTerminalOutputEnabled(true);

    auto commit = [&] { return QMetaObject::invokeMethod(engine, "renderFrame", Qt::DirectConnection); };
    QVERIFY(commit());

    const std::string startOutput = readAvailable(pipe.readEnd);
    // Verify OSC 11 was sent with the formatted hex color
    QVERIFY(startOutput.find("\033]11;#123456\a") != std::string::npos);

    // Dynamic color change writes new OSC 11 sequence
    terminal.setColor(Color("#654321"));
    const std::string colorChangeOutput = readAvailable(pipe.readEnd);
    QVERIFY(colorChangeOutput.find("\033]11;#654321\a") != std::string::npos);

    // Changing to transparent writes OSC 111 reset sequence
    terminal.setColor(Color("transparent"));
    const std::string transparentOutput = readAvailable(pipe.readEnd);
    QVERIFY(transparentOutput.find("\033]111\a") != std::string::npos);

    // Setting color again and stopping writes OSC 111 on exit
    terminal.setColor(Color("#abcdef"));
    (void)readAvailable(pipe.readEnd);
    engine->feedInput("\033]11;rgb:1313/1313/1313\033\\");
    engine->stop();
    const std::string stopOutput = readAvailable(pipe.readEnd);
    QVERIFY(stopOutput.find("\033]11;#131313\a") != std::string::npos);
    QVERIFY(stopOutput.find("\033]111\a") != std::string::npos);
}

void TestTerminalDevice::testResizeRepaint()
{
    Terminal terminal;
    terminal.setColor(Color("#123456"));
    terminal.engine()->setTestViewportSize(QSize(8, 1));

    Pipe pipe;
    QVERIFY(pipe.open());
    TerminalDevice device(pipe.writeEnd);
    FrameRenderer renderer;
    renderer.setOutputEnabled(true);
    renderer.resize(8, 1);
    renderer.start(&terminal, device);
    (void)readAvailable(pipe.readEnd); // discard clear-screen prologue

    // Initial frame renders content and clear prologue
    QVERIFY(renderer.renderFrame(&terminal, device));
    std::string frame = readAvailable(pipe.readEnd);
    QVERIFY(!frame.empty());
    QVERIFY(frame.find("\033[2J\033[H") != std::string::npos);

    // Second render with unchanged scene writes nothing
    QVERIFY(renderer.renderFrame(&terminal, device));
    QVERIFY(readAvailable(pipe.readEnd).empty());
    QCOMPARE(renderer.lastFrameStats().diffBytes, qint64(0));

    // Resizing to identical dimensions must not clear buffer or trigger repaint
    renderer.resize(8, 1);
    QVERIFY(renderer.renderFrame(&terminal, device));
    QVERIFY(readAvailable(pipe.readEnd).empty());
    QCOMPARE(renderer.lastFrameStats().diffBytes, qint64(0));

    // Resizing to new dimensions must emit clear sequence and redraw frame
    renderer.resize(12, 1);
    QVERIFY(renderer.renderFrame(&terminal, device));
    frame = readAvailable(pipe.readEnd);
    QVERIFY(!frame.empty());
    QVERIFY(frame.find("\033[2J\033[H") != std::string::npos);
    QVERIFY(frame.find("48;2;18;52;86") != std::string::npos);

    // Resizing with an empty scene must still emit the clear sequence
    Terminal emptyTerminal;
    emptyTerminal.engine()->setTestViewportSize(QSize(10, 2));
    FrameRenderer blankRenderer;
    blankRenderer.setOutputEnabled(true);
    blankRenderer.resize(10, 2);
    blankRenderer.start(&emptyTerminal, device);
    (void)readAvailable(pipe.readEnd);

    QVERIFY(blankRenderer.renderFrame(&emptyTerminal, device));
    frame = readAvailable(pipe.readEnd);
    QVERIFY(frame.find("\033[2J\033[H") != std::string::npos);

    // Unchanged blank frame writes nothing
    QVERIFY(blankRenderer.renderFrame(&emptyTerminal, device));
    QVERIFY(readAvailable(pipe.readEnd).empty());

    // Resizing blank frame to new size must emit clear sequence
    blankRenderer.resize(14, 2);
    QVERIFY(blankRenderer.renderFrame(&emptyTerminal, device));
    frame = readAvailable(pipe.readEnd);
    QVERIFY(!frame.empty());
    QVERIFY(frame.find("\033[2J\033[H") != std::string::npos);
}

QTEST_MAIN(TestTerminalDevice)
#include "test_terminal_device.moc"
