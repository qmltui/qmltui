#pragma once

#include "../primitives/TerminalDevice.h"

namespace qtui {

class Terminal;
class TextGridBuffer;

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void start(Terminal* rootWindow, TerminalDevice& device) = 0;
    virtual void stop(Terminal* rootWindow, TerminalDevice& device) = 0;
    virtual void resize(int width, int height) = 0;
    virtual bool renderFrame(Terminal* rootWindow, TerminalDevice& device) = 0;
    virtual bool usesAlternateScreen(Terminal*) const { return false; }
    virtual void print(TerminalDevice& device, const std::string& text) {}
    virtual const TextGridBuffer* currentBuffer() const { return nullptr; }
    virtual void setOutputEnabled(bool enabled) { m_outputEnabled = enabled; }

protected:
    bool outputEnabled() const { return m_outputEnabled; }

private:
    bool m_outputEnabled = true;
};

} // namespace qtui
