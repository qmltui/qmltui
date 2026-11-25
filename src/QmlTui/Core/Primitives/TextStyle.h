#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/Cell.h>
#include <QVariant>
#include <QtGlobal>
#include <cstdint>

namespace qmltui {

// Sparse character style overlay shared by Span and HighlightRange.
//
// Each boolean attribute tracks both its value (m_values) and whether it was
// explicitly set (m_specified), so unset attributes inherit from the owning
// item while an explicit false clears an inherited attribute.
class QMLTUI_CORE_EXPORT TextStyle {
public:
    TextStyle() = default;
    TextStyle(Color foreground, Color background = Color::Invalid)
        : m_color(foreground), m_backgroundColor(background) {}

    Color color() const { return m_color; }
    void setColor(Color color) { m_color = color; }
    void resetColor() { m_color = Color::Invalid; }

    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(Color color) { m_backgroundColor = color; }
    void resetBackgroundColor() { m_backgroundColor = Color::Invalid; }

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

    bool strikethrough() const { return (m_values & bit(TextAttribute::Strikethrough)) != 0; }
    bool hasStrikethrough() const { return (m_specified & bit(TextAttribute::Strikethrough)) != 0; }
    void setStrikethrough(bool value) { setAttribute(TextAttribute::Strikethrough, value); }
    void resetStrikethrough() { resetAttribute(TextAttribute::Strikethrough); }

    bool isEmpty() const {
        return !m_color.isValid() && !m_backgroundColor.isValid() && m_specified == 0;
    }

    // Applies specified attributes onto base, clearing bits explicitly set to false.
    TextAttribute resolveAttributes(TextAttribute base) const;
    uint32_t resolveForeground(uint32_t baseFg) const {
        return m_color.isValid() && !m_color.isTransparent() ? m_color.argb32() : baseFg;
    }
    uint32_t resolveBackground(uint32_t baseBg) const {
        return m_backgroundColor.isValid() && !m_backgroundColor.isTransparent()
                   ? m_backgroundColor.argb32()
                   : baseBg;
    }
    // Overwrites only the colors and attributes that overlay explicitly specifies.
    void applyOverlay(const TextStyle& overlay);
    // Applies style properties from a QVariantMap ({ color, backgroundColor, bold, dim, italic,
    // underline, reversed }).
    void applyVariantMap(const QVariantMap& map, bool warnOnInvalid = false);

    friend bool operator==(const TextStyle& lhs, const TextStyle& rhs) noexcept {
        return lhs.m_color == rhs.m_color && lhs.m_backgroundColor == rhs.m_backgroundColor &&
               lhs.m_values == rhs.m_values && lhs.m_specified == rhs.m_specified;
    }
    friend bool operator!=(const TextStyle& lhs, const TextStyle& rhs) noexcept {
        return !(lhs == rhs);
    }

private:
    static constexpr quint8 bit(TextAttribute attribute) { return static_cast<quint8>(attribute); }
    void setAttribute(TextAttribute attribute, bool value);
    void resetAttribute(TextAttribute attribute);

    Color m_color = Color::Invalid;
    Color m_backgroundColor = Color::Invalid;
    quint8 m_values = 0;
    quint8 m_specified = 0;
};

} // namespace qmltui
