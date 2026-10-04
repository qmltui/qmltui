#pragma once

#include <cstdint>

namespace qtui {

enum class MarkdownRole : uint8_t {
    Transparent,
    Text,
    TextMuted,
    Heading1,
    Heading2,
    Heading3,
    Heading4,
    Heading5,
    Heading6,
    Link,
    Code,
    CodeBackground,
    Quote,
    Border,
    Success,
    Warning,
    SyntaxKeyword,
    SyntaxType,
    SyntaxString,
    SyntaxComment,
    SyntaxNumber,
    SyntaxPreprocessor,
    SyntaxSymbol
};

} // namespace qtui
