#include <QmlTui/Core/Items/Border.h>
#include <QmlTui/Core/Items/Item.h>

namespace qmltui {

namespace {
struct RegisterBorderConverters {
    RegisterBorderConverters() {
        QMetaType::registerConverter<QString, Border::Style>([](const QString &str) {
            return Border::styleFromString(str, Border::None);
        });
        QMetaType::registerConverter<Border::Style, QString>([](Border::Style style) {
            return Border::styleToString(style);
        });
    }
} s_registerBorderConverters;
}

Border::Border(Item* owner) : QObject(owner), m_owner(owner) {
}

Border::~Border() = default;

void Border::setStyle(Style style) {
    if (m_style == style) return;
    m_style = style;
    emit styleChanged();
    if (m_owner) {
        m_owner->markDirty();
    }
}

void Border::setStyle(const QString& style) {
    setStyle(styleFromString(style, None));
}

Border::Style Border::styleFromString(QStringView str, Style defaultStyle) {
    if (str.compare(u"none", Qt::CaseInsensitive) == 0) return None;
    if (str.compare(u"single", Qt::CaseInsensitive) == 0) return Single;
    if (str.compare(u"double", Qt::CaseInsensitive) == 0) return Double;
    if (str.compare(u"rounded", Qt::CaseInsensitive) == 0) return Rounded;
    if (str.compare(u"ascii", Qt::CaseInsensitive) == 0) return Ascii;
    return defaultStyle;
}

QString Border::styleToString(Style style) {
    switch (style) {
    case None: return QStringLiteral("none");
    case Single: return QStringLiteral("single");
    case Double: return QStringLiteral("double");
    case Rounded: return QStringLiteral("rounded");
    case Ascii: return QStringLiteral("ascii");
    }
    return QStringLiteral("none");
}

void Border::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    emit colorChanged();
    if (m_owner) {
        m_owner->markDirty();
    }
}

} // namespace qmltui
