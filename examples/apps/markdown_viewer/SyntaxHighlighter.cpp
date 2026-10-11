#include "SyntaxHighlighter.h"
#include <QSet>
#include <QRegularExpression>

namespace qamui {

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
    if (tag == "hs" || tag == "haskell" || tag == "lhs")
        return Language::Haskell;
    if (tag == "go" || tag == "golang")
        return Language::Go;
    if (tag == "yaml" || tag == "yml")
        return Language::Yaml;
    if (tag == "html" || tag == "xml" || tag == "svg")
        return Language::Html;
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

    static const QSet<QString> haskellKeywords = {
        "module", "where", "import", "as", "qualified", "hiding", "case", "of",
        "if", "then", "else", "let", "in", "do", "data", "type", "newtype",
        "class", "instance", "deriving", "default", "foreign", "infix", "infixl",
        "infixr", "forall", "mdo", "family", "pattern"
    };

    static const QSet<QString> goKeywords = {
        "package", "import", "func", "var", "const", "type", "struct", "interface",
        "return", "if", "else", "for", "range", "switch", "case", "default",
        "defer", "go", "select", "chan", "map", "fallthrough", "break", "continue",
        "goto", "nil", "true", "false"
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
    case Language::Haskell:
        return haskellKeywords.contains(word);
    case Language::Go:
        return goKeywords.contains(word);
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
        "Popup", "Timer", "MouseArea", "Repeater", "Loader", "Sparkline", "TabBar", "Toast", "BusyIndicator", "Palette"
    };

    static const QSet<QString> haskellTypes = {
        "Int", "Integer", "Float", "Double", "Bool", "Char", "String", "IO",
        "Maybe", "Either", "Ordering", "FilePath", "Show", "Eq", "Ord", "Monad",
        "Functor", "Applicative"
    };

    static const QSet<QString> goTypes = {
        "int", "int8", "int16", "int32", "int64", "uint", "uint8", "uint16", "uint32", "uint64",
        "float32", "float64", "complex64", "complex128", "string", "bool", "byte", "rune",
        "error", "any", "uintptr"
    };

    if (lang == Language::Cpp) return cppTypes.contains(word);
    if (lang == Language::Qml) return qmlTypes.contains(word);
    if (lang == Language::Haskell) return haskellTypes.contains(word) || (word.length() > 0 && word[0].isUpper());
    if (lang == Language::Go) return goTypes.contains(word);
    return false;
}

QList<StyledSpan> SyntaxHighlighter::highlightLine(const QString &line, Language lang) {
    QList<StyledSpan> spans;
    if (line.isEmpty()) return spans;
    if (lang == Language::None) {
        spans.append({line, MdRole::Code, MdRole::Transparent,
                      TextAttribute::None});
        return spans;
    }

    static const QSet<QString> haskellFunctions = {
        "putStrLn", "print", "putStr", "getLine", "getContents", "intercalate",
        "map", "filter", "foldr", "foldl", "scanl", "scanr", "concat", "concatMap",
        "head", "tail", "init", "last", "null", "length", "reverse", "take", "drop",
        "splitAt", "takeWhile", "dropWhile", "span", "break", "lines", "words",
        "unlines", "unwords", "show", "read", "error", "undefined", "seq", "pure",
        "return", "fmap", "liftA2", "mempty", "mappend", "mconcat"
    };

    int i = 0;
    const int len = line.length();

    auto flushToken = [&](const QString &token, MdRole role,
                          TextAttribute attr = TextAttribute::None) {
        if (!token.isEmpty())
            spans.append({token, role, MdRole::Transparent, attr});
    };

    while (i < len) {
        // Comments
        // C++/QML/JS/Rust/Go line comment //
        if ((lang == Language::Cpp || lang == Language::Qml || lang == Language::JavaScript ||
             lang == Language::Rust || lang == Language::Go) &&
            i + 1 < len && line[i] == '/' && line[i + 1] == '/') {
            flushToken(line.mid(i), MdRole::SyntaxComment, TextAttribute::Italic);
            break;
        }

        // Haskell line comment --
        if (lang == Language::Haskell && i + 1 < len && line[i] == '-' && line[i + 1] == '-') {
            flushToken(line.mid(i), MdRole::SyntaxComment, TextAttribute::Italic);
            break;
        }

        // Python/Bash/CMake/Yaml line comment #
        if ((lang == Language::Python || lang == Language::Bash || lang == Language::CMake ||
             lang == Language::Yaml) && line[i] == '#') {
            flushToken(line.mid(i), MdRole::SyntaxComment, TextAttribute::Italic);
            break;
        }

        // C++ Preprocessor directive (#include, #pragma, etc.)
        if (lang == Language::Cpp && line[i] == '#' && (i == 0 || line.left(i).trimmed().isEmpty())) {
            flushToken(line.mid(i), MdRole::SyntaxPreprocessor, TextAttribute::Bold);
            break;
        }

        // Multi-character symbols & operators (e.g. ::, ->, <-, =>, <$>, <*>, <>, ++, ==, !=, <=, >=, &&, ||, <<, >>, $, $)
        if (i + 2 < len) {
            const QString tri = line.mid(i, 3);
            if (tri == "<$>" || tri == "<*>" || tri == "...") {
                flushToken(tri, MdRole::SyntaxSymbol);
                i += 3;
                continue;
            }
        }
        if (i + 1 < len) {
            const QString duo = line.mid(i, 2);
            if (duo == "::" || duo == "->" || duo == "<-" || duo == "=>" || duo == "<>" ||
                duo == "++" || duo == "==" || duo == "!=" || duo == "<=" || duo == ">=" ||
                duo == "&&" || duo == "||" || duo == "<<" || duo == ">>" || duo == ":=" ||
                duo == "+=" || duo == "-=" || duo == "*=" || duo == "/=") {
                flushToken(duo, MdRole::SyntaxSymbol);
                i += 2;
                continue;
            }
        }

        // Single character operators like $
        if (line[i] == '$' && lang == Language::Haskell) {
            flushToken(QStringLiteral("$"), MdRole::SyntaxSymbol);
            i++;
            continue;
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
            flushToken(line.mid(strStart, i - strStart), MdRole::SyntaxString);
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
            flushToken(line.mid(numStart, i - numStart), MdRole::SyntaxNumber);
            continue;
        }

        // Identifiers & Keywords
        if (line[i].isLetter() || line[i] == '_' || line[i] == '$') {
            int wordStart = i;
            while (i < len && (line[i].isLetterOrNumber() || line[i] == '_' || line[i] == '$' || line[i] == '.' || line[i] == '\'')) {
                // If dot is followed by letter (like Data.List), consume it as part of qualified name
                if (line[i] == '.' && (i + 1 >= len || !line[i + 1].isLetter()))
                    break;
                i++;
            }
            QString word = line.mid(wordStart, i - wordStart);
            if (isKeyword(word, lang)) {
                flushToken(word, MdRole::SyntaxKeyword, TextAttribute::Bold);
            } else if (isType(word, lang) || (word.length() > 1 && word[0].isUpper())) {
                flushToken(word, MdRole::SyntaxType);
            } else if (lang == Language::Haskell && (haskellFunctions.contains(word) || wordStart == 0 || line.mid(i).trimmed().startsWith("::") || line.mid(i).trimmed().startsWith("="))) {
                flushToken(word, MdRole::SyntaxFunction);
            } else if (i < len && line[i] == '(') {
                flushToken(word, MdRole::SyntaxFunction);
            } else {
                flushToken(word, MdRole::Code);
            }
            continue;
        }

        // Symbols / Punctuation
        if (line[i] == '{' || line[i] == '}' || line[i] == '(' || line[i] == ')' ||
            line[i] == '[' || line[i] == ']' || line[i] == ';' || line[i] == ',' ||
            line[i] == '=' || line[i] == '+' || line[i] == '-' || line[i] == '*' ||
            line[i] == '/' || line[i] == '&' || line[i] == '|' || line[i] == '!' ||
            line[i] == '<' || line[i] == '>' || line[i] == ':' || line[i] == '\\') {
            flushToken(QString(line[i]), MdRole::SyntaxSymbol);
            i++;
            continue;
        }

        // Whitespace and other characters
        int wsStart = i;
        while (i < len && line[i].isSpace()) {
            i++;
        }
        if (i > wsStart) {
            flushToken(line.mid(wsStart, i - wsStart), MdRole::Code);
        } else {
            flushToken(QString(line[i]), MdRole::Code);
            i++;
        }
    }

    return spans;
}

} // namespace qamui
