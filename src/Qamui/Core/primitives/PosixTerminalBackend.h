#pragma once

#include <Qamui/Core/primitives/TerminalBackend.h>

#include <termios.h>
#include <memory>

class QSocketNotifier;

namespace qamui {

// POSIX terminal backend: termios raw mode on standard input, write(2) and
// poll(2) on the output descriptor, a read notifier on standard input, and a
// process-wide signal bridge shared by every backend instance. While in raw
// mode it also owns the process's handlers for fatal signals and job control:
// a crash, SIGTERM or Ctrl+Z restores the terminal before the default action,
// and SIGCONT re-enters raw mode and emits resumed().
class PosixTerminalBackend final : public TerminalBackend {
    Q_OBJECT

public:
    explicit PosixTerminalBackend(int outputFd, QObject* parent = nullptr);
    ~PosixTerminalBackend() override;

    bool enterRawMode() override;
    bool leaveRawMode() override;
    bool needsUtf8Designation() const override { return true; }
    void setKeyboardProtocolActive(bool active) override;

    WriteStatus writeSome(const char* data, size_t size, size_t& offset) override;
    bool writeAll(std::string& pending, int timeoutMs) override;
    bool supportsAsyncOutput() const override { return true; }
    bool setNonBlockingOutput(bool enabled) override;
    void setWritableNotifications(bool enabled) override;

    QSize size() const override;
    bool isInteractive() const override;
    bool isInputInteractive() const override;

    void startInput() override;
    void stopInput() override;

    int outputFd() const { return m_outputFd; }

private:
    void readInput();

    int m_outputFd;
    int m_inputFd;
    termios m_originalTermios{};
    bool m_termiosValid = false;
    int m_originalFlags = -1;
    bool m_inputStarted = false;
    std::unique_ptr<QSocketNotifier> m_writeNotifier;
    std::unique_ptr<QSocketNotifier> m_readNotifier;
};

} // namespace qamui
