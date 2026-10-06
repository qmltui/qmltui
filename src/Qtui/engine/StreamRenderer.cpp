#include "StreamRenderer.h"
#include "../items/Terminal.h"
#include "ItemPaintTraversal_p.h"
#include "../primitives/AnsiEncoder_p.h"
#include <QElapsedTimer>
#include <sstream>
#include <algorithm>

namespace qtui {

namespace {

void appendCellStyle(std::ostringstream& output, const Cell& cell, ColorDepth depth) {
    std::string style;
    ansi::appendFullStyle(style, cell, depth);
    output << style;
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
        if (!cell.continuation && !cell.character().isEmpty() && cell.character() != " ") {
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
            appendCellStyle(output, cell, capabilities().colorDepth);
            previousCell = cell;
            hasPreviousStyle = true;
        }

        if (!cell.continuation) {
            const QByteArrayView glyph = cell.character();
            if (glyph.isEmpty())
                output << ' ';
            else
                output.write(glyph.data(), glyph.size());
        }
    }
    return output.str();
}

bool StreamRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device) {
    return renderFrame(rootWindow, device, {});
}

bool StreamRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device,
                                 std::string_view scrollback) {
    m_rootWindow = rootWindow;
    if (!rootWindow) return false;

    QElapsedTimer timer;
    timer.start();
    ItemPaintTraversal::layoutSubtree(rootWindow);
    const int requiredHeight = std::clamp(rootWindow->height(), 0, std::max(0, m_terminalHeight - 1));

    const int previousPromptHeight = m_lastPromptHeight;
    m_lastPromptHeight = requiredHeight;
    m_promptBuffer.resize(m_terminalWidth, m_lastPromptHeight);
    m_promptBuffer.clear();
    ItemPaintTraversal::paintSubtree(rootWindow, m_promptBuffer, 0, 0);
    FrameStats stats;
    stats.dirtyDuringPaint = RenderPhase::dirtyDuringPaintCount();
    stats.paintMicros = timer.nsecsElapsed() / 1000;

    if (!outputEnabled()) {
        m_lastStats = stats;
        return true;
    }

    std::ostringstream output;
    if (previousPromptHeight > 0) {
        output << "\r\033[" << previousPromptHeight << "A";
    }
    if (!scrollback.empty()) {
        output << "\r\033[J";
        output.write(scrollback.data(), static_cast<std::streamsize>(scrollback.size()));
    }
    for (int row = 0; row < m_lastPromptHeight; ++row) {
        output << "\r\033[2K" << promptRowToAnsi(row) << "\033[0m\r\n";
    }
    if (previousPromptHeight > m_lastPromptHeight) {
        output << "\033[J";
    }
    const std::string ansi = output.str();
    const qint64 encodedNs = timer.nsecsElapsed();
    stats.diffMicros = encodedNs / 1000 - stats.paintMicros;
    stats.diffBytes = static_cast<qint64>(ansi.size());
    const bool written = device.write(ansi);
    stats.writeMicros = (timer.nsecsElapsed() - encodedNs) / 1000;
    if (written)
        stats.bytesWritten = stats.diffBytes;
    m_lastStats = stats;
    return written;
}

} // namespace qtui
