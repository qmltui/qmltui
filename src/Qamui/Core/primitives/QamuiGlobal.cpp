#include <Qamui/Core/primitives/QamuiGlobal.h>

#include <QDebug>

namespace qamui {

Color QamuiGlobal::rgba(qreal red, qreal green, qreal blue, qreal alpha) const
{
    return Color::fromRgba(red, green, blue, alpha);
}


Span QamuiGlobal::span(const QString &text) const
{
    return Span(text);
}

Span QamuiGlobal::span(const QString &text, const QVariantMap &options) const
{
    Span result(text);
    for (auto it = options.cbegin(); it != options.cend(); ++it) {
        const QString &key = it.key();
        const QVariant &value = it.value();
        if (key == "color" || key == "backgroundColor") {
            Color color;
            if (value.metaType() == QMetaType::fromType<Color>())
                color = value.value<Color>();
            else if (value.metaType() == QMetaType::fromType<QString>())
                color = Color(value.toString());
            if (!color.isValid()) {
                qWarning().noquote() << "Qamui.span: invalid color for" << key;
                continue;
            }
            if (key == "color") result.setColor(color);
            else result.setBackgroundColor(color);
            continue;
        }
        if (key == "bold") {
            if (value.metaType() == QMetaType::fromType<bool>()) result.setBold(value.toBool());
            else qWarning().noquote() << "Qamui.span: expected boolean for" << key;
            continue;
        }
        if (key == "dim") {
            if (value.metaType() == QMetaType::fromType<bool>()) result.setDim(value.toBool());
            else qWarning().noquote() << "Qamui.span: expected boolean for" << key;
            continue;
        }
        if (key == "italic") {
            if (value.metaType() == QMetaType::fromType<bool>()) result.setItalic(value.toBool());
            else qWarning().noquote() << "Qamui.span: expected boolean for" << key;
            continue;
        }
        if (key == "underline") {
            if (value.metaType() == QMetaType::fromType<bool>()) result.setUnderline(value.toBool());
            else qWarning().noquote() << "Qamui.span: expected boolean for" << key;
            continue;
        }
        if (key == "reversed") {
            if (value.metaType() == QMetaType::fromType<bool>()) result.setReversed(value.toBool());
            else qWarning().noquote() << "Qamui.span: expected boolean for" << key;
            continue;
        }
        qWarning().noquote() << "Qamui.span: unknown option" << key;
    }
    return result;
}

} // namespace qamui
