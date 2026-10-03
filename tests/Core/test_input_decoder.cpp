#include <QTest>
#include <QObject>
#include <vector>
#include "InputDecoder.h"

using namespace qtui;

class TestInputDecoder : public QObject {
    Q_OBJECT

private slots:
    void testAsciiKeys();
    void testCtrlC();
    void testArrowKeys();
    void testModifiedNavigationKeys();
    void testShiftTab();
    void testSplitSequence();
    void testMouseSGR();
    void testBracketedPaste();
    void testUtf8();
    void testMalformedMouseSGR();
};

void TestInputDecoder::testAsciiKeys() {
    InputDecoder decoder;
    int receivedKey = 0;
    QString receivedText;

    decoder.setKeyCallback([&](int qtKey, const QString& text, int modifiers) {
        receivedKey = qtKey;
        receivedText = text;
    });

    const char* buf = "a";
    decoder.decode(buf, 1);

    QCOMPARE(receivedKey, (int)Qt::Key_A);
    QCOMPARE(receivedText, QString("a"));
    
    buf = "\n";
    decoder.decode(buf, 1);
    QCOMPARE(receivedKey, Qt::Key_Return);
    QCOMPARE(receivedText, QString("\n"));
}

void TestInputDecoder::testCtrlC() {
    InputDecoder decoder;
    int receivedKey = 0;
    int receivedModifiers = 0;

    decoder.setKeyCallback([&](int qtKey, const QString& text, int modifiers) {
        receivedKey = qtKey;
        receivedModifiers = modifiers;
    });

    const char* buf = "\003"; // Ctrl+C
    decoder.decode(buf, 1);

    QCOMPARE(receivedKey, Qt::Key_C);
    QCOMPARE(receivedModifiers, Qt::ControlModifier);
}

void TestInputDecoder::testArrowKeys() {
    InputDecoder decoder;
    int receivedKey = 0;

    decoder.setKeyCallback([&](int qtKey, const QString& text, int modifiers) {
        receivedKey = qtKey;
    });

    const char* buf = "\033[A";
    decoder.decode(buf, 3);
    QCOMPARE(receivedKey, Qt::Key_Up);

    buf = "\033[B";
    decoder.decode(buf, 3);
    QCOMPARE(receivedKey, Qt::Key_Down);

    buf = "\033[3~";
    decoder.decode(buf, 4);
    QCOMPARE(receivedKey, Qt::Key_Delete);
}

void TestInputDecoder::testModifiedNavigationKeys() {
    InputDecoder decoder;
    QList<QPair<int, int>> received;

    decoder.setKeyCallback([&](int key, const QString&, int modifiers) {
        received.append({key, modifiers});
    });

    decoder.decode("\033[1;2D", 6); // Shift+Left
    decoder.decode("\033[1;5C", 6); // Ctrl+Right
    decoder.decode("\033[1;6H", 6); // Ctrl+Shift+Home
    decoder.decode("\033[1;3F", 6); // Alt+End
    decoder.decode("\033[1;2~", 6); // Shift+Home (VT form)
    decoder.decode("\033[4;6~", 6); // Ctrl+Shift+End (VT form)
    decoder.decode("\033[3;5~", 6); // Ctrl+Delete

    QCOMPARE(received.size(), 7);
    QCOMPARE(received[0], qMakePair(static_cast<int>(Qt::Key_Left),
                                    static_cast<int>(Qt::ShiftModifier)));
    QCOMPARE(received[1], qMakePair(static_cast<int>(Qt::Key_Right),
                                    static_cast<int>(Qt::ControlModifier)));
    QCOMPARE(received[2], qMakePair(
        static_cast<int>(Qt::Key_Home),
        static_cast<int>(Qt::ShiftModifier | Qt::ControlModifier)));
    QCOMPARE(received[3], qMakePair(static_cast<int>(Qt::Key_End),
                                    static_cast<int>(Qt::AltModifier)));
    QCOMPARE(received[4], qMakePair(static_cast<int>(Qt::Key_Home),
                                    static_cast<int>(Qt::ShiftModifier)));
    QCOMPARE(received[5], qMakePair(
        static_cast<int>(Qt::Key_End),
        static_cast<int>(Qt::ShiftModifier | Qt::ControlModifier)));
    QCOMPARE(received[6], qMakePair(static_cast<int>(Qt::Key_Delete),
                                    static_cast<int>(Qt::ControlModifier)));
}

void TestInputDecoder::testShiftTab() {
    InputDecoder decoder;
    QList<int> receivedKeys;
    QList<int> receivedModifiers;

    decoder.setKeyCallback([&](int qtKey, const QString&, int modifiers) {
        receivedKeys.append(qtKey);
        receivedModifiers.append(modifiers);
    });

    decoder.decode("\033[Z", 3);
    decoder.decode("\033[1;2Z", 6);
    decoder.decode("\033[9;2u", 6);
    decoder.decode("\033[27;2;9~", 9);

    QCOMPARE(receivedKeys.size(), 4);
    for (qsizetype index = 0; index < receivedKeys.size(); ++index) {
        const int key = receivedKeys[index];
        QCOMPARE(key, Qt::Key_Backtab);
        QCOMPARE(receivedModifiers[index], static_cast<int>(Qt::ShiftModifier));
    }
}

void TestInputDecoder::testSplitSequence() {
    InputDecoder decoder;
    int receivedKey = 0;

    decoder.setKeyCallback([&](int qtKey, const QString& text, int modifiers) {
        receivedKey = qtKey;
    });

    decoder.decode("\033", 1);
    QCOMPARE(receivedKey, 0); // Should be buffered

    decoder.decode("[", 1);
    QCOMPARE(receivedKey, 0); // Still incomplete

    decoder.decode("C", 1);
    QCOMPARE(receivedKey, Qt::Key_Right); // Now it resolves
}

void TestInputDecoder::testMouseSGR() {
    InputDecoder decoder;
    int recBtn = -1, recX = -1, recY = -1;
    bool recRel = false;
    bool recMotion = false;
    int callbackCount = 0;

    decoder.setMouseCallback([&](int button, int x, int y, bool release, bool motion) {
        recBtn = button;
        recX = x;
        recY = y;
        recRel = release;
        recMotion = motion;
        callbackCount++;
    });

    // SGR Mouse: \033[<cb;cx;cyM
    const char* buf = "\033[<0;15;20M"; // Button 0, X=15, Y=20, pressed
    decoder.decode(buf, strlen(buf));

    QCOMPARE(callbackCount, 1);
    QCOMPARE(recBtn, 0);
    QCOMPARE(recX, 14); // internally cx - 1
    QCOMPARE(recY, 19); // internally cy - 1
    QCOMPARE(recRel, false);
    QCOMPARE(recMotion, false);

    buf = "\033[<32;16;20M";
    decoder.decode(buf, strlen(buf));
    QCOMPARE(callbackCount, 2);
    QCOMPARE(recBtn, 0);
    QCOMPARE(recX, 15);
    QCOMPARE(recMotion, true);

    buf = "\033[<35;17;20M";
    decoder.decode(buf, strlen(buf));
    QCOMPARE(callbackCount, 3);
    QCOMPARE(recBtn, 3); // Motion with no button held.
    QCOMPARE(recX, 16);
    QCOMPARE(recMotion, true);
}

void TestInputDecoder::testBracketedPaste() {
    InputDecoder decoder;
    QString receivedText;
    int callbackCount = 0;
    decoder.setKeyCallback([&](int key, const QString &text, int modifiers) {
        Q_UNUSED(key)
        Q_UNUSED(modifiers)
        receivedText += text;
        ++callbackCount;
    });

    const QByteArray first = QByteArrayLiteral("\033[200~hello\n");
    decoder.decode(first.constData(), first.size());
    QCOMPARE(callbackCount, 0);
    const QByteArray second = QByteArrayLiteral("world\033[201~");
    decoder.decode(second.constData(), second.size());
    QCOMPARE(callbackCount, 1);
    QCOMPARE(receivedText, QStringLiteral("hello\nworld"));

    receivedText.clear();
    const QByteArray largePayload(8192, 'x');
    const QByteArray largePaste = QByteArrayLiteral("\033[200~") + largePayload
        + QByteArrayLiteral("\033[201~");
    decoder.decode(largePaste.constData(), largePaste.size());
    QCOMPARE(callbackCount, 2);
    QCOMPARE(receivedText.toUtf8(), largePayload);
}

void TestInputDecoder::testUtf8() {
    InputDecoder decoder;
    int receivedKey = 0;
    QString receivedText;

    decoder.setKeyCallback([&](int qtKey, const QString& text, int modifiers) {
        receivedKey = qtKey;
        receivedText = text;
    });

    // "🌟" is F0 9F 8C 9F
    const char utf8_buf[] = "\xF0\x9F\x8C\x9F";
    decoder.decode(utf8_buf, 4);

    QString expected = QString::fromUtf8(utf8_buf, 4);
    QCOMPARE(receivedText, expected);
    QCOMPARE(receivedKey, static_cast<int>(expected.toUcs4().first()));
    
    // Test split UTF-8
    receivedText = "";
    decoder.decode("\xF0", 1);
    QCOMPARE(receivedText, QString("")); // Buffered
    
    decoder.decode("\x9F", 1);
    QCOMPARE(receivedText, QString("")); // Buffered
    
    decoder.decode("\x8C", 1);
    QCOMPARE(receivedText, QString("")); // Buffered
    
    decoder.decode("\x9F", 1);
    QCOMPARE(receivedText, expected); // Resolves
}

void TestInputDecoder::testMalformedMouseSGR() {
    InputDecoder decoder;
    int callbackCount = 0;
    decoder.setMouseCallback([&](int, int, int, bool, bool) {
        ++callbackCount;
    });

    const QByteArray overflowing = QByteArrayLiteral("\033[<999999999999999999;1;1M");
    decoder.decode(overflowing.constData(), overflowing.size());
    const QByteArray malformed = QByteArrayLiteral("\033[<1;;2M");
    decoder.decode(malformed.constData(), malformed.size());
    QCOMPARE(callbackCount, 0);
}

QTEST_MAIN(TestInputDecoder)
#include "test_input_decoder.moc"
