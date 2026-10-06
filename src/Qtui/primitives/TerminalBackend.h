#pragma once

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <cstddef>
#include <memory>
#include <string>

namespace qtui {

// Platform seam between Qtui and the operating system's terminal.
//
// Everything above this interface is shared: TerminalDevice owns the output
// queue and raw-mode bookkeeping, TerminalEngine decodes input bytes and
// drives the renderers. Everything below it is platform code, one class per
// platform:
//
//   PosixTerminalBackend  termios raw mode, write(2)/poll(2), a QSocketNotifier
//                         on stdin, and a process-wide SIGWINCH bridge.
//   Win32ConsoleBackend   console modes with virtual-terminal input and output,
//                         WriteFile, and ReadConsoleInputW driven by a
//                         QWinEventNotifier on the console input handle.
//
// Input is always delivered as raw VT byte sequences through inputReceived(),
// so InputDecoder needs no platform knowledge. Geometry changes arrive through
// resized(); the receiver queries size() afterwards.
class TerminalBackend : public QObject {
    Q_OBJECT

public:
    enum class WriteStatus { Done, WouldBlock, Error };

    explicit TerminalBackend(QObject* parent = nullptr);
    ~TerminalBackend() override;

    // Mode ----------------------------------------------------------------
    virtual bool enterRawMode() = 0;
    virtual bool leaveRawMode() = 0;
    // True when the terminal parser must be told the byte stream is UTF-8
    // (ESC % G) right after entering raw mode.
    virtual bool needsUtf8Designation() const { return false; }

    // Output --------------------------------------------------------------
    // Writes as much of [data + offset, data + size) as the terminal accepts
    // right now and advances offset. WouldBlock is only possible after
    // setNonBlockingOutput(true).
    virtual WriteStatus writeSome(const char* data, size_t size, size_t& offset) = 0;
    // Delivers everything in `pending`, erasing delivered bytes, waiting for
    // the terminal as needed but never past timeoutMs. False on error/timeout.
    virtual bool writeAll(std::string& pending, int timeoutMs) = 0;
    // Whether output can be made non-blocking and drained from the event loop.
    virtual bool supportsAsyncOutput() const = 0;
    // Switches the output channel between blocking and non-blocking. Disabling
    // when already blocking succeeds. Returns false when unsupported or on error.
    virtual bool setNonBlockingOutput(bool enabled) = 0;
    // Enables or disables writable() notifications for a non-blocking channel.
    virtual void setWritableNotifications(bool enabled) = 0;

    // Geometry and identity ----------------------------------------------
    // Terminal size in cells, or an invalid QSize when unknown.
    virtual QSize size() const = 0;
    // True when output goes to a real terminal rather than a pipe or file.
    virtual bool isInteractive() const = 0;

    // Input ---------------------------------------------------------------
    // Begins delivering inputReceived() and resized(). Idempotent.
    virtual void startInput() = 0;
    virtual void stopInput() = 0;

signals:
    // The non-blocking output channel can accept more bytes.
    void writable();
    // Raw VT bytes read from the terminal.
    void inputReceived(const QByteArray& bytes);
    // The terminal geometry changed; call size().
    void resized();
};

// Creates the backend for this platform. outputFd < 0 selects the process's
// standard output; any other descriptor is used for output and size queries
// while input always comes from the process's standard input.
std::unique_ptr<TerminalBackend> createTerminalBackend(int outputFd = -1);

} // namespace qtui
