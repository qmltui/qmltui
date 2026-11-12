#include <QmlTui/Core/Engine/InlineRenderer.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Engine/ItemPaintTraversal_p.h>
#include <QmlTui/Core/Primitives/AnsiEncoder_p.h>
#include <QElapsedTimer>
#include <algorithm>

namespace qmltui {

InlineRenderer::InlineRenderer() = default;

void InlineRenderer::start(Terminal* rootWindow, TerminalDevice& device) {
    Q_UNUSED(device);
    m_rootWindow = rootWindow;
    m_lastPromptHeight = 0;
    m_hasCommittedPrompt = false;
    invalidateMouseOrigin();
}

void InlineRenderer::invalidateMouseOrigin() {
    m_mouseRowOffset.reset();
    if (m_cursorQueryPending) m_cursorQueryStale = true;
}

void InlineRenderer::discardCommittedState() {
    m_lastPromptHeight = 0;
    m_hasCommittedPrompt = false;
    invalidateMouseOrigin();
}

std::optional<int> InlineRenderer::mouseRowOffset() const {
    if (!outputEnabled()) return (std::max)(0, m_terminalHeight - 1 - m_lastPromptHeight);
    return m_mouseRowOffset;
}

void InlineRenderer::queryCursorPosition(TerminalDevice& device) {
    // Only one outstanding request: terminals that do not answer must not
    // accumulate queries. Column one below a nonempty prompt gives a report
    // with row >= 2 and column == 1, distinct from modified F3 (CSI 1;mod R).
    if (outputEnabled() && m_lastPromptHeight > 0 && !m_cursorQueryPending &&
        device.write("\r\033[6n")) {
        m_cursorQueryPending = true;
        m_cursorQueryStale = false;
    }
}

void InlineRenderer::handleCursorPosition(int row, TerminalDevice& device) {
    if (!m_cursorQueryPending) return;
    m_cursorQueryPending = false;
    if (m_cursorQueryStale) {
        queryCursorPosition(device);
        return;
    }
    // row is zero-based and names the cursor row following the prompt.
    if (row >= m_lastPromptHeight && row < m_terminalHeight)
        m_mouseRowOffset = row - m_lastPromptHeight;
}

static bool promptRowEqual(const TextGridBuffer& a, const TextGridBuffer& b, int row) {
    const int w = a.width();
    if (w != b.width() || row < 0 || row >= a.height() || row >= b.height()) return false;
    for (int x = 0; x < w; ++x) {
        const Cell& ca = a.cellAt(x, row);
        const Cell& cb = b.cellAt(x, row);
        if (ca.linkId == 0 && cb.linkId == 0) {
            if (ca != cb) return false;
        } else {
            if (ca.fgColor != cb.fgColor || ca.bgColor != cb.bgColor ||
                ca.displayWidth != cb.displayWidth || ca.continuation != cb.continuation ||
                ca.attributes != cb.attributes || ca.character() != cb.character() ||
                a.linkUrl(ca.linkId) != b.linkUrl(cb.linkId)) {
                return false;
            }
        }
    }
    return true;
}

void InlineRenderer::stop(Terminal* rootWindow, TerminalDevice& device) {
    Q_UNUSED(rootWindow);
    m_hasCommittedPrompt = false;
    if (!outputEnabled()) return;
    if (m_lastPromptHeight > 0) {
        m_ansi.clear();
        m_ansi += "\r\033[";
        ansi::appendInt(m_ansi, m_lastPromptHeight);
        m_ansi += "A\033[J";
        (void)device.write(m_ansi);
        m_lastPromptHeight = 0;
    }
    m_rootWindow = nullptr;
}

void InlineRenderer::resize(int width, int height) {
    invalidateMouseOrigin();
    m_hasCommittedPrompt = false;
    m_terminalWidth = width;
    m_terminalHeight = height;
    m_promptBuffer.resize(width, m_lastPromptHeight);
}

bool InlineRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device) {
    return renderFrame(rootWindow, device, {});
}

bool InlineRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device,
                                 std::string_view scrollback) {
    m_rootWindow = rootWindow;
    if (!rootWindow) return false;

    QPointer<Terminal> rootGuard(rootWindow);
    QPointer<TerminalEngine> engine = rootWindow->engine();
    const bool ownedByEngine = engine && (engine->renderer() == this);
    QElapsedTimer timer;
    timer.start();
    ItemPaintTraversal::layoutSubtree(rootWindow);
    if (!rootGuard || (ownedByEngine && (!engine || engine->renderer() != this))) return false;
    const int requiredHeight =
        std::clamp(rootWindow->height(), 0, (std::max)(0, m_terminalHeight - 1));

    const int previousPromptHeight = m_lastPromptHeight;
    if (previousPromptHeight != requiredHeight || !scrollback.empty()) invalidateMouseOrigin();
    m_lastPromptHeight = requiredHeight;
    // resize() unconditionally clears cells and links.
    m_promptBuffer.resize(m_terminalWidth, m_lastPromptHeight);
    ItemPaintTraversal::paintSubtree(rootWindow, m_promptBuffer, 0, 0);
    if (!rootGuard || (ownedByEngine && (!engine || engine->renderer() != this))) return false;
    if (engine) {
        emit engine->framePainted();
        if (!rootGuard || !engine || (ownedByEngine && engine->renderer() != this)) return false;
        engine->takeInvalidatedCommittedRects();
    }
    FrameStats stats;
    stats.dirtyDuringPaint = RenderPhase::dirtyDuringPaintCount();
    stats.paintMicros = timer.nsecsElapsed() / 1000;

    const QByteArray controls = rootWindow->takeControlSequences();
    const QByteArray postControls = rootWindow->takePostControlSequences();
    if (!outputEnabled()) {
        m_lastStats = stats;
        return true;
    }

    const ColorDepth colorDepth = capabilities().colorDepth;
    const bool canDiffRows = scrollback.empty() && m_hasCommittedPrompt &&
                             previousPromptHeight == m_lastPromptHeight &&
                             m_committedColorDepth == colorDepth &&
                             m_committedPrompt.width() == m_promptBuffer.width() &&
                             m_committedPrompt.height() == m_lastPromptHeight;

    int firstChangedRow = 0;
    if (canDiffRows) {
        while (firstChangedRow < m_lastPromptHeight &&
               promptRowEqual(m_committedPrompt, m_promptBuffer, firstChangedRow)) {
            ++firstChangedRow;
        }
    }

    const bool synchronize = capabilities().syncOutput;
    m_ansi.clear();
    if (canDiffRows && firstChangedRow == m_lastPromptHeight && controls.isEmpty() &&
        postControls.isEmpty()) {
        const qint64 encodedNs = timer.nsecsElapsed();
        stats.diffMicros = encodedNs / 1000 - stats.paintMicros;
        if (!m_mouseRowOffset) queryCursorPosition(device);
        stats.writeMicros = (timer.nsecsElapsed() - encodedNs) / 1000;
        m_lastStats = stats;
        return true;
    }

    if (synchronize) m_ansi += "\033[?2026h";
    m_ansi.append(controls.constData(), static_cast<size_t>(controls.size()));
    if (canDiffRows) {
        const int rowsUp = m_lastPromptHeight - firstChangedRow;
        if (rowsUp > 0) {
            m_ansi += "\r\033[";
            ansi::appendInt(m_ansi, rowsUp);
            m_ansi += 'A';
        }
        for (int row = firstChangedRow; row < m_lastPromptHeight; ++row) {
            if (row > firstChangedRow && promptRowEqual(m_committedPrompt, m_promptBuffer, row)) {
                m_ansi += "\r\n";
                continue;
            }
            m_ansi += "\r\033[2K";
            m_promptBuffer.appendRowAnsi(m_ansi, row, colorDepth);
            m_ansi += "\033[0m\r\n";
        }
    } else {
        if (previousPromptHeight > 0) {
            m_ansi += "\r\033[";
            ansi::appendInt(m_ansi, previousPromptHeight);
            m_ansi += 'A';
        }
        if (!scrollback.empty()) {
            m_ansi += "\r\033[J";
            m_ansi.append(scrollback.data(), scrollback.size());
        }
        for (int row = 0; row < m_lastPromptHeight; ++row) {
            m_ansi += "\r\033[2K";
            m_promptBuffer.appendRowAnsi(m_ansi, row, colorDepth);
            m_ansi += "\033[0m\r\n";
        }
        if (previousPromptHeight > m_lastPromptHeight) {
            m_ansi += "\033[J";
        }
    }
    // Cursor-painted graphics land after the live region is drawn.
    m_ansi.append(postControls.constData(), static_cast<size_t>(postControls.size()));
    if (synchronize) m_ansi += "\033[?2026l";
    const qint64 encodedNs = timer.nsecsElapsed();
    stats.diffMicros = encodedNs / 1000 - stats.paintMicros;
    stats.diffBytes = static_cast<qint64>(m_ansi.size());
    const bool written = device.write(m_ansi);
    if (written) {
        m_committedPrompt = m_promptBuffer;
        m_committedColorDepth = colorDepth;
        m_hasCommittedPrompt = true;
        if (!m_mouseRowOffset) queryCursorPosition(device);
    } else {
        m_hasCommittedPrompt = false;
    }
    stats.writeMicros = (timer.nsecsElapsed() - encodedNs) / 1000;
    if (written) stats.bytesWritten = stats.diffBytes;
    m_lastStats = stats;
    return written;
}

} // namespace qmltui
