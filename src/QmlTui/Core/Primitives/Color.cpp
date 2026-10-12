#include <QmlTui/Core/Primitives/Color.h>

#include <QHash>

#include <algorithm>
#include <cmath>

namespace qmltui {
namespace {

// xterm's default palette for the sixteen ANSI entries. Used only to give an
// ANSI-indexed color an RGB value where one is unavoidable (gradients, dimming,
// image capture); the encoder never converts an index to RGB.
constexpr quint32 ansiPaletteRgb[16] = {
    0xff000000U, 0xff800000U, 0xff008000U, 0xff808000U,
    0xff000080U, 0xff800080U, 0xff008080U, 0xffc0c0c0U,
    0xff808080U, 0xffff0000U, 0xff00ff00U, 0xffffff00U,
    0xff0000ffU, 0xffff00ffU, 0xff00ffffU, 0xffffffffU,
};

// The CSS names whose RGB is exactly an xterm default palette entry. They parse
// to that ANSI index so the user's terminal theme decides their appearance.
const QHash<QString, int> &ansiNamedColors()
{
    static const QHash<QString, int> names {
        {QStringLiteral("black"), 0},   {QStringLiteral("maroon"), 1},
        {QStringLiteral("green"), 2},   {QStringLiteral("olive"), 3},
        {QStringLiteral("navy"), 4},    {QStringLiteral("purple"), 5},
        {QStringLiteral("teal"), 6},    {QStringLiteral("silver"), 7},
        {QStringLiteral("gray"), 8},    {QStringLiteral("grey"), 8},
        {QStringLiteral("red"), 9},     {QStringLiteral("lime"), 10},
        {QStringLiteral("yellow"), 11}, {QStringLiteral("blue"), 12},
        {QStringLiteral("fuchsia"), 13}, {QStringLiteral("magenta"), 13},
        {QStringLiteral("aqua"), 14},   {QStringLiteral("cyan"), 14},
        {QStringLiteral("white"), 15},
    };
    return names;
}

// Names recognized under the "ansi:" prefix. This namespace follows ANSI's own
// numbering, not CSS: the eight colour words are SGR 30-37 (indices 0-7) and a
// "bright" prefix moves them to SGR 90-97 (indices 8-15). That is deliberately
// not the same authority as ansiNamedColors() above, where "red" is CSS red and
// so lands on index 9. CSS-only words (maroon, olive, navy, purple, teal,
// silver, lime, fuchsia, aqua) are not ANSI names and are rejected here; they
// already work without the prefix.
const QHash<QString, int> &ansiSemanticNames()
{
    static const QHash<QString, int> names {
        // Standard (0-7): SGR 30-37. "dark" is accepted as a synonym of the
        // base word for people coming from Windows console naming.
        {QStringLiteral("black"), 0},
        {QStringLiteral("red"), 1},         {QStringLiteral("darkred"), 1},
        {QStringLiteral("green"), 2},       {QStringLiteral("darkgreen"), 2},
        {QStringLiteral("yellow"), 3},      {QStringLiteral("darkyellow"), 3},
        {QStringLiteral("blue"), 4},        {QStringLiteral("darkblue"), 4},
        {QStringLiteral("magenta"), 5},     {QStringLiteral("darkmagenta"), 5},
        {QStringLiteral("cyan"), 6},        {QStringLiteral("darkcyan"), 6},
        {QStringLiteral("white"), 7},       {QStringLiteral("lightgray"), 7},
        {QStringLiteral("lightgrey"), 7},

        // Bright (8-15): SGR 90-97. Index 8 is ANSI's "bright black"; the gray
        // spellings are accepted for it because that is what it renders as.
        {QStringLiteral("brightblack"), 8}, {QStringLiteral("gray"), 8},
        {QStringLiteral("grey"), 8},        {QStringLiteral("darkgray"), 8},
        {QStringLiteral("darkgrey"), 8},
        {QStringLiteral("brightred"), 9},
        {QStringLiteral("brightgreen"), 10},
        {QStringLiteral("brightyellow"), 11},
        {QStringLiteral("brightblue"), 12},
        {QStringLiteral("brightmagenta"), 13},
        {QStringLiteral("brightcyan"), 14},
        {QStringLiteral("brightwhite"), 15},
    };
    return names;
}

const char *const ansiCanonicalNames[16] = {
    "black", "maroon", "green", "olive", "navy", "purple", "teal", "silver",
    "gray", "red", "lime", "yellow", "blue", "magenta", "cyan", "white",
};

const QHash<QString, quint32> &namedColors()
{
    static const QHash<QString, quint32> colors {
        // These two differ only in whether the cell is painted. "default" is
        // alpha 0: the cell is written and the terminal picks the colour
        // (SGR 39 for a foreground, SGR 49 for a background), so the surface
        // still occludes what it covers. "transparent" is tagged: nothing is
        // written at all and whatever is behind shows through. A popup wants
        // "default"; a Rectangle with no colour set wants "transparent".
        {QStringLiteral("transparent"), Color::TransparentTag},
        {QStringLiteral("default"), 0x00000000U},
        {QStringLiteral("orange"), 0xffffa500U},
        {QStringLiteral("pink"), 0xffffc0cbU},
        {QStringLiteral("lightblue"), 0xffadd8e6U},
        {QStringLiteral("lightgreen"), 0xff90ee90U},
        {QStringLiteral("darkblue"), 0xff00008bU},
        {QStringLiteral("darkgreen"), 0xff006400U},
        {QStringLiteral("aliceblue"), 0xfff0f8ffU},
        {QStringLiteral("antiquewhite"), 0xfffaebd7U},
        {QStringLiteral("aquamarine"), 0xff7fffd4U},
        {QStringLiteral("azure"), 0xfff0ffffU},
        {QStringLiteral("beige"), 0xfff5f5dcU},
        {QStringLiteral("bisque"), 0xffffe4c4U},
        {QStringLiteral("blanchedalmond"), 0xffffebcdU},
        {QStringLiteral("blueviolet"), 0xff8a2be2U},
        {QStringLiteral("brown"), 0xffa52a2aU},
        {QStringLiteral("burlywood"), 0xffdeb887U},
        {QStringLiteral("cadetblue"), 0xff5f9ea0U},
        {QStringLiteral("chartreuse"), 0xff7fff00U},
        {QStringLiteral("chocolate"), 0xffd2691eU},
        {QStringLiteral("coral"), 0xffff7f50U},
        {QStringLiteral("cornflowerblue"), 0xff6495edU},
        {QStringLiteral("cornsilk"), 0xfffff8dcU},
        {QStringLiteral("crimson"), 0xffdc143cU},
        {QStringLiteral("darkcyan"), 0xff008b8bU},
        {QStringLiteral("darkgoldenrod"), 0xffb8860bU},
        {QStringLiteral("darkgray"), 0xffa9a9a9U},
        {QStringLiteral("darkgrey"), 0xffa9a9a9U},
        {QStringLiteral("darkkhaki"), 0xffbdb76bU},
        {QStringLiteral("darkmagenta"), 0xff8b008bU},
        {QStringLiteral("darkolivegreen"), 0xff556b2fU},
        {QStringLiteral("darkorange"), 0xffff8c00U},
        {QStringLiteral("darkorchid"), 0xff9932ccU},
        {QStringLiteral("darkred"), 0xff8b0000U},
        {QStringLiteral("darksalmon"), 0xffe9967aU},
        {QStringLiteral("darkseagreen"), 0xff8fbc8fU},
        {QStringLiteral("darkslateblue"), 0xff483d8bU},
        {QStringLiteral("darkslategray"), 0xff2f4f4fU},
        {QStringLiteral("darkslategrey"), 0xff2f4f4fU},
        {QStringLiteral("darkturquoise"), 0xff00ced1U},
        {QStringLiteral("darkviolet"), 0xff9400d3U},
        {QStringLiteral("deeppink"), 0xffff1493U},
        {QStringLiteral("deepskyblue"), 0xff00bfffU},
        {QStringLiteral("dimgray"), 0xff696969U},
        {QStringLiteral("dimgrey"), 0xff696969U},
        {QStringLiteral("dodgerblue"), 0xff1e90ffU},
        {QStringLiteral("firebrick"), 0xffb22222U},
        {QStringLiteral("floralwhite"), 0xfffffaf0U},
        {QStringLiteral("forestgreen"), 0xff228b22U},
        {QStringLiteral("gainsboro"), 0xffdcdcdcU},
        {QStringLiteral("ghostwhite"), 0xfff8f8ffU},
        {QStringLiteral("gold"), 0xffffd700U},
        {QStringLiteral("goldenrod"), 0xffdaa520U},
        {QStringLiteral("greenyellow"), 0xffadff2fU},
        {QStringLiteral("honeydew"), 0xfff0fff0U},
        {QStringLiteral("hotpink"), 0xffff69b4U},
        {QStringLiteral("indianred"), 0xffcd5c5cU},
        {QStringLiteral("indigo"), 0xff4b0082U},
        {QStringLiteral("ivory"), 0xfffffff0U},
        {QStringLiteral("khaki"), 0xfff0e68cU},
        {QStringLiteral("lavender"), 0xffe6e6faU},
        {QStringLiteral("lavenderblush"), 0xfffff0f5U},
        {QStringLiteral("lawngreen"), 0xff7cfc00U},
        {QStringLiteral("lemonchiffon"), 0xfffffacdU},
        {QStringLiteral("lightcoral"), 0xfff08080U},
        {QStringLiteral("lightcyan"), 0xffe0ffffU},
        {QStringLiteral("lightgoldenrodyellow"), 0xfffafad2U},
        {QStringLiteral("lightgray"), 0xffd3d3d3U},
        {QStringLiteral("lightgrey"), 0xffd3d3d3U},
        {QStringLiteral("lightpink"), 0xffffb6c1U},
        {QStringLiteral("lightsalmon"), 0xffffa07aU},
        {QStringLiteral("lightseagreen"), 0xff20b2aaU},
        {QStringLiteral("lightskyblue"), 0xff87cefaU},
        {QStringLiteral("lightslategray"), 0xff778899U},
        {QStringLiteral("lightslategrey"), 0xff778899U},
        {QStringLiteral("lightsteelblue"), 0xffb0c4deU},
        {QStringLiteral("lightyellow"), 0xffffffe0U},
        {QStringLiteral("linen"), 0xfffaf0e6U},
        {QStringLiteral("mediumaquamarine"), 0xff66cdaaU},
        {QStringLiteral("mediumblue"), 0xff0000cdU},
        {QStringLiteral("mediumorchid"), 0xffba55d3U},
        {QStringLiteral("mediumpurple"), 0xff9370dbU},
        {QStringLiteral("mediumseagreen"), 0xff3cb371U},
        {QStringLiteral("mediumslateblue"), 0xff7b68eeU},
        {QStringLiteral("mediumspringgreen"), 0xff00fa9aU},
        {QStringLiteral("mediumturquoise"), 0xff48d1ccU},
        {QStringLiteral("mediumvioletred"), 0xffc71585U},
        {QStringLiteral("midnightblue"), 0xff191970U},
        {QStringLiteral("mintcream"), 0xfff5fffaU},
        {QStringLiteral("mistyrose"), 0xffffe4e1U},
        {QStringLiteral("moccasin"), 0xffffe4b5U},
        {QStringLiteral("navajowhite"), 0xffffdeadU},
        {QStringLiteral("oldlace"), 0xfffdf5e6U},
        {QStringLiteral("olivedrab"), 0xff6b8e23U},
        {QStringLiteral("orangered"), 0xffff4500U},
        {QStringLiteral("orchid"), 0xffda70d6U},
        {QStringLiteral("palegoldenrod"), 0xffeee8aaU},
        {QStringLiteral("palegreen"), 0xff98fb98U},
        {QStringLiteral("paleturquoise"), 0xffafeeeeU},
        {QStringLiteral("palevioletred"), 0xffdb7093U},
        {QStringLiteral("papayawhip"), 0xffffefd5U},
        {QStringLiteral("peachpuff"), 0xffffdab9U},
        {QStringLiteral("peru"), 0xffcd853fU},
        {QStringLiteral("plum"), 0xffdda0ddU},
        {QStringLiteral("powderblue"), 0xffb0e0e6U},
        {QStringLiteral("rosybrown"), 0xffbc8f8fU},
        {QStringLiteral("royalblue"), 0xff4169e1U},
        {QStringLiteral("saddlebrown"), 0xff8b4513U},
        {QStringLiteral("salmon"), 0xfffa8072U},
        {QStringLiteral("sandybrown"), 0xfff4a460U},
        {QStringLiteral("seagreen"), 0xff2e8b57U},
        {QStringLiteral("seashell"), 0xfffff5eeU},
        {QStringLiteral("sienna"), 0xffa0522dU},
        {QStringLiteral("skyblue"), 0xff87ceebU},
        {QStringLiteral("slateblue"), 0xff6a5acdU},
        {QStringLiteral("slategray"), 0xff708090U},
        {QStringLiteral("slategrey"), 0xff708090U},
        {QStringLiteral("snow"), 0xfffffafaU},
        {QStringLiteral("springgreen"), 0xff00ff7fU},
        {QStringLiteral("steelblue"), 0xff4682b4U},
        {QStringLiteral("tan"), 0xffd2b48cU},
        {QStringLiteral("thistle"), 0xffd8bfd8U},
        {QStringLiteral("tomato"), 0xffff6347U},
        {QStringLiteral("turquoise"), 0xff40e0d0U},
        {QStringLiteral("violet"), 0xffee82eeU},
        {QStringLiteral("wheat"), 0xfff5deb3U},
        {QStringLiteral("whitesmoke"), 0xfff5f5f5U},
        {QStringLiteral("yellowgreen"), 0xff9acd32U},
    };
    return colors;
}

} // namespace

Color::Color(const QString &value)
    : Color(fromString(value))
{
}

Color::Color(const char *value)
    : Color(QString::fromLatin1(value ? value : ""))
{
}

Color Color::fromRgba(qreal red, qreal green, qreal blue, qreal alpha)
{
    if (!std::isfinite(red) || !std::isfinite(green)
        || !std::isfinite(blue) || !std::isfinite(alpha)) {
        return {};
    }

    const auto channel = [](qreal value) {
        return static_cast<quint32>(
            std::lround(std::clamp(value, qreal(0), qreal(1)) * qreal(255)));
    };

    return fromArgb32((channel(alpha) << 24)
                      | (channel(red) << 16)
                      | (channel(green) << 8)
                      | channel(blue));
}

Color Color::fromString(const QString &value)
{
    const QString text = value.trimmed();
    if (text.isEmpty())
        return {};

    if (text.startsWith(QLatin1Char('#'))) {
        bool ok = false;
        if (text.size() == 4) {
            const quint32 r = text.mid(1, 1).toUInt(&ok, 16);
            if (!ok)
                return {};
            const quint32 g = text.mid(2, 1).toUInt(&ok, 16);
            if (!ok)
                return {};
            const quint32 b = text.mid(3, 1).toUInt(&ok, 16);
            if (!ok)
                return {};
            return fromArgb32(0xff000000U | (r << 20) | (r << 16)
                              | (g << 12) | (g << 8) | (b << 4) | b);
        }

        const quint32 parsed = text.mid(1).toUInt(&ok, 16);
        if (!ok)
            return {};
        if (text.size() == 7)
            return fromArgb32(0xff000000U | parsed);
        if (text.size() == 9)
            return fromArgb32(parsed);
        return {};
    }

    const QString lower = text.toLower();
    if (lower.startsWith(QLatin1String("ansi:"))) {
        QString suffix = lower.mid(5);
        bool ok = false;
        const int index = suffix.toInt(&ok);
        if (ok && index >= 0 && index < 16)
            return fromAnsiIndex(index);
        suffix.remove(QLatin1Char('-'))
              .remove(QLatin1Char('_'))
              .remove(QLatin1Char(' '));
        const auto it = ansiSemanticNames().constFind(suffix);
        if (it != ansiSemanticNames().cend())
            return fromAnsiIndex(it.value());
        return {};
    }
    const auto ansi = ansiNamedColors().constFind(lower);
    if (ansi != ansiNamedColors().cend())
        return fromAnsiIndex(ansi.value());
    const auto it = namedColors().constFind(lower);
    return it == namedColors().cend() ? Color() : fromArgb32(it.value());
}

Color Color::toRgb() const
{
    if (isAnsiIndex())
        return fromArgb32(ansiPaletteRgb[m_argb & 0x0FU]);
    // Transparency has no RGB; it reads as fully unset.
    if (isTransparent())
        return fromArgb32(0x00000000U);
    return *this;
}

QString Color::toString() const
{
    if (!m_valid)
        return {};
    if (isAnsiIndex())
        return QLatin1String(ansiCanonicalNames[m_argb & 0x0FU]);
    if (isTransparent())
        return QStringLiteral("transparent");
    // Alpha 0 with no RGB is the terminal's own colour, and "default" is the
    // spelling that parses back to it; "#00000000" would not.
    if (m_argb == 0)
        return QStringLiteral("default");
    if ((m_argb >> 24) == 0xffU)
        return QStringLiteral("#%1").arg(m_argb & 0x00ffffffU, 6, 16, QLatin1Char('0'));
    return QStringLiteral("#%1").arg(m_argb, 8, 16, QLatin1Char('0'));
}

} // namespace qmltui
