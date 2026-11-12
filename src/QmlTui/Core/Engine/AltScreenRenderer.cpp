#include <QmlTui/Core/Engine/AltScreenRenderer.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Engine/ItemPaintTraversal_p.h>
#include <QmlTui/Core/Primitives/AnsiEncoder_p.h>
#include <QElapsedTimer>

namespace qmltui {

void AltScreenRenderer::start(Terminal* rootWindow, TerminalDevice& device) {
    if (!outputEnabled()) return;
    if (rootWindow) {
        device.write("\033[?1049h");
    }
    device.clearScreen();
}

void AltScreenRenderer::stop(Terminal* rootWindow, TerminalDevice& device) {
    m_hasCommittedFrame = false;
    m_lastHardwareCursor.reset();
    if (!outputEnabled()) return;
    if (rootWindow) {
        device.write("\033[?1049l"); // Disable alternate screen buffer
    } else {
        device.clearScreen();
    }
}

void AltScreenRenderer::resize(int width, int height) {
    if (m_currentBuffer.width() == width && m_currentBuffer.height() == height) return;
    m_currentBuffer.resize(width, height);
    m_nextBuffer.resize(width, height);
    m_repaintAll = true;
    m_hasCommittedFrame = false;
    m_lastHardwareCursor.reset();
}

bool AltScreenRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device) {
    if (!rootWindow) return false;

    QElapsedTimer timer;
    timer.start();

    QPointer<Terminal> rootGuard(rootWindow);
    QPointer<TerminalEngine> engine = rootWindow->engine();
    const bool ownedByEngine = engine && (engine->renderer() == this);
    if (engine) {
        engine->clearHardwareCursorPosition();
    }

    const int bufW = m_nextBuffer.width();
    const int bufH = m_nextBuffer.height();
    const QRect viewportRect(0, 0, bufW, bufH);

    ItemPaintTraversal::layoutSubtree(rootWindow);
    if (!rootGuard || (ownedByEngine && (!engine || engine->renderer() != this))) return false;

    const bool sixelActive = capabilities().sixelGraphics && !capabilities().kittyGraphics;
    bool canPartial = m_hasCommittedFrame && !m_repaintAll && engine && !engine->hasFullDamage() &&
                      rootWindow->activePopups().isEmpty() && !sixelActive &&
                      m_currentBuffer.width() == bufW && m_currentBuffer.height() == bufH &&
                      bufW > 0 && bufH > 0 && m_currentBuffer.linkCount() < 200;

    QRect dmg;
    if (canPartial) {
        bool treeFullDamage = false;
        dmg = engine->damageRect();
        ItemPaintTraversal::collectDirtyDamage(rootWindow, viewportRect, dmg, treeFullDamage);
        if (treeFullDamage) {
            canPartial = false;
        } else if (!dmg.isEmpty()) {
            dmg = dmg.intersected(viewportRect);
            if (!dmg.isEmpty()) {
                int left = (std::max)(0, dmg.left() - 1);
                int right = (std::min)(bufW - 1, dmg.right() + 1);
                const int top = dmg.top();
                const int bottom = dmg.bottom();
                while (left > 0) {
                    bool straddles = false;
                    for (int y = top; y <= bottom; ++y) {
                        if (m_currentBuffer.cellAt(left, y).continuation) {
                            straddles = true;
                            break;
                        }
                    }
                    if (!straddles) break;
                    --left;
                }
                while (right + 1 < bufW) {
                    bool straddles = false;
                    for (int y = top; y <= bottom; ++y) {
                        if (m_currentBuffer.cellAt(right + 1, y).continuation) {
                            straddles = true;
                            break;
                        }
                    }
                    if (!straddles) break;
                    ++right;
                }
                dmg = QRect(left, top, right - left + 1, bottom - top + 1);
                if (static_cast<qint64>(dmg.width()) * dmg.height() * 2 >=
                    static_cast<qint64>(bufW) * bufH) {
                    canPartial = false;
                }
            }
        }
    }

    if (engine) {
        engine->clearDamage();
    }

    if (canPartial) {
        m_nextBuffer.copyFrom(m_currentBuffer);
        if (dmg.isEmpty()) {
            ItemPaintTraversal::clearPaintDirty(rootWindow);
            if (engine && m_lastHardwareCursor.has_value() && rootWindow->focusedItem() &&
                rootWindow->focusedItem()->isEffectivelyVisible()) {
                engine->setHardwareCursorPosition(m_lastHardwareCursor->x(),
                                                  m_lastHardwareCursor->y());
            }
        } else {
            m_nextBuffer.clearRect(dmg.x(), dmg.y(), dmg.width(), dmg.height());
            ItemPaintTraversal::paintSubtree(rootWindow, m_nextBuffer, 0, 0,
                                             ClipRect{dmg.x(), dmg.y(), dmg.width(), dmg.height()});
            if (!rootGuard || (ownedByEngine && (!engine || engine->renderer() != this))) {
                if (engine) engine->markFullDamage();
                return false;
            }
            if (engine && !engine->hardwareCursorPosition().has_value() &&
                m_lastHardwareCursor.has_value() && !dmg.contains(*m_lastHardwareCursor) &&
                rootWindow->focusedItem() && rootWindow->focusedItem()->isEffectivelyVisible()) {
                engine->setHardwareCursorPosition(m_lastHardwareCursor->x(),
                                                  m_lastHardwareCursor->y());
            }
        }
    } else {
        m_nextBuffer.clear();
        ItemPaintTraversal::paintSubtree(rootWindow, m_nextBuffer, 0, 0);
        if (!rootGuard || (ownedByEngine && (!engine || engine->renderer() != this))) {
            if (engine) engine->markFullDamage();
            return false;
        }
    }

    if (engine) {
        emit engine->framePainted();
        if (!rootGuard || !engine || (ownedByEngine && engine->renderer() != this)) {
            if (engine) engine->markFullDamage();
            return false;
        }
    }
    const qint64 paintedNs = timer.nsecsElapsed();

    AnsiEncodeOptions options;
    options.colorDepth = capabilities().colorDepth;
    // Synchronized output (DEC private mode 2026) lets the terminal apply the
    // whole frame at once; terminals without it ignore the sequences.
    const bool synchronize = outputEnabled() && capabilities().syncOutput;
    m_ansi.clear();
    if (synchronize) m_ansi += "\033[?2026h";
    const bool repaintingAll = m_repaintAll;
    if (m_repaintAll) {
        // The screen content is unknown: clear it and diff against blank.
        m_repaintAll = false;
        m_currentBuffer.clear();
        if (outputEnabled()) m_ansi += "\033[2J\033[H";
    }
    const size_t prefixSize = m_ansi.size();
    const QByteArray controls = rootWindow->takeControlSequences();
    if (outputEnabled()) m_ansi.append(controls.constData(), controls.size());
    if (TerminalEngine* engine = rootWindow->engine()) {
        const QList<QRect> invalidated = engine->takeInvalidatedCommittedRects();
        if (outputEnabled()) {
            for (const QRect& rect : invalidated) {
                const int left = (std::max)(0, rect.left());
                const int right = (std::min)(m_currentBuffer.width() - 1, rect.right());
                const int top = (std::max)(0, rect.top());
                const int bottom = (std::min)(m_currentBuffer.height() - 1, rect.bottom());
                for (int y = top; y <= bottom; ++y) {
                    if (left > 0 && m_currentBuffer.cellAt(left, y).continuation)
                        m_currentBuffer.cellAt(left - 1, y) = Cell{};
                    if (right + 1 < m_currentBuffer.width() &&
                        m_currentBuffer.cellAt(right + 1, y).continuation)
                        m_currentBuffer.cellAt(right + 1, y) = Cell{};
                    for (int x = left; x <= right; ++x)
                        m_currentBuffer.cellAt(x, y) = Cell{};
                }
            }
        }
    }
    m_currentBuffer.diff(m_nextBuffer, m_ansi, options);
    // Cursor-painted graphics (sixel) go on top of the cells just written.
    const QByteArray postControls = rootWindow->takePostControlSequences();
    if (outputEnabled()) m_ansi.append(postControls.constData(), postControls.size());
    if (outputEnabled()) {
        if (const TerminalEngine* engine = rootWindow->engine()) {
            if (const auto cursor = engine->hardwareCursorPosition();
                cursor.has_value() && m_nextBuffer.contains(cursor->x(), cursor->y())) {
                m_ansi += "\033[";
                ansi::appendInt(m_ansi, cursor->y() + 1);
                m_ansi += ';';
                ansi::appendInt(m_ansi, cursor->x() + 1);
                m_ansi += 'H';
            }
        }
    }
    if (!repaintingAll && m_ansi.size() == prefixSize)
        m_ansi.clear(); // nothing changed: write nothing at all
    else if (synchronize)
        m_ansi += "\033[?2026l";
    const qint64 diffedNs = timer.nsecsElapsed();

    FrameStats stats;
    stats.dirtyDuringPaint = RenderPhase::dirtyDuringPaintCount();
    stats.paintMicros = paintedNs / 1000;
    stats.diffMicros = (diffedNs - paintedNs) / 1000;
    stats.diffBytes = static_cast<qint64>(m_ansi.size());
    if (outputEnabled() && !m_ansi.empty()) {
        if (!device.write(m_ansi)) {
            if (engine) engine->markFullDamage();
            m_hasCommittedFrame = false;
            return false;
        }
        stats.bytesWritten = stats.diffBytes;
    }
    stats.queuedBytes = static_cast<qint64>(device.pendingBytes());
    stats.writeMicros = (timer.nsecsElapsed() - diffedNs) / 1000;
    m_lastStats = stats;

    m_currentBuffer.swap(m_nextBuffer);
    m_hasCommittedFrame = true;
    m_lastHardwareCursor = engine ? engine->hardwareCursorPosition() : std::nullopt;
    return true;
}

bool AltScreenRenderer::usesAlternateScreen(Terminal* rootWindow) const {
    return outputEnabled() && rootWindow;
}

} // namespace qmltui
