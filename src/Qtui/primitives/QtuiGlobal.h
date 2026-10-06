#pragma once

#include <QtuiExport.h>

#include "Color.h"

#include <QObject>

#include <QtQml/qqmlregistration.h>

namespace qtui {

class QTUI_CORE_EXPORT QtuiGlobal : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Qtui)
    QML_SINGLETON

public:
    explicit QtuiGlobal(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    Q_INVOKABLE Color rgba(qreal red, qreal green, qreal blue,
                           qreal alpha) const;
};

} // namespace qtui
