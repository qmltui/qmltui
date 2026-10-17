#pragma once

#include <QmlTui/Core/Primitives/TerminalDevice.h>
#include <QmlTui/Core/Primitives/TerminalCapabilities.h>
#include <QtGlobal>
#include <optional>

namespace qmltui {

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

// Strategy interface for painting the Terminal item tree and committing ANSI
// output to a TerminalDevice. Implemented by AltScreenRenderer and InlineRenderer.
class Renderer {
public:
    virtual ~Renderer() = default;

    const FrameStats& lastFrameStats() const { return m_lastStats; }

    // Enters the renderer's screen mode and prepares initial terminal state.
    virtual void start(Terminal* rootWindow, TerminalDevice& device) = 0;
    // Restores terminal screen state and cursor visibility on shutdown or mode switch.
    virtual void stop(Terminal* rootWindow, TerminalDevice& device) = 0;
    virtual void resize(int width, int height) = 0;
    // Layouts, paints, diffs, and writes one frame. Returns false if the device write failed.
    virtual bool renderFrame(Terminal* rootWindow, TerminalDevice& device) = 0;
    virtual bool usesAlternateScreen(Terminal*) const { return false; }
    virtual const TextGridBuffer* currentBuffer() const { return nullptr; }
    // Screen row of the committed frame's first row; unknown until an inline
    // renderer has located its live region in the normal terminal buffer.
    virtual std::optional<int> mouseRowOffset() const { return 0; }
    virtual void setOutputEnabled(bool enabled) { m_outputEnabled = enabled; }
    // Bytes of committed frames were discarded after an output error: forget
    // what the terminal is believed to show so the next frame repaints it all.
    virtual void discardCommittedState() {}

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

} // namespace qmltui
