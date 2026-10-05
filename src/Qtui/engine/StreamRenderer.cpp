#include "StreamRenderer.h"
#include "../items/Terminal.h"
#include "ItemPaintTraversal_p.h"
#include <sstream>

namespace qtui {

namespace {

std::string withTerminalLineEndings(const std::string& text) {
    std::string result;
    result.reserve(text.size() + 2);

    char previous = '\0';
    for (char character : text) {
        if (character == '\n' && previous != '\r') {
            result.push_back('\r');
        }
        result.push_back(character);
        previous = character;
    }

    if (result.empty() || result.back() != '\n') {
        if (!result.empty() && result.back() == '\r') {
            result.push_back('\n');
        } else {
            result.append("\r\n");
        }
    }
    return result;
}

void appendCellStyle(std::ostringstream& output, const Cell& cell) {
    output << "\033[0m";

    if ((cell.fgColor >> 24) != 0) {
        output << "\033[38;2;"
               << ((cell.fgColor >> 16) & 0xFF) << ";"
               << ((cell.fgColor >> 8) & 0xFF) << ";"
               << (cell.fgColor & 0xFF) << "m";
    }
    if ((cell.bgColor >> 24) != 0) {
        output << "\033[48;2;"
               << ((cell.bgColor >> 16) & 0xFF) << ";"
               << ((cell.bgColor >> 8) & 0xFF) << ";"
               << (cell.bgColor & 0xFF) << "m";
    }

    const auto attributes = static_cast<uint8_t>(cell.attributes);
    if (attributes & static_cast<uint8_t>(TextAttribute::Bold)) output << "\033[1m";
    if (attributes & static_cast<uint8_t>(TextAttribute::Italic)) output << "\033[3m";
    if (attributes & static_cast<uint8_t>(TextAttribute::Underline)) output << "\033[4m";
    if (attributes & static_cast<uint8_t>(TextAttribute::Reversed)) output << "\033[7m";
    if (attributes & static_cast<uint8_t>(TextAttribute::Blink)) output << "\033[5m";
    if (attributes & static_cast<uint8_t>(TextAttribute::Conceal)) output << "\033[8m";
    if (attributes & static_cast<uint8_t>(TextAttribute::Strikethrough)) output << "\033[9m";
}

} // namespace

StreamRenderer::StreamRenderer() = default;

void StreamRenderer::start(Terminal* rootWindow, TerminalDevice& device) {
    m_rootWindow = rootWindow;
    m_lastPromptHeight = 0;
}

void StreamRenderer::stop(Terminal* rootWindow, TerminalDevice& device) {
    if (!outputEnabled())
        return;
    if (m_lastPromptHeight > 0) {
        std::ostringstream oss;
        oss << "\r\033[" << m_lastPromptHeight << "A\033[J";
        (void)device.write(oss.str());
        m_lastPromptHeight = 0;
    }
    m_rootWindow = nullptr;
}

void StreamRenderer::resize(int width, int height) {
    m_terminalWidth = width;
    m_terminalHeight = height;
    m_promptBuffer.resize(width, m_lastPromptHeight);
}

void StreamRenderer::print(TerminalDevice& device, const std::string& text) {
    if (!outputEnabled())
        return;

    int newPromptHeight = 0;
    if (m_rootWindow) {
        newPromptHeight = m_rootWindow->implicitHeight();
        if (newPromptHeight <= 0) newPromptHeight = m_rootWindow->Item::height();
        if (newPromptHeight <= 0) newPromptHeight = m_rootWindow->height();
        if (newPromptHeight < 0) newPromptHeight = 0;
        const int maxH = m_terminalHeight > 0 ? m_terminalHeight - 1 : 0;
        if (newPromptHeight > maxH) newPromptHeight = maxH;
    } else {
        newPromptHeight = m_lastPromptHeight;
    }

    m_promptBuffer.resize(m_terminalWidth, newPromptHeight);
    if (m_rootWindow && newPromptHeight > 0) {
        ItemPaintTraversal::paintSubtree(m_rootWindow, m_promptBuffer, 0, 0);
    }

    std::ostringstream output;
    if (m_lastPromptHeight > 0) {
        output << "\r\033[" << m_lastPromptHeight << "A\033[J";
    }
    output << withTerminalLineEndings(text);
    for (int row = 0; row < newPromptHeight; ++row) {
        output << "\r\033[2K" << promptRowToAnsi(row) << "\033[0m\r\n";
    }
    m_lastPromptHeight = newPromptHeight;
    (void)device.write(output.str());
}

bool StreamRenderer::clearPromptRegion(TerminalDevice& device) {
    if (m_lastPromptHeight > 0) {
        std::ostringstream oss;
        oss << "\r\033[" << m_lastPromptHeight << "A\033[J";
        return device.write(oss.str());
    }
    return true;
}

bool StreamRenderer::drawPromptRegion(TerminalDevice& device) {
    if (m_lastPromptHeight <= 0) return true;

    std::ostringstream output;
    for (int row = 0; row < m_lastPromptHeight; ++row) {
        output << "\r\033[2K" << promptRowToAnsi(row) << "\033[0m\r\n";
    }
    return device.write(output.str());
}

std::string StreamRenderer::promptRowToAnsi(int row) const {
    int lastContentColumn = -1;
    for (int column = m_promptBuffer.width() - 1; column >= 0; --column) {
        const Cell& cell = m_promptBuffer.cellAt(column, row);
        if (!cell.continuation && !cell.character.isEmpty() && cell.character != " ") {
            lastContentColumn = column;
            break;
        }
        if ((cell.bgColor >> 24) != 0 || cell.attributes != TextAttribute::None) {
            lastContentColumn = column;
            break;
        }
    }

    std::ostringstream output;
    Cell previousCell;
    bool hasPreviousStyle = false;
    for (int column = 0; column <= lastContentColumn; ++column) {
        const Cell& cell = m_promptBuffer.cellAt(column, row);
        if (!hasPreviousStyle
            || cell.fgColor != previousCell.fgColor
            || cell.bgColor != previousCell.bgColor
            || cell.attributes != previousCell.attributes) {
            appendCellStyle(output, cell);
            previousCell = cell;
            hasPreviousStyle = true;
        }

        if (!cell.continuation)
            output << (cell.character.isEmpty() ? std::string(" ") : cell.character.toStdString());
    }
    return output.str();
}

bool StreamRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device) {
    m_rootWindow = rootWindow;
    if (!rootWindow) return false;

    int requiredHeight = rootWindow->implicitHeight();
    if (requiredHeight <= 0) {
        requiredHeight = rootWindow->Item::height();
    }
    if (requiredHeight <= 0) {
        requiredHeight = rootWindow->height();
    }
    if (requiredHeight < 0) requiredHeight = 0;
    const int maximumPromptHeight = m_terminalHeight > 0 ? m_terminalHeight - 1 : 0;
    if (requiredHeight > maximumPromptHeight) requiredHeight = maximumPromptHeight;

    const int previousPromptHeight = m_lastPromptHeight;
    m_lastPromptHeight = requiredHeight;
    m_promptBuffer.resize(m_terminalWidth, m_lastPromptHeight);
    ItemPaintTraversal::paintSubtree(rootWindow, m_promptBuffer, 0, 0);

    if (!outputEnabled()) return true;

    std::ostringstream output;
    if (previousPromptHeight > 0) {
        output << "\r\033[" << previousPromptHeight << "A";
    }
    for (int row = 0; row < m_lastPromptHeight; ++row) {
        output << "\r\033[2K" << promptRowToAnsi(row) << "\033[0m\r\n";
    }
    if (previousPromptHeight > m_lastPromptHeight) {
        output << "\033[J";
    }
    return device.write(output.str());
}

} // namespace qtui
