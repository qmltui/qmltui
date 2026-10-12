#include <Qamui/Core/Engine/StreamRenderer.h>
#include <Qamui/Core/Items/Terminal.h>
#include <Qamui/Core/Engine/ItemPaintTraversal_p.h>
#include <Qamui/Core/Primitives/AnsiEncoder_p.h>
#include <QElapsedTimer>
#include <sstream>
#include <algorithm>

namespace qamui {

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
    std::string output;
    m_promptBuffer.appendRowAnsi(output, row, capabilities().colorDepth);
    return output;
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
    const int requiredHeight = std::clamp(rootWindow->height(), 0, (std::max)(0, m_terminalHeight - 1));

    const int previousPromptHeight = m_lastPromptHeight;
    m_lastPromptHeight = requiredHeight;
    m_promptBuffer.resize(m_terminalWidth, m_lastPromptHeight);
    m_promptBuffer.clear();
    ItemPaintTraversal::paintSubtree(rootWindow, m_promptBuffer, 0, 0);
    FrameStats stats;
    stats.dirtyDuringPaint = RenderPhase::dirtyDuringPaintCount();
    stats.paintMicros = timer.nsecsElapsed() / 1000;

    const QByteArray controls = rootWindow->takeControlSequences();
    const QByteArray postControls = rootWindow->takePostControlSequences();
    if (!outputEnabled()) {
        m_lastStats = stats;
        return true;
    }

    std::ostringstream output;
    output.write(controls.constData(), controls.size());
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
    // Cursor-painted graphics land after the live region is drawn.
    output.write(postControls.constData(), postControls.size());
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

} // namespace qamui
