#pragma once

#include "Renderer.h"
#include "../primitives/TextGridBuffer.h"

namespace qtui {

class FrameRenderer : public Renderer {
public:
    void start(Terminal* rootWindow, TerminalDevice& device) override;
    void stop(Terminal* rootWindow, TerminalDevice& device) override;
    void resize(int width, int height) override;
    void renderFrame(Terminal* rootWindow, TerminalDevice& device) override;

    // For exposure to engine if needed
    int terminalWidth() const { return m_currentBuffer.width(); }
    int terminalHeight() const { return m_currentBuffer.height(); }

private:
    TextGridBuffer m_currentBuffer;
    TextGridBuffer m_nextBuffer;
};

} // namespace qtui
