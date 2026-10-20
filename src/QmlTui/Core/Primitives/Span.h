#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/Cell.h>
#include <QmlTui/Core/Primitives/TextStyle.h>
#include <QList>
#include <QVariant>
#include <QVariantMap>
#include <utility>

namespace qmltui {

// A literal text run. Attribute presence is distinct from its boolean value.
class QMLTUI_CORE_EXPORT Span {
    Q_GADGET
    QML_VALUE_TYPE(span)
    QML_STRUCTURED_VALUE

    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor RESET resetColor)
    Q_PROPERTY(qmltui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor RESET
                   resetBackgroundColor)
    Q_PROPERTY(bool bold READ bold WRITE setBold RESET resetBold)
    Q_PROPERTY(bool dim READ dim WRITE setDim RESET resetDim)
    Q_PROPERTY(bool italic READ italic WRITE setItalic RESET resetItalic)
    Q_PROPERTY(bool underline READ underline WRITE setUnderline RESET resetUnderline)
    Q_PROPERTY(bool reversed READ reversed WRITE setReversed RESET resetReversed)

public:
    Span() = default;
    explicit Span(QString text) : m_text(std::move(text)) {}
    Span(QString text, Color foreground, Color background = {})
        : m_text(std::move(text)), m_style(foreground, background) {}
    Span(QString text, TextStyle style) : m_text(std::move(text)), m_style(std::move(style)) {}

    // Populates out from a { text, color, backgroundColor, bold, dim, italic, underline, reversed }
    // map.
    static bool fromVariantMap(const QVariantMap& map, Span& out);
    // Extracts a list of Spans from a QVariant holding a Span, QList<Span>, QJSValue,
    // QVariantList of Spans/maps, or (when allowVariantMap is true) a single QVariantMap.
    static bool tryExtractList(const QVariant& data, bool allowVariantMap, QList<Span>& outSpans);
    static QString plainText(const QList<Span>& spans);

    QString text() const { return m_text; }
    void setText(const QString& text) { m_text = text; }

    TextStyle style() const { return m_style; }
    void setStyle(const TextStyle& style) { m_style = style; }

    Color color() const { return m_style.color(); }
    void setColor(Color color) { m_style.setColor(color); }
    void resetColor() { m_style.resetColor(); }

    Color backgroundColor() const { return m_style.backgroundColor(); }
    void setBackgroundColor(Color color) { m_style.setBackgroundColor(color); }
    void resetBackgroundColor() { m_style.resetBackgroundColor(); }

    bool bold() const { return m_style.bold(); }
    bool hasBold() const { return m_style.hasBold(); }
    void setBold(bool value) { m_style.setBold(value); }
    void resetBold() { m_style.resetBold(); }

    bool dim() const { return m_style.dim(); }
    bool hasDim() const { return m_style.hasDim(); }
    void setDim(bool value) { m_style.setDim(value); }
    void resetDim() { m_style.resetDim(); }

    bool italic() const { return m_style.italic(); }
    bool hasItalic() const { return m_style.hasItalic(); }
    void setItalic(bool value) { m_style.setItalic(value); }
    void resetItalic() { m_style.resetItalic(); }

    bool underline() const { return m_style.underline(); }
    bool hasUnderline() const { return m_style.hasUnderline(); }
    void setUnderline(bool value) { m_style.setUnderline(value); }
    void resetUnderline() { m_style.resetUnderline(); }

    bool reversed() const { return m_style.reversed(); }
    bool hasReversed() const { return m_style.hasReversed(); }
    void setReversed(bool value) { m_style.setReversed(value); }
    void resetReversed() { m_style.resetReversed(); }

    TextAttribute resolveAttributes(TextAttribute base) const {
        return m_style.resolveAttributes(base);
    }

    friend bool operator==(const Span&, const Span&) = default;

private:
    QString m_text;
    TextStyle m_style;
};

} // namespace qmltui

Q_DECLARE_METATYPE(qmltui::Span)
