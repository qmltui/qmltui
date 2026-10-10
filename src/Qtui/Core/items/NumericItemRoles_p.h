#pragma once

// Internal. Reads Qt's numeric standard item roles and item flags for a model
// index and turns them into terminal-ready values under Widgets-style names,
// the part of QStyledItemDelegate that a QML delegate cannot do itself because
// these roles have no entry in roleNames(). QtuiCore does not link QtGui: a
// QColor is decoded through the QVariant string converter QtGui registers
// when it is loaded, QBrush/QIcon/QFont are ignored.

#include <QAbstractItemModel>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <QVariantMap>
#include <Qtui/Core/primitives/ColorMapper.h>

namespace qtui::NumericItemRoles {

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

// Every standard value for one index, keyed by the names delegates see.
inline QVariantMap read(const QAbstractItemModel *model, const QModelIndex &index)
{
    QVariantMap values;
    if (!model || !index.isValid()) return values;
    values.insert(QStringLiteral("foreground"), colorString(model->data(index, Qt::ForegroundRole)));
    values.insert(QStringLiteral("background"), colorString(model->data(index, Qt::BackgroundRole)));
    values.insert(QStringLiteral("textAlignment"), alignmentName(model->data(index, Qt::TextAlignmentRole)));
    const QVariant check = model->data(index, Qt::CheckStateRole);
    values.insert(QStringLiteral("checkState"), check.isValid() && check.canConvert<int>() ? check.toInt() : -1);
    values.insert(QStringLiteral("decoration"), decorationGlyph(model->data(index, Qt::DecorationRole)));
    const Qt::ItemFlags flags = model->flags(index);
    values.insert(QStringLiteral("enabled"), flags.testFlag(Qt::ItemIsEnabled));
    values.insert(QStringLiteral("checkable"), flags.testFlag(Qt::ItemIsUserCheckable));
    values.insert(QStringLiteral("modelIndex"), QVariant::fromValue(QModelIndex(index)));
    return values;
}

// Adds (or refreshes) the standard values in a delegate's role map, leaving
// alone any name the model already claims through roleNames(). On creation
// only values that carry information are published (a set color, a non-left
// alignment, a check state, a glyph, a disabled or checkable item), so
// unstyled rows cost no per-delegate properties. Callers that refresh or
// rebind an existing delegate pass includeUnset so a cleared style is
// published as cleared.
inline void apply(const QAbstractItemModel *model, const QModelIndex &index, QVariantMap &values,
                  bool includeUnset = false)
{
    if (!model || !index.isValid()) return;
    // roleNames() is an implicitly shared hash of a handful of entries; compare
    // against it directly rather than building a set per row.
    const QHash<int, QByteArray> names = model->roleNames();
    const auto put = [&](const char *key, const QVariant &value, bool unset) {
        if (!includeUnset && unset) return;
        const QString name = QLatin1String(key);
        for (auto it = names.cbegin(); it != names.cend(); ++it)
            if (name == QLatin1String(it.value())) return;
        values.insert(name, value);
    };
    const QString foreground = colorString(model->data(index, Qt::ForegroundRole));
    put("foreground", foreground, foreground.isEmpty());
    const QString background = colorString(model->data(index, Qt::BackgroundRole));
    put("background", background, background.isEmpty());
    const QString alignment = alignmentName(model->data(index, Qt::TextAlignmentRole));
    put("textAlignment", alignment, alignment.isEmpty());
    const QVariant check = model->data(index, Qt::CheckStateRole);
    const int checkState = check.isValid() && check.canConvert<int>() ? check.toInt() : -1;
    put("checkState", checkState, checkState < 0);
    const QString glyph = decorationGlyph(model->data(index, Qt::DecorationRole));
    put("decoration", glyph, glyph.isEmpty());
    const Qt::ItemFlags flags = model->flags(index);
    const bool enabled = flags.testFlag(Qt::ItemIsEnabled);
    const bool checkable = flags.testFlag(Qt::ItemIsUserCheckable);
    put("enabled", enabled, enabled);
    put("checkable", checkable, !checkable);
    put("modelIndex", QVariant::fromValue(QModelIndex(index)), !checkable);
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

} // namespace qtui::NumericItemRoles
