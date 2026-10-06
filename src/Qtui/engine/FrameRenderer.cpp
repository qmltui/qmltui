#include "FrameRenderer.h"
#include "../items/Terminal.h"
#include "ItemPaintTraversal_p.h"
#include <QElapsedTimer>

namespace qtui {

void FrameRenderer::start(Terminal* rootWindow, TerminalDevice& device) {
    if (!outputEnabled())
        return;
    if (rootWindow) {
        device.write("\033[?1049h");
    }
    device.clearScreen();
}

void FrameRenderer::stop(Terminal* rootWindow, TerminalDevice& device) {
    if (!outputEnabled())
        return;
    if (rootWindow) {
        device.write("\033[?1049l"); // Disable alternate screen buffer
    } else {
        device.clearScreen();
    }
}

void FrameRenderer::resize(int width, int height) {
    m_currentBuffer.resize(width, height);
    m_nextBuffer.resize(width, height);
}

bool FrameRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device) {
    if (!rootWindow) return false;

    QElapsedTimer timer;
    timer.start();

    m_nextBuffer.clear();
    ItemPaintTraversal::renderSubtree(rootWindow, m_nextBuffer, 0, 0);
    const qint64 paintedNs = timer.nsecsElapsed();

    AnsiEncodeOptions options;
    options.colorDepth = capabilities().colorDepth;
    // Synchronized output (DEC private mode 2026) lets the terminal apply the
    // whole frame at once; terminals without it ignore the sequences.
    const bool synchronize = outputEnabled() && capabilities().syncOutput;
    m_ansi.clear();
    if (synchronize)
        m_ansi += "\033[?2026h";
    const size_t prefixSize = m_ansi.size();
    m_currentBuffer.diff(m_nextBuffer, m_ansi, options);
    if (m_ansi.size() == prefixSize)
        m_ansi.clear();          // nothing changed: write nothing at all
    else if (synchronize)
        m_ansi += "\033[?2026l";
    const qint64 diffedNs = timer.nsecsElapsed();

    FrameStats stats;
    stats.dirtyDuringPaint = RenderPhase::dirtyDuringPaintCount();
    stats.paintMicros = paintedNs / 1000;
    stats.diffMicros = (diffedNs - paintedNs) / 1000;
    stats.diffBytes = static_cast<qint64>(m_ansi.size());
    if (outputEnabled() && !m_ansi.empty()) {
        if (!device.write(m_ansi))
            return false;
        stats.bytesWritten = stats.diffBytes;
    }
    stats.queuedBytes = static_cast<qint64>(device.pendingBytes());
    stats.writeMicros = (timer.nsecsElapsed() - diffedNs) / 1000;
    m_lastStats = stats;

    m_currentBuffer.swap(m_nextBuffer);
    return true;
}

bool FrameRenderer::usesAlternateScreen(Terminal* rootWindow) const {
    return outputEnabled() && rootWindow;
}

} // namespace qtui
