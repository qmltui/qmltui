#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>
#include <Qamui/Core/Primitives/Color.h>
#include <Qamui/Core/Primitives/Cell.h>
#include <utility>

namespace qamui {

// A literal text run. Attribute presence is distinct from its boolean value.
class QAMUI_CORE_EXPORT Span
{
    Q_GADGET
    QML_VALUE_TYPE(span)
    QML_STRUCTURED_VALUE

    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(qamui::Color color READ color WRITE setColor RESET resetColor)
    Q_PROPERTY(qamui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor RESET resetBackgroundColor)
    Q_PROPERTY(bool bold READ bold WRITE setBold RESET resetBold)
    Q_PROPERTY(bool dim READ dim WRITE setDim RESET resetDim)
    Q_PROPERTY(bool italic READ italic WRITE setItalic RESET resetItalic)
    Q_PROPERTY(bool underline READ underline WRITE setUnderline RESET resetUnderline)
    Q_PROPERTY(bool reversed READ reversed WRITE setReversed RESET resetReversed)

public:
    Span() = default;
    explicit Span(QString text) : m_text(std::move(text)) {}
    Span(QString text, Color foreground, Color background = {})
        : m_text(std::move(text)), m_color(foreground), m_backgroundColor(background) {}

    QString text() const { return m_text; }
    void setText(const QString &text) { m_text = text; }
    Color color() const { return m_color; }
    void setColor(Color color) { m_color = color; }
    void resetColor() { m_color = {}; }
    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(Color color) { m_backgroundColor = color; }
    void resetBackgroundColor() { m_backgroundColor = {}; }

    bool bold() const { return (m_values & bit(TextAttribute::Bold)) != 0; }
    bool hasBold() const { return (m_specified & bit(TextAttribute::Bold)) != 0; }
    void setBold(bool value) { setAttribute(TextAttribute::Bold, value); }
    void resetBold() { resetAttribute(TextAttribute::Bold); }
    bool dim() const { return (m_values & bit(TextAttribute::Dim)) != 0; }
    bool hasDim() const { return (m_specified & bit(TextAttribute::Dim)) != 0; }
    void setDim(bool value) { setAttribute(TextAttribute::Dim, value); }
    void resetDim() { resetAttribute(TextAttribute::Dim); }
    bool italic() const { return (m_values & bit(TextAttribute::Italic)) != 0; }
    bool hasItalic() const { return (m_specified & bit(TextAttribute::Italic)) != 0; }
    void setItalic(bool value) { setAttribute(TextAttribute::Italic, value); }
    void resetItalic() { resetAttribute(TextAttribute::Italic); }
    bool underline() const { return (m_values & bit(TextAttribute::Underline)) != 0; }
    bool hasUnderline() const { return (m_specified & bit(TextAttribute::Underline)) != 0; }
    void setUnderline(bool value) { setAttribute(TextAttribute::Underline, value); }
    void resetUnderline() { resetAttribute(TextAttribute::Underline); }
    bool reversed() const { return (m_values & bit(TextAttribute::Reversed)) != 0; }
    bool hasReversed() const { return (m_specified & bit(TextAttribute::Reversed)) != 0; }
    void setReversed(bool value) { setAttribute(TextAttribute::Reversed, value); }
    void resetReversed() { resetAttribute(TextAttribute::Reversed); }

    TextAttribute resolveAttributes(TextAttribute base) const;
    friend bool operator==(const Span &, const Span &) = default;

private:
    static constexpr quint8 bit(TextAttribute attribute) { return static_cast<quint8>(attribute); }
    void setAttribute(TextAttribute attribute, bool value);
    void resetAttribute(TextAttribute attribute);

    QString m_text;
    Color m_color;
    Color m_backgroundColor;
    quint8 m_values = 0;
    quint8 m_specified = 0;
};

} // namespace qamui

Q_DECLARE_METATYPE(qamui::Span)
