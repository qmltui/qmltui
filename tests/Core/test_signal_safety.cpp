// Runs a real Qtui process under a pseudo-terminal and checks that the
// terminal is restored on Ctrl+Z, taken back on fg, and restored again when
// the process is killed. The memory backend cannot cover this: the point is
// what reaches the real descriptor and what the tty's modes are afterwards.
#include <QTest>
#include <QElapsedTimer>

#include <cerrno>
#include <csignal>
#include <cstring>
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

class Child {
public:
    ~Child() { kill(); }

    bool start()
    {
        winsize size{};
        size.ws_col = 80;
        size.ws_row = 24;
        pid = ::forkpty(&master, nullptr, nullptr, &size);
        if (pid < 0) return false;
        if (pid == 0) {
            ::setenv("TERM", "xterm-256color", 1);
            for (const char* name : {"QTUI_SCRIPT", "QTUI_EVAL", "QTUI_CAPTURE_AFTER_MS",
                                     "QTUI_TEST_EMBEDDED_SCENARIO", "QTUI_TEST_LIVE"})
                ::unsetenv(name);
            ::execl(QTUI_SIGNAL_FIXTURE, QTUI_SIGNAL_FIXTURE, nullptr);
            ::_exit(127);
        }
        return true;
    }

    // Reads until `needle` appears in the bytes read since the last clear().
    bool readUntil(const char* needle, int timeoutMs)
    {
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

    void drain(int settleMs)
    {
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
    // termios through the master; elsewhere the answer is "unknown".
    enum class Echo { On, Off, Unknown };
    Echo echo() const
    {
#if defined(__linux__)
        termios modes{};
        if (::tcgetattr(master, &modes) != 0) return Echo::Unknown;
        return (modes.c_lflag & ECHO) ? Echo::On : Echo::Off;
#else
        return Echo::Unknown;
#endif
    }

    void kill()
    {
        if (pid > 0) {
            ::kill(pid, SIGKILL);
            int status = 0;
            ::waitpid(pid, &status, 0);
            pid = -1;
        }
        if (master != -1) {
            ::close(master);
            master = -1;
        }
    }

    pid_t pid = -1;
    int master = -1;
    std::string output;
};

} // namespace

class TestSignalSafety : public QObject {
    Q_OBJECT
private slots:
    void suspendResumeAndTerminate()
    {
        // forkpty() makes the child a session leader, so its process group is
        // orphaned and the *default* action of a stop signal is discarded: the
        // child does not actually stop. That does not matter here. What is
        // tested is that the caught handler runs and restores the terminal
        // before it hands control back, and that a crash still re-raises. So
        // the assertions are on the bytes the pty receives and the tty modes,
        // not on WIFSTOPPED, which an orphaned group can never reach.
        Child child;
        QVERIFY2(child.start(), "forkpty failed");
        QVERIFY2(child.readUntil(kAltScreenOn, 5000), "the fixture never entered the alternate screen");
        child.drain(100);
        if (child.echo() != Child::Echo::Unknown)
            QCOMPARE(int(child.echo()), int(Child::Echo::Off));   // raw mode is on

        // Ctrl+Z: the handler restores the terminal before yielding.
        child.output.clear();
        QCOMPARE(::kill(child.pid, SIGTSTP), 0);
        child.drain(200);
        QVERIFY2(child.output.find(kAltScreenOff) != std::string::npos, "normal screen not restored on stop");
        QVERIFY2(child.output.find(kCursorShow) != std::string::npos, "cursor not shown on stop");
        if (child.echo() != Child::Echo::Unknown)
            QCOMPARE(int(child.echo()), int(Child::Echo::On));    // cooked again for the shell

        // fg: SIGCONT takes raw mode back and repaints the alternate screen.
        child.output.clear();
        QCOMPARE(::kill(child.pid, SIGCONT), 0);
        QVERIFY2(child.readUntil(kAltScreenOn, 5000), "alternate screen not re-entered on resume");
        child.drain(100);
        if (child.echo() != Child::Echo::Unknown)
            QCOMPARE(int(child.echo()), int(Child::Echo::Off));

        // SIGTERM: restored again, and the process dies of the signal itself.
        child.output.clear();
        QCOMPARE(::kill(child.pid, SIGTERM), 0);
        int status = 0;
        QCOMPARE(::waitpid(child.pid, &status, 0), child.pid);
        QVERIFY2(WIFSIGNALED(status), "the process did not die of the signal");
        QCOMPARE(WTERMSIG(status), SIGTERM);                       // re-raised, not swallowed
        child.pid = -1;
        child.drain(100);
        QVERIFY2(child.output.find(kAltScreenOff) != std::string::npos, "normal screen not restored on SIGTERM");
        QVERIFY2(child.output.find(kCursorShow) != std::string::npos, "cursor not shown on SIGTERM");
        if (child.echo() != Child::Echo::Unknown)
            QCOMPARE(int(child.echo()), int(Child::Echo::On));
    }
};

QTEST_GUILESS_MAIN(TestSignalSafety)
#include "test_signal_safety.moc"
