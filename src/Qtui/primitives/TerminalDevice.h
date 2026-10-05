#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

namespace qtui {

class TerminalDeviceNotifier;

// Raw terminal byte I/O: mode switching, size queries and output.
//
// Output is blocking by default. With setAsyncOutput(true) the descriptor is
// switched to O_NONBLOCK and bytes the terminal cannot take immediately are
// queued and drained from the Qt event loop, so a slow terminal (ssh, a busy
// emulator) never stalls the application. Ordering is preserved because every
// write goes through the same queue.
class TerminalDevice {
public:
    // outputFd < 0 selects STDOUT_FILENO.
    explicit TerminalDevice(int outputFd = -1);
    ~TerminalDevice();

    bool enableRawMode();
    bool disableRawMode();

    void hideCursor();
    void showCursor();
    void clearScreen();

    // Writes (or queues, in async mode) a string of ANSI sequences.
    bool write(const std::string& data);

    // Get terminal dimensions
    int getWidth() const;
    int getHeight() const;

    int outputFd() const { return m_outputFd; }

    // Non-blocking output with an event-loop drained queue. Requires a
    // QCoreApplication; unsupported on Windows. If disabling times out, returns
    // false and leaves async mode and the queue intact so delivery can resume.
    bool setAsyncOutput(bool enabled, int timeoutMs = 2000);
    bool asyncOutput() const { return m_async; }
    size_t pendingBytes() const { return m_pending.size(); }

    // Writes everything still queued, waiting for the terminal if necessary.
    // Returns false on error or when timeoutMs elapses.
    bool flushBlocking(int timeoutMs = 2000);

    // Final teardown: queue cursor/style restoration, flush within one deadline,
    // then restore descriptor flags. Returns false and discards undelivered bytes
    // on timeout/error. Idempotent; further writes fail until async output is
    // explicitly enabled again. Callers must report failure to their users.
    bool finishOutput(int timeoutMs = 2000);

    // Called from the event loop when the queue has drained completely.
    void setDrainedCallback(std::function<void()> callback);

private:
    friend class TerminalDeviceNotifier;
    enum class WriteStatus { Done, WouldBlock, Error };
    WriteStatus writeSome(const char* data, size_t size, size_t& offset);
    void drain();
    void setNotifierEnabled(bool enabled);

#ifdef _WIN32
    DWORD m_originalConsoleMode = 0;
    DWORD m_originalOutputMode = 0;
#else
    termios m_originalTermios;
    bool m_termiosValid = false;
    int m_originalFlags = -1;
#endif
    int m_outputFd = 1;
    bool m_isRawMode = false;
    bool m_hasWrittenOutput = false;
    bool m_async = false;
    bool m_outputFinished = false;
    bool m_finishSucceeded = true;
    std::string m_pending;
    std::unique_ptr<TerminalDeviceNotifier> m_notifier;
    std::function<void()> m_drainedCallback;
};

} // namespace qtui
