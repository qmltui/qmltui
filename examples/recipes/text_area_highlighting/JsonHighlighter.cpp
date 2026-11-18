#include "JsonHighlighter.h"

namespace qmltui::demo {

JsonHighlighter::JsonHighlighter(QObject* parent) : QObject(parent) {}

JsonHighlighter::~JsonHighlighter() {
    detachTarget();
}

void JsonHighlighter::detachTarget() {
    if (m_target) {
        disconnect(m_target.data(), &TextArea::textChanged, this,
                   &JsonHighlighter::onTargetTextChanged);
        m_target->clearHighlightLayer(QStringLiteral("syntax"));
        m_target->clearHighlightLayer(QStringLiteral("search"));
    }
}

void JsonHighlighter::setTarget(TextArea* target) {
    if (m_target == target) return;

    detachTarget();
    m_target = target;

    if (m_target) {
        connect(m_target.data(), &TextArea::textChanged, this,
                &JsonHighlighter::onTargetTextChanged);
        tokenize();
        applySyntaxHighlighting();
        applySearchHighlighting();
    } else {
        m_cachedTokens.clear();
    }

    emit targetChanged();
}

void JsonHighlighter::setSearchTerm(const QString& term) {
    if (m_searchTerm == term) return;

    m_searchTerm = term;
    emit searchTermChanged();
    applySearchHighlighting();
}

void JsonHighlighter::setStringColor(Color color) {
    if (m_stringColor == color) return;
    m_stringColor = color;
    emit stylesChanged();
    applySyntaxHighlighting();
}

void JsonHighlighter::setNumberColor(Color color) {
    if (m_numberColor == color) return;
    m_numberColor = color;
    emit stylesChanged();
    applySyntaxHighlighting();
}

void JsonHighlighter::setKeywordColor(Color color) {
    if (m_keywordColor == color) return;
    m_keywordColor = color;
    emit stylesChanged();
    applySyntaxHighlighting();
}

void JsonHighlighter::setCommentColor(Color color) {
    if (m_commentColor == color) return;
    m_commentColor = color;
    emit stylesChanged();
    applySyntaxHighlighting();
}

void JsonHighlighter::setSearchBgColor(Color color) {
    if (m_searchBgColor == color) return;
    m_searchBgColor = color;
    emit stylesChanged();
    applySearchHighlighting();
}

void JsonHighlighter::rehighlight() {
    tokenize();
    applySyntaxHighlighting();
    applySearchHighlighting();
}

void JsonHighlighter::onTargetTextChanged() {
    tokenize();
    applySyntaxHighlighting();
    applySearchHighlighting();
}

void JsonHighlighter::tokenize() {
    m_cachedTokens.clear();
    if (!m_target) return;

    const QString text = m_target->text();
    const int len = static_cast<int>(text.size());
    int i = 0;

    while (i < len) {
        // Skip whitespace
        if (text[i].isSpace()) {
            ++i;
            continue;
        }

        // Line comment: //
        if (i + 1 < len && text[i] == u'/' && text[i + 1] == u'/') {
            const int start = i;
            i += 2;
            while (i < len && text[i] != u'\n')
                ++i;
            m_cachedTokens.append(Token{start, i - start, Comment});
            continue;
        }

        // Block comment: /* ... */
        if (i + 1 < len && text[i] == u'/' && text[i + 1] == u'*') {
            const int start = i;
            i += 2;
            while (i + 1 < len && !(text[i] == u'*' && text[i + 1] == u'/'))
                ++i;
            if (i + 1 < len)
                i += 2; // skip */
            else
                i = len;
            m_cachedTokens.append(Token{start, i - start, Comment});
            continue;
        }

        // String: "..."
        if (text[i] == u'"') {
            const int start = i;
            ++i;
            while (i < len) {
                if (text[i] == u'\\') {
                    i += 2; // skip escaped char
                } else if (text[i] == u'"') {
                    ++i;
                    break;
                } else {
                    ++i;
                }
            }
            // A trailing backslash skips past the end; an out-of-range token
            // would make setHighlightLayer() reject the whole layer.
            i = qMin(i, len);
            m_cachedTokens.append(Token{start, i - start, String});
            continue;
        }

        // Numbers: -?[0-9]...
        if (text[i].isDigit() || (text[i] == u'-' && i + 1 < len && text[i + 1].isDigit())) {
            const int start = i;
            if (text[i] == u'-') ++i;
            while (i < len && (text[i].isDigit() || text[i] == u'.' || text[i] == u'e' ||
                               text[i] == u'E' || text[i] == u'+' || text[i] == u'-')) {
                // simple number scanner
                if ((text[i] == u'+' || text[i] == u'-') && i > start && text[i - 1] != u'e' &&
                    text[i - 1] != u'E')
                    break;
                ++i;
            }
            m_cachedTokens.append(Token{start, i - start, Number});
            continue;
        }

        // Keywords: true, false, null
        if (text.sliced(i).startsWith(QStringLiteral("true")) &&
            (i + 4 == len || !text[i + 4].isLetterOrNumber())) {
            m_cachedTokens.append(Token{i, 4, Keyword});
            i += 4;
            continue;
        }
        if (text.sliced(i).startsWith(QStringLiteral("false")) &&
            (i + 5 == len || !text[i + 5].isLetterOrNumber())) {
            m_cachedTokens.append(Token{i, 5, Keyword});
            i += 5;
            continue;
        }
        if (text.sliced(i).startsWith(QStringLiteral("null")) &&
            (i + 4 == len || !text[i + 4].isLetterOrNumber())) {
            m_cachedTokens.append(Token{i, 4, Keyword});
            i += 4;
            continue;
        }

        // Any other character
        ++i;
    }
}

void JsonHighlighter::applySyntaxHighlighting() {
    if (!m_target) return;

    QList<HighlightRange> ranges;
    ranges.reserve(m_cachedTokens.size());

    for (const Token& tok : m_cachedTokens) {
        HighlightRange r(tok.start, tok.length);
        switch (tok.type) {
        case String:
            r.setColor(m_stringColor);
            break;
        case Number:
            r.setColor(m_numberColor);
            break;
        case Keyword:
            r.setColor(m_keywordColor);
            r.setBold(true);
            break;
        case Comment:
            r.setColor(m_commentColor);
            r.setItalic(true);
            break;
        }
        ranges.append(r);
    }

    m_target->setHighlightLayer(QStringLiteral("syntax"), ranges, m_target->textRevision(), 0);
}

void JsonHighlighter::applySearchHighlighting() {
    if (!m_target) return;

    if (m_searchTerm.isEmpty()) {
        m_target->clearHighlightLayer(QStringLiteral("search"));
        return;
    }

    const QString text = m_target->text();
    QList<HighlightRange> searchRanges;
    int index = 0;
    while ((index = text.indexOf(m_searchTerm, index, Qt::CaseInsensitive)) != -1) {
        HighlightRange r(index, static_cast<int>(m_searchTerm.size()));
        r.setBackgroundColor(m_searchBgColor);
        r.setBold(true);
        searchRanges.append(r);
        index += static_cast<int>(m_searchTerm.size());
    }

    m_target->setHighlightLayer(QStringLiteral("search"), searchRanges, m_target->textRevision(),
                                10);
}

} // namespace qmltui::demo
