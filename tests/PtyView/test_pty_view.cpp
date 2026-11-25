#include <QSignalSpy>
#include <QTest>
#include <QProcessEnvironment>
#include <QQmlComponent>
#include <QQmlEngine>

#include <QmlTui/Core/Engine/Shortcut.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/PtyView/PtyView.h>
#include <QmlTui/PtyView/UnixPtySession.h>
#include <QmlTui/PtyView/VTermAdapter.h>
#include "../TestPaint.h"

using namespace qmltui;
using qmltui::testing::paintItem;

class TestPtyView : public QObject {
    Q_OBJECT

private slots:
    void adapterRenderingAndOsc();
    void adapterInputEncoding();
    void qmlImport();
    void ptyLifecycleAndRendering();
    void processGroupTerminate();
    void scrollbarDefaultColorsAndShortcutSuppression();
    void explicitArgumentsDoNotSplitProgram();
};

void TestPtyView::adapterRenderingAndOsc() {
    VTermAdapter adapter(2, 8);
    QString title;
    QString clipboard;
    QString notification;
    adapter.setTitleCallback([&](const QString& value) { title = value; });
    adapter.setClipboardCallback([&](const QString& value) { clipboard = value; });
    adapter.setNotificationCallback([&](const QString& value) { notification = value; });
    adapter.feed(QByteArrayLiteral("\033[31;3;5;9mA\033[0;8mB\033[0;2mD"
                                   "\033]2;QmlTui terminal\a"
                                   "\033]52;c;aGVsbG8=\a"
                                   "\033]9;build complete\a"));

    TextGridBuffer buffer;
    QVERIFY(buffer.resize(8, 2));
    adapter.render(buffer, 0, 0, ClipRect{0, 0, 8, 2}, 8, 2, 0);

    QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray("A"));
    QVERIFY((buffer.cellAt(0, 0).attributes & TextAttribute::Italic) != TextAttribute::None);
    QVERIFY((buffer.cellAt(0, 0).attributes & TextAttribute::Blink) != TextAttribute::None);
    QVERIFY((buffer.cellAt(0, 0).attributes & TextAttribute::Strikethrough) != TextAttribute::None);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffe00000u);
    QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QByteArray(" "));
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::None); // SGR 2 is unsupported upstream.
    QCOMPARE(title, QStringLiteral("QmlTui terminal"));
    QCOMPARE(clipboard, QStringLiteral("hello"));
    QCOMPARE(notification, QStringLiteral("build complete"));
}

void TestPtyView::adapterInputEncoding() {
    VTermAdapter adapter(2, 8);
    QByteArray output;
    adapter.setOutputCallback([&](const QByteArray& bytes) { output += bytes; });

    adapter.feed(QByteArrayLiteral("\033[?2004h"));
    adapter.sendKey(Qt::Key_C, QString(), Qt::ControlModifier);
    adapter.sendPaste(QStringLiteral("hello"));

    QVERIFY(output.startsWith(QByteArray(1, '\x03')));
    QVERIFY(output.contains(QByteArrayLiteral("\033[200~hello\033[201~")));

    output.clear();
    adapter.sendKey(Qt::Key_F5, QString(), 0);
    QVERIFY(output.contains(QByteArrayLiteral("\033[15~")));

    output.clear();
    adapter.feed(QByteArrayLiteral("\033[?1000h\033[?1006h"));
    adapter.sendMouseMove(2, 3, 0);
    adapter.sendMouseButton(0, true, 0);
    QVERIFY(output.contains(QByteArrayLiteral("\033[<0;4;3M")));
}

void TestPtyView::qmlImport() {
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral(QMLTUI_QML_IMPORT_PATH));
    QQmlComponent component(&engine);
    component.setData(QByteArrayLiteral("import QmlTui\n"
                                        "import QmlTui.PtyView\n"
                                        "PtyView { width: 20; height: 5 }\n"),
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object != nullptr);
    auto* view = qobject_cast<PtyView*>(object.get());
    QVERIFY(view != nullptr);
    QCOMPARE(view->width(), 20);
    QCOMPARE(view->processState(), PtyView::NotRunning);
}

void TestPtyView::ptyLifecycleAndRendering() {
    PtyView view;
    view.setWidth(30);
    view.setHeight(4);
    view.setProgram(QStringLiteral("/bin/sh"));
    view.setArguments(
        {QStringLiteral("-c"),
         QStringLiteral(
             "printf '\\033[31mOK\\033[0m\\r\\n'; stty size; printf \"$QMLTUI_TEST_VALUE\"")});
    view.setEnvironment({{QStringLiteral("QMLTUI_TEST_VALUE"), QStringLiteral("env-ok")}});

    QSignalSpy startedSpy(&view, &PtyView::started);
    QSignalSpy finishedSpy(&view, &PtyView::finished);
    view.start();
    QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 1, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 5000);
    QCOMPARE(view.exitCode(), 0);
    QCOMPARE(view.processState(), PtyView::NotRunning);

    TextGridBuffer buffer;
    QVERIFY(buffer.resize(30, 4));
    paintItem(view, buffer, 0, 0, ClipRect{0, 0, 30, 4});
    QByteArray rendered;
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 30; ++column)
            if (!buffer.cellAt(column, row).continuation)
                rendered += buffer.cellAt(column, row).character().toByteArray();
    QVERIFY(rendered.contains("OK"));
    QVERIFY(rendered.contains("4 30"));
    QVERIFY(rendered.contains("env-ok"));
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffe00000u);
}

void TestPtyView::processGroupTerminate() {
    UnixPtySession session;
    QSignalSpy dataSpy(&session, &UnixPtySession::dataReceived);
    QSignalSpy finishedSpy(&session, &UnixPtySession::finished);
    QVERIFY(session.start(
        QStringLiteral("/bin/sh"),
        {QStringLiteral("-c"),
         QStringLiteral("trap 'exit 42' TERM; printf READY; while :; do sleep 10; done")},
        QString(), QProcessEnvironment::systemEnvironment(), 4, 20));
    QTRY_VERIFY_WITH_TIMEOUT(!dataSpy.isEmpty(), 5000);
    session.terminate();
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 5000);
    QCOMPARE(finishedSpy.first().first().toInt(), 42);
}

void TestPtyView::scrollbarDefaultColorsAndShortcutSuppression() {
    // 1. Default textColor/backgroundColor preserve parent fill + scrollbar uses terminal defaults
    {
        PtyView view;
        view.setWidth(20);
        view.setHeight(4);
        view.setScrollBarVisible(true);
        view.setProgram(QStringLiteral("/bin/sh"));
        view.setArguments(
            {QStringLiteral("-c"),
             QStringLiteral(
                 "for i in 1 2 3 4 5 6 7 8 9 10; do printf \"L%s\\r\\n\" \"$i\"; done")});
        QSignalSpy finishedSpy(&view, &PtyView::finished);
        view.start();
        QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 5000);
        QVERIFY(view.scrollbackSize() > 0);

        TextGridBuffer buffer;
        QVERIFY(buffer.resize(20, 4));
        buffer.fillRect(0, 0, 20, 4, " ", Color(), Color(QStringLiteral("navy")));
        paintItem(view, buffer, 0, 0, ClipRect{0, 0, 20, 4});
        QCOMPARE(buffer.cellAt(0, 0).fgColor, Color::DefaultTag);
        QCOMPARE(buffer.cellAt(0, 0).bgColor, Color(QStringLiteral("navy")).argb32());

        bool sawThumb = false;
        bool sawTrack = false;
        for (int y = 0; y < 4; ++y) {
            const Cell& sbCell = buffer.cellAt(19, y);
            QCOMPARE(sbCell.fgColor, Color::DefaultTag);
            QCOMPARE(sbCell.bgColor, Color::DefaultTag);
            if (sbCell.character() == QByteArray("\xE2\x96\x88")) {
                sawThumb = true;
                QCOMPARE(sbCell.attributes, TextAttribute::None);
            } else if (sbCell.character() == QByteArray("\xE2\x96\x91")) {
                sawTrack = true;
                QCOMPARE(sbCell.attributes, TextAttribute::Dim);
            }
        }
        QVERIFY(sawThumb);
        QVERIFY(sawTrack);

        // Once scrollback exists and scrollBarVisible is true, TIOCSWINSZ reserves column w - 1.
        PtyView colView;
        colView.setWidth(20);
        colView.setHeight(4);
        colView.setScrollBarVisible(true);
        colView.setProgram(QStringLiteral("/bin/sh"));
        colView.setArguments({QStringLiteral("-c"),
                              QStringLiteral("for i in 1 2 3 4 5 6; do printf 'X\\r\\n'; done; "
                                             "sleep 0.05; stty size")});
        QSignalSpy colDone(&colView, &PtyView::finished);
        colView.start();
        QTRY_COMPARE_WITH_TIMEOUT(colDone.count(), 1, 5000);
        TextGridBuffer colBuf;
        QVERIFY(colBuf.resize(20, 4));
        paintItem(colView, colBuf, 0, 0, ClipRect{0, 0, 20, 4});
        QByteArray colText;
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 19; ++col)
                if (!colBuf.cellAt(col, row).continuation)
                    colText += colBuf.cellAt(col, row).character().toByteArray();
        QVERIFY2(colText.contains("4 19"), colText.constData());
    }

    // 2. Focused running PtyView suppresses unmodified single-key Shortcut
    {
        Terminal terminal;
        terminal.engine()->setTerminalOutputEnabled(false);
        terminal.engine()->setTestViewportSize(QSize(30, 4));

        Shortcut quitShortcut(&terminal);
        quitShortcut.setSequence(Qt::Key_Q);
        int quitActivations = 0;
        connect(&quitShortcut, &Shortcut::activated, [&]() { ++quitActivations; });

        PtyView view(&terminal);
        view.setWidth(30);
        view.setHeight(4);
        view.setProgram(QStringLiteral("/bin/sh"));
        view.setArguments(
            {QStringLiteral("-c"),
             QStringLiteral("stty raw -echo; printf READY; dd bs=1 count=1 2>/dev/null | tr q Z")});
        QSignalSpy startedSpy(&view, &PtyView::started);
        QSignalSpy finishedSpy(&view, &PtyView::finished);
        view.forceActiveFocus();
        view.start();
        QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 1, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(
            [&]() {
                TextGridBuffer frame;
                frame.resize(30, 4);
                paintItem(view, frame, 0, 0, ClipRect{0, 0, 30, 4});
                QByteArray row;
                for (int x = 0; x < frame.width(); ++x)
                    row += frame.cellAt(x, 0).character().toByteArray();
                return row.contains("READY");
            }(),
            5000);

        terminal.engine()->handleKeyInput(Qt::Key_Q, QStringLiteral("q"), 0);
        QCOMPARE(quitActivations, 0);

        QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 5000);
        TextGridBuffer frame;
        QVERIFY(frame.resize(30, 4));
        paintItem(view, frame, 0, 0, ClipRect{0, 0, 30, 4});
        QByteArray row;
        for (int x = 0; x < frame.width(); ++x)
            row += frame.cellAt(x, 0).character().toByteArray();
        QVERIFY2(row.contains("READYZ"), row.constData());
    }
}

void TestPtyView::explicitArgumentsDoNotSplitProgram() {
    // When explicit arguments are set, a program string with spaces must NOT be split via
    // QProcess::splitCommand() (which would otherwise inject arguments before m_arguments).
    PtyView view;
    view.setWidth(20);
    view.setHeight(4);
    view.setProgram(QStringLiteral("/bin/sh -c"));
    view.setArguments({QStringLiteral("echo SHOULD_NOT_RUN")});

    QSignalSpy errorSpy(&view, &PtyView::errorOccurred);
    view.start();
    QTRY_VERIFY_WITH_TIMEOUT(!errorSpy.isEmpty(), 3000);
    QCOMPARE(view.processState(), PtyView::NotRunning);
}

QTEST_GUILESS_MAIN(TestPtyView)
#include "test_pty_view.moc"
