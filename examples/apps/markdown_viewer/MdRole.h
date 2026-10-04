#pragma once

#include <cstdint>

namespace qtui {

enum class MdRole : uint8_t {
    Transparent,
    Text,
    TextMuted,
    Heading1,
    Heading1Bg,
    Heading2,
    Heading3,
    Heading4,
    Heading5,
    Heading6,
    Link,
    Code,
    CodeBackground,
    InlineCode,
    InlineCodeBackground,
    Quote,
    Border,
    Success,
    Warning,
    SyntaxKeyword,
    SyntaxType,
    SyntaxFunction,
    SyntaxString,
    SyntaxComment,
    SyntaxNumber,
    SyntaxPreprocessor,
    SyntaxSymbol
};

} // namespace qtui
