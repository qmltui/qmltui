#pragma once

#include <Qamui/Core/Engine/Renderer.h>
#include <Qamui/Core/Primitives/TextGridBuffer.h>
#include <string>

namespace qamui {

class InlineRenderer : public Renderer {
public:
    InlineRenderer();

    void start(Terminal* rootWindow, TerminalDevice& device) override;
    void stop(Terminal* rootWindow, TerminalDevice& device) override;
    void resize(int width, int height) override;
    bool renderFrame(Terminal* rootWindow, TerminalDevice& device) override;

    bool renderFrame(Terminal* rootWindow, TerminalDevice& device, std::string_view scrollback);
    const TextGridBuffer* currentBuffer() const override { return &m_promptBuffer; }
    std::optional<int> mouseRowOffset() const override;
    void handleCursorPosition(int row, TerminalDevice& device);
    // The cursor position is unknown after discarded output: do not move up
    // over rows that may never have been drawn.
    void discardCommittedState() override;

private:
    bool clearPromptRegion(TerminalDevice& device);
    bool drawPromptRegion(TerminalDevice& device);
    std::string promptRowToAnsi(int row) const;
    void queryCursorPosition(TerminalDevice& device);
    void invalidateMouseOrigin();

    TextGridBuffer m_promptBuffer;
    Terminal* m_rootWindow = nullptr;
    int m_terminalWidth = 80;
    int m_terminalHeight = 24;
    int m_lastPromptHeight = 0;
    std::optional<int> m_mouseRowOffset;
    bool m_cursorQueryPending = false;
    bool m_cursorQueryStale = false;
};

} // namespace qamui
