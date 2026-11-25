#include <QmlTui/Core/Primitives/QmlTuiGlobal.h>
#include <QDebug>

namespace qmltui {

Color QmlTuiGlobal::color(const QString& value) const {
    return Color::fromString(value);
}

Color QmlTuiGlobal::rgba(qreal red, qreal green, qreal blue, qreal alpha) const {
    return Color::fromRgba(red, green, blue, alpha);
}

Color QmlTuiGlobal::ansi(int index) const {
    return Color::fromAnsiIndex(index);
}

Span QmlTuiGlobal::span(const QString& text) const {
    return Span(text);
}

Span QmlTuiGlobal::span(const QString& text, const QVariantMap& options) const {
    QString link;
    QVariantMap styleOptions = options;
    if (styleOptions.contains(QStringLiteral("link"))) {
        const QVariant linkVal = styleOptions.take(QStringLiteral("link"));
        if (linkVal.metaType() == QMetaType::fromType<QString>()) {
            link = linkVal.toString();
        } else {
            qWarning().noquote() << "QmlTui.span: expected string for link";
        }
    }
    TextStyle style;
    style.applyVariantMap(styleOptions, /*warnOnInvalid=*/true);
    return Span(text, style, std::move(link));
}

} // namespace qmltui
