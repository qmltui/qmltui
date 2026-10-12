#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/ColorMapper.h>
#include <QmlTui/Core/Primitives/Span.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>
#include <QList>
#include <QByteArray>
#include <QString>
#include <QPointer>
#include <QmlTui/Core/Items/Gradient.h>
#include <cstdint>
#include <vector>

namespace qmltui {

/**
 * @brief Terminal text rendering element with Unicode formatting and alignment.
 *
 * Text renders a single-line or styled string into character cells. Supports
 * ANSI escape parsing, rich text spans, horizontal and vertical alignments,
 * bold/dim/italic/underline/reverse video attributes, and color gradients.
 *
 * Automatically measures implicit width and height based on string character
 * length and East Asian full-width Unicode characters.
 *
 * @thread_safety Main/engine thread only.
 * @see Span, Item
 */
class QMLTUI_CORE_EXPORT Text : public Item {
    Q_OBJECT
    QML_ELEMENT

    /// @brief Formatted text spans with individual colors and attributes.
    Q_PROPERTY(QList<qmltui::Span> spans READ spans WRITE setSpans NOTIFY spansChanged)
    /// @brief Plain or formatted text content to display.
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// @brief Text foreground color. Defaults to "default" (host terminal foreground).
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    /// @brief Whether text is rendered with the bold / bright attribute.
    Q_PROPERTY(bool bold READ bold WRITE setBold NOTIFY boldChanged)
    /// @brief Whether text is rendered with dim / faint attribute.
    Q_PROPERTY(bool dim READ dim WRITE setDim NOTIFY dimChanged)
    /// @brief Whether text is rendered in italics (requires terminal emulator support).
    Q_PROPERTY(bool italic READ italic WRITE setItalic NOTIFY italicChanged)
    /// @brief Whether text is rendered with an underline.
    Q_PROPERTY(bool underline READ underline WRITE setUnderline NOTIFY underlineChanged)
    /// @brief Whether text is rendered in reverse video (swapped foreground/background).
    Q_PROPERTY(bool reversed READ reversed WRITE setReversed NOTIFY reversedChanged)
    /// @brief Horizontal alignment: Text.AlignLeft, Text.AlignRight, Text.AlignHCenter.
    Q_PROPERTY(int horizontalAlignment READ horizontalAlignment WRITE setHorizontalAlignment NOTIFY horizontalAlignmentChanged)
    /// @brief Vertical alignment: Text.AlignTop, Text.AlignBottom, Text.AlignVCenter.
    Q_PROPERTY(int verticalAlignment READ verticalAlignment WRITE setVerticalAlignment NOTIFY verticalAlignmentChanged)
    /// @brief Text parsing format (PlainText, RichText, AnsiText, AutoText).
    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)
    /// @brief Optional color gradient applied across the characters.
    Q_PROPERTY(qmltui::Gradient* gradient READ gradient WRITE setGradient NOTIFY gradientChanged)
    /// @brief Text wrapping mode: Text.NoWrap, Text.WordWrap, Text.WrapAnywhere, Text.Wrap.
    Q_PROPERTY(WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)
    /// @brief Background color of character cells. Defaults to "transparent".
    Q_PROPERTY(qmltui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

public:
    enum TextFormat : uint8_t {
        PlainText,
        RichText,
        AnsiText,
        AutoText,
    };
    Q_ENUM(TextFormat)

    enum WrapMode : uint8_t {
        NoWrap = 0,
        WordWrap = 1,
        WrapAnywhere = 2,
        Wrap = 3,
    };
    Q_ENUM(WrapMode)

    explicit Text(QObject *parent = nullptr);
    ~Text() override;

    QList<Span> spans() const { return m_spans; }
    void setSpans(const QList<Span> &spans);

    QString text() const { return m_text; }
    void setText(const QString& text);

    Color color() const { return m_color; }
    void setColor(Color color);

    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(Color color);

    bool bold() const { return m_bold; }
    void setBold(bool bold);
    bool dim() const { return m_dim; }
    void setDim(bool dim);

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

    WrapMode wrapMode() const { return m_wrapMode; }
    void setWrapMode(WrapMode mode);

    Gradient* gradient() const { return m_gradient.data(); }
    void setGradient(Gradient* gradient);

    int visualLength(const QString &line) const;

    void paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) const override;
    void mousePressEvent(class MouseEvent *event) override;

signals:
    void spansChanged();
    void textChanged();
    void colorChanged();
    void backgroundColorChanged();
    void boldChanged();
    void dimChanged();
    void italicChanged();
    void underlineChanged();
    void reversedChanged();
    void horizontalAlignmentChanged();
    void verticalAlignmentChanged();
    void textFormatChanged();
    void wrapModeChanged();
    void gradientChanged();
    void linkActivated(const QString &link);

private:
    // One grapheme cluster after inline ANSI/HTML markup has been resolved.
    struct GlyphEntry {
        uint32_t fgColor = 0xFFFFFFFF;
        uint32_t bgColor = 0x00000000;
        TextAttribute attributes = TextAttribute::None;
        uint8_t advance = 1;    // raw cluster width: drives layout and clipping
        uint8_t cellWidth = 1;  // width written to the cell (controls become visible glyphs)
        uint16_t linkIndex = 0; // 0 = no link, 1-based index into m_links
        uint32_t glyphOffset = 0;   // into m_glyphBytes
        uint32_t glyphLength = 0;
    };
    struct LineLayout {
        int visualWidth = 0;
        int firstGlyph = 0;
        int glyphCount = 0;
    };

    // Parses one line of markup into glyph entries (when entries is non-null)
    // and returns its visual width.
    int parseLine(const QString &line, std::vector<GlyphEntry> *entries,
                  QByteArray *bytes) const;
    TextAttribute baseAttributes() const;
    int appendGrapheme(const GraphemeRef &cluster, uint32_t fg, uint32_t bg,
                       TextAttribute attributes, const QString &href,
                       std::vector<GlyphEntry> *entries, QByteArray *bytes) const;
    bool isSpaceGlyph(const GlyphEntry &g) const;
    void wrapGlyphs(int startGlyph, int glyphCount, int availW) const;
    uint16_t registerLink(const QString &href) const;
    void layoutSpans() const;
    void ensureLayout() const;
    void invalidateLayout() { m_layoutDirty = true; }
    void updateImplicitSize();

    QList<Span> m_spans;
    QString m_text;
    mutable std::vector<GlyphEntry> m_glyphs;
    mutable std::vector<LineLayout> m_lines;
    mutable std::vector<QString> m_links;
    mutable QByteArray m_glyphBytes;
    mutable bool m_layoutDirty = true;
    // "default" is the terminal's foreground (alpha 0 -> SGR 39),
    // not invisible ink: a fixed "white" default is unreadable on a light theme.
    Color m_color = "default";
    // Derived from m_color, never written by hand: "white" is ANSI index 15,
    // not literal RGB, and setColor() early-returns when the color is unchanged.
    uint32_t m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    Color m_backgroundColor = "transparent";
    uint32_t m_resolvedBackgroundColor = 0x00000000;
    bool m_bold = false;
    bool m_dim = false;
    bool m_italic = false;
    bool m_underline = false;
    bool m_reversed = false;
    int m_horizontalAlignment = Qt::AlignLeft;
    int m_verticalAlignment = Qt::AlignTop;
    TextFormat m_textFormat = AutoText;
    WrapMode m_wrapMode = NoWrap;
    QPointer<Gradient> m_gradient;
};

} // namespace qmltui
