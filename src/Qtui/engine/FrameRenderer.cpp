#include "FrameRenderer.h"
#include "../items/Terminal.h"
#include "ItemPaintTraversal_p.h"

namespace qtui {

void FrameRenderer::start(Terminal* rootWindow, TerminalDevice& device) {
    if (!outputEnabled())
        return;
    if (rootWindow && rootWindow->useAlternateScreen()) {
        device.write("\033[?1049h");
    }
    device.clearScreen();
}

void FrameRenderer::stop(Terminal* rootWindow, TerminalDevice& device) {
    if (!outputEnabled())
        return;
    if (rootWindow && rootWindow->useAlternateScreen()) {
        device.write("\033[?1049l"); // Disable alternate screen buffer
    } else {
        device.clearScreen();
    }
}

void FrameRenderer::resize(int width, int height) {
    m_currentBuffer.resize(width, height);
    m_nextBuffer.resize(width, height);
}

bool FrameRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device) {
    if (!rootWindow) return false;
    
    m_nextBuffer.clear();
    ItemPaintTraversal::paintSubtree(rootWindow, m_nextBuffer, 0, 0);

    std::string ansi = m_currentBuffer.diff(m_nextBuffer);
    if (outputEnabled() && !ansi.empty() && !device.write(ansi))
        return false;
    m_currentBuffer.swap(m_nextBuffer);
    return true;
}

bool FrameRenderer::usesAlternateScreen(Terminal* rootWindow) const {
    return outputEnabled() && rootWindow && rootWindow->useAlternateScreen();
}

} // namespace qtui
