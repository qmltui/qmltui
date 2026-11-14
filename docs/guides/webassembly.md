# QmlTui in the Browser (Qt for WebAssembly)

A QmlTui application compiles to WebAssembly and runs inside [xterm.js](https://xtermjs.org/) on a static web page. Your application's `main.cpp` and `.qml` files stay unchanged: when compiled under Emscripten, `createTerminalBackend()` selects `WasmTerminalBackend` instead of a POSIX TTY or Windows console backend.

**Licensing note**: Qt for WebAssembly is distributed by The Qt Company under the GPLv3 or a commercial Qt license, and WebAssembly builds link Qt statically. While QmlTui source code is licensed under the LGPLv3, combining QmlTui with open-source Qt for WebAssembly produces a combined binary governed by the GPLv3 unless you hold commercial licenses for both Qt and QmlTui. See [LICENSE.md](../../LICENSE.md). Desktop builds link Qt dynamically under the LGPLv3 and are unaffected.

## Prerequisites

| Requirement | Supported Version | Notes |
| :--- | :--- | :--- |
| Qt for WebAssembly kit | 6.8+ `wasm_singlethread` (6.11.1 or 6.12.0 recommended) | Installed alongside a matching desktop host kit (`gcc_64`), which provides `qmlimportscanner` and `qmlcachegen` (or pass `-DQT_HOST_PATH=/usr` if host Qt is installed system-wide) |
| Emscripten (`emsdk`) | **4.0.7** (Qt 6.11) / **5.0.5** (Qt 6.12) | Each Qt minor release pins one Emscripten version (Qt 6.8 pairs with `3.1.56`, Qt 6.9 with `3.1.70`, Qt 6.11 with `4.0.7`, Qt 6.12 with `5.0.5`) |
| Python 3 | any | Optional local HTTP server (`python3 -m http.server`) |

Install and activate the Emscripten version required by your Qt kit:

```bash
git clone https://github.com/emscripten-core/emsdk ~/emsdk
~/emsdk/emsdk install 4.0.7 && ~/emsdk/emsdk activate 4.0.7
source ~/emsdk/emsdk_env.sh
```

Use Qt's **single-threaded** kit (`wasm_singlethread`), not `wasm_multithread`. Multi-threaded WebAssembly requires `SharedArrayBuffer` behind `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` response headers that static hosts such as GitHub Pages cannot set, and QmlTui's rendering engine runs on a single thread.

## Building Your Application for the Browser

Both the QmlTui build tree and `find_package(QmlTui)` automatically include `cmake/QmlTuiWasm.cmake` when `EMSCRIPTEN` is true, providing `qmltui_wasm_app()`. In your `CMakeLists.txt`, call `qmltui_wasm_app(<target>)` in the same directory scope that creates `<target>`:

```cmake
qt_add_executable(my_app main.cpp)
qt_add_qml_module(my_app
    URI MyApp
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)
target_link_libraries(my_app PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(my_app)
endif()
```

Configure with your WebAssembly kit's `qt-cmake` wrapper and build (for example, using [`examples/helloworld/`](../../examples/helloworld/)):

```bash
source ~/emsdk/emsdk_env.sh
~/Qt/6.11.1/wasm_singlethread/bin/qt-cmake -S examples/helloworld -B build-wasm -G Ninja \
  -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-wasm --parallel
python3 -m http.server -d build-wasm 8000
```

Open `http://localhost:8000/hello_qmltui.html` in a browser. Serve `.wasm` assets with Brotli or gzip compression in production (both reduce download size by roughly 4x).

### `qmltui_wasm_app(<target> [OUTPUT_DIRECTORY <dir>])`

`qmltui_wasm_app()` configures `<target>` for terminal-in-browser execution in four steps:

1. **Disables Qt's GUI `<canvas>` shell**: Sets `NO_WASM_DEFAULT_FILES ON` on `<target>` so Qt omits its graphical `wasm_shell.html`, `qtloader.js`, and `qtlogo.svg`.
2. **Applies output directory and initial memory settings**:
   - `OUTPUT_DIRECTORY <dir>` (optional): sets `RUNTIME_OUTPUT_DIRECTORY` on `<target>` (defaulting to the target's existing `RUNTIME_OUTPUT_DIRECTORY` or `${CMAKE_CURRENT_BINARY_DIR}`, and copying `<target>.html` into `$<CONFIG>/` on multi-config generators).
   - `QMLTUI_WASM_INITIAL_MEMORY` (CMake cache string): when non-empty, sets `QT_WASM_INITIAL_MEMORY` on `<target>` (for example `-DQMLTUI_WASM_INITIAL_MEMORY=96MB`).
3. **Resolves target and module factory names**: Respects the target's `OUTPUT_NAME` property for `.html`/`.js` filenames and `QT_WASM_EXPORT_NAME` (or Qt's `<sanitized_target>_entry` convention) for the JavaScript module factory.
4. **Generates `<target>.html` alongside `<target>.js` and `<target>.wasm`**: Writes a self-contained `xterm.js` host page that:
   - Loads `xterm.js`, `@xterm/addon-fit`, and `@xterm/addon-unicode-graphemes` (`15-graphemes`) so emoji and grapheme cluster widths match `UnicodeText`.
   - Connects `qmltuiOnOutput`, `qmltuiFeedInput`, and `qmltuiResize`.
   - Replies to QmlTui's OSC 4/10/11 palette queries and forwards OSC 52 clipboard writes (`Clipboard.copy()`) to `navigator.clipboard.writeText()`.
   - Forwards repeatable `?arg=...` URL query parameters as `argc`/`argv` to `callMain(args)` (for example `my_app.html?arg=--help`).
   - Handles `Qt.quit()` (`onAbort` / `onExit`) by resetting the terminal and offering a restart action.

Do not pass `-sMODULARIZE`, `-sEXPORT_NAME`, `-sEXPORTED_RUNTIME_METHODS`, or `-sASYNCIFY` manually; Qt's WebAssembly finalizer sets the required modularization flags, and Asyncify is not needed.

When `EMSCRIPTEN` is active, QmlTui automatically defaults `QMLTUI_BUILD_TESTS`, `QMLTUI_BUILD_BENCHMARKS`, and `QMLTUI_ENABLE_RUNNER` to `OFF`. `QMLTUI_ENABLE_PRIVATE_QT` uses `Qt6::QmlPrivate` when the kit ships it (enabling `Behavior`) and falls back with a warning otherwise.

## Embedding in a Custom Web Page

If you embed `<target>.js` inside your own web application instead of using the generated `<target>.html`, wire `WasmTerminalBackend`'s three JavaScript hooks to an `xterm.js` instance:

| Direction | Hook | Payload |
| :--- | :--- | :--- |
| App to page | `Module.qmltuiOnOutput(bytes)` | `Uint8Array` copy of each ANSI output chunk. Pass raw bytes directly to `term.write(bytes)` so UTF-8 sequences split across chunks reassemble cleanly. |
| Page to app | `instance.qmltuiFeedInput(bytes)` | `Uint8Array` of input bytes delivered to `InputDecoder`. Encode `term.onData` strings with `TextEncoder` so NUL bytes (`Ctrl+Space`) survive intact. |
| Page to app | `instance.qmltuiResize(cols, rows)` | Stores viewport dimensions and emits `resized()`. Call once before `callMain()` so the initial frame renders at the right size. |

Minimal initialization sequence for a target built as `my_app`:

```js
const factory = window.my_app_entry; // Qt names the factory <target>_entry
const instance = await factory({
  noInitialRun: true,
  qmltuiOnOutput: (bytes) => term.write(bytes),
  print: (t) => console.log(t),
  printErr: (t) => console.warn(t),
  onExit: () => onFinished(),
  onAbort: () => onFinished(),
});
instance.qmltuiResize(term.cols, term.rows);
term.onData((s) => instance.qmltuiFeedInput(new TextEncoder().encode(s)));
term.onBinary((s) => instance.qmltuiFeedInput(Uint8Array.from(s, (c) => c.charCodeAt(0))));
term.onResize(({ cols, rows }) => instance.qmltuiResize(cols, rows));
try {
  instance.callMain([]);
} catch (e) {
  if (e !== 'unwind') throw e;
}
```

### Startup, Shutdown, and Cell Widths

- **Grapheme widths must match**: `xterm.js` defaults to Unicode 6 width tables where many emoji occupy 1 column instead of 2. Load `@xterm/addon-unicode-graphemes` (`allowProposedApi: true`) and set `term.unicode.activeVersion = '15-graphemes'` so `xterm.js` matches QmlTui's `UnicodeText` grapheme segmentation.
- **Event-loop unwind on startup**: Qt for WebAssembly's `QCoreApplication::exec()` yields back to the browser event loop by throwing `"unwind"` while keeping the C++ stack live. Pass `noInitialRun: true` to the module factory, set initial terminal dimensions, and invoke `callMain([])` yourself.
- **Clean reset on `Qt.quit()`**: Without Asyncify, `Qt.quit()` terminates the WebAssembly instance via `onAbort`. Treat `onAbort` as normal application exit and call `term.reset()` on the `xterm.js` instance.
- **Environment defaults**: `WasmTerminalBackend` sets `TERM=xterm-256color` and `COLORTERM=truecolor` when unset and reports the session as interactive so capability negotiation runs normally against `xterm.js`.

## Browser Constraints

Host OS facilities are unavailable inside the browser sandbox: `QProcess`, `/proc`, D-Bus, raw TCP/UDP sockets (use WebSockets or `QNetworkAccessManager` over Fetch/CORS), and `QmlTui.PtyView`. `QFile` reads Emscripten's virtual in-memory filesystem and embedded Qt resources (`qrc:/...`), not the visitor's local disk.

## Troubleshooting

| Symptom | Cause and Fix |
| :--- | :--- |
| Configure error mentioning the Emscripten version | Your Qt kit pins a specific `emcc` version. Activate that exact version in `emsdk` and `source emsdk_env.sh`. |
| `-std=c99 not allowed with C++` while building `utf8proc` | `CMAKE_AUTOMOC` leaked into a C target. Avoid setting `CMAKE_AUTOMOC ON` globally before `FetchContent_MakeAvailable(QmlTui)`. |
| Blank terminal; console says `module "QmlTui" is not installed` | Static QML plugin was not linked. Verify `DEPENDENCIES TARGET qmltui::core` on `qt_add_qml_module()` so `qmlimportscanner` links `QmlTuiCoreplugin`. |
| Blank terminal with no console error | `qmltuiOnOutput` was attached after calling the module factory, or `qmltuiResize` was not called before `callMain`. |
| Console logs `Terminating since we don't have asyncify` then `Aborted()` | Normal behavior when the app calls `Qt.quit()`. Handle `onAbort` in your host page and reset `xterm.js`. |
| Out of memory on startup | Pass `-DQMLTUI_WASM_INITIAL_MEMORY=96MB` at configure time (growth remains enabled). |
| Browser warns about `.wasm` MIME type | Configure your HTTP server to serve `.wasm` files as `application/wasm` for streaming compilation. |
