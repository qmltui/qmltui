#pragma once

#include <QString>
#include <QList>
#include <Cell.h>

#include "MarkdownStyle.h"

namespace qtui {

struct StyledSpan {
    QString text;
    MarkdownRole foreground = MarkdownRole::Code;
    MarkdownRole background = MarkdownRole::Transparent;
    TextAttribute attributes = TextAttribute::None;
};

class SyntaxHighlighter {
public:
    enum class Language {
        None,
        Cpp,
        Qml,
        JavaScript,
        Python,
        Bash,
        Rust,
        Json,
        CMake,
        Markdown
    };

    static Language detectLanguage(const QString &langTag);
    static QList<StyledSpan> highlightLine(const QString &line, Language lang);

private:
    static bool isKeyword(const QString &word, Language lang);
    static bool isType(const QString &word, Language lang);
};

} // namespace qtui
