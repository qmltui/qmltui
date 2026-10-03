#pragma once

#include "Item.h"
#include <QString>
#include <QStringList>
#include <QList>

namespace qtui {

struct FormattedChar {
    QString text;
    int width = 1;
    uint32_t fg = 0xFFFFFFFF;
    uint32_t bg = 0x00000000;
    TextAttribute attr = TextAttribute::None;
    QString href;
};

using FormattedLine = QList<FormattedChar>;

class Paragraph : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)
    Q_PROPERTY(QString alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)
    Q_PROPERTY(bool wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)
    Q_PROPERTY(int scrollY READ scrollY WRITE setScrollY NOTIFY scrollYChanged)
    Q_PROPERTY(QString borderStyle READ borderStyle WRITE setBorderStyle NOTIFY borderStyleChanged)
    Q_PROPERTY(QString borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)

public:
    enum TextFormat {
        AutoText,
        PlainText,
        RichText
    };
    Q_ENUM(TextFormat)

    explicit Paragraph(QObject *parent = nullptr);
    ~Paragraph() override = default;

    QString text() const { return m_text; }
    void setText(const QString &text);

    TextFormat textFormat() const { return m_textFormat; }
    void setTextFormat(TextFormat format);

    QString color() const { return m_color; }
    void setColor(const QString &color);

    QString bgColor() const { return m_bgColor; }
    void setBgColor(const QString &color);

    QString alignment() const { return m_alignment; }
    void setAlignment(const QString &align);

    bool wrapMode() const { return m_wrapMode; }
    void setWrapMode(bool wrap);

    int scrollY() const { return m_scrollY; }
    void setScrollY(int scroll);

    QString borderStyle() const { return m_borderStyle; }
    void setBorderStyle(const QString &style);

    QString borderColor() const { return m_borderColor; }
    void setBorderColor(const QString &color);

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;
    void mousePressEvent(MouseEvent *event) override;

signals:
    void textChanged();
    void textFormatChanged();
    void colorChanged();
    void bgColorChanged();
    void alignmentChanged();
    void wrapModeChanged();
    void scrollYChanged();
    void borderStyleChanged();
    void borderColorChanged();

    void linkActivated(const QString &link);

private:
    void invalidateTextCache();
    FormattedLine parseRichLine(const QString &line) const;
    QList<FormattedLine> wordWrapFormatted(const FormattedLine &rawLine, int maxW) const;
    QList<FormattedLine> getRenderedLines(int availW) const;

    QString m_text;
    TextFormat m_textFormat = AutoText;

    QString m_color = "white";
    uint32_t m_resolvedColor = 0xFFFFFFFF;

    QString m_bgColor = "transparent";
    uint32_t m_resolvedBgColor = 0x00000000;

    QString m_alignment = "left";
    bool m_wrapMode = true;
    int m_scrollY = 0;

    QString m_borderStyle = "none";
    QString m_borderColor = "gray";
    uint32_t m_resolvedBorderColor = 0xFF808080;
    mutable int m_cachedWidth = -1;
    mutable QList<FormattedLine> m_cachedLines;
};

} // namespace qtui
