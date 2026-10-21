#pragma once

// Internal: normal-buffer stream renderer for Terminal.Inline mode.

#include <QmlTui/Core/Engine/Renderer.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <string>

namespace qmltui {

// Stream renderer for Terminal.Inline mode. Prints queued Scrollback blocks
// into the normal terminal buffer above a live, in-place prompt region at the
// current cursor position, querying DSR 6n to map terminal mouse rows into the
// prompt region.
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
    void queryCursorPosition(TerminalDevice& device);
    void invalidateMouseOrigin();

    TextGridBuffer m_promptBuffer;
    std::string m_ansi;
    Terminal* m_rootWindow = nullptr;
    int m_terminalWidth = 80;
    int m_terminalHeight = 24;
    int m_lastPromptHeight = 0;
    std::optional<int> m_mouseRowOffset;
    bool m_cursorQueryPending = false;
    bool m_cursorQueryStale = false;
};

} // namespace qmltui
