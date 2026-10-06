#include "MdView.h"

#include <MouseEvent.h>
#include <UnicodeText.h>
#include <algorithm>

namespace qtui {

MdView::MdView(QObject *parent) : Item(parent) {
    setImplicitWidth(80);
    setImplicitHeight(24);
    connect(this, &Item::widthChanged, this, &MdView::updateDimensions);
    connect(this, &Item::heightChanged, this, &MdView::clampScroll);
}

void MdView::setDocument(MdDoc *document) {
    if (m_document == document)
        return;
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);

    m_document = document;
    if (m_document) {
        connect(m_document, &MdDoc::parsedChanged, this, [this]() {
            rebuildSearchMatches();
            updateDimensions();
            clampScroll();
            emit contentHeightChanged();
            markDirty();
        });
        connect(m_document, &QObject::destroyed, this, [this]() {
            m_document = nullptr;
            m_searchMatches.clear();
            m_scrollY = 0;
            emit documentChanged();
            emit contentHeightChanged();
            emit scrollYChanged();
            markDirty();
        });
        updateDimensions();
    } else {
        m_searchMatches.clear();
        clampScroll();
    }

    emit documentChanged();
    emit contentHeightChanged();
    markDirty();
}

void MdView::setStyle(MdViewStyle *style) {
    if (m_style == style)
        return;
    if (m_style)
        disconnect(m_style, nullptr, this, nullptr);
    m_style = style;
    if (m_style) {
        connect(m_style, &MdViewStyle::styleChanged, this, [this]() {
            updateDimensions();
            clampScroll();
            markDirty();
        });
    }
    emit styleChanged();
    updateDimensions();
    clampScroll();
    markDirty();
}

MdPalette MdView::effectivePalette() const {
    if (m_explicitPalette)
        return m_palette;
    if (m_style && m_style->palette() != MdPalette())
        return m_style->palette();
    return m_palette;
}

int MdView::contentHeight() const {
    return m_document ? m_document->lineCount() : 0;
}

int MdView::viewportHeight() const {
    return std::max(0, height());
}

int MdView::lineNumberDigits() const {
    const int lines = std::max(1, contentHeight());
    int digits = 1;
    for (int value = lines; value >= 10; value /= 10)
        ++digits;
    return digits;
}

int MdView::lineNumberGutterWidth() const {
    if (!showLineNumbers())
        return 0;
    // Right-aligned number, one space, a separator, then one content margin.
    return lineNumberDigits() + 3;
}

bool MdView::showLineNumbers() const {
    if (m_explicitShowLineNumbers)
        return m_showLineNumbers;
    if (m_style)
        return m_style->showLineNumbers();
    return m_showLineNumbers;
}

void MdView::setShowLineNumbers(bool show) {
    m_explicitShowLineNumbers = true;
    if (m_showLineNumbers == show)
        return;
    m_showLineNumbers = show;
    emit showLineNumbersChanged();
    updateDimensions();
    clampScroll();
    markDirty();
}

QString MdView::renderStyle() const {
    return m_document ? m_document->renderStyle() : QStringLiteral("modern");
}

void MdView::setRenderStyle(const QString &style) {
    if (m_document)
        m_document->setRenderStyle(style);
    emit renderStyleChanged();
    markDirty();
}

void MdView::setScrollY(int y) {
    const int maximum = std::max(0, contentHeight() - viewportHeight());
    const int clamped = std::clamp(y, 0, maximum);
    if (m_scrollY == clamped)
        return;
    m_scrollY = clamped;
    emit scrollYChanged();
    markDirty();
}

void MdView::clampScroll() {
    setScrollY(m_scrollY);
}

void MdView::setSearchQuery(const QString &query) {
    if (m_searchQuery == query)
        return;
    m_searchQuery = query;
    rebuildSearchMatches();
    emit searchQueryChanged();
    markDirty();
}

void MdView::rebuildSearchMatches() {
    const int previousIndex = m_activeMatchIndex;
    m_searchMatches = m_document ? m_document->searchMatches(m_searchQuery)
                                 : QList<SearchMatch>{};
    if (m_searchMatches.isEmpty())
        m_activeMatchIndex = 0;
    else
        m_activeMatchIndex = std::clamp(m_activeMatchIndex, 0,
                                        static_cast<int>(m_searchMatches.size() - 1));
    if (m_activeMatchIndex != previousIndex)
        emit activeMatchIndexChanged();
}

void MdView::setActiveMatchIndex(int index) {
    const int clamped = m_searchMatches.isEmpty()
        ? 0 : std::clamp(index, 0, static_cast<int>(m_searchMatches.size() - 1));
    if (m_activeMatchIndex == clamped)
        return;
    m_activeMatchIndex = clamped;
    emit activeMatchIndexChanged();
    markDirty();
}

void MdView::setPalette(const MdPalette &palette) {
    m_explicitPalette = true;
    if (m_palette == palette)
        return;
    m_palette = palette;
    emit paletteChanged();
    markDirty();
}

void MdView::updateDimensions() {
    if (!m_document || m_updatingDimensions)
        return;

    m_updatingDimensions = true;
    int gutterWidth = lineNumberGutterWidth();
    while (true) {
        m_document->setWrapWidth(std::max(1, width() - gutterWidth));
        const int updatedGutterWidth = lineNumberGutterWidth();
        if (updatedGutterWidth == gutterWidth)
            break;
        gutterWidth = updatedGutterWidth;
    }
    m_updatingDimensions = false;
    clampScroll();
}

void MdView::scrollToLine(int lineIndex) {
    setScrollY(lineIndex);
}

void MdView::scrollBy(int delta) {
    setScrollY(m_scrollY + delta);
}

void MdView::pageUp() {
    scrollBy(-std::max(1, viewportHeight() - 1));
}

void MdView::pageDown() {
    scrollBy(std::max(1, viewportHeight() - 1));
}

void MdView::paint(TextGridBuffer &buffer, int absX, int absY,
                    const ClipRect &clipRect) const {
    const ClipRect ownClip = clipRect.intersected(ClipRect{absX, absY, width(), height()});
    if (ownClip.width <= 0 || ownClip.height <= 0)
        return;

    const int startX = absX;
    const int startY = absY;
    const int availableWidth = width();
    const int availableHeight = height();
    if (availableWidth <= 0 || availableHeight <= 0 || !m_document)
        return;

    const auto &lines = m_document->renderedLines();
    const int numberDigits = lineNumberDigits();
    const int gutterWidth = lineNumberGutterWidth();
    const MdPalette pal = effectivePalette();

    for (int row = 0; row < availableHeight; ++row) {
        const int lineIndex = m_scrollY + row;
        if (lineIndex >= lines.size())
            break;
        const DocumentLine &line = lines[lineIndex];
        const int y = startY + row;

        if (showLineNumbers()) {
            const QString lineNumber = QStringLiteral("%1").arg(lineIndex + 1, numberDigits, 10,
                                                                 QLatin1Char(' '));
            if (ownClip.contains(startX, y)
                && ownClip.contains(startX + numberDigits - 1, y)) {
                buffer.writeText(startX, y, lineNumber, pal.resolvedTextMuted());
            }
            const int separatorX = startX + numberDigits + 1;
            if (ownClip.contains(separatorX, y)) {
                buffer.writeText(separatorX, y, QStringLiteral("│"),
                                 pal.resolvedBorder());
            }
        }

        const QString quotePrefix = m_document ? m_document->effectiveStyle()->blockquotePrefix()
                                               : QStringLiteral("│ ");
        const int qWidth = std::max(1, UnicodeText::displayWidth(quotePrefix));
        int prefixWidth = 0;
        for (int depth = 0; depth < line.quoteDepth; ++depth) {
            const int x = startX + gutterWidth + prefixWidth;
            if (ownClip.contains(x, y) && (qWidth <= 1 || ownClip.contains(x + qWidth - 1, y))) {
                buffer.writeText(x, y, quotePrefix,
                                 pal.resolve(MdRole::Quote), 0x00000000,
                                 TextAttribute::Bold);
            }
            prefixWidth += qWidth;
        }

        const int contentX = startX + gutterWidth + prefixWidth + line.indent;
        const int contentWidth = std::max(0, availableWidth - gutterWidth
                                             - prefixWidth - line.indent);
        int writtenWidth = 0;
        int utf16Column = 0;
        for (const FormattedSegment &segment : line.segments) {
            for (const Grapheme &cluster : UnicodeText::graphemes(segment.text)) {
                if (writtenWidth + cluster.width > contentWidth)
                    break;
                const int clusterStart = utf16Column;
                const int clusterEnd = clusterStart + cluster.utf16Length;
                uint32_t foreground = pal.resolve(segment.foreground);
                uint32_t background = pal.resolve(segment.background);
                TextAttribute attributes = segment.attr;

                for (int matchIndex = 0; matchIndex < m_searchMatches.size(); ++matchIndex) {
                    const SearchMatch &match = m_searchMatches[matchIndex];
                    if (match.line != lineIndex)
                        continue;
                    const int matchEnd = match.utf16Column + match.utf16Length;
                    if (clusterStart < matchEnd && clusterEnd > match.utf16Column) {
                        if (matchIndex == m_activeMatchIndex) {
                            foreground = pal.resolvedActiveMatchBg();
                            background = pal.resolvedActiveMatchBg();
                            attributes = attributes | TextAttribute::Bold;
                        } else {
                            foreground = pal.resolvedSearchMatchFg();
                            background = pal.resolvedSearchMatchBg();
                        }
                        break;
                    }
                }

                const int x = contentX + writtenWidth;
                const bool fullyClipped = cluster.width == 0
                    ? ownClip.contains(x, y)
                    : ownClip.contains(x, y) && ownClip.contains(x + cluster.width - 1, y);
                if (fullyClipped) {
                    buffer.writeText(x, y, QString::fromUtf8(cluster.utf8), foreground,
                                     background, attributes);
                }
                writtenWidth += cluster.width;
                utf16Column = clusterEnd;
            }
        }
    }
}

void MdView::mousePressEvent(MouseEvent *event) {
    if (!m_document || width() <= 0 || height() <= 0)
        return;
    const int lineIndex = event->y() + m_scrollY;
    const auto &lines = m_document->renderedLines();
    if (lineIndex < 0 || lineIndex >= lines.size())
        return;

    const DocumentLine &line = lines[lineIndex];
    const int clickX = event->x() - lineNumberGutterWidth();
    if (clickX < 0)
        return;
    const QString quotePrefix = m_document->effectiveStyle()->blockquotePrefix();
    const int qWidth = std::max(1, UnicodeText::displayWidth(quotePrefix));
    int x = line.indent + line.quoteDepth * qWidth;
    for (const FormattedSegment &segment : line.segments) {
        const int width = UnicodeText::displayWidth(segment.text);
        if (clickX >= x && clickX < x + width && !segment.href.isEmpty()) {
            emit linkActivated(segment.href);
            event->setAccepted(true);
            return;
        }
        x += width;
    }
}

void MdView::wheelEvent(MouseEvent *event) {
    if (event->wheelDeltaY() > 0) {
        scrollBy(-3);
        event->setAccepted(true);
    } else if (event->wheelDeltaY() < 0) {
        scrollBy(3);
        event->setAccepted(true);
    }
}

} // namespace qtui
