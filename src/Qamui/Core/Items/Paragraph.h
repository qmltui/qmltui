#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>

#include <Qamui/Core/Items/Item.h>
#include <cstdint>
#include <QString>
#include <QStringList>
#include <QList>
#include <Qamui/Core/Primitives/ColorMapper.h>

namespace qamui {

struct QAMUI_CORE_EXPORT FormattedChar {
    QString text;
    int width = 1;
    uint32_t fg = 0xFFFFFFFF;
    uint32_t bg = 0x00000000;
    TextAttribute attr = TextAttribute::None;
    QString href;
};

using FormattedLine = QList<FormattedChar>;

class QAMUI_CORE_EXPORT Paragraph : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)
    Q_PROPERTY(qamui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qamui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QString alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)
    Q_PROPERTY(bool wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)
    Q_PROPERTY(int scrollY READ scrollY WRITE setScrollY NOTIFY scrollYChanged)

public:
    enum TextFormat : uint8_t {
        AutoText,
        PlainText,
        RichText,
    };
    Q_ENUM(TextFormat)

    explicit Paragraph(QObject *parent = nullptr);
    ~Paragraph() override = default;

    QString text() const { return m_text; }
    void setText(const QString &text);

    TextFormat textFormat() const { return m_textFormat; }
    void setTextFormat(TextFormat format);

    Color color() const { return m_color; }
    void setColor(Color color);

    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(Color color);

    QString alignment() const { return m_alignment; }
    void setAlignment(const QString &align);

    bool wrapMode() const { return m_wrapMode; }
    void setWrapMode(bool wrap);

    int scrollY() const { return m_scrollY; }
    void setScrollY(int scroll);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;
    void mousePressEvent(MouseEvent *event) override;

signals:
    void textChanged();
    void textFormatChanged();
    void colorChanged();
    void backgroundColorChanged();
    void alignmentChanged();
    void wrapModeChanged();
    void scrollYChanged();
    void linkActivated(const QString &link);

private:
    void invalidateTextCache();
    FormattedLine parseRichLine(const QString &line) const;
    QList<FormattedLine> wordWrapFormatted(const FormattedLine &rawLine, int maxW) const;
    const QList<FormattedLine>& renderedLines(int availW) const;

    QString m_text;
    TextFormat m_textFormat = AutoText;

    // "default" is the terminal's foreground (alpha 0 -> SGR 39),
    // not invisible ink: a fixed "white" default is unreadable on a light theme.
    Color m_color = "default";
    // Resolved caches mirror the Color members: an ANSI-named default must not be
    // written as literal RGB, or the terminal theme stops applying to it (setters
    // early-return when the color is unchanged, so the initializer is the only chance).
    uint32_t m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);

    Color m_backgroundColor = "transparent";
    uint32_t m_resolvedBackgroundColor = 0x00000000;

    QString m_alignment = "left";
    bool m_wrapMode = true;
    int m_scrollY = 0;

    mutable int m_cachedWidth = -1;
    mutable QList<FormattedLine> m_cachedLines;
};

} // namespace qamui
