#pragma once

// Internal. Reads Qt's numeric standard item roles and item flags for a model
// index and turns them into terminal-ready values under Widgets-style names,
// the part of QStyledItemDelegate that a QML delegate cannot do itself because
// these roles have no entry in roleNames(). QamuiCore does not link QtGui: a
// QColor is decoded through the QVariant string converter QtGui registers
// when it is loaded, QBrush/QIcon/QFont are ignored.

#include <QAbstractItemModel>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <QVariantMap>
#include <Qamui/Core/Primitives/ColorMapper.h>

namespace qamui::NumericItemRoles {

// QColor and QBrush cannot be named here (no QtGui), but QtGui registers
// their QDataStream operators on the metatype, and that format is stable. A
// QBrush streams as a style byte followed by its color; a QColor as a spec
// byte (1 = RGB) followed by 16-bit alpha, red, green, blue. QStandardItem's
// setForeground()/setBackground() store brushes, custom models usually store
// colors. HSV, HSL, CMYK and extended-RGB specs are rare in models and ignored.
inline QString colorFromStreamedQColor(const QVariant &value)
{
    const QMetaType type = value.metaType();
    const bool isColor = qstrcmp(type.name(), "QColor") == 0;
    const bool isBrush = qstrcmp(type.name(), "QBrush") == 0;
    if ((!isColor && !isBrush) || !type.hasRegisteredDataStreamOperators()) return {};
    QByteArray bytes;
    {
        QDataStream out(&bytes, QIODevice::WriteOnly);
        if (!type.save(out, value.constData())) return {};
    }
    QDataStream in(bytes);
    if (isBrush) {
        quint8 style = 0;
        in >> style;
        if (style == 0) return {}; // Qt::NoBrush
    }
    qint8 spec = 0;
    quint16 alpha = 0, red = 0, green = 0, blue = 0;
    in >> spec >> alpha >> red >> green >> blue;
    if (in.status() != QDataStream::Ok || spec != 1) return {};
    return QStringLiteral("#%1%2%3")
        .arg(red >> 8, 2, 16, QLatin1Char('0'))
        .arg(green >> 8, 2, 16, QLatin1Char('0'))
        .arg(blue >> 8, 2, 16, QLatin1Char('0'));
}

inline QString colorString(const QVariant &value)
{
    if (!value.isValid() || value.isNull()) return {};
    if (value.metaType() == QMetaType::fromType<QString>()) return value.toString();
    const QString streamed = colorFromStreamedQColor(value);
    if (!streamed.isEmpty()) return streamed;
    if (!value.canConvert<QString>()) return {};
    QString text = value.toString();
    // Some converters stringify colors as #aarrggbb; terminals have no alpha.
    if (text.size() == 9 && text.startsWith(QLatin1Char('#'))) text = QLatin1Char('#') + text.mid(3);
    return text;
}

inline QString alignmentName(const QVariant &alignment)
{
    if (!alignment.isValid() || !alignment.canConvert<int>()) return {};
    const auto flags = Qt::Alignment(alignment.toInt());
    if (flags & Qt::AlignRight) return QStringLiteral("right");
    if (flags & Qt::AlignHCenter) return QStringLiteral("center");
    if (flags & Qt::AlignLeft) return QStringLiteral("left");
    return {};
}

// Terminals draw glyphs, not images: only string and character decorations.
inline QString decorationGlyph(const QVariant &decoration)
{
    const int type = decoration.metaType().id();
    if (type == QMetaType::QString || type == QMetaType::QChar) return decoration.toString();
    return {};
}

// Built-in painters need only these values, not a delegate's property map or
// modelIndex. Read live on each paint so time-dependent model colors stay fresh.
struct PaintData {
    uint32_t foreground;
    uint32_t background;
    QString alignment;
    QString prefix;
    bool enabled = true;
};

inline PaintData readForPaint(const QAbstractItemModel *model, const QModelIndex &index,
                             uint32_t foreground, uint32_t background,
                             bool readForeground, bool readBackground)
{
    PaintData values{foreground, background, {}, {}};
    if (!model || !index.isValid()) return values;
    const auto color = [&](int role, uint32_t fallback) {
        const QString text = colorString(model->data(index, role));
        return text.isEmpty() ? fallback : ColorMapper::fromString(text, fallback);
    };
    if (readForeground) values.foreground = color(Qt::ForegroundRole, foreground);
    if (readBackground) values.background = color(Qt::BackgroundRole, background);
    values.alignment = alignmentName(model->data(index, Qt::TextAlignmentRole));
    const QVariant check = model->data(index, Qt::CheckStateRole);
    const QString glyph = decorationGlyph(model->data(index, Qt::DecorationRole));
    if (!glyph.isEmpty()) values.prefix = glyph + QLatin1Char(' ');
    if (check.isValid() && check.canConvert<int>() && check.toInt() >= 0) {
        const int state = check.toInt();
        values.prefix += state == Qt::Checked ? QStringLiteral("[x] ")
                       : state == Qt::PartiallyChecked ? QStringLiteral("[-] ") : QStringLiteral("[ ] ");
    }
    values.enabled = model->flags(index).testFlag(Qt::ItemIsEnabled);
    return values;
}

// Which of the standard names a model already publishes through roleNames().
//
// apply() resolved this for every index: a virtual roleNames() call, a hash
// copy, and a string comparison of each standard key against every published
// name, on a path that runs for every visible cell of every frame. A view that
// holds a model for a while resolves it once instead and passes it in.
//
// A plain bit mask rather than a struct, so a public header can cache one
// without including this private header.
enum ClaimedName : quint16 {
    ClaimsNothing     = 0,
    ClaimsForeground  = 1u << 0,
    ClaimsBackground  = 1u << 1,
    ClaimsAlignment   = 1u << 2,
    ClaimsCheckState  = 1u << 3,
    ClaimsDecoration  = 1u << 4,
    ClaimsEnabled     = 1u << 5,
    ClaimsCheckable   = 1u << 6,
    ClaimsModelIndex  = 1u << 7,
};

inline quint16 claimedNames(const QAbstractItemModel *model)
{
    if (!model) return ClaimsNothing;
    quint16 claimed = ClaimsNothing;
    const QHash<int, QByteArray> names = model->roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        const QByteArray &name = it.value();
        if (name == "foreground") claimed |= ClaimsForeground;
        else if (name == "background") claimed |= ClaimsBackground;
        else if (name == "textAlignment") claimed |= ClaimsAlignment;
        else if (name == "checkState") claimed |= ClaimsCheckState;
        else if (name == "decoration") claimed |= ClaimsDecoration;
        else if (name == "enabled") claimed |= ClaimsEnabled;
        else if (name == "checkable") claimed |= ClaimsCheckable;
        else if (name == "modelIndex") claimed |= ClaimsModelIndex;
    }
    return claimed;
}

// Adds (or refreshes) the standard values in a delegate's role map, leaving
// alone any name the model already claims through roleNames(). On creation
// only values that carry information are published (a set color, a non-left
// alignment, a check state, a glyph, a disabled or checkable item), so
// unstyled rows cost no per-delegate properties. Callers that refresh or
// rebind an existing delegate pass includeUnset so a cleared style is
// published as cleared.
inline void apply(const QAbstractItemModel *model, const QModelIndex &index, QVariantMap &values,
                  bool includeUnset, quint16 claimed)
{
    if (!model || !index.isValid()) return;
    const auto put = [&](const QString &name, quint16 claim, const QVariant &value, bool unset) {
        if (!includeUnset && unset) return;
        if (claimed & claim) return;
        values.insert(name, value);
    };
    const QString foreground = colorString(model->data(index, Qt::ForegroundRole));
    put(QStringLiteral("foreground"), ClaimsForeground, foreground, foreground.isEmpty());
    const QString background = colorString(model->data(index, Qt::BackgroundRole));
    put(QStringLiteral("background"), ClaimsBackground, background, background.isEmpty());
    const QString alignment = alignmentName(model->data(index, Qt::TextAlignmentRole));
    put(QStringLiteral("textAlignment"), ClaimsAlignment, alignment, alignment.isEmpty());
    const QVariant check = model->data(index, Qt::CheckStateRole);
    const int checkState = check.isValid() && check.canConvert<int>() ? check.toInt() : -1;
    put(QStringLiteral("checkState"), ClaimsCheckState, checkState, checkState < 0);
    const QString glyph = decorationGlyph(model->data(index, Qt::DecorationRole));
    put(QStringLiteral("decoration"), ClaimsDecoration, glyph, glyph.isEmpty());
    const Qt::ItemFlags flags = model->flags(index);
    const bool enabled = flags.testFlag(Qt::ItemIsEnabled);
    const bool checkable = flags.testFlag(Qt::ItemIsUserCheckable);
    put(QStringLiteral("enabled"), ClaimsEnabled, enabled, enabled);
    put(QStringLiteral("checkable"), ClaimsCheckable, checkable, !checkable);
    put(QStringLiteral("modelIndex"), ClaimsModelIndex,
        QVariant::fromValue(QModelIndex(index)), !index.isValid());
}

// For callers with nowhere to cache the mask; resolves it per call, as before.
inline void apply(const QAbstractItemModel *model, const QModelIndex &index, QVariantMap &values,
                  bool includeUnset = false)
{
    apply(model, index, values, includeUnset, claimedNames(model));
}

// "[x] " / "[ ] " / "[-] " and the decoration glyph, for built-in painters.
inline QString prefix(const QVariantMap &values)
{
    QString result;
    const QString glyph = values.value(QStringLiteral("decoration")).toString();
    if (!glyph.isEmpty()) result += glyph + QLatin1Char(' ');
    const QVariant check = values.value(QStringLiteral("checkState"));
    if (check.isValid() && check.canConvert<int>() && check.toInt() >= 0) {
        const int state = check.toInt();
        result += state == Qt::Checked ? QStringLiteral("[x] ")
                : state == Qt::PartiallyChecked ? QStringLiteral("[-] ") : QStringLiteral("[ ] ");
    }
    return result;
}

inline uint32_t color(const QVariantMap &values, const char *name, uint32_t fallback)
{
    const QString text = values.value(QLatin1String(name)).toString();
    return text.isEmpty() ? fallback : ColorMapper::fromString(text, fallback);
}

inline bool enabled(const QVariantMap &values)
{
    return values.value(QStringLiteral("enabled"), true).toBool();
}

} // namespace qamui::NumericItemRoles
