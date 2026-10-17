#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Primitives/Color.h>

#include <QmlTui/Core/Primitives/Span.h>
#include <QVariantMap>
#include <QObject>

#include <QtQml/qqmlregistration.h>

namespace qmltui {

// QmlTui QML singleton providing value-type factory helpers (QmlTui.span and
// QmlTui.rgba).
class QMLTUI_CORE_EXPORT QmlTuiGlobal : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(QmlTui)
    QML_SINGLETON

public:
    explicit QmlTuiGlobal(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    Q_INVOKABLE qmltui::Span span(const QString &text) const;
    Q_INVOKABLE qmltui::Span span(const QString &text, const QVariantMap &options) const;

    Q_INVOKABLE Color rgba(qreal red, qreal green, qreal blue,
                           qreal alpha) const;
};

} // namespace qmltui
