#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>
#include <QmlTui/Core/Primitives/TerminalBackend.h>

#include <QByteArray>
#include <QSize>
#include <string>

namespace qmltui {

// In-memory TerminalBackend for tests and as the reference implementation of
// the extension point: output bytes are captured, input bytes and resizes are
// injected by the test. Install it with TerminalBackend::setFactory() before
// the Terminal under test completes.
class QMLTUI_RUNNER_EXPORT MemoryTerminalBackend final : public TerminalBackend {
    Q_OBJECT

public:
    explicit MemoryTerminalBackend(QSize size = QSize(80, 24), QObject* parent = nullptr);

    // TerminalBackend
    bool enterRawMode() override;
    bool leaveRawMode() override;
    WriteStatus writeSome(const char* data, size_t size, size_t& offset) override;
    bool writeAll(std::string& pending, int timeoutMs) override;
    bool supportsAsyncOutput() const override { return false; }
    bool setNonBlockingOutput(bool enabled) override { return !enabled; }
    void setWritableNotifications(bool) override {}
    QSize size() const override { return m_size; }
    bool isInteractive() const override { return m_interactive; }
    void startInput() override { m_inputStarted = true; }
    void stopInput() override { m_inputStarted = false; }

    // Test controls
    void feedInput(const QByteArray& bytes);   // emits inputReceived()
    void setSize(const QSize& size);           // emits resized() when it changes
    void setInteractive(bool interactive) { m_interactive = interactive; }

    const std::string& output() const { return m_output; }
    void clearOutput() { m_output.clear(); }
    bool rawMode() const { return m_rawMode; }
    bool inputStarted() const { return m_inputStarted; }

private:
    QSize m_size;
    std::string m_output;
    bool m_rawMode = false;
    bool m_interactive = true;
    bool m_inputStarted = false;
};

} // namespace qmltui
