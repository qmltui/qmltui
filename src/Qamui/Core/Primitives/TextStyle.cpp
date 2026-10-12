#include <Qamui/Core/Primitives/TextStyle.h>

namespace qamui {

void TextStyle::setAttribute(TextAttribute attribute, bool value)
{
    m_specified |= bit(attribute);
    if (value)
        m_values |= bit(attribute);
    else
        m_values &= ~bit(attribute);
}

void TextStyle::resetAttribute(TextAttribute attribute)
{
    m_specified &= ~bit(attribute);
    m_values &= ~bit(attribute);
}

TextAttribute TextStyle::resolveAttributes(TextAttribute base) const
{
    return static_cast<TextAttribute>((bit(base) & ~m_specified) | m_values);
}

void TextStyle::applyOverlay(const TextStyle &overlay)
{
    if (overlay.m_color.isValid())
        m_color = overlay.m_color;
    if (overlay.m_backgroundColor.isValid())
        m_backgroundColor = overlay.m_backgroundColor;

    m_specified |= overlay.m_specified;
    m_values = (m_values & ~overlay.m_specified) | (overlay.m_values & overlay.m_specified);
}

} // namespace qamui
