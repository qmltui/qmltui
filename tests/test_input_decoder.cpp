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
    void testShiftTab();
    void testSplitSequence();
    void testMouseSGR();
    void testUtf8();
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
}

void TestInputDecoder::testShiftTab() {
    InputDecoder decoder;
    QList<int> receivedKeys;

    decoder.setKeyCallback([&](int qtKey, const QString&, int) {
        receivedKeys.append(qtKey);
    });

    decoder.decode("\033[Z", 3);
    decoder.decode("\033[1;2Z", 6);
    decoder.decode("\033[9;2u", 6);
    decoder.decode("\033[27;2;9~", 9);

    QCOMPARE(receivedKeys.size(), 4);
    for (int key : receivedKeys) {
        QCOMPARE(key, Qt::Key_Backtab);
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
    int callbackCount = 0;

    decoder.setMouseCallback([&](int button, int x, int y, bool release) {
        recBtn = button;
        recX = x;
        recY = y;
        recRel = release;
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
    QCOMPARE(receivedKey, expected[0].unicode());
    
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

QTEST_MAIN(TestInputDecoder)
#include "test_input_decoder.moc"
