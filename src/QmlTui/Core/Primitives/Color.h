#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QMetaType>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace qmltui {

// Terminal color value type (`color` in QML) representing 32-bit ARGB colors,
// 16-color ANSI palette indices, `"default"` (terminal foreground/background),
// and `"transparent"`.
class QMLTUI_CORE_EXPORT Color {
    Q_GADGET
    QML_VALUE_TYPE(color)
    QML_CONSTRUCTIBLE_VALUE

    Q_PROPERTY(qreal r READ red CONSTANT)
    Q_PROPERTY(qreal g READ green CONSTANT)
    Q_PROPERTY(qreal b READ blue CONSTANT)
    Q_PROPERTY(qreal a READ alpha CONSTANT)
    Q_PROPERTY(bool valid READ isValid CONSTANT)

public:
    // Alpha 0 is the terminal's own colour for whatever role the value fills:
    // the encoder emits SGR 39 for a foreground and SGR 49 for a background.
    // "default" is that value, and is the honest spelling for it.
    static constexpr quint32 DefaultTag = 0x00000000U;

    // Alpha byte 0x01 is reserved: such a value is not a translucent RGB color
    // but one of the sixteen ANSI palette entries (index in the low byte). The
    // encoder emits those as SGR 30-37/90-97 at every color depth, so the
    // terminal's own theme decides how they look. The sixteen CSS names whose
    // RGB equals xterm's default palette ("red", "white", "gray", ...) parse to
    // these entries; hex values are always literal RGB.
    static constexpr quint32 AnsiTag = 0x01000000U;

    // "transparent" is not a colour but the absence of one: a painter must
    // leave those cells alone so that whatever is behind shows through. It is
    // tagged rather than sharing alpha 0, because a surface that wants the
    // terminal's background still has to be painted in order to occlude what
    // it covers. Anything that reaches the terminal normalizes back to the
    // default, so the tag never appears in the output.
    static constexpr quint32 TransparentTag = 0x04000000U;

    enum class Special : quint32 {
        Invalid = 0,
        Default = 1,
        Transparent = 2,
    };
    static constexpr Special Invalid = Special::Invalid;
    static constexpr Special Default = Special::Default;
    static constexpr Special Transparent = Special::Transparent;

    constexpr Color() = default;
    constexpr Color(Special special)
        : m_argb(special == Special::Transparent ? TransparentTag : DefaultTag),
          m_valid(special != Special::Invalid) {}
    Q_INVOKABLE Color(const QString& value);

    static constexpr Color fromArgb32(quint32 argb) { return Color(argb, true); }
    static constexpr Color fromAnsiIndex(int index) {
        return (index >= 0 && index < 16) ? Color(AnsiTag | static_cast<quint32>(index), true)
                                          : Color(Invalid);
    }
    static constexpr Color invalid() { return Color(Invalid); }
    static constexpr Color defaultColor() { return Color(Default); }
    static constexpr Color transparent() { return Color(Transparent); }

    static constexpr bool isDefault(quint32 argb) { return argb == DefaultTag; }
    static constexpr bool isAnsiIndex(quint32 argb) { return (argb >> 24) == 0x01U; }
    static constexpr bool isTransparent(quint32 argb) { return (argb >> 24) == 0x04U; }
    static constexpr uint8_t ansiIndex(quint32 argb) { return static_cast<uint8_t>(argb & 0x0FU); }
    static constexpr quint32 paintedArgb32(quint32 argb) {
        return isTransparent(argb) ? DefaultTag : argb;
    }
    static quint32 toArgb(quint32 argb) { return fromArgb32(argb).toRgb().argb32(); }

    static uint8_t toIndexed256(quint32 argb);
    static uint8_t toBasic16(quint32 argb);

    static constexpr quint32 resolve(Color color, quint32 defaultColor = 0xFFFFFFFFU) {
        return color.isValid() ? color.argb32() : defaultColor;
    }
    static quint32 resolve(const QString& colorString, quint32 defaultColor = 0xFFFFFFFFU) {
        const Color c = fromString(colorString);
        return c.isValid() ? c.argb32() : defaultColor;
    }

    constexpr bool isDefault() const { return m_valid && isDefault(m_argb); }
    constexpr bool isAnsiIndex() const { return m_valid && isAnsiIndex(m_argb); }
    // True when nothing should be painted at all, as opposed to painting with
    // the terminal's default colour.
    constexpr bool isTransparent() const { return m_valid && isTransparent(m_argb); }
    // The value to store once something is being painted. Transparency is an
    // instruction to the painter, not a colour, and has no cell representation:
    // a painted cell that was asked for it shows the terminal default.
    constexpr quint32 paintedArgb32() const { return isTransparent() ? DefaultTag : m_argb; }
    constexpr int ansiIndex() const {
        return isAnsiIndex() ? static_cast<int>(ansiIndex(m_argb)) : -1;
    }
    uint8_t toIndexed256() const { return toIndexed256(m_argb); }
    uint8_t toBasic16() const { return toBasic16(m_argb); }

    // The literal RGB this color has under xterm's default palette; identity for RGB colors.
    Color toRgb() const;

    static Color fromRgba(qreal red, qreal green, qreal blue, qreal alpha = 1.0);
    static Color fromString(const QString& value);
    static Color fromString(const QString& value, Color defaultColor) {
        const Color c = fromString(value);
        return c.isValid() ? c : defaultColor;
    }

    constexpr bool isValid() const { return m_valid; }
    constexpr quint32 argb32() const { return m_argb; }
    qreal red() const { return toRgb().channel(16); }
    qreal green() const { return toRgb().channel(8); }
    qreal blue() const { return toRgb().channel(0); }
    qreal alpha() const { return toRgb().channel(24); }

    Q_INVOKABLE QString toString() const;

    friend constexpr bool operator==(Color lhs, Color rhs) noexcept {
        return lhs.m_argb == rhs.m_argb && lhs.m_valid == rhs.m_valid;
    }
    friend constexpr bool operator!=(Color lhs, Color rhs) noexcept { return !(lhs == rhs); }

private:
    constexpr Color(quint32 argb, bool valid) : m_argb(argb), m_valid(valid) {}

    constexpr qreal channel(int shift) const {
        return static_cast<qreal>((m_argb >> shift) & 0xffU) / 255.0;
    }

    quint32 m_argb = DefaultTag;
    bool m_valid = true;
};

} // namespace qmltui

Q_DECLARE_METATYPE(qmltui::Color)
