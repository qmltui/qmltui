#include <QObject>
#include <QProcessEnvironment>
#include <QTest>

#include <QmlTui/Core/Primitives/TerminalCapabilities.h>

using namespace qmltui;

namespace {

QProcessEnvironment environmentWith(const QList<QPair<QString, QString>> &values)
{
    QProcessEnvironment environment;   // empty, not the system environment
    for (const auto &pair : values)
        environment.insert(pair.first, pair.second);
    return environment;
}

} // namespace

class TestTerminalCapabilities : public QObject {
    Q_OBJECT

private slots:
    void testDefaultsToTrueColor();
    void testColorTermAndDirect();
    void testIndexedFromTerm();
    void testWindowsTerminalIsTrueColor();
    void testTermProgramTable();
    void testMonoFromNoColorAndDumb();
    void testOverrideWins();
    void testSyncOutput();
    void testDepthNames();
};

void TestTerminalCapabilities::testDefaultsToTrueColor()
{
    const TerminalCapabilities capabilities = TerminalCapabilities::detect(environmentWith({}), true);
    QCOMPARE(capabilities.colorDepth, ColorDepth::TrueColor);
    QVERIFY(capabilities.syncOutput);
    QVERIFY(capabilities.isTty);
    QVERIFY(capabilities.termProgram.isEmpty());

    // An unknown TERM without COLORTERM keeps the previous unconditional behaviour.
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "xterm"}})).colorDepth,
             ColorDepth::TrueColor);
}

void TestTerminalCapabilities::testColorTermAndDirect()
{
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "screen-256color"}, {"COLORTERM", "truecolor"}})).colorDepth,
             ColorDepth::TrueColor);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "xterm-256color"}, {"COLORTERM", "24bit"}})).colorDepth,
             ColorDepth::TrueColor);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "xterm-direct"}})).colorDepth,
             ColorDepth::TrueColor);
}

void TestTerminalCapabilities::testIndexedFromTerm()
{
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "xterm-256color"}})).colorDepth,
             ColorDepth::Indexed256);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "screen-256color"}})).colorDepth,
             ColorDepth::Indexed256);
}

void TestTerminalCapabilities::testWindowsTerminalIsTrueColor()
{
    // Windows Terminal supports true colour even when TERM says 256; otherwise
    // desaturated true colours quantise to grey.
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "xterm-256color"}, {"WT_SESSION", "abc"}})).colorDepth,
             ColorDepth::TrueColor);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "xterm-256color"}, {"WSL_DISTRO_NAME", "Ubuntu"}})).colorDepth,
             ColorDepth::TrueColor);
    // NO_COLOR and an explicit override still win over the Windows heuristic.
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"WT_SESSION", "abc"}, {"NO_COLOR", "1"}})).colorDepth,
             ColorDepth::Mono);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"WT_SESSION", "abc"}, {"QMLTUI_COLOR_DEPTH", "256"}})).colorDepth,
             ColorDepth::Indexed256);
}

void TestTerminalCapabilities::testTermProgramTable()
{
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "xterm-256color"}, {"TERM_PROGRAM", "Apple_Terminal"}})).colorDepth,
             ColorDepth::Indexed256);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "xterm-256color"}, {"TERM_PROGRAM", "iTerm.app"}})).colorDepth,
             ColorDepth::TrueColor);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"TERM", "xterm-256color"}, {"TERM_PROGRAM", "WezTerm"}})).colorDepth,
             ColorDepth::TrueColor);
    const TerminalCapabilities kitty = TerminalCapabilities::detect(environmentWith({{"TERM", "xterm-kitty"}, {"TERM_PROGRAM", "kitty"}}));
    QCOMPARE(kitty.colorDepth, ColorDepth::TrueColor);
    QCOMPARE(kitty.termProgram, QStringLiteral("kitty"));
}

void TestTerminalCapabilities::testMonoFromNoColorAndDumb()
{
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"COLORTERM", "truecolor"}, {"NO_COLOR", "1"}})).colorDepth,
             ColorDepth::Mono);
    const TerminalCapabilities dumb = TerminalCapabilities::detect(environmentWith({{"TERM", "dumb"}}));
    QCOMPARE(dumb.colorDepth, ColorDepth::Mono);
    QVERIFY(!dumb.syncOutput);
}

void TestTerminalCapabilities::testOverrideWins()
{
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"COLORTERM", "truecolor"}, {"QMLTUI_COLOR_DEPTH", "16"}})).colorDepth,
             ColorDepth::Basic16);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"NO_COLOR", "1"}, {"QMLTUI_COLOR_DEPTH", "truecolor"}})).colorDepth,
             ColorDepth::TrueColor);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"QMLTUI_COLOR_DEPTH", "256"}})).colorDepth,
             ColorDepth::Indexed256);
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"QMLTUI_COLOR_DEPTH", "mono"}})).colorDepth,
             ColorDepth::Mono);
    // Unknown values fall through to detection.
    QCOMPARE(TerminalCapabilities::detect(environmentWith({{"QMLTUI_COLOR_DEPTH", "bogus"}, {"TERM", "xterm-256color"}})).colorDepth,
             ColorDepth::Indexed256);
}

void TestTerminalCapabilities::testSyncOutput()
{
    QVERIFY(TerminalCapabilities::detect(environmentWith({}), true).syncOutput);
    const TerminalCapabilities piped = TerminalCapabilities::detect(environmentWith({}), false);
    QVERIFY(!piped.syncOutput);
    QVERIFY(!piped.isTty);
    QVERIFY(!TerminalCapabilities::detect(environmentWith({{"QMLTUI_SYNC_OUTPUT", "0"}}), true).syncOutput);
    QVERIFY(TerminalCapabilities::detect(environmentWith({{"QMLTUI_SYNC_OUTPUT", "1"}}), false).syncOutput);
}

void TestTerminalCapabilities::testDepthNames()
{
    QCOMPARE(TerminalCapabilities::colorDepthName(ColorDepth::Mono), QStringLiteral("mono"));
    QCOMPARE(TerminalCapabilities::colorDepthName(ColorDepth::Basic16), QStringLiteral("16"));
    QCOMPARE(TerminalCapabilities::colorDepthName(ColorDepth::Indexed256), QStringLiteral("256"));
    QCOMPARE(TerminalCapabilities::colorDepthName(ColorDepth::TrueColor), QStringLiteral("truecolor"));
}

QTEST_MAIN(TestTerminalCapabilities)
#include "test_terminal_capabilities.moc"
