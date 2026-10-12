#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>

#include <Qamui/Core/Primitives/Color.h>

#include <Qamui/Core/Primitives/Span.h>
#include <QVariantMap>
#include <QObject>

#include <QtQml/qqmlregistration.h>

namespace qamui {

class QAMUI_CORE_EXPORT QamuiGlobal : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Qamui)
    QML_SINGLETON

public:
    explicit QamuiGlobal(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    Q_INVOKABLE qamui::Span span(const QString &text) const;
    Q_INVOKABLE qamui::Span span(const QString &text, const QVariantMap &options) const;

    Q_INVOKABLE Color rgba(qreal red, qreal green, qreal blue,
                           qreal alpha) const;
};

} // namespace qamui
