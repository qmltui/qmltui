#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>
#include <Qamui/Core/Primitives/TerminalBackend.h>

#include <QByteArray>
#include <QSize>
#include <string>

namespace qamui {

// Browser backend for Qt for WebAssembly. Output bytes are handed to the page
// through the Module.qamuiOnOutput callback; key bytes and the terminal geometry
// come back through the embind entry points Module.qamuiFeedInput(Uint8Array)
// and Module.qamuiResize(cols, rows). The page is expected to host an xterm.js
// instance, but any terminal emulator that speaks VT and UTF-8 will do.
//
// One live instance per page: createTerminalBackend() runs once per
// TerminalDevice, and the bridge forwards input to the most recently
// constructed backend. Output is synchronous (supportsAsyncOutput() is false),
// which is exact for a callback that copies the bytes out immediately.
class QAMUI_CORE_EXPORT WasmTerminalBackend final : public TerminalBackend {
    Q_OBJECT

public:
    explicit WasmTerminalBackend(QObject* parent = nullptr);
    ~WasmTerminalBackend() override;

    // TerminalBackend
    bool enterRawMode() override { return true; }   // xterm.js has no cooked mode
    bool leaveRawMode() override { return true; }
    WriteStatus writeSome(const char* data, size_t size, size_t& offset) override;
    bool writeAll(std::string& pending, int timeoutMs) override;
    bool supportsAsyncOutput() const override { return false; }
    bool setNonBlockingOutput(bool enabled) override { return !enabled; }
    void setWritableNotifications(bool) override {}
    QSize size() const override;                    // last size pushed from JS, 80x24 until then
    bool isInteractive() const override { return true; }
    void startInput() override { m_inputStarted = true; }
    void stopInput() override { m_inputStarted = false; }

    // Bridge side, called from the embind entry points.
    void deliverInput(const QByteArray& bytes);     // emits inputReceived() once started
    void deliverResize();                           // emits resized()

private:
    bool m_inputStarted = false;
};

} // namespace qamui
