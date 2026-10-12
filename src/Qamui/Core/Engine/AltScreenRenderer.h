#pragma once

#include <Qamui/Core/Engine/Renderer.h>
#include <Qamui/Core/Primitives/TextGridBuffer.h>

namespace qamui {

class AltScreenRenderer : public Renderer {
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
    void discardCommittedState() override { m_repaintAll = true; }

private:
    TextGridBuffer m_currentBuffer;
    TextGridBuffer m_nextBuffer;
    std::string m_ansi;   // reused across frames
    bool m_repaintAll = false;
};

} // namespace qamui
