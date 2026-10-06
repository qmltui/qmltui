#pragma once

#include "../primitives/TerminalDevice.h"
#include "../primitives/TerminalCapabilities.h"
#include <QtGlobal>

namespace qtui {

class Terminal;
class TextGridBuffer;

// Cost of the most recently committed frame. Filled by the renderer with a
// few clock reads per frame; the engine assigns the sequence number.
struct FrameStats {
    quint64 sequence = 0;
    qint64 paintMicros = 0;       // item tree traversal into the cell buffer
    qint64 diffMicros = 0;        // cell diff and ANSI encoding
    qint64 writeMicros = 0;       // time spent in TerminalDevice::write
    qint64 diffBytes = 0;         // encoded bytes, recorded even when output is disabled
    qint64 bytesWritten = 0;      // bytes handed to the terminal device
    qint64 queuedBytes = 0;       // bytes still waiting in the device's output queue
    int dirtyDuringPaint = 0;     // invalidations raised while painting
};

class Renderer {
public:
    virtual ~Renderer() = default;

    const FrameStats& lastFrameStats() const { return m_lastStats; }

    virtual void start(Terminal* rootWindow, TerminalDevice& device) = 0;
    virtual void stop(Terminal* rootWindow, TerminalDevice& device) = 0;
    virtual void resize(int width, int height) = 0;
    virtual bool renderFrame(Terminal* rootWindow, TerminalDevice& device) = 0;
    virtual bool usesAlternateScreen(Terminal*) const { return false; }
    virtual const TextGridBuffer* currentBuffer() const { return nullptr; }
    virtual void setOutputEnabled(bool enabled) { m_outputEnabled = enabled; }

    // Terminal capabilities the encoder should target (colour depth, sync).
    virtual void setCapabilities(const TerminalCapabilities& capabilities) { m_capabilities = capabilities; }
    const TerminalCapabilities& capabilities() const { return m_capabilities; }

protected:
    bool outputEnabled() const { return m_outputEnabled; }

    FrameStats m_lastStats;

private:
    bool m_outputEnabled = true;
    TerminalCapabilities m_capabilities;
};

} // namespace qtui
