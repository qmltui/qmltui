#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/Cell.h>
#include <QtGlobal>

namespace qmltui {

class QMLTUI_CORE_EXPORT TextStyle
{
public:
    TextStyle() = default;
    TextStyle(Color foreground, Color background = {})
        : m_color(foreground), m_backgroundColor(background) {}

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

    bool isEmpty() const {
        return !m_color.isValid() && !m_backgroundColor.isValid() && m_specified == 0;
    }

    TextAttribute resolveAttributes(TextAttribute base) const;
    void applyOverlay(const TextStyle &overlay);

    friend bool operator==(const TextStyle &, const TextStyle &) = default;

private:
    static constexpr quint8 bit(TextAttribute attribute) { return static_cast<quint8>(attribute); }
    void setAttribute(TextAttribute attribute, bool value);
    void resetAttribute(TextAttribute attribute);

    Color m_color;
    Color m_backgroundColor;
    quint8 m_values = 0;
    quint8 m_specified = 0;
};

} // namespace qmltui
