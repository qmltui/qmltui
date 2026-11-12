#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace qmltui {

// Platform seam between QmlTui and the operating system's terminal.
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
//   WasmTerminalBackend   Qt for WebAssembly: bytes out through a JS callback,
//                         key bytes and geometry in through embind entry
//                         points, driven by an xterm.js instance on the page.
//
// Input is always delivered as raw VT byte sequences through inputReceived(),
// so InputDecoder needs no platform knowledge. Geometry changes arrive through
// resized(); the receiver queries size() afterwards.
class QMLTUI_CORE_EXPORT TerminalBackend : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TerminalBackend)

public:
    enum class WriteStatus : uint8_t { Done, WouldBlock, Error };

    explicit TerminalBackend(QObject* parent = nullptr);
    ~TerminalBackend() override;

    // A factory installed here replaces the platform backend for every
    // TerminalDevice created afterwards: serial consoles, in-process hosts,
    // WebAssembly bridges, or an in-memory backend for tests. Install it before
    // the first Terminal completes; pass nullptr to restore the platform default.
    // This is a C++ extension point and may still change before 1.0.
    using Factory = std::function<std::unique_ptr<TerminalBackend>(int outputFd)>;
    static void setFactory(Factory factory);
    static const Factory& factory();

    virtual bool enterRawMode() = 0;
    virtual bool leaveRawMode() = 0;

    // True when the terminal parser must be told the stream is UTF-8 (ESC % G) right after raw
    // mode.
    virtual bool needsUtf8Designation() const { return false; }

    virtual void setKeyboardProtocolActive(bool) {}

    // Writes bytes from data[offset..size) and advances offset by the count written.
    virtual WriteStatus writeSome(const char* data, size_t size, size_t& offset) = 0;

    // Writes all pending bytes (erasing as written), waiting up to timeoutMs (-1 for indefinite).
    virtual bool writeAll(std::string& pending, int timeoutMs) = 0;

    virtual bool supportsAsyncOutput() const = 0;
    virtual bool setNonBlockingOutput(bool enabled) = 0;
    virtual void setWritableNotifications(bool enabled) = 0;

    // Terminal window geometry in character cells (columns, rows).
    virtual QSize size() const = 0;

    // True when output is connected to an interactive terminal rather than a pipe or file.
    virtual bool isInteractive() const = 0;
    virtual bool isInputInteractive() const { return isInteractive(); }

    virtual void startInput() = 0;
    virtual void stopInput() = 0;

signals:
    // Non-blocking output channel is ready to accept more bytes.
    void writable();

    // Raw VT byte sequence read from the terminal input stream.
    void inputReceived(const QByteArray& bytes);

    // Terminal window dimensions changed (e.g. SIGWINCH or console buffer resize).
    void resized();

    // Process resumed after job-control suspension (SIGCONT); terminal modes
    // and screen contents must be re-established.
    void resumed();
};

// Creates a backend: the installed TerminalBackend::factory() when there is
// one, otherwise this platform's backend. outputFd < 0 selects the process's
// standard output; any other descriptor is used for output and size queries
// while input always comes from the process's standard input.
QMLTUI_CORE_EXPORT std::unique_ptr<TerminalBackend> createTerminalBackend(int outputFd = -1);

} // namespace qmltui
