#pragma once

#include <QMetaType>
#include <QtQml/qqmlregistration.h>
#include <Qtui/Core/primitives/ColorMapper.h>

#include "MdRole.h"

namespace qtui {

class MdPalette {
    Q_GADGET
    QML_VALUE_TYPE(mdPalette)

    Q_PROPERTY(QString text MEMBER m_text)
    Q_PROPERTY(QString textMuted MEMBER m_textMuted)
    Q_PROPERTY(QString heading MEMBER m_heading)
    Q_PROPERTY(QString heading1Bg MEMBER m_heading1Bg)
    Q_PROPERTY(QString heading2 MEMBER m_heading2)
    Q_PROPERTY(QString heading3 MEMBER m_heading3)
    Q_PROPERTY(QString heading4 MEMBER m_heading4)
    Q_PROPERTY(QString heading5 MEMBER m_heading5)
    Q_PROPERTY(QString code MEMBER m_code)
    Q_PROPERTY(QString codeBg MEMBER m_codeBg)
    Q_PROPERTY(QString inlineCode MEMBER m_inlineCode)
    Q_PROPERTY(QString inlineCodeBg MEMBER m_inlineCodeBg)
    Q_PROPERTY(QString quote MEMBER m_quote)
    Q_PROPERTY(QString border MEMBER m_border)
    Q_PROPERTY(QString link MEMBER m_link)
    Q_PROPERTY(QString success MEMBER m_success)
    Q_PROPERTY(QString warning MEMBER m_warning)
    Q_PROPERTY(QString syntaxKeyword MEMBER m_syntaxKeyword)
    Q_PROPERTY(QString syntaxType MEMBER m_syntaxType)
    Q_PROPERTY(QString syntaxFunction MEMBER m_syntaxFunction)
    Q_PROPERTY(QString syntaxString MEMBER m_syntaxString)
    Q_PROPERTY(QString syntaxComment MEMBER m_syntaxComment)
    Q_PROPERTY(QString syntaxNumber MEMBER m_syntaxNumber)
    Q_PROPERTY(QString syntaxPreprocessor MEMBER m_syntaxPreprocessor)
    Q_PROPERTY(QString syntaxSymbol MEMBER m_syntaxSymbol)
    Q_PROPERTY(QString searchMatchBg MEMBER m_searchMatchBg)
    Q_PROPERTY(QString searchMatchFg MEMBER m_searchMatchFg)
    Q_PROPERTY(QString activeMatchBg MEMBER m_activeMatchBg)
    Q_PROPERTY(QString activeMatchFg MEMBER m_activeMatchFg)

public:
    MdPalette() = default;
    ~MdPalette() = default;

    bool operator==(const MdPalette &other) const = default;

    uint32_t resolvedText() const { return ColorMapper::fromString(m_text, 0xFFC0CAF5); }
    uint32_t resolvedTextMuted() const { return ColorMapper::fromString(m_textMuted, 0xFF565F89); }
    uint32_t resolvedHeading() const { return ColorMapper::fromString(m_heading, 0xFF7AA2F7); }
    uint32_t resolvedHeading1Bg() const { return ColorMapper::fromString(m_heading1Bg, 0xFF5856D6); }
    uint32_t resolvedHeading2() const { return ColorMapper::fromString(m_heading2, 0xFFE0AF68); }
    uint32_t resolvedHeading3() const { return ColorMapper::fromString(m_heading3, 0xFFBB9AF7); }
    uint32_t resolvedHeading4() const { return ColorMapper::fromString(m_heading4, 0xFF9ECE6A); }
    uint32_t resolvedHeading5() const { return ColorMapper::fromString(m_heading5, 0xFF7DCFFF); }
    uint32_t resolvedCode() const { return ColorMapper::fromString(m_code, 0xFFE0AF68); }
    uint32_t resolvedCodeBg() const { return ColorMapper::fromString(m_codeBg, 0xFF24283B); }
    uint32_t resolvedInlineCode() const { return ColorMapper::fromString(m_inlineCode, resolvedCode()); }
    uint32_t resolvedInlineCodeBg() const { return ColorMapper::fromString(m_inlineCodeBg, 0xFF4C1D24); }
    uint32_t resolvedQuote() const { return ColorMapper::fromString(m_quote, 0xFF7DCFFF); }
    uint32_t resolvedBorder() const { return ColorMapper::fromString(m_border, 0xFF414868); }
    uint32_t resolvedLink() const { return ColorMapper::fromString(m_link, 0xFF7AA2F7); }
    uint32_t resolvedSuccess() const { return ColorMapper::fromString(m_success, 0xFF9ECE6A); }
    uint32_t resolvedWarning() const { return ColorMapper::fromString(m_warning, 0xFFE0AF68); }
    uint32_t resolvedSearchMatchBg() const { return ColorMapper::fromString(m_searchMatchBg, 0xFFE0AF68); }
    uint32_t resolvedSearchMatchFg() const { return ColorMapper::fromString(m_searchMatchFg, 0xFF1A1B26); }
    uint32_t resolvedActiveMatchBg() const { return ColorMapper::fromString(m_activeMatchBg, 0xFF7AA2F7); }
    uint32_t resolvedActiveMatchFg() const { return ColorMapper::fromString(m_activeMatchFg, 0xFF1A1B26); }
    uint32_t resolvedSyntaxFunction() const { return ColorMapper::fromString(m_syntaxFunction, 0xFF50FA7B); }

    uint32_t resolve(MdRole role) const {
        switch (role) {
        case MdRole::Transparent: return 0x00000000;
        case MdRole::Text: return resolvedText();
        case MdRole::TextMuted: return resolvedTextMuted();
        case MdRole::Heading1: return resolvedHeading();
        case MdRole::Heading1Bg: return resolvedHeading1Bg();
        case MdRole::Heading2: return resolvedHeading2();
        case MdRole::Heading3: return resolvedHeading3();
        case MdRole::Heading4: return resolvedHeading4();
        case MdRole::Heading5: return resolvedHeading5();
        case MdRole::Heading6: return resolvedTextMuted();
        case MdRole::Link: return resolvedLink();
        case MdRole::Code: return resolvedCode();
        case MdRole::CodeBackground: return resolvedCodeBg();
        case MdRole::InlineCode: return resolvedInlineCode();
        case MdRole::InlineCodeBackground: return resolvedInlineCodeBg();
        case MdRole::Quote: return resolvedQuote();
        case MdRole::Border: return resolvedBorder();
        case MdRole::Success: return resolvedSuccess();
        case MdRole::Warning: return resolvedWarning();
        case MdRole::SyntaxKeyword:
            return ColorMapper::fromString(m_syntaxKeyword, 0xFFBB9AF7);
        case MdRole::SyntaxType:
            return ColorMapper::fromString(m_syntaxType, 0xFF7DCFFF);
        case MdRole::SyntaxFunction:
            return resolvedSyntaxFunction();
        case MdRole::SyntaxString:
            return ColorMapper::fromString(m_syntaxString, 0xFF9ECE6A);
        case MdRole::SyntaxComment:
            return ColorMapper::fromString(m_syntaxComment, 0xFF565F89);
        case MdRole::SyntaxNumber:
            return ColorMapper::fromString(m_syntaxNumber, 0xFFE0AF68);
        case MdRole::SyntaxPreprocessor:
            return ColorMapper::fromString(m_syntaxPreprocessor, 0xFFFF9E64);
        case MdRole::SyntaxSymbol:
            return ColorMapper::fromString(m_syntaxSymbol, 0xFFF7768E);
        }
        return resolvedText();
    }

    QString m_text = "#c0caf5";
    QString m_textMuted = "#565f89";
    QString m_heading = "#7aa2f7";
    QString m_heading1Bg = "#5856d6";
    QString m_heading2 = "#e0af68";
    QString m_heading3 = "#bb9af7";
    QString m_heading4 = "#9ece6a";
    QString m_heading5 = "#7dcfff";
    QString m_code = "#e0af68";
    QString m_codeBg = "#24283b";
    QString m_inlineCode = "#f43f5e";
    QString m_inlineCodeBg = "#4c1d24";
    QString m_quote = "#7dcfff";
    QString m_border = "#414868";
    QString m_link = "#7aa2f7";
    QString m_success = "#9ece6a";
    QString m_warning = "#e0af68";
    QString m_syntaxKeyword = "#bb9af7";
    QString m_syntaxType = "#7dcfff";
    QString m_syntaxFunction = "#50fa7b";
    QString m_syntaxString = "#9ece6a";
    QString m_syntaxComment = "#565f89";
    QString m_syntaxNumber = "#e0af68";
    QString m_syntaxPreprocessor = "#ff9e64";
    QString m_syntaxSymbol = "#f7768e";
    QString m_searchMatchBg = "#e0af68";
    QString m_searchMatchFg = "#1a1b26";
    QString m_activeMatchBg = "#7aa2f7";
    QString m_activeMatchFg = "#1a1b26";
};

} // namespace qtui

Q_DECLARE_METATYPE(qtui::MdPalette)
