#pragma once

#include <QString>
#include <QList>
#include <Qamui/Core/primitives/Cell.h>

#include "MdRole.h"

namespace qamui {

struct StyledSpan {
    QString text;
    MdRole foreground = MdRole::Code;
    MdRole background = MdRole::Transparent;
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
        Markdown,
        Haskell,
        Go,
        Yaml,
        Html
    };

    static Language detectLanguage(const QString &langTag);
    static QList<StyledSpan> highlightLine(const QString &line, Language lang);

private:
    static bool isKeyword(const QString &word, Language lang);
    static bool isType(const QString &word, Language lang);
};

} // namespace qamui
