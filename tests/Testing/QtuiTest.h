#pragma once

#include <QTest>
#include <QString>
#include "TextGridBuffer.h"
#include "Terminal.h"
#include "Item.h"

namespace qtui {
namespace test {

inline bool expectGridText(const TextGridBuffer& buffer, int x, int y, const QString& expectedText) {
    for (int i = 0; i < expectedText.length(); ++i) {
        int cx = x + i;
        if (cx < 0 || cx >= buffer.width() || y < 0 || y >= buffer.height()) {
            QFAIL(qPrintable(QString("Grid index out of bounds at (%1, %2)").arg(cx).arg(y)));
            return false;
        }
        QString cellChar = QString::fromUtf8(buffer.cellAt(cx, y).character);
        QString expChar = QString(expectedText.at(i));
        if (cellChar != expChar) {
            QCOMPARE(cellChar, expChar);
            return false;
        }
    }
    return true;
}

inline bool expectFocusedItem(Terminal* terminal, Item* expectedItem) {
    if (!terminal) {
        QFAIL("Terminal pointer is null");
        return false;
    }
    QCOMPARE(terminal->focusedItem(), expectedItem);
    return terminal->focusedItem() == expectedItem;
}

inline bool expectCellAttribute(const TextGridBuffer& buffer, int x, int y, TextAttribute expectedAttr) {
    if (x < 0 || x >= buffer.width() || y < 0 || y >= buffer.height()) {
        QFAIL(qPrintable(QString("Grid index out of bounds at (%1, %2)").arg(x).arg(y)));
        return false;
    }
    QCOMPARE(static_cast<int>(buffer.cellAt(x, y).attributes), static_cast<int>(expectedAttr));
    return buffer.cellAt(x, y).attributes == expectedAttr;
}

} // namespace test
} // namespace qtui
