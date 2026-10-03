#include "FrameRenderer.h"
#include "../items/Terminal.h"

namespace qtui {

void FrameRenderer::start(Terminal* rootWindow, TerminalDevice& device) {
    if (rootWindow && rootWindow->useAlternateScreen()) {
        device.write("\033[?1049h");
    }
    device.clearScreen();
}

void FrameRenderer::stop(Terminal* rootWindow, TerminalDevice& device) {
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

void FrameRenderer::renderFrame(Terminal* rootWindow, TerminalDevice& device) {
    if (!rootWindow) return;
    
    m_nextBuffer.clear();
    rootWindow->render(m_nextBuffer, 0, 0);

    std::string ansi = m_currentBuffer.diff(m_nextBuffer);
    if (!ansi.empty() && !device.write(ansi))
        return;
    m_currentBuffer.swap(m_nextBuffer);
}

} // namespace qtui
