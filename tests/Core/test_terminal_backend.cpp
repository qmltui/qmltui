// The TerminalBackend extension point, exercised end to end with the in-memory
// backend: an application-installed factory replaces the platform backend,
// engine output reaches it, and injected input bytes and resizes flow back
// through InputDecoder and the engine to QML.
#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTest>

#include <memory>

#include <Qamui/Runner/MemoryTerminalBackend.h>
#include <Qamui/Core/Items/Terminal.h>
#include <Qamui/Core/Primitives/TerminalBackend.h>
#include <Qamui/Core/Engine/TerminalEngine.h>

using namespace qamui;

namespace {

MemoryTerminalBackend* s_lastBackend = nullptr;

std::unique_ptr<TerminalBackend> makeMemoryBackend(int)
{
    auto backend = std::make_unique<MemoryTerminalBackend>(QSize(80, 24));
    s_lastBackend = backend.get();
    return backend;
}

struct Session {
    std::unique_ptr<QObject> root;
    Terminal* terminal = nullptr;
    MemoryTerminalBackend* backend = nullptr;
};

Session createSession(QQmlEngine& qmlEngine, const char* qml)
{
    QQmlComponent component(&qmlEngine);
    component.setData(qml, QUrl());
    Session session;
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
        return session;
    }
    s_lastBackend = nullptr;
    session.root.reset(component.create());
    session.terminal = qobject_cast<Terminal*>(session.root.get());
    session.backend = s_lastBackend;
    return session;
}

} // namespace

class TestTerminalBackend : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void factoryReplacesPlatformBackend();
    void engineOutputReachesBackend();
    void inputBytesReachKeyHandlers();
    void resizeUpdatesTerminalSize();
    void nullFactoryRestoresPlatformBackend();

private:
    QQmlEngine m_qmlEngine;
};

void TestTerminalBackend::initTestCase()
{
    // No test-session variable here: output must stay enabled so it reaches the backend.
    TerminalBackend::setFactory(&makeMemoryBackend);
}

void TestTerminalBackend::cleanupTestCase()
{
    TerminalBackend::setFactory(nullptr);
}

void TestTerminalBackend::factoryReplacesPlatformBackend()
{
    const std::unique_ptr<TerminalBackend> backend = createTerminalBackend();
    QVERIFY(dynamic_cast<MemoryTerminalBackend*>(backend.get()) != nullptr);
}

void TestTerminalBackend::engineOutputReachesBackend()
{
    Session session = createSession(m_qmlEngine,
        "import QtQml\nimport Qamui\n"
        "Terminal { Text { text: \"HELLO BACKEND\" } }");
    QVERIFY(session.terminal);
    QVERIFY(session.backend);
    QVERIFY(session.backend->rawMode());
    QVERIFY(session.backend->inputStarted());
    QTRY_VERIFY(session.terminal->engine()->frameSequence() > 0);
    const std::string& output = session.backend->output();
    QString printable = QString::fromStdString(output);
    printable.replace(QChar(0x1b), QStringLiteral("\\e"));
    // The renderer skips blank cells with cursor moves, so the words arrive as separate runs.
    QVERIFY2(output.find("HELLO") != std::string::npos && output.find("BACKEND") != std::string::npos,
             qPrintable(QStringLiteral("rendered text was not written to the backend; output (%1 bytes): %2")
                            .arg(output.size()).arg(printable.left(600))));
    QVERIFY2(output.find("\033[?1049h") != std::string::npos, "alternate screen was not entered");
    QVERIFY2(output.find("\033[") != std::string::npos, "no escape sequences were written");
}

void TestTerminalBackend::inputBytesReachKeyHandlers()
{
    Session session = createSession(m_qmlEngine,
        "import QtQml\nimport Qamui\n"
        "Terminal {\n"
        "    property int presses: 0\n"
        "    property string lastText\n"
        "    focus: true\n"
        "    Keys.onPressed: (event) => { presses++; lastText = event.text; event.accepted = true }\n"
        "}");
    QVERIFY(session.terminal);
    QVERIFY(session.backend);
    QTRY_VERIFY(session.terminal->engine()->frameSequence() > 0);

    session.backend->feedInput(QByteArrayLiteral("q"));
    QTRY_COMPARE(session.terminal->property("presses").toInt(), 1);
    QCOMPARE(session.terminal->property("lastText").toString(), QStringLiteral("q"));

    // A CSI sequence goes through the decoder like any terminal byte stream.
    session.backend->feedInput(QByteArrayLiteral("\033[A"));
    QTRY_COMPARE(session.terminal->property("presses").toInt(), 2);
}

void TestTerminalBackend::resizeUpdatesTerminalSize()
{
    Session session = createSession(m_qmlEngine, "import QtQml\nimport Qamui\nTerminal {}");
    QVERIFY(session.terminal);
    QVERIFY(session.backend);
    QCOMPARE(session.terminal->terminalWidth(), 80);
    QCOMPARE(session.terminal->terminalHeight(), 24);

    QSignalSpy sizeSpy(session.terminal, &Terminal::terminalSizeChanged);
    session.backend->setSize(QSize(120, 40));
    QTRY_VERIFY(sizeSpy.count() >= 1);
    QCOMPARE(session.terminal->terminalWidth(), 120);
    QCOMPARE(session.terminal->terminalHeight(), 40);
}

void TestTerminalBackend::nullFactoryRestoresPlatformBackend()
{
    TerminalBackend::setFactory(nullptr);
    const std::unique_ptr<TerminalBackend> backend = createTerminalBackend();
    QVERIFY(backend);
    QVERIFY(dynamic_cast<MemoryTerminalBackend*>(backend.get()) == nullptr);
    TerminalBackend::setFactory(&makeMemoryBackend);
}

QTEST_MAIN(TestTerminalBackend)
#include "test_terminal_backend.moc"
