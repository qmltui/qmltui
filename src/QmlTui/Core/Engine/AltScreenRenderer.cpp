#include <QmlTui/Core/Engine/AltScreenRenderer.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Engine/ItemPaintTraversal_p.h>
#include <QElapsedTimer>

namespace qmltui {

void AltScreenRenderer::start(Terminal* rootWindow, TerminalDevice& device) {
    if (!outputEnabled())
        return;
    if (rootWindow) {
        device.write("\033[?1049h");
    }
    device.clearScreen();
}

void AltScreenRenderer::stop(Terminal* rootWindow, TerminalDevice& device) {
    if (!outputEnabled())
        return;
    if (rootWindow) {
        device.write("\033[?1049l"); // Disable alternate screen buffer
    } else {
        device.clearScreen();
    }
}

void AltScreenRenderer::resize(int width, int height) {
    if (m_currentBuffer.width() == width && m_currentBuffer.height() == height)
        return;
    m_currentBuffer.resize(width, height);
    m_nextBuffer.resize(width, height);
    m_repaintAll = true;
}

bool AltScreenRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device) {
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
    const bool repaintingAll = m_repaintAll;
    if (m_repaintAll) {
        // The screen content is unknown: clear it and diff against blank.
        m_repaintAll = false;
        m_currentBuffer.clear();
        if (outputEnabled())
            m_ansi += "\033[2J\033[H";
    }
    const size_t prefixSize = m_ansi.size();
    const QByteArray controls = rootWindow->takeControlSequences();
    if (outputEnabled()) m_ansi.append(controls.constData(), controls.size());
    m_currentBuffer.diff(m_nextBuffer, m_ansi, options);
    // Cursor-painted graphics (sixel) go on top of the cells just written.
    const QByteArray postControls = rootWindow->takePostControlSequences();
    if (outputEnabled()) m_ansi.append(postControls.constData(), postControls.size());
    if (!repaintingAll && m_ansi.size() == prefixSize)
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

bool AltScreenRenderer::usesAlternateScreen(Terminal* rootWindow) const {
    return outputEnabled() && rootWindow;
}

} // namespace qmltui
