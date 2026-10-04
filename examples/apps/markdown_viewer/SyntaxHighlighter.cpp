#include "SyntaxHighlighter.h"
#include <QSet>
#include <QRegularExpression>

namespace qtui {

SyntaxHighlighter::Language SyntaxHighlighter::detectLanguage(const QString &langTag) {
    QString tag = langTag.trimmed().toLower();
    if (tag == "cpp" || tag == "c++" || tag == "c" || tag == "h" || tag == "hpp" || tag == "cc" || tag == "cxx")
        return Language::Cpp;
    if (tag == "qml")
        return Language::Qml;
    if (tag == "js" || tag == "javascript" || tag == "ts" || tag == "typescript")
        return Language::JavaScript;
    if (tag == "py" || tag == "python")
        return Language::Python;
    if (tag == "sh" || tag == "bash" || tag == "zsh" || tag == "shell")
        return Language::Bash;
    if (tag == "rs" || tag == "rust")
        return Language::Rust;
    if (tag == "json")
        return Language::Json;
    if (tag == "cmake")
        return Language::CMake;
    if (tag == "md" || tag == "markdown")
        return Language::Markdown;
    return Language::None;
}

bool SyntaxHighlighter::isKeyword(const QString &word, Language lang) {
    static const QSet<QString> cppKeywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
        "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
        "class", "compl", "concept", "const", "consteval", "constexpr", "constinit",
        "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype",
        "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
        "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
        "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
        "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected",
        "public", "reflexpr", "register", "reinterpret_cast", "requires", "return",
        "short", "signed", "sizeof", "static", "static_assert", "static_cast",
        "struct", "switch", "template", "this", "thread_local", "throw", "true",
        "try", "typedef", "typeid", "typename", "union", "unsigned", "using",
        "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
        "override", "final", "import", "module"
    };

    static const QSet<QString> qmlKeywords = {
        "import", "as", "property", "signal", "alias", "function", "var", "let", "const",
        "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue",
        "return", "try", "catch", "finally", "throw", "new", "delete", "typeof", "instanceof",
        "in", "this", "true", "false", "null", "undefined", "readonly", "required"
    };

    static const QSet<QString> pythonKeywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await", "break",
        "class", "continue", "def", "del", "elif", "else", "except", "finally",
        "for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal",
        "not", "or", "pass", "raise", "return", "try", "while", "with", "yield"
    };

    static const QSet<QString> bashKeywords = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for", "select", "while",
        "until", "do", "done", "in", "function", "time", "export", "local", "return"
    };

    static const QSet<QString> rustKeywords = {
        "as", "async", "await", "break", "const", "continue", "crate", "dyn", "else",
        "enum", "extern", "false", "fn", "for", "if", "impl", "in", "let", "loop",
        "match", "mod", "move", "mut", "pub", "ref", "return", "self", "Self",
        "static", "struct", "super", "trait", "true", "type", "unsafe", "use",
        "where", "while"
    };

    static const QSet<QString> cmakeKeywords = {
        "set", "option", "include", "find_package", "add_executable", "add_library",
        "target_link_libraries", "target_include_directories", "target_compile_definitions",
        "qt_add_executable", "qt_add_qml_module", "if", "elseif", "else", "endif",
        "foreach", "endforeach", "while", "endwhile", "function", "endfunction", "macro", "endmacro",
        "FetchContent_Declare", "FetchContent_MakeAvailable"
    };

    switch (lang) {
    case Language::Cpp:
        return cppKeywords.contains(word);
    case Language::Qml:
    case Language::JavaScript:
        return qmlKeywords.contains(word);
    case Language::Python:
        return pythonKeywords.contains(word);
    case Language::Bash:
        return bashKeywords.contains(word);
    case Language::Rust:
        return rustKeywords.contains(word);
    case Language::CMake:
        return cmakeKeywords.contains(word);
    default:
        return false;
    }
}

bool SyntaxHighlighter::isType(const QString &word, Language lang) {
    static const QSet<QString> cppTypes = {
        "int", "char", "bool", "float", "double", "void", "size_t", "int64_t", "uint64_t",
        "int32_t", "uint32_t", "int16_t", "uint16_t", "int8_t", "uint8_t",
        "QString", "QByteArray", "QList", "QVector", "QMap", "QHash", "QSet", "QObject",
        "QWidget", "QSharedPointer", "std::string", "std::vector", "std::shared_ptr", "std::unique_ptr",
        "Item", "Text", "Paragraph", "Terminal", "Rectangle", "Border", "ListView", "ScrollView"
    };

    static const QSet<QString> qmlTypes = {
        "int", "double", "string", "bool", "real", "var", "list", "color", "date", "point", "size", "rect",
        "Item", "Rectangle", "Text", "Paragraph", "Terminal", "Column", "Row", "Grid", "GridLayout",
        "ListView", "ScrollView", "Button", "CheckBox", "RadioButton", "Slider", "Switch", "Dialog",
        "Popup", "Timer", "MouseArea", "Repeater", "Loader", "BigText", "Gauge", "Sparkline", "StatusBadge"
    };

    if (lang == Language::Cpp) return cppTypes.contains(word);
    if (lang == Language::Qml) return qmlTypes.contains(word);
    return false;
}

QList<StyledSpan> SyntaxHighlighter::highlightLine(const QString &line, Language lang) {
    QList<StyledSpan> spans;
    if (line.isEmpty()) return spans;
    if (lang == Language::None) {
        spans.append({line, MarkdownRole::Code, MarkdownRole::Transparent,
                      TextAttribute::None});
        return spans;
    }

    int i = 0;
    const int len = line.length();

    auto flushToken = [&](const QString &token, MarkdownRole role,
                          TextAttribute attr = TextAttribute::None) {
        if (!token.isEmpty())
            spans.append({token, role, MarkdownRole::Transparent, attr});
    };

    while (i < len) {
        // Comments
        // C++/QML/JS/Rust line comment //
        if ((lang == Language::Cpp || lang == Language::Qml || lang == Language::JavaScript || lang == Language::Rust) &&
            i + 1 < len && line[i] == '/' && line[i + 1] == '/') {
            flushToken(line.mid(i), MarkdownRole::SyntaxComment, TextAttribute::Italic);
            break;
        }

        // Python/Bash/CMake line comment #
        if ((lang == Language::Python || lang == Language::Bash || lang == Language::CMake) && line[i] == '#') {
            flushToken(line.mid(i), MarkdownRole::SyntaxComment, TextAttribute::Italic);
            break;
        }

        // C++ Preprocessor directive (#include, #pragma, etc.)
        if (lang == Language::Cpp && line[i] == '#' && (i == 0 || line.left(i).trimmed().isEmpty())) {
            flushToken(line.mid(i), MarkdownRole::SyntaxPreprocessor, TextAttribute::Bold);
            break;
        }

        // Strings ("..." or '...')
        if (line[i] == '"' || line[i] == '\'') {
            const QChar quoteChar = line[i];
            int strStart = i;
            i++;
            while (i < len && line[i] != quoteChar) {
                if (line[i] == '\\' && i + 1 < len)
                    i += 2;
                else
                    i++;
            }
            if (i < len && line[i] == quoteChar)
                i++;
            flushToken(line.mid(strStart, i - strStart), MarkdownRole::SyntaxString);
            continue;
        }

        // Numbers
        if (line[i].isDigit()) {
            int numStart = i;
            while (i < len && (line[i].isDigit() || line[i] == '.' || line[i] == 'x' || line[i] == 'X' ||
                               (line[i] >= 'a' && line[i] <= 'f') || (line[i] >= 'A' && line[i] <= 'F') ||
                               line[i] == 'u' || line[i] == 'U' || line[i] == 'l' || line[i] == 'L' || line[i] == 'f')) {
                i++;
            }
            flushToken(line.mid(numStart, i - numStart), MarkdownRole::SyntaxNumber);
            continue;
        }

        // Identifiers & Keywords
        if (line[i].isLetter() || line[i] == '_' || line[i] == '$') {
            int wordStart = i;
            while (i < len && (line[i].isLetterOrNumber() || line[i] == '_' || line[i] == '$' || line[i] == ':')) {
                i++;
            }
            QString word = line.mid(wordStart, i - wordStart);
            if (isKeyword(word, lang)) {
                flushToken(word, MarkdownRole::SyntaxKeyword, TextAttribute::Bold);
            } else if (isType(word, lang) || (word.length() > 1 && word[0].isUpper())) {
                flushToken(word, MarkdownRole::SyntaxType);
            } else {
                flushToken(word, MarkdownRole::Code);
            }
            continue;
        }

        // Symbols / Punctuation
        if (line[i] == '{' || line[i] == '}' || line[i] == '(' || line[i] == ')' ||
            line[i] == '[' || line[i] == ']' || line[i] == ';' || line[i] == ',' ||
            line[i] == '=' || line[i] == '+' || line[i] == '-' || line[i] == '*' ||
            line[i] == '/' || line[i] == '&' || line[i] == '|' || line[i] == '!' ||
            line[i] == '<' || line[i] == '>') {
            flushToken(QString(line[i]), MarkdownRole::SyntaxSymbol);
            i++;
            continue;
        }

        // Whitespace and other characters
        int wsStart = i;
        while (i < len && line[i].isSpace()) {
            i++;
        }
        if (i > wsStart) {
            flushToken(line.mid(wsStart, i - wsStart), MarkdownRole::Code);
        } else {
            flushToken(QString(line[i]), MarkdownRole::Code);
            i++;
        }
    }

    return spans;
}

} // namespace qtui
