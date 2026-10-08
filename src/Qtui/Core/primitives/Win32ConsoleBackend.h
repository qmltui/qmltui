#pragma once

#include <Qtui/Core/primitives/TerminalBackend.h>

#include <memory>

class QWinEventNotifier;

namespace qtui {

// Windows console backend targeting Windows Terminal (and any conhost that
// honours virtual-terminal sequences).
//
// Output: ENABLE_VIRTUAL_TERMINAL_PROCESSING so the shared ANSI encoder works
// unchanged, DISABLE_NEWLINE_AUTO_RETURN so the renderer owns line breaks, and
// UTF-8 code pages so box drawing and wide glyphs survive.
//
// Input: ENABLE_VIRTUAL_TERMINAL_INPUT makes the console deliver keystrokes as
// the same VT byte sequences a POSIX terminal produces, so InputDecoder needs
// no Windows knowledge. ENABLE_WINDOW_INPUT adds resize records to the same
// stream. Records are read with ReadConsoleInputW when a QWinEventNotifier on
// the console input handle fires.
//
// Console writes never block, so output is always synchronous here and
// supportsAsyncOutput() is false.
class Win32ConsoleBackend final : public TerminalBackend {
    Q_OBJECT

public:
    explicit Win32ConsoleBackend(int outputFd, QObject* parent = nullptr);
    ~Win32ConsoleBackend() override;

    bool enterRawMode() override;
    bool leaveRawMode() override;

    WriteStatus writeSome(const char* data, size_t size, size_t& offset) override;
    bool writeAll(std::string& pending, int timeoutMs) override;
    bool supportsAsyncOutput() const override { return false; }
    bool setNonBlockingOutput(bool enabled) override;
    void setWritableNotifications(bool enabled) override;

    QSize size() const override;
    bool isInteractive() const override;
    bool isInputInteractive() const override;

    void startInput() override;
    void stopInput() override;

private:
    void readInput();

    void* m_input = nullptr;    // HANDLE, kept opaque so <windows.h> stays out of headers
    void* m_output = nullptr;
    unsigned long m_originalInputMode = 0;   // DWORD
    unsigned long m_originalOutputMode = 0;
    unsigned int m_originalInputCodePage = 0;   // UINT
    unsigned int m_originalOutputCodePage = 0;
    bool m_inputModeValid = false;
    bool m_outputModeValid = false;
    bool m_rawMode = false;
    bool m_inputStarted = false;
    std::unique_ptr<QWinEventNotifier> m_inputNotifier;
};

} // namespace qtui
