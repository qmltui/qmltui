#include <QmlTui/Core/Primitives/WasmTerminalBackend.h>

#include <QCoreApplication>

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

Bridge& bridge() {
    static Bridge b;
    return b;
}

} // namespace

// Copy out of the heap before handing over: xterm.js buffers write() chunks
// asynchronously and ALLOW_MEMORY_GROWTH can move the heap under a view.
// Bytes, not a decoded string: writeSome() may split a UTF-8 sequence, and
// xterm.js reassembles bytes across write() calls.
EM_JS(void, qmltui_wasm_js_output, (const char* data, int length), {
    if (Module['qmltuiOnOutput']) Module['qmltuiOnOutput'](HEAPU8.slice(data, data + length));
});

// Work around a Qt for WebAssembly QEventDispatcherWasm bug: updateNativeTimer()
// truncates QTimerInfoList::timerWait() from nanoseconds to milliseconds, and
// onTimer() only re-arms the native timer when activateTimers() > 0. Whenever
// window.setTimeout fires even a fraction of a millisecond before the nanosecond
// deadline (guaranteed whenever the sub-millisecond remainder is non-zero, or
// when browser timers fire slightly early), activateTimers() returns 0 and the
// native timer is dropped until an unrelated event wakes the dispatcher.
//
// Guard Qt's QWasmTimer callbacks so they never run before performance.now()
// passes (start + delay + 1ms), guaranteeing steady_clock::now() reaches the
// nanosecond deadline without affecting non-Qt page timers.
// clang-format off
EM_JS(void, qmltui_wasm_install_timer_guard, (), {
    var g = typeof window !== 'undefined' ? window : globalThis;
    if (!g || typeof g.setTimeout !== 'function' || typeof g.clearTimeout !== 'function') return;

    var qtModules = g.__qmltuiQtModules || (g.__qmltuiQtModules = []);
    if (!Module['__qmltuiQtTimerRegistered']) {
        Module['__qmltuiQtTimerRegistered'] = true;
        qtModules.push(function() { return Module['qtSuspendResumeControl']; });
    }

    if (g.__qmltuiQtTimerGuardInstalled) return;
    g.__qmltuiQtTimerGuardInstalled = true;

    var origSetTimeout = g.setTimeout.bind(g);
    var origClearTimeout = g.clearTimeout.bind(g);
    var now = (typeof performance !== 'undefined' && typeof performance.now === 'function')
        ? function() { return performance.now(); }
        : function() { return Date.now(); };

    function isQtHandler(fn) {
        if (typeof fn !== 'function') return false;
        for (var i = 0; i < qtModules.length; ++i) {
            var ctrl = qtModules[i]();
            var handlers = ctrl && ctrl.eventHandlers;
            if (!handlers) continue;
            for (var k in handlers) {
                if (handlers[k] === fn) return true;
            }
        }
        return false;
    }

    var nextId = 0x40000000;
    var active = new Map();

    g.setTimeout = function(fn, delay) {
        var extraArgs = arguments.length > 2 ? Array.prototype.slice.call(arguments, 2) : null;
        if (!isQtHandler(fn)) {
            return extraArgs
                ? origSetTimeout.apply(null, [fn, delay].concat(extraArgs))
                : origSetTimeout(fn, delay);
        }

        var d = Number(delay);
        if (!Number.isFinite(d) || d < 0) d = 0;
        var id = nextId++;
        if (nextId >= 0x7fffffff) nextId = 0x40000000;
        var deadline = now() + d + 1.0;

        function attempt() {
            if (!active.has(id)) return;
            var remaining = deadline - now();
            if (remaining > 0) {
                active.set(id, origSetTimeout(attempt, Math.ceil(remaining)));
                return;
            }
            active.delete(id);
            if (extraArgs) fn.apply(null, extraArgs);
            else fn();
        }

        active.set(id, origSetTimeout(attempt, Math.ceil(d + 1)));
        return id;
    };

    g.clearTimeout = function(id) {
        var realId = active.get(id);
        if (realId !== undefined) {
            active.delete(id);
            origClearTimeout(realId);
            return;
        }
        origClearTimeout(id);
    };
});
// clang-format on

WasmTerminalBackend::WasmTerminalBackend(QObject* parent) : TerminalBackend(parent) {
    qmltui_wasm_install_timer_guard();
    // The engine reads terminal capabilities from the environment, and the
    // browser has none. The page is an xterm.js-class emulator, so describe
    // it as one unless the host set something else (Module.ENV is not
    // exported by default in Emscripten 4, so this is the reliable place).
    ::setenv("TERM", "xterm-256color", /*overwrite=*/0);
    ::setenv("COLORTERM", "truecolor", /*overwrite=*/0);
    bridge().live = this;
}

WasmTerminalBackend::~WasmTerminalBackend() {
    if (bridge().live == this) bridge().live = nullptr;
}

TerminalBackend::WriteStatus WasmTerminalBackend::writeSome(const char* data, size_t size,
                                                            size_t& offset) {
    if (offset < size) qmltui_wasm_js_output(data + offset, static_cast<int>(size - offset));
    offset = size;
    return WriteStatus::Done;
}

bool WasmTerminalBackend::writeAll(std::string& pending, int timeoutMs) {
    Q_UNUSED(timeoutMs);
    size_t offset = 0;
    writeSome(pending.data(), pending.size(), offset);
    pending.clear();
    return true;
}

QSize WasmTerminalBackend::size() const {
    return bridge().size;
}

void WasmTerminalBackend::deliverInput(const QByteArray& bytes) {
    if (m_inputStarted && !bytes.isEmpty()) emit inputReceived(bytes);
}

void WasmTerminalBackend::deliverResize() {
    emit resized();
}

namespace {

// embind copies a JS Uint8Array / ArrayBuffer into std::string as raw bytes,
// so NUL (Ctrl+Space) survives and there is no UTF-16 length confusion. A JS
// string is accepted too and arrives as UTF-8.
void feedInput(const std::string& bytes) {
    if (WasmTerminalBackend* backend = bridge().live)
        backend->deliverInput(QByteArray(bytes.data(), static_cast<qsizetype>(bytes.size())));
}

void resize(int cols, int rows) {
    const QSize size(cols, rows);
    if (bridge().size == size) return;
    bridge().size = size;
    if (WasmTerminalBackend* backend = bridge().live) backend->deliverResize();
}

} // namespace

EMSCRIPTEN_BINDINGS(qmltui_wasm_bridge) {
    emscripten::function("qmltuiFeedInput", &feedInput);
    emscripten::function("qmltuiResize", &resize);
}

} // namespace qmltui

static void qmltuiInstallWasmTimerGuard() {
    qmltui::qmltui_wasm_install_timer_guard();
}

Q_COREAPP_STARTUP_FUNCTION(qmltuiInstallWasmTimerGuard)
