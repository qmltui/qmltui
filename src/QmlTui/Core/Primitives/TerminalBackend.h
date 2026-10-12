#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <cstddef>
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

public:
    enum class WriteStatus { Done, WouldBlock, Error };

    explicit TerminalBackend(QObject* parent = nullptr);
    ~TerminalBackend() override;

    // Application-provided backends -----------------------------------------
    // A factory installed here replaces the platform backend for every
    // TerminalDevice created afterwards: serial consoles, in-process hosts,
    // WebAssembly bridges, or an in-memory backend for tests. Install it before
    // the first Terminal completes; pass nullptr to restore the platform default.
    // This is a C++ extension point and may still change before 1.0.
    using Factory = std::function<std::unique_ptr<TerminalBackend>(int outputFd)>;
    static void setFactory(Factory factory);
    static const Factory& factory();

    // Mode ----------------------------------------------------------------
    /**
     * @brief Configures the terminal device in raw mode (disabling echo and canonical input).
     * @return True on success, false on platform error.
     */
    virtual bool enterRawMode() = 0;

    /**
     * @brief Restores the terminal device to its original cooked mode.
     * @return True on success, false on error.
     */
    virtual bool leaveRawMode() = 0;

    /// @brief True when the terminal parser must be told the stream is UTF-8 (ESC % G) right after raw mode.
    virtual bool needsUtf8Designation() const { return false; }

    /// @brief Informs the backend whether kitty keyboard protocol is currently active.
    virtual void setKeyboardProtocolActive(bool) {}

    // Output --------------------------------------------------------------
    /**
     * @brief Writes up to @p size bytes from @p data starting at @p offset.
     * @param data   Byte buffer to write.
     * @param size   Total byte length of @p data.
     * @param offset Current read offset into @p data (advanced on successful writes).
     * @return WriteStatus::Done, WouldBlock, or Error.
     */
    virtual WriteStatus writeSome(const char* data, size_t size, size_t& offset) = 0;

    /**
     * @brief Writes all pending bytes, waiting up to @p timeoutMs.
     * @param pending String containing pending bytes (erased as bytes are written).
     * @param timeoutMs Timeout in milliseconds (-1 for indefinite).
     * @return True if all bytes were written before timeout.
     */
    virtual bool writeAll(std::string& pending, int timeoutMs) = 0;

    /// @brief Whether output channel supports non-blocking writes and event loop notifications.
    virtual bool supportsAsyncOutput() const = 0;

    /// @brief Switches the output channel between blocking and non-blocking modes.
    virtual bool setNonBlockingOutput(bool enabled) = 0;

    /// @brief Enables or disables writable() signal notifications.
    virtual void setWritableNotifications(bool enabled) = 0;

    // Geometry and identity ----------------------------------------------
    /// @brief Returns terminal window geometry in character cells (width, height).
    virtual QSize size() const = 0;

    /// @brief True when stdout is connected to an interactive TTY rather than a pipe or file.
    virtual bool isInteractive() const = 0;

    /// @brief True when stdin is connected to an interactive terminal device.
    virtual bool isInputInteractive() const { return isInteractive(); }

    // Input ---------------------------------------------------------------
    /// @brief Starts monitoring input descriptor for incoming bytes and resize signals.
    virtual void startInput() = 0;

    /// @brief Stops monitoring input descriptor.
    virtual void stopInput() = 0;

signals:
    /// @brief Emitted when a non-blocking output channel is ready to accept more bytes.
    void writable();

    /// @brief Emitted when raw VT bytes are read from the terminal input stream.
    void inputReceived(const QByteArray& bytes);

    /// @brief Emitted when terminal window dimensions change (e.g., SIGWINCH).
    void resized();

    /**
     * @brief Emitted when process continues after a SIGCONT job-control pause.
     *
     * Screen state and terminal modes are invalid and must be re-rendered.
     */
    void resumed();
};

// Creates a backend: the installed TerminalBackend::factory() when there is
// one, otherwise this platform's backend. outputFd < 0 selects the process's
// standard output; any other descriptor is used for output and size queries
// while input always comes from the process's standard input.
QMLTUI_CORE_EXPORT std::unique_ptr<TerminalBackend> createTerminalBackend(int outputFd = -1);

} // namespace qmltui
