#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace qtui {

class TerminalBackend;

// Terminal byte output on top of a platform TerminalBackend: raw-mode
// bookkeeping, cursor helpers, size queries, and an ordered output queue.
//
// Output is blocking by default. With setAsyncOutput(true) the backend's
// output channel is switched to non-blocking and bytes the terminal cannot
// take immediately are queued and drained from the Qt event loop, so a slow
// terminal (ssh, a busy emulator) never stalls the application. Ordering is
// preserved because every write goes through the same queue. Backends whose
// writes never block (the Windows console) report supportsAsyncOutput() false
// and keep output synchronous.
//
// Input and resize notifications are the backend's job; TerminalEngine
// connects to them through backend().
class TerminalDevice {
public:
    // outputFd < 0 selects the process's standard output.
    explicit TerminalDevice(int outputFd = -1);
    ~TerminalDevice();

    TerminalDevice(const TerminalDevice&) = delete;
    TerminalDevice& operator=(const TerminalDevice&) = delete;

    TerminalBackend* backend() const { return m_backend.get(); }

    bool enableRawMode();
    bool disableRawMode();

    void hideCursor();
    void showCursor();
    void clearScreen();

    // Writes (or queues, in async mode) a string of ANSI sequences.
    bool write(const std::string& data);

    // Terminal dimensions in cells, with 80x24 as the fallback.
    int getWidth() const;
    int getHeight() const;

    int outputFd() const { return m_outputFd; }

    // Non-blocking output with an event-loop drained queue. Requires a
    // QCoreApplication and a backend that supports it. If disabling times out,
    // returns false and leaves async mode and the queue intact so delivery can
    // resume.
    bool setAsyncOutput(bool enabled, int timeoutMs = 2000);
    bool asyncOutput() const { return m_async; }
    size_t pendingBytes() const { return m_pending.size(); }

    // Writes everything still queued, waiting for the terminal if necessary.
    // Returns false on error or when timeoutMs elapses.
    bool flushBlocking(int timeoutMs = 2000);

    // Final teardown: queue cursor/style restoration, flush within one deadline,
    // then restore the output channel. Returns false and discards undelivered
    // bytes on timeout/error. Idempotent; further writes fail until async output
    // is explicitly enabled again. Callers must report failure to their users.
    bool finishOutput(int timeoutMs = 2000);

    // Called from the event loop when the queue has drained completely.
    void setDrainedCallback(std::function<void()> callback);

private:
    void drain();
    void setNotifierEnabled(bool enabled);

    std::unique_ptr<TerminalBackend> m_backend;
    int m_outputFd = 1;
    bool m_isRawMode = false;
    bool m_hasWrittenOutput = false;
    bool m_async = false;
    bool m_outputFinished = false;
    bool m_finishSucceeded = true;
    std::string m_pending;
    std::function<void()> m_drainedCallback;
};

} // namespace qtui
