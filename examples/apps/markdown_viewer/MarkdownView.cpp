#include "MarkdownView.h"

#include <MouseEvent.h>
#include <UnicodeText.h>
#include <algorithm>

namespace qtui {

MarkdownView::MarkdownView(QObject *parent) : Item(parent) {
    setImplicitWidth(80);
    setImplicitHeight(24);
    connect(this, &Item::widthChanged, this, &MarkdownView::updateDimensions);
    connect(this, &Item::heightChanged, this, &MarkdownView::clampScroll);
}

void MarkdownView::setDocument(MarkdownDocument *document) {
    if (m_document == document)
        return;
    if (m_document)
        disconnect(m_document, nullptr, this, nullptr);

    m_document = document;
    if (m_document) {
        connect(m_document, &MarkdownDocument::parsedChanged, this, [this]() {
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

int MarkdownView::contentHeight() const {
    return m_document ? m_document->lineCount() : 0;
}

int MarkdownView::viewportHeight() const {
    return std::max(0, height() - (m_borderStyle == QStringLiteral("none") ? 0 : 2));
}

int MarkdownView::lineNumberDigits() const {
    const int lines = std::max(1, contentHeight());
    int digits = 1;
    for (int value = lines; value >= 10; value /= 10)
        ++digits;
    return digits;
}

int MarkdownView::lineNumberGutterWidth() const {
    // Right-aligned number, one space, a separator, then one content margin.
    return lineNumberDigits() + 3;
}

void MarkdownView::setScrollY(int y) {
    const int maximum = std::max(0, contentHeight() - viewportHeight());
    const int clamped = std::clamp(y, 0, maximum);
    if (m_scrollY == clamped)
        return;
    m_scrollY = clamped;
    emit scrollYChanged();
    markDirty();
}

void MarkdownView::clampScroll() {
    setScrollY(m_scrollY);
}

void MarkdownView::setSearchQuery(const QString &query) {
    if (m_searchQuery == query)
        return;
    m_searchQuery = query;
    rebuildSearchMatches();
    emit searchQueryChanged();
    markDirty();
}

void MarkdownView::rebuildSearchMatches() {
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

void MarkdownView::setActiveMatchIndex(int index) {
    const int clamped = m_searchMatches.isEmpty()
        ? 0 : std::clamp(index, 0, static_cast<int>(m_searchMatches.size() - 1));
    if (m_activeMatchIndex == clamped)
        return;
    m_activeMatchIndex = clamped;
    emit activeMatchIndexChanged();
    markDirty();
}

void MarkdownView::setPalette(const MarkdownPalette &palette) {
    if (m_palette == palette)
        return;
    m_palette = palette;
    emit paletteChanged();
    markDirty();
}

void MarkdownView::setBorderStyle(const QString &style) {
    const QString normalized = style.trimmed().toLower();
    const QString value = normalized.isEmpty() ? QStringLiteral("none") : normalized;
    if (m_borderStyle == value)
        return;
    m_borderStyle = value;
    emit borderStyleChanged();
    updateDimensions();
    clampScroll();
    markDirty();
}

void MarkdownView::updateDimensions() {
    if (!m_document || m_updatingDimensions)
        return;

    m_updatingDimensions = true;
    const int borderWidth = m_borderStyle == QStringLiteral("none") ? 0 : 2;
    int gutterWidth = lineNumberGutterWidth();
    while (true) {
        m_document->setWrapWidth(std::max(1, width() - borderWidth - gutterWidth));
        const int updatedGutterWidth = lineNumberGutterWidth();
        if (updatedGutterWidth == gutterWidth)
            break;
        gutterWidth = updatedGutterWidth;
    }
    m_updatingDimensions = false;
    clampScroll();
}

void MarkdownView::scrollToLine(int lineIndex) {
    setScrollY(lineIndex);
}

void MarkdownView::scrollBy(int delta) {
    setScrollY(m_scrollY + delta);
}

void MarkdownView::pageUp() {
    scrollBy(-std::max(1, viewportHeight() - 1));
}

void MarkdownView::pageDown() {
    scrollBy(std::max(1, viewportHeight() - 1));
}

void MarkdownView::drawBorder(TextGridBuffer &buffer, int absX, int absY,
                              const ClipRect &clipRect) const {
    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0)
        return;

    QString topLeft = QStringLiteral("┌");
    QString topRight = QStringLiteral("┐");
    QString bottomLeft = QStringLiteral("└");
    QString bottomRight = QStringLiteral("┘");
    QString horizontal = QStringLiteral("─");
    QString vertical = QStringLiteral("│");
    if (m_borderStyle == QStringLiteral("double")) {
        topLeft = QStringLiteral("╔"); topRight = QStringLiteral("╗");
        bottomLeft = QStringLiteral("╚"); bottomRight = QStringLiteral("╝");
        horizontal = QStringLiteral("═"); vertical = QStringLiteral("║");
    } else if (m_borderStyle == QStringLiteral("ascii")) {
        topLeft = topRight = bottomLeft = bottomRight = QStringLiteral("+");
        horizontal = QStringLiteral("-"); vertical = QStringLiteral("|");
    }

    const uint32_t color = m_palette.resolve(MarkdownRole::Border);
    auto write = [&](int x, int y, const QString &text) {
        if (clipRect.contains(x, y))
            buffer.writeText(x, y, text, color);
    };
    write(absX, absY, topLeft);
    if (w > 1)
        write(absX + w - 1, absY, topRight);
    if (h > 1) {
        write(absX, absY + h - 1, bottomLeft);
        if (w > 1)
            write(absX + w - 1, absY + h - 1, bottomRight);
    }
    for (int x = 1; x < w - 1; ++x) {
        write(absX + x, absY, horizontal);
        if (h > 1)
            write(absX + x, absY + h - 1, horizontal);
    }
    for (int y = 1; y < h - 1; ++y) {
        write(absX, absY + y, vertical);
        if (w > 1)
            write(absX + w - 1, absY + y, vertical);
    }
}

void MarkdownView::render(TextGridBuffer &buffer, int absX, int absY,
                          const ClipRect &clipRect) {
    if (!visible() || width() <= 0 || height() <= 0)
        return;

    Item::render(buffer, absX, absY, clipRect);
    const ClipRect ownClip = clipRect.intersected({absX, absY, width(), height()});
    const bool bordered = m_borderStyle != QStringLiteral("none");
    if (bordered)
        drawBorder(buffer, absX, absY, ownClip);

    const int startX = absX + (bordered ? 1 : 0);
    const int startY = absY + (bordered ? 1 : 0);
    const int availableWidth = width() - (bordered ? 2 : 0);
    const int availableHeight = height() - (bordered ? 2 : 0);
    if (availableWidth <= 0 || availableHeight <= 0 || !m_document)
        return;

    const auto &lines = m_document->renderedLines();
    const int numberDigits = lineNumberDigits();
    const int gutterWidth = lineNumberGutterWidth();
    for (int row = 0; row < availableHeight; ++row) {
        const int lineIndex = m_scrollY + row;
        if (lineIndex >= lines.size())
            break;
        const DocumentLine &line = lines[lineIndex];
        const int y = startY + row;

        const QString lineNumber = QStringLiteral("%1").arg(lineIndex + 1, numberDigits, 10,
                                                             QLatin1Char(' '));
        if (ownClip.contains(startX, y)
            && ownClip.contains(startX + numberDigits - 1, y)) {
            buffer.writeText(startX, y, lineNumber, m_palette.resolvedTextMuted());
        }
        const int separatorX = startX + numberDigits + 1;
        if (ownClip.contains(separatorX, y)) {
            buffer.writeText(separatorX, y, QStringLiteral("│"),
                             m_palette.resolvedBorder());
        }

        int prefixWidth = 0;
        for (int depth = 0; depth < line.quoteDepth; ++depth) {
            const int x = startX + gutterWidth + prefixWidth;
            if (ownClip.contains(x, y) && ownClip.contains(x + 1, y)) {
                buffer.writeText(x, y, QStringLiteral("│ "),
                                 m_palette.resolve(MarkdownRole::Quote), 0x00000000,
                                 TextAttribute::Bold);
            }
            prefixWidth += 2;
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
                uint32_t foreground = m_palette.resolve(segment.foreground);
                uint32_t background = m_palette.resolve(segment.background);
                TextAttribute attributes = segment.attr;

                for (int matchIndex = 0; matchIndex < m_searchMatches.size(); ++matchIndex) {
                    const SearchMatch &match = m_searchMatches[matchIndex];
                    if (match.line != lineIndex)
                        continue;
                    const int matchEnd = match.utf16Column + match.utf16Length;
                    if (clusterStart < matchEnd && clusterEnd > match.utf16Column) {
                        if (matchIndex == m_activeMatchIndex) {
                            foreground = m_palette.resolvedActiveMatchFg();
                            background = m_palette.resolvedActiveMatchBg();
                            attributes = attributes | TextAttribute::Bold;
                        } else {
                            foreground = m_palette.resolvedSearchMatchFg();
                            background = m_palette.resolvedSearchMatchBg();
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

void MarkdownView::mousePressEvent(MouseEvent *event) {
    if (!m_document || width() <= 0 || height() <= 0)
        return;
    const int borderOffset = m_borderStyle == QStringLiteral("none") ? 0 : 1;
    const int lineIndex = event->y() - borderOffset + m_scrollY;
    const auto &lines = m_document->renderedLines();
    if (lineIndex < 0 || lineIndex >= lines.size())
        return;

    const DocumentLine &line = lines[lineIndex];
    const int clickX = event->x() - borderOffset - lineNumberGutterWidth();
    if (clickX < 0)
        return;
    int x = line.indent + line.quoteDepth * 2;
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

void MarkdownView::wheelEvent(MouseEvent *event) {
    if (event->wheelDeltaY() > 0) {
        scrollBy(-3);
        event->setAccepted(true);
    } else if (event->wheelDeltaY() < 0) {
        scrollBy(3);
        event->setAccepted(true);
    }
}

} // namespace qtui
