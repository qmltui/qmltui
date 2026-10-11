#include <QTest>
#include <QObject>
#include <vector>
#include <tuple>
#include <Qamui/Core/primitives/InputDecoder.h>

using namespace qamui;

class TestInputDecoder : public QObject {
    Q_OBJECT

private slots:
    void testAsciiKeys();
    void testPrintableKeyCodes();
    void testCtrlC();
    void testCtrlQ();
    void testArrowKeys();
    void testModifiedNavigationKeys();
    void testShiftTab();
    void testSplitSequence();
    void testMouseSGR();
    void testDetailedMouseSGR();
    void testBracketedPaste();
    void testFunctionKeys();
    void testUtf8();
    void testMalformedMouseSGR();
    void testControlReplies();
    void testDiscardRecovery();
    void testPasteReplyIsolation();
    void testParameterDefaults();
    void testAltPrefixedKeysAreNotStrings();
    void testKittyKeys_data();
    void testKittyKeys();
    void testKittyEventTypes();
    void testKittyIgnoredKeys();
    void testKittyQueryReplyIsNotAKey();
    void testPendingEscapeFlush();
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

void TestInputDecoder::testPrintableKeyCodes() {
    // The contract Runner::typeText() relies on: a keypress of any printable
    // character carries the key code Qt Quick handlers compare against.
    QCOMPARE(InputDecoder::keyForCharacter(U'a'), static_cast<int>(Qt::Key_A));
    QCOMPARE(InputDecoder::keyForCharacter(U'Z'), static_cast<int>(Qt::Key_Z));
    QCOMPARE(InputDecoder::keyForCharacter(U'2'), static_cast<int>(Qt::Key_2));
    QCOMPARE(InputDecoder::keyForCharacter(U'+'), static_cast<int>(Qt::Key_Plus));
    QCOMPARE(InputDecoder::keyForCharacter(U' '), static_cast<int>(Qt::Key_Space));
    QCOMPARE(InputDecoder::keyForCharacter(U'é'), 0xE9);

    InputDecoder decoder;
    std::vector<std::pair<int, QString>> received;
    decoder.setKeyCallback([&](int qtKey, const QString& text, int) {
        received.emplace_back(qtKey, text);
    });
    const char* buf = "w2+";
    decoder.decode(buf, 3);
    QCOMPARE(received.size(), std::size_t(3));
    QCOMPARE(received[0].first, static_cast<int>(Qt::Key_W));
    QCOMPARE(received[0].second, QString("w"));
    QCOMPARE(received[1].first, static_cast<int>(Qt::Key_2));
    QCOMPARE(received[2].first, static_cast<int>(Qt::Key_Plus));
    QCOMPARE(received[2].second, QString("+"));
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

void TestInputDecoder::testCtrlQ() {
    InputDecoder decoder;
    int receivedKey = 0;
    int receivedModifiers = 0;

    decoder.setKeyCallback([&](int qtKey, const QString&, int modifiers) {
        receivedKey = qtKey;
        receivedModifiers = modifiers;
    });

    const char* buffer = "\021"; // Ctrl+Q; Shift is not distinguishable here.
    decoder.decode(buffer, 1);

    QCOMPARE(receivedKey, Qt::Key_Q);
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

void TestInputDecoder::testDetailedMouseSGR() {
    InputDecoder decoder;
    QList<InputDecoder::MouseInput> received;
    decoder.setDetailedMouseCallback([&](const InputDecoder::MouseInput& input) {
        received.append(input);
    });

    decoder.decode("\033[<52;4;7M", 11); // Ctrl+Shift button motion.
    decoder.decode("\033[<64;4;7M", 11); // Wheel up.
    decoder.decode("\033[<65;4;7M", 11); // Wheel down.

    QCOMPARE(received.size(), 3);
    QCOMPARE(received[0].button, 0);
    QVERIFY(received[0].motion);
    QCOMPARE(received[0].modifiers,
             static_cast<int>(Qt::ShiftModifier | Qt::ControlModifier));
    QCOMPARE(received[1].wheelDeltaY, 1);
    QCOMPARE(received[2].wheelDeltaY, -1);
    QCOMPARE(received[1].x, 3);
    QCOMPARE(received[1].y, 6);
}

void TestInputDecoder::testBracketedPaste() {
    InputDecoder decoder;
    QString receivedText;
    int callbackCount = 0;
    decoder.setPasteCallback([&](const QString &text) {
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

    decoder.decode("\033[200~\033[201~", 12);
    QCOMPARE(callbackCount, 3);
}

void TestInputDecoder::testFunctionKeys() {
    InputDecoder decoder;
    QList<QPair<int, int>> received;
    decoder.setKeyCallback([&](int key, const QString&, int modifiers) {
        received.append({key, modifiers});
    });

    decoder.decode("\033[15~", 5);
    decoder.decode("\033[17;2~", 7);
    decoder.decode("\033[24;5~", 7);

    QCOMPARE(received.size(), 3);
    QCOMPARE(received[0], qMakePair(static_cast<int>(Qt::Key_F5), 0));
    QCOMPARE(received[1], qMakePair(static_cast<int>(Qt::Key_F6),
                                    static_cast<int>(Qt::ShiftModifier)));
    QCOMPARE(received[2], qMakePair(static_cast<int>(Qt::Key_F12),
                                    static_cast<int>(Qt::ControlModifier)));
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

void TestInputDecoder::testControlReplies() {
    const QList<QByteArray> replies = {
        "\033[?0u", "\033[?2026;2$y", "\033[6;20;10t",
        "\033_Gi=31;OK\033\\", "\033]11;rgb:00/00/00\a",
        "\033P>|payload\aend\033\\", "\033[?97:65:;6:1u"
    };
    for (const auto& bytes : replies) {
        for (qsizetype split = 1; split < bytes.size(); ++split) {
            InputDecoder decoder;
            int keys = 0, repliesReceived = 0;
            decoder.setKeyCallback([&](int, const QString&, int) { ++keys; });
            decoder.setReplyCallback([&](const InputDecoder::ControlSequence& sequence) {
                ++repliesReceived;
                if (bytes.contains("2026")) {
                    QCOMPARE(sequence.privateMarker, '?');
                    QCOMPARE(sequence.parameter(0), 2026);
                    QCOMPARE(sequence.parameter(1), 2);
                    QCOMPARE(sequence.intermediates, QByteArrayView("$"));
                    QCOMPARE(sequence.finalByte, 'y');
                }
                if (bytes.contains("Gi=")) {
                    QCOMPARE(sequence.kind, InputDecoder::ControlSequence::Apc);
                    QCOMPARE(sequence.payload, QByteArrayView("Gi=31;OK"));
                }
                if (bytes.contains("97:")) {
                    QCOMPARE(sequence.parameter(0, 1), 65);
                    QCOMPARE(sequence.parameter(0, 2, -1), -1);
                    QCOMPARE(sequence.parameter(1, 1), 1);
                }
            });
            decoder.decode(bytes.data(), split);
            QCOMPARE(repliesReceived, 0);
            decoder.decode(bytes.data() + split, bytes.size() - split);
            QCOMPARE(repliesReceived, 1);
            QCOMPARE(keys, 0);
        }
    }
}

void TestInputDecoder::testDiscardRecovery() {
    const QList<QByteArray> sequences = {
        QByteArray("\033[") + QByteArray(100, ';') + "A",
        QByteArray("\033_G") + QByteArray(10000, 'x') + "\aMORE\033\\",
        QByteArray("\033]11;") + QByteArray(10000, 'x') + "\a",
        QByteArray("\033P$r") + QByteArray(10000, 'x') + "\033\\",
        "\033[999999999999999999999999A",
        "\033[1$2A",
        "\033[123\x18",
        "\033_Gpartial\x1a"
    };
    for (const auto& bytes : sequences) {
        for (bool split : {false, true}) {
            InputDecoder decoder;
            QList<int> keys;
            int replies = 0;
            decoder.setKeyCallback([&](int key, const QString&, int) { keys.append(key); });
            decoder.setReplyCallback([&](const auto&) { ++replies; });
            if (split) {
                for (char byte : bytes) decoder.decode(&byte, 1);
            } else decoder.decode(bytes.data(), bytes.size());
            QVERIFY(keys.isEmpty());
            QCOMPARE(replies, 0);
            decoder.decode("q", 1);
            QCOMPARE(keys, QList<int>{Qt::Key_Q});
        }
    }
    InputDecoder decoder;
    QList<int> keys;
    decoder.setKeyCallback([&](int key, const QString&, int) { keys.append(key); });
    const QByteArray unterminated = QByteArray("\033_G") + QByteArray(10000, 'x');
    decoder.decode(unterminated.data(), unterminated.size());
    decoder.decode("payload", 7);
    QVERIFY(keys.isEmpty());
    decoder.decode("\033", 1);
    decoder.decode("[A", 2);
    QCOMPARE(keys, QList<int>{Qt::Key_Up});
    const QByteArray unfinishedCsi = QByteArray("\033[") + QByteArray(100, ';');
    decoder.decode(unfinishedCsi.data(), unfinishedCsi.size());
    decoder.decode("\033", 1);
    decoder.decode("[B", 2);
    QCOMPARE(keys, (QList<int>{Qt::Key_Up, Qt::Key_Down}));
}

void TestInputDecoder::testPasteReplyIsolation() {
    InputDecoder decoder;
    QString pasted;
    int replies = 0;
    decoder.setPasteCallback([&](const QString& text) { pasted = text; });
    decoder.setReplyCallback([&](const auto&) { ++replies; });
    const QByteArray payload = "\033[?0u\033_Gi=31;OK\033\\";
    const QByteArray bytes = "\033[200~" + payload + "\033[201~";
    for (char byte : bytes) decoder.decode(&byte, 1);
    QCOMPARE(pasted.toUtf8(), payload);
    QCOMPARE(replies, 0);
}

void TestInputDecoder::testAltPrefixedKeysAreNotStrings() {
    // metaSendsEscape terminals emit ESC followed by the key. ESC ], ESC _ and
    // ESC P must not open a string control that swallows the typing after
    // them, and ESC O followed by a non-final byte is Alt+Shift+O, not SS3.
    struct Case { QByteArray bytes; QList<int> keys; };
    const QList<Case> cases = {
        {"\033]hi", {Qt::Key_Escape, ']', Qt::Key_H, Qt::Key_I}},
        {"\033_x", {Qt::Key_Escape, '_', Qt::Key_X}},
        {"\033Pq", {Qt::Key_Escape, 'P', Qt::Key_Q}},
        {"\033O\r1", {Qt::Key_Escape, 'O', Qt::Key_Return, Qt::Key_1}},
        {"\033OA", {Qt::Key_Up}},                        // a real SS3 still decodes
        {"\033[\rq", {Qt::Key_Return, Qt::Key_Q}},       // Alt+[ then Enter: Enter survives
    };
    for (const auto& testCase : cases) {
        for (bool split : {false, true}) {
            InputDecoder decoder;
            QList<int> keys;
            int replies = 0;
            decoder.setKeyCallback([&](int key, const QString&, int) { keys.append(key); });
            decoder.setReplyCallback([&](const auto&) { ++replies; });
            if (split) {
                for (char byte : testCase.bytes) decoder.decode(&byte, 1);
            } else decoder.decode(testCase.bytes.data(), testCase.bytes.size());
            QCOMPARE(keys, testCase.keys);
            QCOMPARE(replies, 0);
        }
    }
    // A reply with the same introducer is still framed and never becomes keys.
    InputDecoder decoder;
    QList<int> keys;
    int replies = 0;
    decoder.setKeyCallback([&](int key, const QString&, int) { keys.append(key); });
    decoder.setReplyCallback([&](const auto&) { ++replies; });
    const QByteArray reply = "\033]11;rgb:00/00/00\a\033_Gi=31;OK\033\\\033P>|kitty\033\\";
    decoder.decode(reply.data(), reply.size());
    QVERIFY(keys.isEmpty());
    QCOMPARE(replies, 3);
}

void TestInputDecoder::testParameterDefaults() {
    InputDecoder decoder;
    QList<QPair<int, int>> keys;
    decoder.setKeyCallback([&](int key, const QString&, int mods) { keys.append({key, mods}); });
    const QByteArray bytes = "\033[1;A\033[;5D\033OP\033[99|q";
    decoder.decode(bytes.data(), bytes.size());
    QCOMPARE(keys.size(), 4);
    QCOMPARE(keys[0], qMakePair(int(Qt::Key_Up), 0));
    QCOMPARE(keys[1], qMakePair(int(Qt::Key_Left), int(Qt::ControlModifier)));
    QCOMPARE(keys[2], qMakePair(int(Qt::Key_F1), 0));
    QCOMPARE(keys[3], qMakePair(int(Qt::Key_Q), 0));
}

// One kitty CSI u form per row: the key, the text a TextInput would insert,
// and the modifiers Qt Quick handlers see.
void TestInputDecoder::testKittyKeys_data() {
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<int>("key");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("modifiers");
    const int shift = Qt::ShiftModifier, alt = Qt::AltModifier, ctrl = Qt::ControlModifier,
              meta = Qt::MetaModifier, keypad = Qt::KeypadModifier;
    QTest::newRow("escape") << QByteArray("\033[27u") << int(Qt::Key_Escape) << QString() << 0;
    QTest::newRow("escape omitted mods") << QByteArray("\033[27;u") << int(Qt::Key_Escape) << QString() << 0;
    QTest::newRow("enter") << QByteArray("\033[13u") << int(Qt::Key_Return) << QString("\n") << 0;
    QTest::newRow("shift+enter") << QByteArray("\033[13;2u") << int(Qt::Key_Return) << QString("\n") << shift;
    QTest::newRow("ctrl+enter") << QByteArray("\033[13;5u") << int(Qt::Key_Return) << QString() << ctrl;
    QTest::newRow("tab") << QByteArray("\033[9u") << int(Qt::Key_Tab) << QString() << 0;
    QTest::newRow("shift+tab") << QByteArray("\033[9;2u") << int(Qt::Key_Backtab) << QString() << shift;
    QTest::newRow("ctrl+shift+tab") << QByteArray("\033[9;6u") << int(Qt::Key_Backtab) << QString() << (shift | ctrl);
    QTest::newRow("backspace") << QByteArray("\033[127u") << int(Qt::Key_Backspace) << QString() << 0;
    QTest::newRow("ctrl+backspace") << QByteArray("\033[127;5u") << int(Qt::Key_Backspace) << QString() << ctrl;
    QTest::newRow("plain letter") << QByteArray("\033[97u") << int(Qt::Key_A) << QString("a") << 0;
    QTest::newRow("shifted letter") << QByteArray("\033[97:65;2u") << int(Qt::Key_A) << QString("A") << shift;
    QTest::newRow("ctrl+letter") << QByteArray("\033[97;5u") << int(Qt::Key_A) << QString() << ctrl;
    QTest::newRow("ctrl+shift+letter") << QByteArray("\033[97:65:;6:1u") << int(Qt::Key_A) << QString() << (shift | ctrl);
    QTest::newRow("alt+letter") << QByteArray("\033[120;3u") << int(Qt::Key_X) << QString() << alt;
    QTest::newRow("ctrl+alt+digit") << QByteArray("\033[50;7u") << int(Qt::Key_2) << QString() << (ctrl | alt);
    QTest::newRow("super+letter") << QByteArray("\033[97;9u") << int(Qt::Key_A) << QString("a") << meta;
    QTest::newRow("meta+letter") << QByteArray("\033[97;33u") << int(Qt::Key_A) << QString("a") << meta;
    QTest::newRow("capslock bit dropped") << QByteArray("\033[97;65u") << int(Qt::Key_A) << QString("a") << 0;
    QTest::newRow("numlock+hyper dropped") << QByteArray("\033[97;145u") << int(Qt::Key_A) << QString("a") << 0;
    QTest::newRow("space") << QByteArray("\033[32;2u") << int(Qt::Key_Space) << QString(" ") << shift;
    QTest::newRow("non-ascii") << QByteArray("\033[233u") << 0xE9 << QString(QChar(0xE9)) << 0;
    QTest::newRow("associated text") << QByteArray("\033[97;2;65u") << int(Qt::Key_A) << QString("A") << shift;
    QTest::newRow("associated text filtered") << QByteArray("\033[97;1;7:65u") << int(Qt::Key_A) << QString("A") << 0;
    QTest::newRow("repeat delivered") << QByteArray("\033[97;1:2u") << int(Qt::Key_A) << QString("a") << 0;
    QTest::newRow("F13") << QByteArray("\033[57376u") << int(Qt::Key_F13) << QString() << 0;
    QTest::newRow("F35") << QByteArray("\033[57398;2u") << int(Qt::Key_F35) << QString() << shift;
    QTest::newRow("keypad 0") << QByteArray("\033[57399u") << int(Qt::Key_0) << QString("0") << keypad;
    QTest::newRow("keypad 9") << QByteArray("\033[57408u") << int(Qt::Key_9) << QString("9") << keypad;
    QTest::newRow("keypad enter") << QByteArray("\033[57414u") << int(Qt::Key_Enter) << QString("\n") << keypad;
    QTest::newRow("keypad plus") << QByteArray("\033[57413u") << int(Qt::Key_Plus) << QString("+") << keypad;
    QTest::newRow("keypad left") << QByteArray("\033[57417;5u") << int(Qt::Key_Left) << QString() << (keypad | ctrl);
    QTest::newRow("capslock key") << QByteArray("\033[57358u") << int(Qt::Key_CapsLock) << QString() << 0;
}

void TestInputDecoder::testKittyKeys() {
    QFETCH(QByteArray, bytes);
    QFETCH(int, key);
    QFETCH(QString, text);
    QFETCH(int, modifiers);
    for (qsizetype split = 1; split <= bytes.size(); ++split) {
        InputDecoder decoder;
        int replies = 0;
        QList<std::tuple<int, QString, int>> keys;
        decoder.setKeyCallback([&](int k, const QString& t, int m) { keys.append({k, t, m}); });
        decoder.setReplyCallback([&](const InputDecoder::ControlSequence&) { ++replies; });
        decoder.decode(bytes.data(), split);
        decoder.decode(bytes.data() + split, bytes.size() - split);
        QCOMPARE(replies, 0);
        QCOMPARE(keys.size(), 1);
        QCOMPARE(std::get<0>(keys[0]), key);
        QCOMPARE(std::get<1>(keys[0]), text);
        QCOMPARE(std::get<2>(keys[0]), modifiers);
    }
}

void TestInputDecoder::testKittyEventTypes() {
    InputDecoder decoder;
    int replies = 0;
    QList<QPair<int, int>> keys;
    decoder.setKeyCallback([&](int key, const QString&, int mods) { keys.append({key, mods}); });
    decoder.setReplyCallback([&](const InputDecoder::ControlSequence&) { ++replies; });
    // press, repeat, release, unknown type; then the legacy forms with the
    // same sub-parameter, which flag 2 attaches to every key.
    const QByteArray bytes = "\033[97;1:1u\033[97;1:2u\033[97;1:3u\033[97;1:9u"
                             "\033[1;2:1A\033[1;2:3A\033[3;5:2~\033[3;5:3~\033[97;65:3ub";
    decoder.decode(bytes.data(), bytes.size());
    QCOMPARE(replies, 0);
    QCOMPARE(keys.size(), 5);
    QCOMPARE(keys[0], qMakePair(int(Qt::Key_A), 0));
    QCOMPARE(keys[1], qMakePair(int(Qt::Key_A), 0));
    QCOMPARE(keys[2], qMakePair(int(Qt::Key_Up), int(Qt::ShiftModifier)));
    QCOMPARE(keys[3], qMakePair(int(Qt::Key_Delete), int(Qt::ControlModifier)));
    QCOMPARE(keys[4], qMakePair(int(Qt::Key_B), 0));   // the byte after a release is still typing
}

void TestInputDecoder::testKittyIgnoredKeys() {
    InputDecoder decoder;
    int replies = 0;
    QList<int> keys;
    QStringList texts;
    decoder.setKeyCallback([&](int key, const QString& text, int) { keys.append(key); texts.append(text); });
    decoder.setReplyCallback([&](const InputDecoder::ControlSequence&) { ++replies; });
    // media key, left shift (flag 8 only), a C0 code point, an unassigned
    // private-use value: all consumed without a key and without text.
    const QByteArray consumed = "\033[57428u\033[57441u\033[1u\033[60000u";
    decoder.decode(consumed.data(), consumed.size());
    QCOMPARE(keys.size(), 0);
    QCOMPARE(replies, 0);
    // Malformed forms are not keys and never become text: they reach the
    // reply callback like any other unhandled control.
    const QByteArray malformed = "\033[u\033[0u\033[1114112u\033[55296u\033[97;1;65;1u";
    decoder.decode(malformed.data(), malformed.size());
    QCOMPARE(keys.size(), 0);
    QCOMPARE(replies, 5);
    decoder.decode("z", 1);
    QCOMPARE(keys, QList<int>{Qt::Key_Z});
    QCOMPARE(texts, QStringList{"z"});
}

void TestInputDecoder::testKittyQueryReplyIsNotAKey() {
    InputDecoder decoder;
    int keys = 0;
    QList<int> flags;
    decoder.setKeyCallback([&](int, const QString&, int) { ++keys; });
    decoder.setReplyCallback([&](const InputDecoder::ControlSequence& sequence) {
        QCOMPARE(sequence.privateMarker, '?');
        QCOMPARE(sequence.finalByte, 'u');
        flags.append(sequence.parameter(0));
    });
    const QByteArray bytes = "\033[?0u\033[?1u\033[?31u";
    decoder.decode(bytes.data(), bytes.size());
    QCOMPARE(keys, 0);
    QCOMPARE(flags, (QList<int>{0, 1, 31}));
}

void TestInputDecoder::testPendingEscapeFlush() {
    InputDecoder decoder;
    QList<int> keys;
    decoder.setKeyCallback([&](int key, const QString&, int) { keys.append(key); });
    QVERIFY(!decoder.hasPendingEscape());
    decoder.decode("\033", 1);
    QVERIFY(decoder.hasPendingEscape());
    QCOMPARE(keys.size(), 0);
    decoder.flushPendingEscape();
    QVERIFY(!decoder.hasPendingEscape());
    QCOMPARE(keys, QList<int>{Qt::Key_Escape});
    decoder.flushPendingEscape();   // nothing pending: no second Escape
    QCOMPARE(keys.size(), 1);
    // An ESC that a sequence start follows is not pending, and neither is
    // one inside a paste or a partial sequence.
    decoder.decode("\033[", 2);
    QVERIFY(!decoder.hasPendingEscape());
    decoder.decode("A", 1);
    QCOMPARE(keys.last(), int(Qt::Key_Up));
    decoder.decode("\033[200~\033", 7);
    QVERIFY(!decoder.hasPendingEscape());
    decoder.flushPendingEscape();
    QCOMPARE(keys.size(), 2);
    decoder.decode("x\033[201~", 7);
    QCOMPARE(keys.size(), 3);   // the pasted ESC arrives inside the text, not as Escape
    QCOMPARE(keys.last(), 0);
    // Split delivery: ESC alone, then the rest, before any flush.
    decoder.decode("\033", 1);
    QVERIFY(decoder.hasPendingEscape());
    decoder.decode("OP", 2);
    QVERIFY(!decoder.hasPendingEscape());
    QCOMPARE(keys.last(), int(Qt::Key_F1));
}

QTEST_MAIN(TestInputDecoder)
#include "test_input_decoder.moc"
