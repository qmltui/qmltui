#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <Qamui/Core/Engine/Clipboard.h>
#include <Qamui/Core/Items/TextInput.h>
#include <Qamui/Core/Items/TextArea.h>
#include <Qamui/Core/Engine/Clock_p.h>
#include <Qamui/Core/Engine/TerminalEngine.h>
#include <Qamui/Runner/MemoryTerminalBackend.h>

using namespace qamui;

class TestClipboard : public QObject {
    Q_OBJECT
    QList<MemoryTerminalBackend*> backends;

    std::unique_ptr<Terminal> createTerminal(QQmlEngine& engine) {
        QQmlComponent component(&engine);
        component.setData(R"(
pragma ComponentBehavior: Bound
import QtQml
import Qamui
Terminal {
    id: root
    property string status: "READY"
    function readClipboard() {
        Clipboard.requestText().then(function(text) { root.status = text },
            function(error) { root.status = error.code })
    }
    Text { text: root.status }
    TextInput { objectName: "editor"; y: 2; width: 28; focus: true }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) { Qt.quit(); event.accepted = true }
    }
}
)", QUrl());
        auto object = std::unique_ptr<Terminal>(qobject_cast<Terminal*>(component.create()));
        if (!object) qFatal("%s", qPrintable(component.errorString()));
        return object;
    }
    Clipboard* clipboard(QQmlEngine& engine) {
        return engine.singletonInstance<Clipboard*>("Qamui", "Clipboard");
    }
    QJSValue watch(QQmlEngine& engine, const QJSValue& promise) {
        return engine.evaluate(QStringLiteral(
            "(function(p) { var result = {status: 'pending'};"
            "p.then(function(text) { result.status = 'resolved'; result.text = text; },"
            "function(error) { result.status = 'rejected'; result.code = error.code;"
            "result.isError = error instanceof Error; }); return result; })"
        )).call({promise});
    }
    QString frame(Terminal* terminal) {
        const auto grid = terminal->engine()->currentFrame();
        if (!grid) return {};
        QString result;
        for (int y = 0; y < grid->height(); ++y) {
            for (int x = 0; x < grid->width(); ++x)
                if (!grid->cellAt(x, y).continuation)
                    result += QString::fromUtf8(grid->cellAt(x, y).character().toByteArray());
            result += '\n';
        }
        return result;
    }

private slots:
    void initTestCase() { Clock::setVirtual(); }
    void init() {
        TerminalBackend::setFactory([this](int) {
            auto backend = std::make_unique<MemoryTerminalBackend>(QSize(32, 5));
            backends.append(backend.get());
            return backend;
        });
    }
    void cleanup() {
        TerminalBackend::setFactory(nullptr);
        backends.clear();
    }
    void routingAndCopyCompatibility() {
        QQmlEngine engine;
        auto* clip = clipboard(engine); // singleton may exist before terminals
        QVERIFY(!clip->terminal());
        auto absent = watch(engine, clip->requestText());
        QTRY_COMPARE(absent.property("code").toString(), "NoTerminal");
        QVERIFY(absent.property("isError").toBool());
        auto first = createTerminal(engine);
        QCOMPARE(clip->terminal(), first.get());
        QSignalSpy copied(first.get(), &Terminal::clipboardCopyRequested);
        backends[0]->clearOutput();
        clip->copy(QStringLiteral("café"));
        QCOMPARE(copied.count(), 1);
        QVERIFY(backends[0]->output().find(TerminalEngine::osc52Sequence(QStringLiteral("café"))) != std::string::npos);
        first->setClipboardMode(Terminal::ClipboardMode::Disabled);
        backends[0]->clearOutput();
        clip->copy(QStringLiteral("disabled"));
        QCOMPARE(copied.count(), 2);
        QVERIFY(backends[0]->output().empty());
        auto disabled = watch(engine, clip->requestText());
        QTRY_COMPARE(disabled.property("code").toString(), "Disabled");
        auto second = createTerminal(engine);
        QVERIFY(!clip->terminal());
        auto ambiguous = watch(engine, clip->requestText());
        QTRY_COMPARE(ambiguous.property("code").toString(), "AmbiguousTerminal");
        clip->setTerminal(second.get());
        QCOMPARE(clip->terminal(), second.get());
        clip->setTerminal(nullptr);
        QVERIFY(!clip->terminal());
        second.reset();
        QCOMPARE(clip->terminal(), first.get());
    }
    void copyNotifications() {
        QQmlEngine engine;
        auto root = createTerminal(engine);
        auto* clip = clipboard(engine);
        QSignalSpy requested(clip, &Clipboard::copyRequested);
        const QString text = QStringLiteral("café 世界");
        clip->copy(text);
        clip->copy(text); // repeated content still represents another copy
        root->copyToClipboard(text);
        QCOMPARE(requested.count(), 3);
        for (const auto& args : requested) QCOMPARE(args.first().toString(), text);

        auto* input = root->findChild<TextInput*>("editor");
        QVERIFY(input);
        input->setText(text);
        input->selectAll();
        input->copy();
        QCOMPARE(requested.count(), 4);
        input->cut();
        QCOMPARE(requested.count(), 5);
        QCOMPARE(requested.last().first().toString(), text);
        QVERIFY(input->text().isEmpty());
        input->copy(); // no selection does not request a copy
        QCOMPARE(requested.count(), 5);

        TextArea area(root.get());
        area.setText(QStringLiteral("first\nsecond"));
        area.selectAll();
        area.copy();
        QCOMPARE(requested.count(), 6);
        area.cut();
        QCOMPARE(requested.count(), 7);
        QCOMPARE(requested.last().first().toString(), "first\nsecond");
        QVERIFY(area.text().isEmpty());

        root->setClipboardMode(Terminal::ClipboardMode::Disabled);
        backends[0]->clearOutput();
        clip->copy(text);
        QCOMPARE(requested.count(), 8);
        QVERIFY(backends[0]->output().empty());
    }
    void copyNotificationRouting() {
        QQmlEngine engine, otherEngine;
        auto* clip = clipboard(engine);
        QSignalSpy requested(clip, &Clipboard::copyRequested);
        clip->copy(QStringLiteral("no terminal"));
        QCOMPARE(requested.count(), 0);
        auto first = createTerminal(engine);
        auto second = createTerminal(engine);
        auto other = createTerminal(otherEngine);
        QSignalSpy otherRequested(clipboard(otherEngine), &Clipboard::copyRequested);
        first->copyToClipboard(QStringLiteral("ambiguous"));
        second->copyToClipboard(QStringLiteral("ambiguous"));
        clip->copy(QStringLiteral("ambiguous"));
        QCOMPARE(requested.count(), 0);

        clip->setTerminal(first.get());
        clip->setTerminal(first.get()); // do not install a duplicate connection
        first->copyToClipboard(QStringLiteral("first"));
        QCOMPARE(requested.count(), 1);
        second->copyToClipboard(QStringLiteral("unselected"));
        other->copyToClipboard(QStringLiteral("other engine"));
        QCOMPARE(requested.count(), 1);
        QCOMPARE(otherRequested.count(), 1);

        clip->setTerminal(second.get());
        first->copyToClipboard(QStringLiteral("old selection"));
        second->copyToClipboard(QStringLiteral("second"));
        QCOMPARE(requested.count(), 2);
        QCOMPARE(requested.last().first().toString(), "second");
        second.reset(); // selected terminal destruction restores automatic routing
        first->copyToClipboard(QStringLiteral("restored"));
        QCOMPARE(requested.count(), 3);
        QCOMPARE(requested.last().first().toString(), "restored");

        auto replacement = createTerminal(engine); // ambiguity disconnects first
        first->copyToClipboard(QStringLiteral("ambiguous again"));
        QCOMPARE(requested.count(), 3);
        clip->setTerminal(first.get());
        clip->setTerminal(nullptr);
        first->copyToClipboard(QStringLiteral("override cleared"));
        QCOMPARE(requested.count(), 3);
        replacement.reset();
        first->copyToClipboard(QStringLiteral("sole terminal"));
        QCOMPARE(requested.count(), 4);
        first.reset();
        clip->copy(QStringLiteral("destroyed"));
        QCOMPARE(requested.count(), 4);
        QCOMPARE(otherRequested.count(), 1);
    }
    void enginesAreIsolated() {
        QQmlEngine a, b;
        auto first = createTerminal(a); // terminal may exist before explicit singleton access
        auto second = createTerminal(b);
        auto* ca = clipboard(a);
        auto* cb = clipboard(b);
        QVERIFY(ca != cb);
        QCOMPARE(ca->terminal(), first.get());
        QCOMPARE(cb->terminal(), second.get());
        ca->setTerminal(second.get()); // invalid assignment leaves effective terminal intact
        QCOMPARE(ca->terminal(), first.get());
        auto ra = watch(a, ca->requestText());
        auto rb = watch(b, cb->requestText());
        backends[0]->feedInput("\033]52;c;YQ==\a");
        QTRY_COMPARE(ra.property("text").toString(), "a");
        QCOMPARE(rb.property("status").toString(), "pending");
        backends[1]->feedInput("\033]52;c;Yg==\a");
        QTRY_COMPARE(rb.property("text").toString(), "b");
    }
    void validResponses_data() {
        QTest::addColumn<QByteArray>("text");
        QTest::addColumn<QByteArray>("terminator");
        QTest::addColumn<int>("chunkSize");
        QTest::newRow("unicode-bel-bytewise") << QStringLiteral("café 世界\nline\t2").toUtf8() << QByteArray("\a") << 1;
        QTest::newRow("empty-st-bytewise") << QByteArray() << QByteArray("\033\\") << 1;
        QTest::newRow("large-st") << QByteArray(1024 * 1024, 'x') << QByteArray("\033\\") << 4096;
    }
    void validResponses() {
        QFETCH(QByteArray, text);
        QFETCH(QByteArray, terminator);
        QFETCH(int, chunkSize);
        QQmlEngine engine;
        auto root = createTerminal(engine);
        auto result = watch(engine, clipboard(engine)->requestText());
        QCOMPARE(result.property("status").toString(), "pending");
        const auto bytes = "\033]52;c;" + text.toBase64() + terminator;
        for (qsizetype offset = 0; offset < bytes.size(); offset += chunkSize)
            backends[0]->feedInput(bytes.mid(offset, chunkSize));
        QTRY_COMPARE(result.property("status").toString(), "resolved");
        QCOMPARE(result.property("text").toString().toUtf8(), text);
        QCOMPARE(root->findChild<QObject*>("editor")->property("text").toString(), QString());
    }
    void invalidResponses_data() {
        QTest::addColumn<QByteArray>("bytes");
        QTest::newRow("base64") << QByteArray("\033]52;c;!bad!\a");
        QTest::newRow("utf8") << QByteArray("\033]52;c;/w==\a");
        QTest::newRow("missing-field") << QByteArray("\033]52;c\a");
        QTest::newRow("decoded-limit") << QByteArray("\033]52;c;") + QByteArray(1024 * 1024 + 1, 'x').toBase64() + '\a';
        QTest::newRow("wire-limit") << QByteArray("\033]52;c;") + QByteArray(1500000, 'x') + '\a';
    }
    void invalidResponses() {
        QFETCH(QByteArray, bytes);
        QQmlEngine engine;
        auto root = createTerminal(engine);
        auto result = watch(engine, clipboard(engine)->requestText());
        backends[0]->feedInput(bytes);
        QTRY_COMPARE(result.property("code").toString(), "InvalidResponse");
        backends[0]->feedInput("ok");
        QCOMPARE(root->findChild<QObject*>("editor")->property("text").toString(), "ok");
    }
    void timeoutAndConcurrency() {
        QQmlEngine engine;
        auto root = createTerminal(engine);
        auto* clip = clipboard(engine);
        clip->setReadTimeout(100);
        clip->setReadTimeout(0);
        QCOMPARE(clip->readTimeout(), 100);
        backends[0]->clearOutput();
        auto a = watch(engine, clip->requestText());
        auto b = watch(engine, clip->requestText());
        QCOMPARE(backends[0]->output(), std::string("\033]52;c;?\a"));
        clip->setReadTimeout(200);
        Clock::instance()->advanceTo(Clock::instance()->elapsed() + 99);
        QCoreApplication::processEvents();
        QCOMPARE(a.property("status").toString(), "pending");
        Clock::instance()->advanceTo(Clock::instance()->elapsed() + 1);
        QTRY_COMPARE(a.property("code").toString(), "Timeout");
        QTRY_COMPARE(b.property("code").toString(), "Timeout");
        backends[0]->feedInput("\033]52;c;bGF0ZQ==\a"); // no pending read: ignore
        auto next = watch(engine, clip->requestText());
        auto shared = watch(engine, clip->requestText());
        backends[0]->feedInput("\033]52;c;bmV3\a");
        QTRY_COMPARE(next.property("text").toString(), "new");
        QTRY_COMPARE(shared.property("text").toString(), "new");
    }
    void cancellations() {
        QQmlEngine engine;
        auto root = createTerminal(engine);
        auto* clip = clipboard(engine);
        auto disabled = watch(engine, clip->requestText());
        root->setClipboardMode(Terminal::ClipboardMode::Disabled);
        QTRY_COMPARE(disabled.property("code").toString(), "Cancelled");
        root->setClipboardMode(Terminal::ClipboardMode::Osc52);
        auto routing = watch(engine, clip->requestText());
        auto second = createTerminal(engine);
        QTRY_COMPARE(routing.property("code").toString(), "Cancelled");
        clip->setTerminal(root.get());
        auto switched = watch(engine, clip->requestText());
        clip->setTerminal(second.get());
        QTRY_COMPARE(switched.property("code").toString(), "Cancelled");
        auto stopped = watch(engine, clip->requestText());
        second->engine()->stop();
        QTRY_COMPARE(stopped.property("code").toString(), "Cancelled");
        auto unavailable = watch(engine, clip->requestText());
        QTRY_COMPARE(unavailable.property("code").toString(), "Unavailable");
        clip->setTerminal(root.get());
        auto destroyed = watch(engine, clip->requestText());
        root.reset();
        QTRY_COMPARE(destroyed.property("code").toString(), "Cancelled");
    }
    void quietAndNoninteractive() {
        QQmlEngine engine;
        auto root = createTerminal(engine);
        auto* clip = clipboard(engine);
        backends[0]->setInteractive(false);
        auto noninteractive = watch(engine, clip->requestText());
        QTRY_COMPARE(noninteractive.property("code").toString(), "Unavailable");
        backends[0]->setInteractive(true);
        auto pending = watch(engine, clip->requestText());
        root->engine()->beginTestSession();
        QTRY_COMPARE(pending.property("code").toString(), "Cancelled");
        backends[0]->clearOutput();
        auto quiet = watch(engine, clip->requestText());
        QTRY_COMPARE(quiet.property("code").toString(), "Unavailable");
        QVERIFY(backends[0]->output().empty());
    }
    void reentrantRead() {
        QQmlEngine engine;
        auto root = createTerminal(engine);
        engine.globalObject().setProperty("clipboard", engine.newQObject(clipboard(engine)));
        engine.evaluate("var secondResult = ''; clipboard.requestText().then(function() {"
                        "clipboard.requestText().then(function(text) { secondResult = text; }); });");
        backends[0]->feedInput("\033]52;c;Zmlyc3Q=\a");
        QTRY_VERIFY(backends[0]->output().find("\033]52;c;?\a", backends[0]->output().find("\033]52;c;?\a") + 1) != std::string::npos);
        backends[0]->feedInput("\033]52;c;c2Vjb25k\a");
        QTRY_COMPARE(engine.globalObject().property("secondResult").toString(), "second");
    }
    void committedFramesAndPasteIsolation() {
        QQmlEngine engine;
        auto root = createTerminal(engine);
        QTRY_VERIFY(frame(root.get()).contains("READY"));
        QVERIFY(QMetaObject::invokeMethod(root.get(), "readClipboard"));
        backends[0]->feedInput("\033[200~\033]52;c;aGlqYWNr\a\033[201~");
        QCOMPARE(root->property("status").toString(), "READY");
        backends[0]->feedInput("\033]52;p;aWdub3Jl\a"); // PRIMARY is not our clipboard query
        QCOMPARE(root->property("status").toString(), "READY");
        backends[0]->feedInput("\033]52;c;UEFTVEVE\a");
        QTRY_VERIFY(frame(root.get()).contains("PASTED"));
        QVERIFY(QMetaObject::invokeMethod(root.get(), "readClipboard"));
        Clock::instance()->advanceTo(Clock::instance()->elapsed() + 2000);
        QTRY_VERIFY(frame(root.get()).contains("Timeout"));
        backends[0]->feedInput("\033[200~normal paste\033[201~");
        QTRY_VERIFY(frame(root.get()).contains("normal paste"));
    }
    void uninitializedOverride() {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import Qamui\nTerminal { Terminal { objectName: \"nested\" } }", QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* nested = root->findChild<Terminal*>("nested");
        QVERIFY(nested);
        auto* clip = clipboard(engine);
        clip->setTerminal(nested);
        auto result = watch(engine, clip->requestText());
        QTRY_COMPARE(result.property("code").toString(), "Unavailable");
        QSignalSpy changed(clip, &Clipboard::terminalChanged);
        delete nested;
        QCOMPARE(clip->terminal(), root.get());
        QCOMPARE(changed.count(), 1);
    }
    void teardownWithPendingPromise() {
        auto engine = std::make_unique<QQmlEngine>();
        auto root = createTerminal(*engine);
        auto promise = clipboard(*engine)->requestText();
        QVERIFY(promise.isObject());
        engine.reset();
        root.reset();
        QCoreApplication::processEvents();
    }
};

QTEST_GUILESS_MAIN(TestClipboard)
#include "test_clipboard.moc"
