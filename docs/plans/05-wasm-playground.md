# Plan 05: WebAssembly Playground (post-launch)

## 1. Goal

An in-browser, fully interactive Qamui app at `/playground`, compiled with Qt
for WebAssembly and rendered in `xterm.js`. This is a real engineering project,
not a wiring step, and it is explicitly **not** a launch dependency. Casts
(Plan 02) cover "see it" on day one. A broken live demo does more damage than
none, so the public route ships only when the acceptance criteria in Section 6
pass.

Status 2026-09-14: the engineering half is done and verified in headless
Firefox (Sections 2 to 6 record what was built). The website half waits for
Plan 01.

---

## 2. What Actually Stood in the Way

The earlier version of this plan listed the POSIX terminal layer as the
obstacle. Plan 00 removed it: `TerminalBackend` is the seam, Core has no POSIX
code outside `PosixTerminalBackend`, and `MemoryTerminalBackend` proves the
interface. What actually blocked a browser build, all found and fixed on
2026-09-14:

| Blocker | Fix |
| :--- | :--- |
| No `EMSCRIPTEN` branch anywhere; Core compiled `PosixTerminalBackend` under Emscripten | `WasmTerminalBackend` selected in `TerminalBackend.cpp` and `src/Qamui/Core/CMakeLists.txt` |
| `TestRunner` owns a `std::thread` watchdog; the wasm kit has no threads; both games link TestRunner whenever the target exists | Wrapper project forces tests, TestRunner, and benchmarks off |
| `examples/apps/CMakeLists.txt` adds 14 apps unconditionally (QProcess, DBus, `/proc`, md4qt) | Wrapper project adds only the two games |
| Static `qt_add_qml_module` creates resource and plugin-init object libraries, and utf8proc joins Core's link interface; `install(EXPORT QamuiTargets)` rejected both | `OUTPUT_TARGETS` installed into the export set in Core, Controls, Image; utf8proc exported when Core is static |
| A consumer that runs `qt_standard_project_setup()` before fetching Qamui turns AUTOMOC on for utf8proc, whose `-std=c99` then hits a C++ moc stub (GCC warns, clang errors) | `AUTOMOC OFF` on the utf8proc target in `cmake/QamuiUtf8proc.cmake` |
| Emscripten's environment is empty, so capability detection saw no `TERM` | Backend sets `TERM` and `COLORTERM` when unset |

Additionally, Emscripten's `Module.print` is line-buffered and useless for a
TUI, and `UTF8ToString` on a chunk boundary corrupts split sequences. Output
bypasses both and travels as bytes.

---

## 3. Architecture

### A. `WasmTerminalBackend` in Core

`src/Qamui/Core/primitives/WasmTerminalBackend.{h,cpp}`, compiled only under
Emscripten, about the size of `MemoryTerminalBackend`:

```cpp
class WasmTerminalBackend final : public TerminalBackend {
    bool enterRawMode() override { return true; }
    bool leaveRawMode() override { return true; }
    WriteStatus writeSome(const char* data, size_t size, size_t& offset) override;  // bytes to JS, offset = size
    bool writeAll(std::string& pending, int timeoutMs) override;
    bool supportsAsyncOutput() const override { return false; }
    bool setNonBlockingOutput(bool enabled) override { return !enabled; }
    void setWritableNotifications(bool) override {}
    QSize size() const override;                   // last size pushed from JS
    bool isInteractive() const override { return true; }
    void startInput() override; void stopInput() override;
    void deliverInput(const QByteArray&);           // bridge side, emits inputReceived()
    void deliverResize();                           // bridge side, emits resized()
};
```

The constructor defaults `TERM=xterm-256color` and `COLORTERM=truecolor`. No
host unit test: `EM_JS` and embind do not compile natively, and the class
mirrors a tested one. The browser run in Section 6 is the test.

### B. The JavaScript bridge

Output, C++ to JS, bytes, copied out of the heap before handing over:

```cpp
EM_JS(void, qamui_wasm_js_output, (const char* data, int length), {
    if (Module['qamuiOnOutput']) Module['qamuiOnOutput'](HEAPU8.slice(data, data + length));
});
```

Input and resize, JS to C++, embind (Qt already links it). A `Uint8Array`
arrives as raw `std::string` bytes, so NUL survives and there is no UTF-16
length confusion:

```cpp
EMSCRIPTEN_BINDINGS(qamui_wasm_bridge) {
    emscripten::function("qamuiFeedInput", &feedInput);   // (const std::string&)
    emscripten::function("qamuiResize", &resize);         // (int cols, int rows)
}
```

Qt's `exec()` does not return: `QEventDispatcherWasm` throws the JS value
`"unwind"` and keeps the C++ stack alive. The page calls the factory with
`noInitialRun`, pushes the size, then calls `callMain`, which treats `unwind`
as a normal return. `Qt.quit()` cannot unwind that stack without Asyncify and
ends in `abort()`; the page handles `onAbort` as "exited" and resets xterm.

### C. Build

Kit `~/Qt/6.11.1/wasm_singlethread` with host `gcc_64`, Emscripten exactly
4.0.7 (Qt's CMake aborts on any other). Single-threaded because GitHub Pages
cannot set the COOP/COEP headers threaded wasm needs.

```bash
source ~/emsdk/emsdk_env.sh
~/Qt/6.11.1/wasm_singlethread/bin/qt-cmake -S examples/wasm -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-wasm --parallel
cmake --build build-wasm --target serve
```

`cmake/QamuiWasm.cmake` (included by the root under `EMSCRIPTEN`) provides
`qamui_wasm_app(target)`, which sets `NO_WASM_DEFAULT_FILES` and a shared
output directory, and `qamui_wasm_add_demo_page()`. Qt's own finalizer already
sets `MODULARIZE`, `EXPORT_NAME=<target>_entry`, embind, the exported runtime
methods, and memory; the earlier snippet here would have clobbered them and
wrongly added `EXPORT_ES6`. Static QML plugins link automatically through the
`qt6_import_qml_plugins` finalizer. `examples/wasm/CMakeLists.txt` is the
wrapper project; it built the two games first and, since 2026-09-15, builds
only the gallery runtime and page (Section 4).

### D. The page

`examples/wasm/www/index.html` with `qamui-playground.js`: plain HTML and an
ES module, xterm.js 5.5 with the fit and grapheme addons from jsdelivr,
CodeMirror from esm.sh. The terminal wiring (`noInitialRun`, `qamuiResize`
before `callMain`, `TextEncoder` on `onData`, `onBinary`, `onResize`,
`onAbort`) is the reference for the website component. The first version was
a separate app-selector page with fixed game binaries; it went when the
gallery replaced it.

### E. The live QML runtime

Added 2026-09-14 after the games worked: a generic binary,
`examples/wasm/playground/main.cpp`, holds a bare `QQmlEngine` and exposes
`qamuiLoadQml(source, fileName)` and `qamuiSetContextProperty(name, value)`
through embind. Each edit re-instantiates the root and errors return as JSON
with line and column. The source it runs is the gallery (`gallery.qml`); the
editor shows the open tile's block of it (Section 4). Sharing is the deflated
whole file in the URL hash. `Qt.quit()` is caught on the engine's `quit` signal
and released with `deleteLater()`, so the runtime survives it. `Qamui.Image`
is opt-in (`QAMUI_WASM_PLAYGROUND_IMAGE`) because Qt Gui doubles the download.
The editor completes QML from `types.json`, generated at build time by
`scripts/qml_type_index.py` from the same qmltypes the reference docs use
(112 types); `qamui-completion.js` supplies cursor context (enclosing type,
ids, `Name.` access, value position). Real type inference would need the QML
language server; the wasm kit ships QmlDom and QmlCompiler but not QmlLS, so
that stays a later spike.
This is the shape of the public `/playground` route: the website component
should wrap `index.html` and `qamui-playground.js`.

---

## 4. Scope

As of 2026-09-15 the browser build is the component gallery and nothing else.
`examples/wasm` builds one runtime (`qamui_playground.wasm`, a bare
`QQmlEngine` behind `qamuiLoadQml`, `qamuiCheckQml` and
`qamuiSetContextProperty`) and one page, `index.html`. The gallery is a
launcher (`examples/apps/gallery/Main.qml`) plus 36 standalone example files
(`examples/apps/gallery/examples/*.qml`, each a complete app with imports and
a `Terminal` root). The launcher instantiates an example from its text after
swapping the root for an `Item`; the page shows the same text in CodeMirror
when a tile is open, compiles edits, and re-runs the gallery with the tile
open (`qamuiGalleryPage`). Escape returns to the grid and empties the editor.
The launcher announces the open file through its terminal title, which
xterm.js exposes as `onTitleChange`, so the app side needs no bridge code of
its own. `InlineMode.qml` (`mode: Terminal.Inline` at the root) makes the
launcher switch to inline mode, which also works in xterm.js.

Two engine facts shaped the launcher: `focus: true` inside a document created
while detached is not recorded (`Item::setFocus` returns early), so the
launcher gives focus explicitly after creation; and `destroy()` deletes at the
end of the event loop cycle, so a demo is reparented to null first or it keeps
painting under the next one.

Dropped on 2026-09-15: the fixed `snake_game` and `twozero48` binaries, the
app selector page, and the 37 single-file presets. Each was a second way to
show what the gallery already shows, and each cost a full download. Their
`qamui_wasm_app()` calls remain in the apps' CMake so any of them can be built
standalone again by adding it to a wrapper project.

Deferred: `btop` and every app that needs DBus, PTYs, processes, or the
filesystem.

Known issue found while driving the gallery with Playwright (headless Firefox,
2026-09-14): under rapid synthetic input the engine intermittently stops
committing frames (about 2 runs in 6; input is still processed, xterm's write
queue is empty, and the next bridge call such as a key or `qamuiResize`
releases the pending frame). Pumping posted events after each bridge call did
not help, and adding logging to `requestRender()` makes it vanish, so the
suspect is a timing race in Qt's WebAssembly timer multiplexing
(`QEventDispatcherWasm::updateNativeTimer`), not the renderer. Not seen with
human-speed typing; a JS-side heartbeat (periodic `qamuiResize` no-op or a
timer that asks the engine to repaint) would be the cheap workaround if it
shows up for users.

---

## 5. Deliverables

Done 2026-09-14:

1. `WasmTerminalBackend` on the Plan 00 seam; `cmake/QamuiWasm.cmake`; the
   static-build export fixes; `examples/wasm/` wrapper, apps page, and the
   live QML playground (binary, editor page, presets).
2. `docs/guide/webassembly.md` with exact versions, commands, and measured
   sizes; `TerminalBackend.md` updated.

Later, with Plan 01:

3. `/playground` route wrapping `index.html` from `examples/wasm/www` (the
   gallery with its editor), hidden until Section 6's public
   criteria pass.
4. Brotli-compressed artifacts under `website/static/wasm/<app>/`, built by a
   workflow that runs on release tags only, published on the release next to
   the source archive (Section 7).
5. Optional CI job: `jurplel/install-qt-action` with `target: wasm`,
   `arch: wasm_singlethread` plus the Linux host kit, `mymindstorm/setup-emsdk`
   at 4.0.7, configure and build only.

---

## 6. Acceptance

Local run, verified 2026-09-14 in headless Firefox 141 (Playwright) against
`python3 -m http.server`:

- Both apps load, `status` reaches "running", the console stays empty.
- Arrow keys steer snake and merge tiles in 2048; the frame changes after
  every key.
- Resizing the viewport refits xterm and the app re-lays out; no torn frame.
- `q` ends the app through `onAbort`; the page resets the terminal.
- Playground: the default preset renders, Enter drives the counter, an edit
  re-renders within a second, a syntax error reports `Main.qml:2:29`, a
  non-Terminal root is rejected with a message, a preset loads, `Qt.quit()`
  from a key or a timer returns the page to "edit or press Run" without
  aborting the runtime, and the console stays empty.
- Completion, eleven cursor contexts in headless Firefox: member after a
  partial word, `anchors.`, color and enum and bool values, `Keys.` handlers,
  `id.` members, `onClicked` for a Button, type names at top level and as
  children, `Timer.interval` from QtQml.
- Sizes: 8.3 MB raw, 1.83 MB Brotli, 2.66 MB gzip per game; the playground
  binary is 8.5 MB, 1.86 MB, 2.70 MB without `Qamui.Image` (15.3 MB and
  3.75 MB Brotli with it). The 6 MB budget holds with room to spare.
- Native suite unchanged: 93 of 94 on the Qt 6.11.1 tree, the one failure
  (`test_component_docs`, pages for modules that tree does not build) predates
  this work.

Public route, still to pass before it leaves hidden status:

- `btop` or another app loads and responds within three seconds on a cold
  cache over a 10 Mbps connection. Measured, not estimated.
- The notices page and the source offer (Section 7) are live.

---

## 7. Licensing and Notices

Qt for WebAssembly is offered under **GPLv3 or a commercial license only**;
there is no LGPL option (see the License section of the Qt for WebAssembly
page). WebAssembly builds link Qt statically, so every playground artifact is
a combined work with Qt and is distributed under GPLv3. This is the only place
where Qamui distributes a GPL binary; desktop users link Qt dynamically under
LGPLv3.

Qamui mirrors this rule for its own WebAssembly builds: GPLv3 or commercial,
no LGPL (Plan 00 section 8, open decision 3). Either way the playground binary
is GPLv3; the rule only matters for third parties shipping Qamui in the
browser.

### What the playground must carry

1. **A notices page**, linked from `/playground`, listing per shipped binary:
   Qt (GPLv3), Qamui (GPLv3 for this build), utf8proc (MIT), libvterm (MIT,
   only if `Qamui.TerminalHost` is linked into that app), and `xterm.js` (MIT).
2. **A corresponding-source offer.** Each `.wasm` is built from a release tag
   by the dedicated workflow in Section 5 and uploaded to that GitHub release
   next to the source archive. The notices page names the tag and the commit
   for every binary served. The repository must be public before the first
   upload.
3. **One line on the playground page**: "The demo binaries are GPLv3 because
   Qt for WebAssembly is GPLv3 or commercial only. The Qamui library is
   LGPLv3, GPLv3, or commercial." Without it a visitor reads the GPL notice and
   assumes it applies to the library.
4. **Build with the open-source Qt for WebAssembly kit.** The GPL grant only
   applies to binaries built from the GPL-licensed Qt packages. Do not build
   the public artifacts with a commercially licensed kit.

### Why this also argues for casts first

Casts (Plan 02) carry no Qt license obligation at all. The plan already makes
them the launch path; the GPL bookkeeping above is one more reason the
playground stays post-launch.
