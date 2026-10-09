#include <Qtui/Core/primitives/Span.h>

namespace qtui {

void Span::setAttribute(TextAttribute attribute, bool value)
{
    m_specified |= bit(attribute);
    if (value)
        m_values |= bit(attribute);
    else
        m_values &= ~bit(attribute);
}

void Span::resetAttribute(TextAttribute attribute)
{
    m_specified &= ~bit(attribute);
    m_values &= ~bit(attribute);
}

TextAttribute Span::resolveAttributes(TextAttribute base) const
{
    return static_cast<TextAttribute>((bit(base) & ~m_specified) | m_values);
}

} // namespace qtui
