#include <QTest>
#include <QObject>
#include "TextGridBuffer.h"
#include "Text.h"
#include <QString>

using namespace qtui;

class TestTextGridBuffer : public QObject {
    Q_OBJECT

private slots:
    void testResize();
    void testClear();
    void testCellAccess();
    void testDiff();
    void testStyledTextSerialization();
};

void TestTextGridBuffer::testResize() {
    TextGridBuffer buffer;
    QCOMPARE(buffer.width(), 0);
    QCOMPARE(buffer.height(), 0);

    buffer.resize(10, 5);
    QCOMPARE(buffer.width(), 10);
    QCOMPARE(buffer.height(), 5);

    Cell c = buffer.cellAt(100, 100);
    QCOMPARE(QString(c.character.data()), QString(""));
}

void TestTextGridBuffer::testClear() {
    TextGridBuffer buffer;
    buffer.resize(5, 5);
    buffer.cellAt(2, 2).character = {'X', '\0', '\0', '\0'};
    QCOMPARE(QString(buffer.cellAt(2, 2).character.data()), QString("X"));

    buffer.clear();
    QCOMPARE(QString(buffer.cellAt(2, 2).character.data()), QString(" "));
}

void TestTextGridBuffer::testCellAccess() {
    TextGridBuffer buffer;
    buffer.resize(2, 2);
    buffer.cellAt(0, 0).character = {'A', '\0', '\0', '\0'};
    buffer.cellAt(1, 1).character = {'B', '\0', '\0', '\0'};

    QCOMPARE(QString(buffer.cellAt(0, 0).character.data()), QString("A"));
    QCOMPARE(QString(buffer.cellAt(1, 1).character.data()), QString("B"));
    QCOMPARE(QString(buffer.cellAt(0, 1).character.data()), QString(" "));
}

void TestTextGridBuffer::testDiff() {
    TextGridBuffer b1, b2;
    b1.resize(2, 2);
    b2.resize(2, 2);

    b2.cellAt(0, 0).character = {'A', '\0', '\0', '\0'};
    
    std::string diff = b1.diff(b2);
    QVERIFY(diff.find("A") != std::string::npos);
}

void TestTextGridBuffer::testStyledTextSerialization() {
    Text text;
    text.setText("Agent");
    text.setBold(true);

    TextGridBuffer buffer;
    buffer.resize(text.width(), text.height());
    text.render(buffer, 0, 0);

    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Bold);
    QVERIFY(buffer.toAnsiString().find("\033[1m") != std::string::npos);
}

QTEST_MAIN(TestTextGridBuffer)
#include "test_text_grid_buffer.moc"
