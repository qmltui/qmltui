#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

namespace qtui {

struct Grapheme {
    QByteArray utf8;
    int utf16Start = 0;
    int utf16Length = 0;
    int width = 1;
};

class UnicodeText {
public:
    static QVector<Grapheme> graphemes(const QString &text);
    static int displayWidth(const QString &text);
    static int nextBoundary(const QString &text, int utf16Position);
    static int previousBoundary(const QString &text, int utf16Position);

private:
    static int clusterWidth(const QVector<int> &codepoints);
};

} // namespace qtui
