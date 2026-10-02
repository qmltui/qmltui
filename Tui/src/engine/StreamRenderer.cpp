#include "StreamRenderer.h"
#include "../items/Terminal.h"
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
}

} // namespace

StreamRenderer::StreamRenderer() = default;

void StreamRenderer::start(Terminal* rootWindow, TerminalDevice& device) {
    m_lastPromptHeight = 0;
}

void StreamRenderer::stop(Terminal* rootWindow, TerminalDevice& device) {
    clearPromptRegion(device);
}

void StreamRenderer::resize(int width, int height) {
    m_terminalWidth = width;
    m_terminalHeight = height;
    m_promptBuffer.resize(width, m_lastPromptHeight);
}

void StreamRenderer::print(TerminalDevice& device, const std::string& text) {
    clearPromptRegion(device);
    device.write(withTerminalLineEndings(text));
    drawPromptRegion(device);
}

void StreamRenderer::clearPromptRegion(TerminalDevice& device) {
    if (m_lastPromptHeight > 0) {
        std::ostringstream oss;
        oss << "\r\033[" << m_lastPromptHeight << "A\033[J";
        device.write(oss.str());
    }
}

void StreamRenderer::drawPromptRegion(TerminalDevice& device) {
    if (m_lastPromptHeight <= 0) return;

    std::ostringstream output;
    for (int row = 0; row < m_lastPromptHeight; ++row) {
        output << "\r\033[2K" << promptRowToAnsi(row) << "\033[0m\r\n";
    }
    device.write(output.str());
}

std::string StreamRenderer::promptRowToAnsi(int row) const {
    int lastContentColumn = -1;
    for (int column = m_promptBuffer.width() - 1; column >= 0; --column) {
        const Cell& cell = m_promptBuffer.cellAt(column, row);
        if (cell.character[0] != ' ' && cell.character[0] != '\0') {
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

        if (cell.character[0] == '\0') {
            output << ' ';
        } else {
            for (char byte : cell.character) {
                if (byte != '\0') output << byte;
            }
        }
    }
    return output.str();
}

void StreamRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device) {
    if (!rootWindow) return;

    int requiredHeight = rootWindow->height();
    if (requiredHeight < 0) requiredHeight = 0;
    const int maximumPromptHeight = m_terminalHeight > 0 ? m_terminalHeight - 1 : 0;
    if (requiredHeight > maximumPromptHeight) requiredHeight = maximumPromptHeight;

    clearPromptRegion(device);
    m_lastPromptHeight = requiredHeight;
    m_promptBuffer.resize(m_terminalWidth, m_lastPromptHeight);
    rootWindow->render(m_promptBuffer, 0, 0);
    drawPromptRegion(device);
}

} // namespace qtui
