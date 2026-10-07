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
    // Alpha byte 0x01 is reserved: such a value is not a translucent RGB color
    // but one of the sixteen ANSI palette entries (index in the low byte). The
    // encoder emits those as SGR 30-37/90-97 at every color depth, so the
    // terminal's own theme decides how they look. The sixteen CSS names whose
    // RGB equals xterm's default palette ("red", "white", "gray", ...) parse to
    // these entries; hex values are always literal RGB.
    static constexpr quint32 AnsiTag = 0x01000000U;

    constexpr Color() = default;
    Q_INVOKABLE Color(const QString &value);
    Color(const char *value);

    static constexpr Color fromArgb32(quint32 argb)
    {
        return Color(argb, true);
    }
    static constexpr Color fromAnsiIndex(int index)
    {
        return Color(AnsiTag | static_cast<quint32>(index & 0x0F), true);
    }
    constexpr bool isAnsiIndex() const { return m_valid && (m_argb >> 24) == 0x01U; }
    constexpr int ansiIndex() const { return isAnsiIndex() ? static_cast<int>(m_argb & 0x0FU) : -1; }
    // The literal RGB this color has under xterm's default palette; identity for RGB colors.
    Color toRgb() const;

    static Color fromRgba(qreal red, qreal green, qreal blue,
                          qreal alpha = 1.0);
    static Color fromString(const QString &value);

    constexpr bool isValid() const { return m_valid; }
    constexpr quint32 argb32() const { return m_argb; }
    qreal red() const { return toRgb().channel(16); }
    qreal green() const { return toRgb().channel(8); }
    qreal blue() const { return toRgb().channel(0); }
    qreal alpha() const { return toRgb().channel(24); }

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
