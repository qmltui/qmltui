#pragma once

#include "Renderer.h"
#include "../primitives/TextGridBuffer.h"

namespace qtui {

class FrameRenderer : public Renderer {
public:
    void start(Terminal* rootWindow, TerminalDevice& device) override;
    void stop(Terminal* rootWindow, TerminalDevice& device) override;
    void resize(int width, int height) override;
    bool renderFrame(Terminal* rootWindow, TerminalDevice& device) override;
    bool usesAlternateScreen(Terminal* rootWindow) const override;

    // For exposure to engine if needed
    int terminalWidth() const { return m_currentBuffer.width(); }
    int terminalHeight() const { return m_currentBuffer.height(); }
    const TextGridBuffer* currentBuffer() const override { return &m_currentBuffer; }

private:
    TextGridBuffer m_currentBuffer;
    TextGridBuffer m_nextBuffer;
};

} // namespace qtui
