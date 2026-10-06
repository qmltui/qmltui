#pragma once

#include <QtuiExport.h>

#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace qtui {

class QTUI_CORE_EXPORT Color
{
    Q_GADGET
    QML_VALUE_TYPE(rgba)
    QML_CONSTRUCTIBLE_VALUE

    Q_PROPERTY(qreal r READ red CONSTANT)
    Q_PROPERTY(qreal g READ green CONSTANT)
    Q_PROPERTY(qreal b READ blue CONSTANT)
    Q_PROPERTY(qreal a READ alpha CONSTANT)
    Q_PROPERTY(bool valid READ isValid CONSTANT)

public:
    constexpr Color() = default;
    Q_INVOKABLE Color(const QString &value);
    Color(const char *value);

    static constexpr Color fromArgb32(quint32 argb)
    {
        return Color(argb, true);
    }

    static Color fromRgba(qreal red, qreal green, qreal blue,
                          qreal alpha = 1.0);
    static Color fromString(const QString &value);

    constexpr bool isValid() const { return m_valid; }
    constexpr quint32 argb32() const { return m_argb; }
    constexpr qreal red() const { return channel(16); }
    constexpr qreal green() const { return channel(8); }
    constexpr qreal blue() const { return channel(0); }
    constexpr qreal alpha() const { return channel(24); }

    QString toString() const;

    friend constexpr bool operator==(Color lhs, Color rhs) = default;

private:
    constexpr Color(quint32 argb, bool valid)
        : m_argb(argb), m_valid(valid)
    {
    }

    constexpr qreal channel(int shift) const
    {
        return static_cast<qreal>((m_argb >> shift) & 0xffU) / 255.0;
    }

    quint32 m_argb = 0;
    bool m_valid = false;
};

} // namespace qtui

Q_DECLARE_METATYPE(qtui::Color)
