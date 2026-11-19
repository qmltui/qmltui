// Runs a real QmlTui process under a pseudo-terminal and checks that the
// terminal is restored on Ctrl+Z, taken back on fg, and restored again when
// the process is killed. The memory backend cannot cover this: the point is
// what reaches the real descriptor and what the tty's modes are afterwards.
#include <QTest>
#include <QElapsedTimer>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <string>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <util.h>
#else
#include <pty.h>
#endif

namespace {

constexpr const char* kAltScreenOn = "\033[?1049h";
constexpr const char* kAltScreenOff = "\033[?1049l";
constexpr const char* kCursorShow = "\033[?25h";
constexpr const char* kKeyboardQuery = "\033[?u";
constexpr const char* kKeyboardPush = "\033[>1u";
constexpr const char* kKeyboardPop = "\033[<u";

class Child {
public:
    ~Child() { kill(); }

    bool start() {
        winsize size{};
        size.ws_col = 80;
        size.ws_row = 24;
        slaveName[0] = '\0';
        pid = ::forkpty(&master, slaveName, nullptr, &size);
        if (pid < 0) return false;
        if (pid == 0) {
            ::setenv("TERM", "xterm-256color", 1);
            ::setenv("QMLTUI_QUERY_TIMEOUT_MS", "5000", 1); // the test answers by hand
            for (const char* name :
                 {"QMLTUI_RUNNER_SCRIPT", "QMLTUI_RUNNER_EVAL", "QMLTUI_RUNNER_CAPTURE_AFTER_MS",
                  "QMLTUI_RUNNER_EMBEDDED_SCENARIO", "QMLTUI_RUNNER_LIVE"})
                ::unsetenv(name);
            ::execl(QMLTUI_SIGNAL_FIXTURE, QMLTUI_SIGNAL_FIXTURE, nullptr);
            ::_exit(127);
        }
        return true;
    }

    // Opens a secondary non-controlling slave descriptor in the parent after
    // the child already owns its controlling terminal, preventing BSD/macOS
    // ptsclose() from flushing unread PTY bytes when the child exits on SIGTERM.
    void holdSlaveForExit() {
        if (slave == -1 && slaveName[0] != '\0') {
            slave = ::open(slaveName, O_RDWR | O_NOCTTY);
        }
    }

    bool waitAndDrainExit(int timeoutMs, int& status) {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < timeoutMs) {
            pollfd descriptor{master, POLLIN, 0};
            if (::poll(&descriptor, 1, 20) > 0 && (descriptor.revents & POLLIN)) {
                char buffer[4096];
                const ssize_t count = ::read(master, buffer, sizeof buffer);
                if (count > 0) output.append(buffer, static_cast<size_t>(count));
            }
            const pid_t waited = ::waitpid(pid, &status, WNOHANG);
            if (waited == pid) {
                pid = -1;
                drain(100);
                return true;
            }
            if (waited < 0 && errno != EINTR) return false;
        }
        return false;
    }

    // Reads until `needle` appears in the bytes read since the last clear().
    bool readUntil(const char* needle, int timeoutMs) {
        QElapsedTimer timer;
        timer.start();
        while (output.find(needle) == std::string::npos) {
            const qint64 remaining = timeoutMs - timer.elapsed();
            if (remaining <= 0) return false;
            pollfd descriptor{master, POLLIN, 0};
            const int ready = ::poll(&descriptor, 1, static_cast<int>(remaining));
            if (ready < 0 && errno == EINTR) continue;
            if (ready <= 0) return false;
            char buffer[4096];
            const ssize_t count = ::read(master, buffer, sizeof buffer);
            if (count <= 0) return false;
            output.append(buffer, static_cast<size_t>(count));
        }
        return true;
    }

    // Types bytes on the child's terminal, as a terminal replying to a query would.
    bool type(const char* bytes) {
        const size_t size = std::strlen(bytes);
        return ::write(master, bytes, size) == static_cast<ssize_t>(size);
    }

    void drain(int settleMs) {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < settleMs) {
            pollfd descriptor{master, POLLIN, 0};
            if (::poll(&descriptor, 1, 20) <= 0) continue;
            char buffer[4096];
            const ssize_t count = ::read(master, buffer, sizeof buffer);
            if (count <= 0) break;
            output.append(buffer, static_cast<size_t>(count));
        }
    }

    // Echo state of the tty the child runs on. Linux exposes the slave's
    // termios through the master; once slave is opened on BSD/macOS it can be
    // queried directly there too.
    enum class Echo { On, Off, Unknown };
    Echo echo() const {
        termios modes{};
        if (slave != -1 && ::tcgetattr(slave, &modes) == 0)
            return (modes.c_lflag & ECHO) ? Echo::On : Echo::Off;
#if defined(__linux__)
        if (master != -1 && ::tcgetattr(master, &modes) == 0)
            return (modes.c_lflag & ECHO) ? Echo::On : Echo::Off;
#endif
        return Echo::Unknown;
    }

    void kill() {
        if (pid > 0) {
            ::kill(pid, SIGKILL);
            int status = 0;
            ::waitpid(pid, &status, 0);
            pid = -1;
        }
        if (slave != -1) {
            ::close(slave);
            slave = -1;
        }
        if (master != -1) {
            ::close(master);
            master = -1;
        }
    }

    pid_t pid = -1;
    int master = -1;
    int slave = -1;
    char slaveName[1024]{};
    std::string output;
};

} // namespace

class TestSignalSafety : public QObject {
    Q_OBJECT
private slots:
    void suspendResumeAndTerminate() {
        // forkpty() makes the child a session leader, so its process group is
        // orphaned and the *default* action of a stop signal is discarded: the
        // child does not actually stop. That does not matter here. What is
        // tested is that the caught handler runs and restores the terminal
        // before it hands control back, and that a crash still re-raises. So
        // the assertions are on the bytes the pty receives and the tty modes,
        // not on WIFSTOPPED, which an orphaned group can never reach.
        Child child;
        QVERIFY2(child.start(), "forkpty failed");
        QVERIFY2(child.readUntil(kAltScreenOn, 5000),
                 "the fixture never entered the alternate screen");
        // Answer the kitty keyboard query like a capable terminal; the engine
        // then pushes the disambiguate flag on the alternate screen.
        QVERIFY2(child.readUntil(kKeyboardQuery, 5000),
                 "the fixture never queried the keyboard protocol");
        QVERIFY(child.type("\033[?0u\033[?1c"));
        QVERIFY2(child.readUntil(kKeyboardPush, 5000),
                 "the keyboard mode was not pushed after the reply");
        child.drain(100);
        if (child.echo() != Child::Echo::Unknown)
            QCOMPARE(int(child.echo()), int(Child::Echo::Off)); // raw mode is on

        // Ctrl+Z: the handler pops the keyboard mode on the screen it was
        // pushed on, then restores the terminal before yielding.
        child.output.clear();
        QCOMPARE(::kill(child.pid, SIGTSTP), 0);
        child.drain(200);
        QVERIFY2(child.output.find(kAltScreenOff) != std::string::npos,
                 "normal screen not restored on stop");
        QVERIFY2(child.output.find(kCursorShow) != std::string::npos, "cursor not shown on stop");
        QVERIFY2(child.output.find(kKeyboardPop) != std::string::npos,
                 "keyboard mode not popped on stop");
        QVERIFY2(child.output.find(kKeyboardPop) < child.output.find(kAltScreenOff),
                 "keyboard mode popped after leaving the alternate screen");
        QCOMPARE(child.output.find(kKeyboardPop, child.output.find(kKeyboardPop) + 1),
                 std::string::npos);
        if (child.echo() != Child::Echo::Unknown)
            QCOMPARE(int(child.echo()), int(Child::Echo::On)); // cooked again for the shell

        // fg: SIGCONT takes raw mode back and repaints the alternate screen.
        child.output.clear();
        QCOMPARE(::kill(child.pid, SIGCONT), 0);
        QVERIFY2(child.readUntil(kAltScreenOn, 5000), "alternate screen not re-entered on resume");
        QVERIFY2(child.readUntil(kKeyboardPush, 5000), "keyboard mode not pushed again on resume");
        QVERIFY2(child.output.find(kAltScreenOn) < child.output.find(kKeyboardPush),
                 "keyboard mode pushed before the alternate screen was re-entered");
        child.drain(100);
        if (child.echo() != Child::Echo::Unknown)
            QCOMPARE(int(child.echo()), int(Child::Echo::Off));

        // SIGTERM: restored again, and the process dies of the signal itself.
        child.holdSlaveForExit();
        child.output.clear();
        QCOMPARE(::kill(child.pid, SIGTERM), 0);
        int status = 0;
        QVERIFY2(child.waitAndDrainExit(5000, status), "child did not exit after SIGTERM");
        QVERIFY2(WIFSIGNALED(status), "the process did not die of the signal");
        QCOMPARE(WTERMSIG(status), SIGTERM); // re-raised, not swallowed
        QVERIFY2(child.output.find(kAltScreenOff) != std::string::npos,
                 "normal screen not restored on SIGTERM");
        QVERIFY2(child.output.find(kKeyboardPop) < child.output.find(kAltScreenOff),
                 "keyboard mode not popped before the normal screen on SIGTERM");
        QVERIFY2(child.output.find(kCursorShow) != std::string::npos,
                 "cursor not shown on SIGTERM");
        if (child.echo() != Child::Echo::Unknown) QCOMPARE(int(child.echo()), int(Child::Echo::On));
    }
};

QTEST_GUILESS_MAIN(TestSignalSafety)
#include "test_signal_safety.moc"
