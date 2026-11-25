#include <QmlTui/Core/Primitives/TextStyle.h>
#include <QDebug>

namespace qmltui {

void TextStyle::setAttribute(TextAttribute attribute, bool value) {
    m_specified |= bit(attribute);
    if (value)
        m_values |= bit(attribute);
    else
        m_values &= ~bit(attribute);
}

void TextStyle::resetAttribute(TextAttribute attribute) {
    m_specified &= ~bit(attribute);
    m_values &= ~bit(attribute);
}

TextAttribute TextStyle::resolveAttributes(TextAttribute base) const {
    return static_cast<TextAttribute>((bit(base) & ~m_specified) | m_values);
}

void TextStyle::applyOverlay(const TextStyle& overlay) {
    if (overlay.m_color.isValid()) m_color = overlay.m_color;
    if (overlay.m_backgroundColor.isValid()) m_backgroundColor = overlay.m_backgroundColor;

    m_specified |= overlay.m_specified;
    m_values = (m_values & ~overlay.m_specified) | (overlay.m_values & overlay.m_specified);
}

void TextStyle::applyVariantMap(const QVariantMap& map, bool warnOnInvalid) {
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();
        if (!warnOnInvalid && (key == QStringLiteral("text") || key == QStringLiteral("link")))
            continue;
        if (key == QStringLiteral("color") || key == QStringLiteral("backgroundColor")) {
            Color color = Color::Invalid;
            if (value.metaType() == QMetaType::fromType<Color>())
                color = value.value<Color>();
            else if (value.metaType() == QMetaType::fromType<QString>())
                color = Color(value.toString());
            if (!color.isValid()) {
                if (warnOnInvalid) qWarning().noquote() << "QmlTui.span: invalid color for" << key;
                continue;
            }
            if (key == QStringLiteral("color"))
                setColor(color);
            else
                setBackgroundColor(color);
            continue;
        }
        const auto applyBool = [&](TextAttribute attr) {
            if (value.metaType() == QMetaType::fromType<bool>())
                setAttribute(attr, value.toBool());
            else if (warnOnInvalid)
                qWarning().noquote() << "QmlTui.span: expected boolean for" << key;
        };
        if (key == QStringLiteral("bold")) {
            applyBool(TextAttribute::Bold);
        } else if (key == QStringLiteral("dim")) {
            applyBool(TextAttribute::Dim);
        } else if (key == QStringLiteral("italic")) {
            applyBool(TextAttribute::Italic);
        } else if (key == QStringLiteral("underline")) {
            applyBool(TextAttribute::Underline);
        } else if (key == QStringLiteral("reversed")) {
            applyBool(TextAttribute::Reversed);
        } else if (key == QStringLiteral("strikethrough")) {
            applyBool(TextAttribute::Strikethrough);
        } else if (warnOnInvalid) {
            qWarning().noquote() << "QmlTui.span: unknown option" << key;
        }
    }
}

} // namespace qmltui
