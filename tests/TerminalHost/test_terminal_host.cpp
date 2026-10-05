#include <QSignalSpy>
#include <QTest>
#include <QProcessEnvironment>
#include <QQmlComponent>
#include <QQmlEngine>

#include "TerminalView.h"
#include "UnixPtySession.h"
#include "VTermAdapter.h"
#include "../TestPaint.h"

using namespace qtui;
using qtui::testing::paintItem;

class TestTerminalHost : public QObject {
    Q_OBJECT

private slots:
    void adapterRenderingAndOsc();
    void adapterInputEncoding();
    void qmlImport();
    void ptyLifecycleAndRendering();
    void processGroupTerminate();
};

void TestTerminalHost::adapterRenderingAndOsc() {
    VTermAdapter adapter(2, 8);
    QString title;
    QString clipboard;
    QString notification;
    adapter.setTitleCallback([&](const QString& value) { title = value; });
    adapter.setClipboardCallback([&](const QString& value) { clipboard = value; });
    adapter.setNotificationCallback([&](const QString& value) { notification = value; });
    adapter.feed(QByteArrayLiteral(
        "\033[31;3;5;9mA\033[0;8mB\033[0;2mD"
        "\033]2;Qtui terminal\a"
        "\033]52;c;aGVsbG8=\a"
        "\033]9;build complete\a"));

    TextGridBuffer buffer;
    QVERIFY(buffer.resize(8, 2));
    adapter.render(buffer, 0, 0, ClipRect{0, 0, 8, 2}, 8, 2, 0);

    QCOMPARE(buffer.cellAt(0, 0).character, QByteArray("A"));
    QVERIFY((buffer.cellAt(0, 0).attributes & TextAttribute::Italic) != TextAttribute::None);
    QVERIFY((buffer.cellAt(0, 0).attributes & TextAttribute::Blink) != TextAttribute::None);
    QVERIFY((buffer.cellAt(0, 0).attributes & TextAttribute::Strikethrough) != TextAttribute::None);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffe00000u);
    QCOMPARE(buffer.cellAt(1, 0).character, QByteArray(" "));
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::None); // SGR 2 is unsupported upstream.
    QCOMPARE(title, QStringLiteral("Qtui terminal"));
    QCOMPARE(clipboard, QStringLiteral("hello"));
    QCOMPARE(notification, QStringLiteral("build complete"));
}

void TestTerminalHost::adapterInputEncoding() {
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

void TestTerminalHost::qmlImport() {
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral(QTUI_QML_IMPORT_PATH));
    QQmlComponent component(&engine);
    component.setData(QByteArrayLiteral(
        "import Qtui\n"
        "import Qtui.TerminalHost\n"
        "TerminalView { width: 20; height: 5 }\n"), QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object != nullptr);
    auto* view = qobject_cast<TerminalView*>(object.get());
    QVERIFY(view != nullptr);
    QCOMPARE(view->width(), 20);
    QCOMPARE(view->processState(), TerminalView::NotRunning);
}

void TestTerminalHost::ptyLifecycleAndRendering() {
    TerminalView view;
    view.setWidth(30);
    view.setHeight(4);
    view.setProgram(QStringLiteral("/bin/sh"));
    view.setArguments({QStringLiteral("-c"), QStringLiteral(
        "printf '\\033[31mOK\\033[0m\\r\\n'; stty size; printf \"$QTUI_TEST_VALUE\"")});
    view.setEnvironment({{QStringLiteral("QTUI_TEST_VALUE"), QStringLiteral("env-ok")}});

    QSignalSpy startedSpy(&view, &TerminalView::started);
    QSignalSpy finishedSpy(&view, &TerminalView::finished);
    view.start();
    QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 1, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 5000);
    QCOMPARE(view.exitCode(), 0);
    QCOMPARE(view.processState(), TerminalView::NotRunning);

    TextGridBuffer buffer;
    QVERIFY(buffer.resize(30, 4));
    paintItem(view, buffer, 0, 0, ClipRect{0, 0, 30, 4});
    QByteArray rendered;
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 30; ++column)
            if (!buffer.cellAt(column, row).continuation)
                rendered += buffer.cellAt(column, row).character;
    QVERIFY(rendered.contains("OK"));
    QVERIFY(rendered.contains("4 30"));
    QVERIFY(rendered.contains("env-ok"));
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffe00000u);
}

void TestTerminalHost::processGroupTerminate() {
    UnixPtySession session;
    QSignalSpy dataSpy(&session, &UnixPtySession::dataReceived);
    QSignalSpy finishedSpy(&session, &UnixPtySession::finished);
    QVERIFY(session.start(QStringLiteral("/bin/sh"),
                          {QStringLiteral("-c"), QStringLiteral(
                              "trap 'exit 42' TERM; printf READY; while :; do sleep 10; done")},
                          QString(), QProcessEnvironment::systemEnvironment(), 4, 20));
    QTRY_VERIFY_WITH_TIMEOUT(!dataSpy.isEmpty(), 5000);
    session.terminate();
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 5000);
    QCOMPARE(finishedSpy.first().first().toInt(), 42);
}

QTEST_GUILESS_MAIN(TestTerminalHost)
#include "test_terminal_host.moc"
