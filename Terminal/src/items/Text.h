#pragma once

#include "Item.h"
#include <QString>

namespace qtui {

class Text : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(bool bold READ bold WRITE setBold NOTIFY boldChanged)
    Q_PROPERTY(bool italic READ italic WRITE setItalic NOTIFY italicChanged)
    Q_PROPERTY(bool underline READ underline WRITE setUnderline NOTIFY underlineChanged)
    Q_PROPERTY(bool reversed READ reversed WRITE setReversed NOTIFY reversedChanged)
    Q_PROPERTY(int horizontalAlignment READ horizontalAlignment WRITE setHorizontalAlignment NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(int verticalAlignment READ verticalAlignment WRITE setVerticalAlignment NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)

public:
    enum TextFormat {
        PlainText,
        RichText,
        AnsiText,
        AutoText
    };
    Q_ENUM(TextFormat)

    explicit Text(QObject *parent = nullptr);
    ~Text() override;

    QString text() const { return m_text; }
    void setText(const QString& text);

    QString color() const { return m_color; }
    void setColor(const QString& color);

    bool bold() const { return m_bold; }
    void setBold(bool bold);

    bool italic() const { return m_italic; }
    void setItalic(bool italic);

    bool underline() const { return m_underline; }
    void setUnderline(bool underline);

    bool reversed() const { return m_reversed; }
    void setReversed(bool reversed);

    int horizontalAlignment() const { return m_horizontalAlignment; }
    void setHorizontalAlignment(int alignment);

    int verticalAlignment() const { return m_verticalAlignment; }
    void setVerticalAlignment(int alignment);

    TextFormat textFormat() const { return m_textFormat; }
    void setTextFormat(TextFormat format);

    int visualLength(const QString &line) const;

    void render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

signals:
    void textChanged();
    void colorChanged();
    void boldChanged();
    void italicChanged();
    void underlineChanged();
    void reversedChanged();
    void horizontalAlignmentChanged();
    void verticalAlignmentChanged();
    void textFormatChanged();

private:
    QString m_text;
    QString m_color = "white"; // default string
    uint32_t m_resolvedColor = 0xFFFFFFFF; // pre-resolved color value
    bool m_bold = false;
    bool m_italic = false;
    bool m_underline = false;
    bool m_reversed = false;
    int m_horizontalAlignment = Qt::AlignLeft;
    int m_verticalAlignment = Qt::AlignTop;
    TextFormat m_textFormat = AutoText;
};

} // namespace qtui
