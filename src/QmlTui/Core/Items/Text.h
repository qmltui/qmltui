#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
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

// Terminal text item supporting plain text, ANSI escape sequences, subset HTML
// rich text (<b>, <i>, <u>, <font color>, <a href>), structured Span lists,
// wrapping, alignment, and foreground gradients.
class QMLTUI_CORE_EXPORT Text : public Item {
    Q_OBJECT
    QML_ELEMENT

    // Structured text runs; when non-empty, takes precedence over `text`.
    Q_PROPERTY(QList<qmltui::Span> spans READ spans WRITE setSpans NOTIFY spansChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(bool bold READ bold WRITE setBold NOTIFY boldChanged)
    Q_PROPERTY(bool dim READ dim WRITE setDim NOTIFY dimChanged)
    Q_PROPERTY(bool italic READ italic WRITE setItalic NOTIFY italicChanged)
    Q_PROPERTY(bool underline READ underline WRITE setUnderline NOTIFY underlineChanged)
    Q_PROPERTY(bool reversed READ reversed WRITE setReversed NOTIFY reversedChanged)
    Q_PROPERTY(
        bool strikethrough READ strikethrough WRITE setStrikethrough NOTIFY strikethroughChanged)
    Q_PROPERTY(Qt::Alignment horizontalAlignment READ horizontalAlignment WRITE
                   setHorizontalAlignment NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(Qt::Alignment verticalAlignment READ verticalAlignment WRITE setVerticalAlignment
                   NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(TextFormat textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)
    Q_PROPERTY(qmltui::Gradient* gradient READ gradient WRITE setGradient NOTIFY gradientChanged)
    Q_PROPERTY(WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)
    Q_PROPERTY(qmltui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
                   backgroundColorChanged)

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

    explicit Text(QObject* parent = nullptr);
    ~Text() override;

    QList<Span> spans() const { return m_spans; }
    void setSpans(const QList<Span>& spans);

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

    bool strikethrough() const { return m_strikethrough; }
    void setStrikethrough(bool strikethrough);

    Qt::Alignment horizontalAlignment() const { return m_horizontalAlignment; }
    void setHorizontalAlignment(Qt::Alignment alignment);

    Qt::Alignment verticalAlignment() const { return m_verticalAlignment; }
    void setVerticalAlignment(Qt::Alignment alignment);

    TextFormat textFormat() const { return m_textFormat; }
    void setTextFormat(TextFormat format);

    WrapMode wrapMode() const { return m_wrapMode; }
    void setWrapMode(WrapMode mode);

    Gradient* gradient() const { return m_gradient.data(); }
    void setGradient(Gradient* gradient);

    int visualLength(const QString& line) const;

    void paint(TextGridBuffer& buffer, int absX, int absY,
               const ClipRect& clipRect = ClipRect()) const override;
    void mousePressEvent(class MouseEvent* event) override;

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
    void strikethroughChanged();
    void horizontalAlignmentChanged();
    void verticalAlignmentChanged();
    void textFormatChanged();
    void wrapModeChanged();
    void gradientChanged();
    void linkActivated(const QString& link);

private:
    struct StyleRun {
        qsizetype endOffset = 0;
        TextStyle style;
        uint16_t linkIndex = 0;
    };

    // One grapheme cluster after inline ANSI/HTML markup or Span runs have been segmented.
    struct GlyphEntry {
        TextStyle style;
        uint8_t advance = 1;      // raw cluster width: drives layout and clipping
        uint8_t cellWidth = 1;    // width written to the cell (controls become visible glyphs)
        uint16_t linkIndex = 0;   // 0 = no link, 1-based index into m_links
        uint32_t glyphOffset = 0; // into m_glyphBytes
        uint32_t glyphLength = 0;
    };
    struct UnwrappedLine {
        int visualWidth = 0;
        int firstGlyph = 0;
        int glyphCount = 0;
    };
    struct LineLayout {
        int visualWidth = 0;
        int firstGlyph = 0;
        int glyphCount = 0;
    };

    // Parses one line of markup into glyph entries (when entries is non-null)
    // and returns its visual width.
    int parseLine(QStringView line, std::vector<GlyphEntry>* entries, QByteArray* bytes) const;
    int layoutLineRuns(QStringView line, const StyleRun* runs, qsizetype runCount,
                       qsizetype lineStartOffset, std::vector<GlyphEntry>* entries,
                       QByteArray* bytes) const;
    void recordUnwrappedLine(int firstGlyph, int lineVisualWidth) const;
    void commitLine(int firstGlyph, int glyphCount, int lineVisualWidth, int availW) const;
    TextAttribute baseAttributes() const;
    int appendGrapheme(const GraphemeRef& cluster, const TextStyle& style, uint16_t linkIndex,
                       std::vector<GlyphEntry>* entries, QByteArray* bytes) const;
    bool isSpaceGlyph(const GlyphEntry& g) const;
    void wrapGlyphs(int startGlyph, int glyphCount, int availW) const;
    uint16_t registerLink(QStringView href) const;
    int contentStartY(int availH, int lineCount) const;
    int lineStartX(int leftLimit, int availW, int visLen) const;
    void parseSpans() const;
    void ensureParsed() const;
    void ensureLayout() const;
    void invalidateParse() {
        m_parseDirty = true;
        m_layoutDirty = true;
    }
    void invalidateWrap() { m_layoutDirty = true; }
    void updateImplicitSize();

    QList<Span> m_spans;
    QString m_text;
    mutable std::vector<GlyphEntry> m_glyphs;
    mutable std::vector<UnwrappedLine> m_unwrappedLines;
    mutable std::vector<LineLayout> m_lines;
    mutable std::vector<QString> m_links;
    mutable QByteArray m_glyphBytes;
    mutable int m_naturalImplicitWidth = 0;
    mutable bool m_parseDirty = true;
    mutable bool m_layoutDirty = true;
    bool m_updatingImplicitSize = false;
    Color m_color;
    uint32_t m_resolvedColor = Color::resolve(m_color, Color::DefaultTag);
    Color m_backgroundColor = Color::Transparent;
    uint32_t m_resolvedBackgroundColor = 0x00000000;
    bool m_bold = false;
    bool m_dim = false;
    bool m_italic = false;
    bool m_underline = false;
    bool m_reversed = false;
    bool m_strikethrough = false;
    Qt::Alignment m_horizontalAlignment = Qt::AlignLeft;
    Qt::Alignment m_verticalAlignment = Qt::AlignTop;
    TextFormat m_textFormat = AutoText;
    WrapMode m_wrapMode = NoWrap;
    QPointer<Gradient> m_gradient;
};

} // namespace qmltui
