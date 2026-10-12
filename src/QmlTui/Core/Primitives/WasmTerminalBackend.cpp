#include <QmlTui/Core/Primitives/WasmTerminalBackend.h>

#include <emscripten.h>
#include <emscripten/bind.h>

#include <cstdlib>

namespace qmltui {

namespace {

// Shared between the backend and the page. The size can arrive before the
// first Terminal exists (the page calls qmltuiResize before callMain), so it
// lives here rather than on the instance.
struct Bridge {
    QSize size{80, 24};
    WasmTerminalBackend* live = nullptr;
};

Bridge& bridge()
{
    static Bridge b;
    return b;
}

} // namespace

// Copy out of the heap before handing over: xterm.js buffers write() chunks
// asynchronously and ALLOW_MEMORY_GROWTH can move the heap under a view.
// Bytes, not a decoded string: writeSome() may split a UTF-8 sequence, and
// xterm.js reassembles bytes across write() calls.
EM_JS(void, qmltui_wasm_js_output, (const char* data, int length), {
    if (Module['qmltuiOnOutput'])
        Module['qmltuiOnOutput'](HEAPU8.slice(data, data + length));
});

WasmTerminalBackend::WasmTerminalBackend(QObject* parent)
    : TerminalBackend(parent)
{
    // The engine reads terminal capabilities from the environment, and the
    // browser has none. The page is an xterm.js-class emulator, so describe
    // it as one unless the host set something else (Module.ENV is not
    // exported by default in Emscripten 4, so this is the reliable place).
    ::setenv("TERM", "xterm-256color", /*overwrite=*/0);
    ::setenv("COLORTERM", "truecolor", /*overwrite=*/0);
    bridge().live = this;
}

WasmTerminalBackend::~WasmTerminalBackend()
{
    if (bridge().live == this)
        bridge().live = nullptr;
}

TerminalBackend::WriteStatus WasmTerminalBackend::writeSome(const char* data, size_t size, size_t& offset)
{
    if (offset < size)
        qmltui_wasm_js_output(data + offset, static_cast<int>(size - offset));
    offset = size;
    return WriteStatus::Done;
}

bool WasmTerminalBackend::writeAll(std::string& pending, int timeoutMs)
{
    Q_UNUSED(timeoutMs);
    size_t offset = 0;
    writeSome(pending.data(), pending.size(), offset);
    pending.clear();
    return true;
}

QSize WasmTerminalBackend::size() const
{
    return bridge().size;
}

void WasmTerminalBackend::deliverInput(const QByteArray& bytes)
{
    if (m_inputStarted && !bytes.isEmpty())
        emit inputReceived(bytes);
}

void WasmTerminalBackend::deliverResize()
{
    emit resized();
}

namespace {

// embind copies a JS Uint8Array / ArrayBuffer into std::string as raw bytes,
// so NUL (Ctrl+Space) survives and there is no UTF-16 length confusion. A JS
// string is accepted too and arrives as UTF-8.
void feedInput(const std::string& bytes)
{
    if (WasmTerminalBackend* backend = bridge().live)
        backend->deliverInput(QByteArray(bytes.data(), static_cast<qsizetype>(bytes.size())));
}

void resize(int cols, int rows)
{
    const QSize size(cols, rows);
    if (bridge().size == size)
        return;
    bridge().size = size;
    if (WasmTerminalBackend* backend = bridge().live)
        backend->deliverResize();
}

} // namespace

EMSCRIPTEN_BINDINGS(qmltui_wasm_bridge)
{
    emscripten::function("qmltuiFeedInput", &feedInput);
    emscripten::function("qmltuiResize", &resize);
}

} // namespace qmltui
