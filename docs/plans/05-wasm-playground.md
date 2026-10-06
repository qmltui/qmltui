# Plan 05: WebAssembly Playground (post-launch)

## 1. Goal

An in-browser, fully interactive Qtui app at `/playground`, compiled with Qt
for WebAssembly and rendered in `xterm.js`. This is a real engineering project,
not a wiring step, and it is explicitly **not** a launch dependency. Casts
(Plan 02) cover "see it" on day one. A broken live demo does more damage than
none, so this ships only when the acceptance criteria in Section 6 pass.

---

## 2. What Actually Stands in the Way

The earlier plan claimed Qtui is "uniquely suited" because it avoids QtGui.
The dependency footprint is favorable, but the terminal I/O layer is POSIX:

| Component | Today | Under Emscripten |
| :--- | :--- | :--- |
| `primitives/TerminalDevice` | `termios` raw mode, `write()` to fd 1 | No tty, no termios |
| `engine/TerminalEngine` | `QSocketNotifier` on `STDIN_FILENO` | stdin is not a readable fd in the browser |
| `TerminalEngine` resize | `SIGWINCH` through a self-pipe and `QSocketNotifier` | No signals |
| `Terminal` size query | `ioctl(TIOCGWINSZ)` | Must come from `xterm.js` |

Additionally, Emscripten's `Module.print` callback is line-buffered and strips
nothing, but a TUI emits cursor-movement sequences without newlines, so frames
arrive late or split. Output must bypass `print` entirely.

---

## 3. Architecture

### A. Backend abstraction in the core

The `TerminalBackend` seam is delivered by Plan 00, where it is needed for
Windows Terminal support. By the time this plan starts, `PosixTerminalBackend`
and `Win32ConsoleBackend` exist and the engine already selects a backend at
construction. This plan adds the third implementation only:

```cpp
class WasmTerminalBackend final : public TerminalBackend {
    bool enterRawMode() override { return true; }            // nothing to configure
    void leaveRawMode() override {}
    void write(std::span<const char> bytes) override;        // EM_JS to xterm.js
    bool flush(int) override { return true; }
    QSize size() const override { return m_size; }           // pushed from JS
    bool isInteractive() const override { return true; }
    // inputReceived() and resized() are emitted from the exported C entry points.
};
```

No engine or device changes are expected. If any are, they belong in Plan 00's
seam, not here.

### B. The JavaScript bridge

Output, C++ to JS, unbuffered:

```cpp
EM_JS(void, qtui_js_write, (const char* data, int len), {
    Module.qtuiOnOutput(UTF8ToString(data, len));
});
```

Input and resize, JS to C++, exported and called with `ccall`:

```cpp
extern "C" {
EMSCRIPTEN_KEEPALIVE void qtui_feed_input(const char* bytes, int len);
EMSCRIPTEN_KEEPALIVE void qtui_resize(int cols, int rows);
}
```

`qtui_feed_input` appends to a queue and emits `inputReceived` on the Qt event
loop. Qt for WebAssembly's `QEventDispatcherWasm` returns from `app.exec()`
immediately and drives the loop from the browser, so no Asyncify is required.

### C. Build

Use the Qt for WebAssembly kit matching the Emscripten version listed in that
Qt release's documentation (Qt 6.8 pairs with emsdk 3.1.56). Configure with the
kit's `qt-cmake`, single-threaded:

```bash
~/Qt/6.8.x/wasm_singlethread/bin/qt-cmake -S examples/apps/btop -B build-wasm \
    -DQTUI_WASM=ON -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-wasm
```

`cmake/QtuiWasm.cmake`:

```cmake
if(EMSCRIPTEN)
    target_link_options(${target} PRIVATE
        -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createQtuiApp
        -sALLOW_MEMORY_GROWTH=1
        -sEXPORTED_FUNCTIONS=_main,_qtui_feed_input,_qtui_resize
        -sEXPORTED_RUNTIME_METHODS=ccall,UTF8ToString)
endif()
```

No `.html` suffix. The site imports the ES module directly. QML JIT is
unavailable in WebAssembly; the interpreter is sufficient for these apps.

### D. `WasmTerminal.svelte` (Svelte 5)

```svelte
<script lang="ts">
  import { Terminal } from '@xterm/xterm';
  import { FitAddon } from '@xterm/addon-fit';
  import '@xterm/xterm/css/xterm.css';
  let { app = 'btop' } = $props();
  let host: HTMLDivElement;

  $effect(() => {
    const term = new Terminal({ cursorBlink: false, theme: { background: '#0d1117', foreground: '#c9d1d9' } });
    const fit = new FitAddon();
    term.loadAddon(fit);
    term.open(host);
    fit.fit();

    let mod: any;
    (async () => {
      const { default: createQtuiApp } = await import(`/wasm/${app}/qtui_${app}.js`);
      mod = await createQtuiApp({
        qtuiOnOutput: (s: string) => term.write(s),
        arguments: [],
        locateFile: (p: string) => `/wasm/${app}/${p}`,
      });
      mod.ccall('qtui_resize', null, ['number', 'number'], [term.cols, term.rows]);
      term.onData((d) => mod.ccall('qtui_feed_input', null, ['string', 'number'], [d, d.length]));
      term.onResize(({ cols, rows }) => mod.ccall('qtui_resize', null, ['number', 'number'], [cols, rows]));
    })();

    const ro = new ResizeObserver(() => fit.fit());
    ro.observe(host);
    return () => { ro.disconnect(); term.dispose(); };
  });
</script>

<div bind:this={host} class="h-[600px] w-full rounded-lg border border-zinc-800 bg-[#0d1117]"></div>
```

The callback object is passed into the factory, so the module sees it; the old
plan's local `Module` const was never visible to the Emscripten script.

---

## 4. Scope for the First Playground

One app, `btop`, with synthetic data since there is no `/proc` in the browser.
Then `snake_game` and `twozero48`, which need no host data at all. Apps that
need DBus, PTYs, or the filesystem stay out.

---

## 5. Deliverables

1. `WasmTerminalBackend` on the Plan 00 seam, and `cmake/QtuiWasm.cmake`.
2. `docs/guide/webassembly.md`: exact emsdk and Qt versions, build commands,
   and measured sizes.
3. `/playground` route, hidden from navigation until acceptance passes.
4. Brotli-compressed artifacts under `website/static/wasm/<app>/`, uploaded by
   a dedicated workflow that runs on release tags only.

---

## 6. Acceptance

- `btop` loads and responds to arrow keys within three seconds on a cold cache
  over a 10 Mbps connection. Measured, not estimated.
- Resizing the browser re-lays out the app with no torn frames.
- Compressed payload per app is under 6 MB, or the page says the real number.
- The POSIX and Windows test suites are unchanged in outcome.
