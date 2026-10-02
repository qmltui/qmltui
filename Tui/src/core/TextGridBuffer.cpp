#include "TextGridBuffer.h"
#include <sstream>
#include <algorithm>

namespace qtui {

TextGridBuffer::TextGridBuffer() = default;

void TextGridBuffer::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_cells.resize(width * height);
    clear();
}

void TextGridBuffer::clear() {
    Cell emptyCell;
    emptyCell.character[0] = ' '; // Initialize with spaces
    std::fill(m_cells.begin(), m_cells.end(), emptyCell);
}

Cell& TextGridBuffer::cellAt(int x, int y) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return m_cells[y * m_width + x];
    }
    return m_emptyCell; // Fallback
}

const Cell& TextGridBuffer::cellAt(int x, int y) const {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return m_cells[y * m_width + x];
    }
    return m_emptyCell;
}

std::string TextGridBuffer::diff(const TextGridBuffer& nextBuffer) const {
    int curX = 0, curY = 0; // Assume cursor starts at 0,0 for relative diff
    return diffInternal(nextBuffer, true, curX, curY);
}

std::string TextGridBuffer::diffRelative(const TextGridBuffer& nextBuffer) const {
    int curX = 0, curY = 0; // Assume cursor starts at top-left of the region
    return diffInternal(nextBuffer, false, curX, curY);
}

std::string TextGridBuffer::diffInternal(const TextGridBuffer& nextBuffer, bool absolute, int& curX, int& curY) const {
    std::ostringstream ansi;
    
    // Simplistic diffing
    for (int y = 0; y < nextBuffer.height(); ++y) {
        for (int x = 0; x < nextBuffer.width(); ++x) {
            if (x >= m_width || y >= m_height || nextBuffer.cellAt(x, y) != cellAt(x, y)) {
                const Cell& nextCell = nextBuffer.cellAt(x, y);
                
                if (absolute) {
                    ansi << "\033[" << (y + 1) << ";" << (x + 1) << "H";
                } else {
                    int dy = y - curY;
                    int dx = x - curX;
                    if (dy > 0) ansi << "\033[" << dy << "B";
                    else if (dy < 0) ansi << "\033[" << -dy << "A";
                    
                    if (dx > 0) ansi << "\033[" << dx << "C";
                    else if (dx < 0) ansi << "\033[" << -dx << "D";
                }
                curX = x + 1; // After writing char, cursor moves right
                curY = y;
                
                // Reset attributes
                ansi << "\033[0m";
                
                // Set fg and bg colors if they are opaque
                if ((nextCell.fgColor >> 24) != 0) {
                    uint8_t fgR = (nextCell.fgColor >> 16) & 0xFF;
                    uint8_t fgG = (nextCell.fgColor >> 8) & 0xFF;
                    uint8_t fgB = nextCell.fgColor & 0xFF;
                    ansi << "\033[38;2;" << (int)fgR << ";" << (int)fgG << ";" << (int)fgB << "m";
                }
                
                if ((nextCell.bgColor >> 24) != 0) {
                    uint8_t bgR = (nextCell.bgColor >> 16) & 0xFF;
                    uint8_t bgG = (nextCell.bgColor >> 8) & 0xFF;
                    uint8_t bgB = nextCell.bgColor & 0xFF;
                    ansi << "\033[48;2;" << (int)bgR << ";" << (int)bgG << ";" << (int)bgB << "m";
                }
                
                // Set attributes
                if (static_cast<uint8_t>(nextCell.attributes & TextAttribute::Bold))      ansi << "\033[1m";
                if (static_cast<uint8_t>(nextCell.attributes & TextAttribute::Italic))    ansi << "\033[3m";
                if (static_cast<uint8_t>(nextCell.attributes & TextAttribute::Underline)) ansi << "\033[4m";
                if (static_cast<uint8_t>(nextCell.attributes & TextAttribute::Reversed))  ansi << "\033[7m";
                
                // Write character
                for (char c : nextCell.character) {
                    if (c != '\0') ansi << c;
                }
                if (nextCell.character[0] == '\0') {
                    ansi << ' ';
                }
            }
        }
    }
    return ansi.str();
}

std::string TextGridBuffer::toAnsiString() const {
    std::ostringstream ansi;
    uint32_t lastFg = 0;
    uint32_t lastBg = 0;
    uint8_t lastAttr = 0;

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            const Cell& cell = cellAt(x, y);
            
            bool colorChanged = false;
            if (cell.fgColor != lastFg) { lastFg = cell.fgColor; colorChanged = true; }
            if (cell.bgColor != lastBg) { lastBg = cell.bgColor; colorChanged = true; }
            if (cell.attributes != static_cast<TextAttribute>(lastAttr)) { lastAttr = static_cast<uint8_t>(cell.attributes); colorChanged = true; }
            
            if (colorChanged) {
                ansi << "\033[0m"; // Reset first to be safe
                if ((lastFg >> 24) != 0) {
                    ansi << "\033[38;2;" << ((lastFg >> 16) & 0xFF) << ";" << ((lastFg >> 8) & 0xFF) << ";" << (lastFg & 0xFF) << "m";
                }
                if ((lastBg >> 24) != 0) {
                    ansi << "\033[48;2;" << ((lastBg >> 16) & 0xFF) << ";" << ((lastBg >> 8) & 0xFF) << ";" << (lastBg & 0xFF) << "m";
                }
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Bold))      ansi << "\033[1m";
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Italic))    ansi << "\033[3m";
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Underline)) ansi << "\033[4m";
                if (lastAttr & static_cast<uint8_t>(TextAttribute::Reversed))  ansi << "\033[7m";
            }
            
            if (cell.character[0] != '\0') {
                for (char c : cell.character) {
                    if (c != '\0') ansi << c;
                }
            } else {
                ansi << ' ';
            }
        }
        ansi << "\033[0m\n"; // Reset at end of line and add newline
        lastFg = 0; lastBg = 0; lastAttr = 0;
    }
    
    // Remove the very last newline if we want it completely flush, but standard logs have \n
    std::string result = ansi.str();
    if (!result.empty() && result.back() == '\n') {
        result.pop_back(); // Remove the trailing newline since print() usually appends it if needed, or we just keep it?
        // Actually print() appends \n if it doesn't end with one. So let's leave it without the trailing newline,
        // to avoid double newlines.
    }
    return result;
}

} // namespace qtui
