#pragma once

// Internal: alternate-screen (?1049h) double-buffered cell renderer.

#include <QmlTui/Core/Engine/Renderer.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

namespace qmltui {

// Full-viewport renderer using the terminal's alternate screen buffer (?1049h).
// Double-buffers the cell grid and emits minimal absolute-cursor ANSI diffs
// bracketed by synchronized-output sequences (?2026h/?2026l) when supported.
class AltScreenRenderer : public Renderer {
public:
    void start(Terminal* rootWindow, TerminalDevice& device) override;
    void stop(Terminal* rootWindow, TerminalDevice& device) override;
    void resize(int width, int height) override;
    bool renderFrame(Terminal* rootWindow, TerminalDevice& device) override;
    bool usesAlternateScreen(Terminal* rootWindow) const override;

    int terminalWidth() const { return m_currentBuffer.width(); }
    int terminalHeight() const { return m_currentBuffer.height(); }
    const TextGridBuffer* currentBuffer() const override { return &m_currentBuffer; }
    void discardCommittedState() override { m_repaintAll = true; }

private:
    TextGridBuffer m_currentBuffer;
    TextGridBuffer m_nextBuffer;
    std::string m_ansi; // reused across frames
    bool m_repaintAll = false;
};

} // namespace qmltui
