#pragma once

#include <Qamui/Core/engine/Renderer.h>
#include <Qamui/Core/primitives/TextGridBuffer.h>
#include <string>

namespace qamui {

class StreamRenderer : public Renderer {
public:
    StreamRenderer();

    void start(Terminal* rootWindow, TerminalDevice& device) override;
    void stop(Terminal* rootWindow, TerminalDevice& device) override;
    void resize(int width, int height) override;
    bool renderFrame(Terminal* rootWindow, TerminalDevice& device) override;

    bool renderFrame(Terminal* rootWindow, TerminalDevice& device, std::string_view scrollback);
    const TextGridBuffer* currentBuffer() const override { return &m_promptBuffer; }
    // The cursor position is unknown after discarded output: do not move up
    // over rows that may never have been drawn.
    void discardCommittedState() override { m_lastPromptHeight = 0; }

private:
    bool clearPromptRegion(TerminalDevice& device);
    bool drawPromptRegion(TerminalDevice& device);
    std::string promptRowToAnsi(int row) const;

    TextGridBuffer m_promptBuffer;
    Terminal* m_rootWindow = nullptr;
    int m_terminalWidth = 80;
    int m_terminalHeight = 24;
    int m_lastPromptHeight = 0;
};

} // namespace qamui
