#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/TextStyle.h>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>
#include <utility>

namespace qmltui {

// UTF-16 character range [start, start + length) paired with a TextStyle
// overlay, used by TextArea::highlights for syntax and search highlighting.
// Unspecified style attributes inherit from the owning TextArea.
class QMLTUI_CORE_EXPORT HighlightRange {
    Q_GADGET
    QML_VALUE_TYPE(highlightRange)
    QML_STRUCTURED_VALUE

    Q_PROPERTY(int start READ start WRITE setStart)
    Q_PROPERTY(int length READ length WRITE setLength)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor RESET resetColor)
    Q_PROPERTY(qmltui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor RESET
                   resetBackgroundColor)
    Q_PROPERTY(bool bold READ bold WRITE setBold RESET resetBold)
    Q_PROPERTY(bool dim READ dim WRITE setDim RESET resetDim)
    Q_PROPERTY(bool italic READ italic WRITE setItalic RESET resetItalic)
    Q_PROPERTY(bool underline READ underline WRITE setUnderline RESET resetUnderline)
    Q_PROPERTY(bool reversed READ reversed WRITE setReversed RESET resetReversed)
    Q_PROPERTY(
        bool strikethrough READ strikethrough WRITE setStrikethrough RESET resetStrikethrough)
    Q_PROPERTY(QString link READ link WRITE setLink RESET resetLink)

public:
    HighlightRange() = default;
    HighlightRange(int start, int length, TextStyle style = {}, QString link = {})
        : m_start(start), m_length(length), m_style(std::move(style)), m_link(std::move(link)) {}
    HighlightRange(int start, int length, Color foreground, Color background = Color::Invalid)
        : m_start(start), m_length(length), m_style(foreground, background) {}

    int start() const { return m_start; }
    void setStart(int start) { m_start = start; }

    int length() const { return m_length; }
    void setLength(int length) { m_length = length; }

    TextStyle style() const { return m_style; }
    void setStyle(const TextStyle& style) { m_style = style; }

    Color color() const { return m_style.color(); }
    void setColor(Color color) { m_style.setColor(color); }
    void resetColor() { m_style.resetColor(); }

    Color backgroundColor() const { return m_style.backgroundColor(); }
    void setBackgroundColor(Color color) { m_style.setBackgroundColor(color); }
    void resetBackgroundColor() { m_style.resetBackgroundColor(); }

    bool bold() const { return m_style.bold(); }
    bool hasBold() const { return m_style.hasBold(); }
    void setBold(bool value) { m_style.setBold(value); }
    void resetBold() { m_style.resetBold(); }

    bool dim() const { return m_style.dim(); }
    bool hasDim() const { return m_style.hasDim(); }
    void setDim(bool value) { m_style.setDim(value); }
    void resetDim() { m_style.resetDim(); }

    bool italic() const { return m_style.italic(); }
    bool hasItalic() const { return m_style.hasItalic(); }
    void setItalic(bool value) { m_style.setItalic(value); }
    void resetItalic() { m_style.resetItalic(); }

    bool underline() const { return m_style.underline(); }
    bool hasUnderline() const { return m_style.hasUnderline(); }
    void setUnderline(bool value) { m_style.setUnderline(value); }
    void resetUnderline() { m_style.resetUnderline(); }

    bool reversed() const { return m_style.reversed(); }
    bool hasReversed() const { return m_style.hasReversed(); }
    void setReversed(bool value) { m_style.setReversed(value); }
    void resetReversed() { m_style.resetReversed(); }

    bool strikethrough() const { return m_style.strikethrough(); }
    bool hasStrikethrough() const { return m_style.hasStrikethrough(); }
    void setStrikethrough(bool value) { m_style.setStrikethrough(value); }
    void resetStrikethrough() { m_style.resetStrikethrough(); }

    QString link() const { return m_link; }
    void setLink(const QString& link) { m_link = link; }
    void resetLink() { m_link.clear(); }

    TextAttribute resolveAttributes(TextAttribute base) const {
        return m_style.resolveAttributes(base);
    }

    friend bool operator==(const HighlightRange& lhs, const HighlightRange& rhs) noexcept {
        return lhs.m_start == rhs.m_start && lhs.m_length == rhs.m_length &&
               lhs.m_style == rhs.m_style && lhs.m_link == rhs.m_link;
    }
    friend bool operator!=(const HighlightRange& lhs, const HighlightRange& rhs) noexcept {
        return !(lhs == rhs);
    }

private:
    int m_start = 0;
    int m_length = 0;
    TextStyle m_style;
    QString m_link;
};

} // namespace qmltui

Q_DECLARE_METATYPE(qmltui::HighlightRange)
