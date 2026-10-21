#include <QmlTui/Core/Primitives/Span.h>
#include <QJSValue>

namespace qmltui {

bool Span::fromVariantMap(const QVariantMap& map, Span& out) {
    if (!map.contains(QStringLiteral("text"))) return false;
    Span result(map.value(QStringLiteral("text")).toString());
    TextStyle style;
    style.applyVariantMap(map, /*warnOnInvalid=*/false);
    result.setStyle(style);
    out = std::move(result);
    return true;
}

bool Span::tryExtractList(const QVariant& rawData, bool allowVariantMap, QList<Span>& outSpans) {
    if (!rawData.isValid()) return false;
    const QMetaType rawType = rawData.metaType();
    if (rawType == QMetaType::fromType<QString>() || rawType == QMetaType::fromType<int>() ||
        rawType == QMetaType::fromType<double>() || rawType == QMetaType::fromType<qlonglong>() ||
        rawType == QMetaType::fromType<bool>()) {
        return false;
    }
    if (rawType == QMetaType::fromType<Span>()) {
        outSpans = {rawData.value<Span>()};
        return true;
    }
    if (rawType == QMetaType::fromType<QList<Span>>()) {
        outSpans = rawData.value<QList<Span>>();
        return true;
    }
    const QVariant data = rawType == QMetaType::fromType<QJSValue>()
                              ? rawData.value<QJSValue>().toVariant()
                              : rawData;
    const QMetaType dataType = data.metaType();
    if (dataType == QMetaType::fromType<Span>()) {
        outSpans = {data.value<Span>()};
        return true;
    }
    if (dataType == QMetaType::fromType<QList<Span>>()) {
        outSpans = data.value<QList<Span>>();
        return true;
    }
    if (dataType == QMetaType::fromType<QVariantList>() ||
        (dataType != QMetaType::fromType<QString>() &&
         dataType != QMetaType::fromType<QVariantMap>() && data.canConvert<QVariantList>())) {
        const QVariantList list = data.toList();
        if (!list.isEmpty()) {
            QList<Span> converted;
            converted.reserve(list.size());
            bool allSpans = true;
            for (const QVariant& rawElem : list) {
                const QVariant elem = rawElem.metaType() == QMetaType::fromType<QJSValue>()
                                          ? rawElem.value<QJSValue>().toVariant()
                                          : rawElem;
                if (elem.metaType() == QMetaType::fromType<Span>()) {
                    converted.append(elem.value<Span>());
                } else if (allowVariantMap && elem.metaType() != QMetaType::fromType<QString>() &&
                           elem.canConvert<QVariantMap>()) {
                    Span span;
                    if (fromVariantMap(elem.toMap(), span)) {
                        converted.append(std::move(span));
                    } else {
                        allSpans = false;
                        break;
                    }
                } else {
                    allSpans = false;
                    break;
                }
            }
            if (allSpans) {
                outSpans = std::move(converted);
                return true;
            }
        }
    }
    if (allowVariantMap && dataType != QMetaType::fromType<QString>() &&
        data.canConvert<QVariantMap>()) {
        Span span;
        if (fromVariantMap(data.toMap(), span)) {
            outSpans = {std::move(span)};
            return true;
        }
    }
    return false;
}

QString Span::plainText(const QList<Span>& spans) {
    QString text;
    for (const Span& span : spans)
        text += span.text();
    return text;
}

} // namespace qmltui
