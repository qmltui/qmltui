#include <QmlTui/Core/Primitives/QmlTuiGlobal.h>

namespace qmltui {

Color QmlTuiGlobal::rgba(qreal red, qreal green, qreal blue, qreal alpha) const {
    return Color::fromRgba(red, green, blue, alpha);
}

Span QmlTuiGlobal::span(const QString& text) const {
    return Span(text);
}

Span QmlTuiGlobal::span(const QString& text, const QVariantMap& options) const {
    TextStyle style;
    style.applyVariantMap(options, /*warnOnInvalid=*/true);
    return Span(text, style);
}

} // namespace qmltui
