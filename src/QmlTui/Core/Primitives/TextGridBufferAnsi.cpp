// ANSI encoding of TextGridBuffer contents: frame diffs and full-frame
// serialisation. Kept separate from the drawing helpers in TextGridBuffer.cpp.
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QmlTui/Core/Primitives/AnsiEncoder_p.h>

#include <algorithm>
#include <cstdlib>

namespace qmltui {

namespace {

// True when the cell shows as an untouched blank: erase-to-end-of-line can
// stand in for a run of these.
inline bool isDefaultBlank(const Cell& cell) {
    // A never-painted cell carries the transparent tag and shows the terminal
    // default, exactly like an alpha-0 one, so both count as blank here.
    const bool defaultBg = cell.bgColor == 0 || ColorMapper::isTransparent(cell.bgColor);
    if (cell.continuation || !defaultBg || cell.attributes != TextAttribute::None ||
        cell.linkId != 0)
        return false;
    const QByteArrayView glyph = cell.character();
    return glyph.isEmpty() || glyph == " ";
}

} // namespace

std::string TextGridBuffer::diff(const TextGridBuffer& nextBuffer) const {
    std::string ansi;
    diff(nextBuffer, ansi);
    return ansi;
}

std::string TextGridBuffer::diffRelative(const TextGridBuffer& nextBuffer) const {
    std::string ansi;
    diffRelative(nextBuffer, ansi);
    return ansi;
}

void TextGridBuffer::diff(const TextGridBuffer& nextBuffer, std::string& out,
                          const AnsiEncodeOptions& options) const {
    diffInternal(nextBuffer, true, out, options);
}

void TextGridBuffer::diffRelative(const TextGridBuffer& nextBuffer, std::string& out,
                                  const AnsiEncodeOptions& options) const {
    diffInternal(nextBuffer, false, out, options);
}

void TextGridBuffer::diffInternal(const TextGridBuffer& nextBuffer, bool absolute,
                                  std::string& ansi, const AnsiEncodeOptions& options) const {
    const int width = nextBuffer.width();
    const int height = nextBuffer.height();
    ansi.reserve(ansi.size() + static_cast<size_t>(width) * static_cast<size_t>(height) / 4);

    // The cursor starts at 0,0 (absolute) or the top-left of the region (relative).
    int curX = 0;
    int curY = 0;
    bool cursorKnown = !absolute;
    ansi::StyleState style;
    uint8_t activeLinkId = 0;
    const bool sameLinkTable = (m_links == nextBuffer.m_links);

    const auto closeActiveLink = [&] {
        if (activeLinkId != 0) {
            ansi::appendOsc8(ansi, QStringView{});
            activeLinkId = 0;
        }
    };

    const auto appendCursorPosition = [&](int x, int y) {
        if (cursorKnown && curX == x && curY == y) return;
        closeActiveLink();
        if (absolute) {
            ansi += "\033[";
            ansi::appendInt(ansi, y + 1);
            ansi += ';';
            ansi::appendInt(ansi, x + 1);
            ansi += 'H';
        } else {
            const int dy = y - curY;
            const int dx = x - curX;
            if (dy != 0) {
                ansi += "\033[";
                ansi::appendInt(ansi, std::abs(dy));
                ansi += dy > 0 ? 'B' : 'A';
            }
            if (dx != 0) {
                ansi += "\033[";
                ansi::appendInt(ansi, std::abs(dx));
                ansi += dx > 0 ? 'C' : 'D';
            }
        }
        curX = x;
        curY = y;
        cursorKnown = true;
    };

    for (int y = 0; y < height; ++y) {
        const bool rowExisted = y < m_height;
        if (sameLinkTable && rowExisted && width > 0 && m_width == width &&
            std::memcmp(&nextBuffer.cellAt(0, y), &cellAt(0, y),
                        static_cast<size_t>(width) * sizeof(Cell)) == 0) {
            continue;
        }

        const auto cellChanged = [&](int column) {
            if (!rowExisted || column >= m_width) return true;
            const Cell& nextCell = nextBuffer.cellAt(column, y);
            const Cell& prevCell = cellAt(column, y);
            if (!sameLinkTable && (nextCell.linkId != 0 || prevCell.linkId != 0) &&
                nextBuffer.linkUrl(nextCell.linkId) != linkUrl(prevCell.linkId)) {
                return true;
            }
            return nextCell != prevCell;
        };

        int trailingBlankStart = -1;
        int x = 0;
        bool rowErased = false;
        while (x < width && !rowErased) {
            while (x < width && !cellChanged(x))
                ++x;
            if (x >= width) break;

            if (trailingBlankStart < 0) {
                // First column from which the rest of the row is default blanks.
                trailingBlankStart = width;
                if (options.eraseToEndOfLine) {
                    while (trailingBlankStart > 0 &&
                           isDefaultBlank(nextBuffer.cellAt(trailingBlankStart - 1, y)))
                        --trailingBlankStart;
                }
            }

            int runEnd = x + 1;
            while (runEnd < width && cellChanged(runEnd))
                ++runEnd;

            for (int column = x; column < runEnd; ++column) {
                if (column >= trailingBlankStart && width - column >= options.eraseMinimumCells) {
                    // Everything from here to the row end is a default blank in
                    // the next frame, so one erase covers this run and any later
                    // ones (unchanged blanks are rewritten identically).
                    closeActiveLink();
                    appendCursorPosition(column, y);
                    ansi::appendEraseStyle(ansi, style);
                    ansi += "\033[K";
                    rowErased = true;
                    break;
                }

                const Cell& nextCell = nextBuffer.cellAt(column, y);
                if (nextCell.continuation) continue;

                appendCursorPosition(column, y);
                ansi::appendStyleTransition(ansi, nextCell, style, options.colorDepth);
                const QString url = nextBuffer.linkUrl(nextCell.linkId);
                const uint8_t nextLinkId = url.isEmpty() ? 0 : nextCell.linkId;
                if (nextLinkId != activeLinkId) {
                    ansi::appendOsc8(ansi, url);
                    activeLinkId = nextLinkId;
                }
                ansi::appendGlyph(ansi, nextCell);
                curX = column + (std::max)(1, static_cast<int>(nextCell.displayWidth));
                curY = y;
            }

            x = runEnd;
        }
        closeActiveLink();
    }
}

int TextGridBuffer::lastContentColumn(int row) const {
    if (row < 0 || row >= m_height) return -1;
    for (int column = m_width - 1; column >= 0; --column) {
        const Cell& cell = cellAt(column, row);
        if (!cell.continuation && !cell.character().isEmpty() && cell.character() != " ")
            return column;
        if ((cell.bgColor >> 24) != 0 || cell.attributes != TextAttribute::None ||
            cell.linkId != 0)
            return column;
    }
    return -1;
}

void TextGridBuffer::appendRowAnsi(std::string& out, int row, ColorDepth depth) const {
    const int last = lastContentColumn(row);
    const Cell* previous = nullptr;
    uint8_t activeLinkId = 0;
    for (int column = 0; column <= last; ++column) {
        const Cell& cell = cellAt(column, row);
        if (!previous || cell.fgColor != previous->fgColor || cell.bgColor != previous->bgColor ||
            cell.attributes != previous->attributes) {
            ansi::appendFullStyle(out, cell, depth);
            previous = &cell;
        }
        const QString url = linkUrl(cell.linkId);
        const uint8_t nextLinkId = url.isEmpty() ? 0 : cell.linkId;
        if (nextLinkId != activeLinkId) {
            ansi::appendOsc8(out, url);
            activeLinkId = nextLinkId;
        }
        if (!cell.continuation) ansi::appendGlyph(out, cell);
    }
    if (activeLinkId != 0) {
        ansi::appendOsc8(out, QStringView{});
    }
}

std::string TextGridBuffer::toAnsiString() const {
    std::string ansi;
    toAnsiString(ansi);
    return ansi;
}

void TextGridBuffer::toAnsiString(std::string& ansi) const {
    ansi.reserve(ansi.size() + static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 2);
    for (int y = 0; y < m_height; ++y) {
        uint32_t lastFg = 0;
        uint32_t lastBg = 0;
        TextAttribute lastAttributes = TextAttribute::None;
        uint8_t activeLinkId = 0;
        for (int x = 0; x < m_width; ++x) {
            const Cell& cell = cellAt(x, y);
            if (cell.fgColor != lastFg || cell.bgColor != lastBg ||
                cell.attributes != lastAttributes) {
                lastFg = cell.fgColor;
                lastBg = cell.bgColor;
                lastAttributes = cell.attributes;
                ansi::appendFullStyle(ansi, cell, ColorDepth::TrueColor);
            }
            const QString url = linkUrl(cell.linkId);
            const uint8_t nextLinkId = url.isEmpty() ? 0 : cell.linkId;
            if (nextLinkId != activeLinkId) {
                ansi::appendOsc8(ansi, url);
                activeLinkId = nextLinkId;
            }
            if (!cell.continuation) ansi::appendGlyph(ansi, cell);
        }
        if (activeLinkId != 0) {
            ansi::appendOsc8(ansi, QStringView{});
        }
        ansi += "\033[0m";
        if (y + 1 < m_height) ansi += '\n';
    }
}

} // namespace qmltui
