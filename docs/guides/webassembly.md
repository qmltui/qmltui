# QmlTui in the browser (Qt for WebAssembly)

A QmlTui app compiles to WebAssembly and runs inside [xterm.js](https://xtermjs.org/)
on a web page. Nothing in the app changes: the same `main.cpp`, the same QML.
What changes is the terminal backend, which talks to the page instead of a tty.

**License notes.** Qt for WebAssembly is distributed by The Qt Company under
the GPLv3 or a commercial Qt license, and wasm builds link Qt statically. While
QmlTui source code is licensed under the LGPLv3, combining QmlTui with
open-source Qt for WebAssembly results in a combined binary governed by the
GPLv3 unless you hold a commercial license for Qt and separate commercial terms
for QmlTui. See [LICENSE.md](../../LICENSE.md). Desktop builds are not
affected; they link Qt dynamically under the LGPLv3.

## Prerequisites

| Requirement | Version used here | Notes |
| :--- | :--- | :--- |
| Qt for WebAssembly kit | 6.11.1 or 6.12.0 `wasm_singlethread` | Installed with the Qt installer next to a host kit of the same version (`gcc_64`), which provides `qmlimportscanner` and `qmlcachegen` (or pass `-DQT_HOST_PATH=/usr` if host Qt is installed system-wide) |
| Emscripten | **4.0.7** (Qt 6.11) / **5.0.5** (Qt 6.12) | Each Qt minor pins one Emscripten version; Qt's CMake aborts on any other. 6.8 pairs with 3.1.56, 6.9 with 3.1.70, 6.11 with 4.0.7, 6.12 with 5.0.5 |
| Python 3 | any | Only for the `serve` target |

Install Emscripten once:

```bash
git clone https://github.com/emscripten-core/emsdk ~/emsdk
~/emsdk/emsdk install 4.0.7 && ~/emsdk/emsdk activate 4.0.7
source ~/emsdk/emsdk_env.sh      # every new shell; exports EMSDK and puts emcc on PATH
```

Single-threaded, not multithreaded: threaded wasm needs `SharedArrayBuffer`,
which browsers only enable behind COOP/COEP response headers that static hosts
such as GitHub Pages cannot set. QmlTui does not use threads anyway.

## Build and run the playground page

`examples/wasm/` builds the library and the WebAssembly playground runtime
(`examples/wasm/playground/`, target `qmltui_playground`), staging the HTML/JS
host page, QML type metadata (`types.json`), and gallery examples into one
directory:

```bash
source ~/emsdk/emsdk_env.sh
~/Qt/6.11.1/wasm_singlethread/bin/qt-cmake -S examples/wasm -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-wasm --parallel
cmake --build build-wasm --target serve       # http://localhost:8000/
```

`build-wasm/www/` then holds `qmltui_playground.js`, `qmltui_playground.wasm`,
`index.html`, `qmltui-playground.js`, `qmltui-completion.js`, `types.json`,
`gallery.qml`, and `examples/*.qml`. Any static web server can serve it.

Serve the `.wasm` with Brotli or gzip content encoding; both compress it by a
factor of four. Pass `-DQMLTUI_WASM_PLAYGROUND_IMAGE=OFF` at configure time to
omit `QmlTui.Image` (and with it Qt Gui and Qt Network) for a smaller binary.

## The page: CodeMirror 6 editor on the left, `xterm.js` terminal on the right

`examples/wasm/www/index.html` and `qmltui-playground.js` split the browser
viewport between a native CodeMirror 6 QML editor on the left and a live
`xterm.js` terminal (`scrollback: 1000`) on the right:

- **▶ Try It Yourself (`Main.qml` with root `Terminal {}`)**: By default,
  `/playground` opens with a complete, editable `Main.qml` application rooted at
  `Terminal { ... }`. Because the user's `Terminal` is loaded as the actual root
  object of `TerminalEngine` via `instance.qmltuiLoadQml(code, 'Main.qml')`,
  both `Terminal.AltScreen` and `Terminal.Inline` (including `Scrollback`
  into `xterm.js` scrollback) work natively. Use the template dropdown to switch
  between `Main.qml (AltScreen App)` and `Main.qml (Inline & Scrollback)`.
- **▦ Gallery Grid (`?mode=gallery` or `?example=<Name>`)**: Click `▦ Gallery Grid`
  (or open `/playground?mode=gallery`) to browse all 44 browser-supported interactive component
  examples in the terminal grid. Opening a tile in the terminal (or choosing one
  from the dropdown) loads `examples/<Name>.qml` into the CodeMirror editor on
  the left for live editing.
- **▶ Try in Playground from Documentation**: Every ` ```qml ` code block in the
  documentation portal includes a **▶ Try in Playground** button. Clicking it
  opens `/playground?code=<base64url>` with the snippet loaded directly in the
  left editor (snippets without a root `Terminal` are automatically wrapped in a
  runnable `Terminal {}` shell).
- **Borderless QML Autocomplete**: `qmltui-completion.js` reads `types.json` to
  provide context-aware QML component, property, enum, and `Layout.`/`Keys.`
  completions in a flat, borderless popup.
- **Clipboard Copy (OSC 52)**: When QmlTui's `Clipboard.copy()` emits an OSC 52
  sequence (`\x1b]52;c;<base64>\x07`), `qmltui-playground.js` decodes the
  payload and forwards it to `navigator.clipboard.writeText(...)`.
- **Image Loading**: `Image.source` with an `http(s)` URL works in the browser
  too: Qt Network goes through the browser's `fetch`, so the image server must
  allow the page's origin (CORS). The gallery's own picture and sprite sheet
  stay embedded in Qt resources for that reason.

Any QmlTui application can also be built and run as a standalone WebAssembly
app—without the playground wrapper—simply by calling `qmltui_wasm_app(<target>)`
in its `CMakeLists.txt` (see [Building your own app for the browser](#building-your-own-app-for-the-browser)).

## How it works

`WasmTerminalBackend` (`src/QmlTui/Core/Primitives/WasmTerminalBackend.cpp`) is
the browser implementation of `TerminalBackend` (alongside `PosixTerminalBackend`,
`Win32ConsoleBackend`, and `MemoryTerminalBackend`), selected by
`createTerminalBackend()` when compiling under Emscripten. It is about as long
as the in-memory test backend and has three touch points with the page:

| Direction | Hook | Payload |
| :--- | :--- | :--- |
| App to page | `Module.qmltuiOnOutput(bytes)` | A `Uint8Array` copy of each output chunk. Bytes, not strings: a chunk may end in the middle of a UTF-8 sequence, and xterm.js reassembles bytes across `write()` calls |
| Page to app | `instance.qmltuiFeedInput(bytes)` | A `Uint8Array` of key bytes, delivered through `inputReceived()`. Encode xterm's `onData` strings with `TextEncoder`; a NUL byte (Ctrl+Space) survives, a JS string length would not |
| Page to app | `instance.qmltuiResize(cols, rows)` | Stored in the backend and re-emitted as `resized()`. Call it once before `callMain` so the first frame has the right geometry |

The two entry points are embind functions, which Qt already links into every
wasm executable. Output is synchronous (`supportsAsyncOutput()` is false), so
`TerminalDevice` never queues and the engine's backpressure path is idle.

Two things the backend does that the page would otherwise have to:

- It sets `TERM=xterm-256color` and `COLORTERM=truecolor` if they are unset.
  The engine detects capabilities from the environment, and Emscripten's
  environment is empty.
- It reports the terminal as interactive, so the engine sends its usual probe
  burst (`CSI ? u`, `CSI 16 t`, `CSI ? 2026 $ p`, `CSI c`). xterm.js answers
  the Primary Device Attributes query, which ends negotiation immediately; the
  other replies never come and the 100 ms timer handles that.

### Start-up and shutdown

Qt for WebAssembly's `exec()` cannot block the browser's main thread, so it
hands control back to the browser by throwing the JS value `"unwind"` while
keeping the C++ stack alive. `main()` therefore never finishes, and the app
runs from the browser event loop. The page calls the module factory with
`noInitialRun: true`, pushes the terminal size, and then calls `callMain()`
itself; Emscripten's `callMain` treats `"unwind"` as a normal return.

`Qt.quit()` is the mirror image: Qt cannot unwind the preserved `exec()` stack
without Asyncify, prints "Terminating since we don't have asyncify", and
aborts the runtime. That is the normal way a QmlTui wasm app ends. The page must
handle `Module.onAbort` as "exited" and call `term.reset()` itself, because the
app's own shutdown sequence (leave the alternate screen, show the cursor) never
runs. `examples/wasm/www/index.html` does that and offers a button to restart
the gallery.

## Embedding in your own page

`boot()` in `examples/wasm/www/index.html` is the reference.
The load-bearing part, for an app built with `qmltui_wasm_app(qmltui_<app>)`:

```js
const factory = window[`qmltui_${app}_entry`];       // Qt names the factory <target>_entry
const instance = await factory({
  noInitialRun: true,
  qmltuiOnOutput: (bytes) => term.write(bytes),
  print: (t) => console.log(t), printErr: (t) => console.warn(t),   // qDebug goes to the console
  onExit: () => finished(), onAbort: () => finished(),
});
instance.qmltuiResize(term.cols, term.rows);
term.onData((s) => instance.qmltuiFeedInput(new TextEncoder().encode(s)));
term.onBinary((s) => instance.qmltuiFeedInput(Uint8Array.from(s, (c) => c.charCodeAt(0))));
term.onResize(({ cols, rows }) => instance.qmltuiResize(cols, rows));
try { instance.callMain([]); } catch (e) { if (e !== 'unwind') throw e; }
```

**Cell widths must match.** QmlTui measures text as grapheme clusters and
gives emoji, regional-indicator flags, and keycap sequences two columns, as
modern terminals do. xterm.js defaults to Unicode 6 tables where an emoji is
one column, so every row holding one shifts and the glyph is clipped. Load
`@xterm/addon-unicode-graphemes` and set `term.unicode.activeVersion =
'15-graphemes'` (needs `allowProposedApi: true`); its rules are the ones
QmlTui's `UnicodeText` applies. The Unicode 11 addon is not enough: it leaves
keycaps and variation-selector emoji one column wide.

Rules: one live app per page (the bridge forwards input to the most recently
created backend); pass bytes in both directions; set `qmltuiOnOutput` in the
factory options, not afterwards, because the first frame is written during
`callMain`; the module factory name is the CMake target name with `_entry`
appended (set the `QT_WASM_EXPORT_NAME` target property to change it).

## Building your own app for the browser

Configure with the kit's `qt-cmake`, then in your project:

```cmake
add_subdirectory(path/to/QmlTui)          # or find_package(QmlTui REQUIRED) / FetchContent
qt_add_executable(my_app main.cpp)
qt_add_qml_module(my_app URI MyApp VERSION 1.0 QML_FILES Main.qml DEPENDENCIES TARGET qmltui::core)
target_link_libraries(my_app PRIVATE Qt6::Core Qt6::Qml qmltui::core)
if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(my_app)               # replaces Qt's canvas shell with an xterm.js <target>.html page
endif()
```

### `qmltui_wasm_app(<target> [OUTPUT_DIRECTORY <dir>])`

`qmltui_wasm_app()` is defined in `cmake/QmlTuiWasm.cmake`, which both the
QmlTui root and `find_package(QmlTui)` include automatically when `EMSCRIPTEN`
is true. Call it in the same directory scope that creates `<target>` so Qt's
deferred executable finalizer sees the target properties. It performs four steps:

1. **Disables Qt's GUI shell**: Sets `NO_WASM_DEFAULT_FILES ON` on `<target>` so
   Qt does not generate its `<canvas>`-based `wasm_shell.html`, `qtloader.js`,
   or `qtlogo.svg`.
2. **Applies output directory and memory overrides**:
   - `OUTPUT_DIRECTORY <dir>` (optional): sets `RUNTIME_OUTPUT_DIRECTORY` on
     `<target>`. When omitted, `qmltui_wasm_app()` uses the target's existing
     `RUNTIME_OUTPUT_DIRECTORY` or falls back to `${CMAKE_CURRENT_BINARY_DIR}`
     (and copies `<target>.html` into `$<CONFIG>/` for multi-config generators).
   - `QMLTUI_WASM_INITIAL_MEMORY` (CMake cache string): when non-empty, sets
     `QT_WASM_INITIAL_MEMORY` on `<target>` (for example `-DQMLTUI_WASM_INITIAL_MEMORY=96MB`).
3. **Resolves target naming**: Respects the target's `OUTPUT_NAME` property for
   the generated `.html`/`.js` filenames and `QT_WASM_EXPORT_NAME` (or Qt's
   `<sanitized_target>_entry` convention) for the JavaScript module factory.
4. **Generates `<target>.html` next to `<target>.js` and `<target>.wasm`**:
   Writes a self-contained `xterm.js` host page that:
   - Loads `xterm.js`, `@xterm/addon-fit`, and `@xterm/addon-unicode-graphemes`
     (`15-graphemes`), matching QmlTui's `UnicodeText` grapheme widths.
   - Connects `qmltuiOnOutput`, `qmltuiFeedInput`, and `qmltuiResize`.
   - Replies to QmlTui's OSC 4/10/11 palette queries and handles OSC 52
     clipboard copy (`navigator.clipboard.writeText`).
   - Forwards repeatable `?arg=...` URL query parameters as `argc`/`argv` to
     `callMain(args)` (for example `my_app.html?arg=--help`).
   - Handles `Qt.quit()` (`onAbort` / `onExit`) by resetting the terminal and
     offering a restart action.

Because `<target>.html` is written directly alongside `<target>.js` and
`<target>.wasm`, pressing **Run** in Qt Creator (which invokes `emrun` on the
target's output path) or serving the build directory with `emrun` /
`python3 -m http.server` works out of the box.

Static Qt handles the QML plugins:
`qmlimportscanner` finds the `QmlTui` module in the build tree and links
`QmlTuiCoreplugin` into the executable. Do not pass `-sMODULARIZE`,
`-sEXPORT_NAME`, or `-sEXPORTED_RUNTIME_METHODS` yourself; Qt's finalizer
sets them. Do not add `-sASYNCIFY`; it is not needed.

Force these off in a wasm configure: `QMLTUI_BUILD_TESTS`, `QMLTUI_BUILD_BENCHMARKS`,
`QMLTUI_ENABLE_RUNNER` (`Runner` owns a `std::thread`), and
`QMLTUI_ENABLE_PTYVIEW` (PTYs). `QMLTUI_ENABLE_PRIVATE_QT` automatically uses
`Qt6QmlPrivate` when the kit ships it (which the official 6.11 kit does) and
falls back with a warning otherwise (pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` to opt out).

What cannot work in the browser: `QProcess`, `/proc`, D-Bus, sockets other than
WebSockets, `QmlTui.PtyView`, and anything reading host files. `QFile`
sees Emscripten's in-memory filesystem, so it does not crash, it just finds
nothing.

## Troubleshooting

| Symptom | Cause and fix |
| :--- | :--- |
| Configure error mentioning the Emscripten version | The kit pins one version. `source emsdk_env.sh` with exactly that version active |
| `install(EXPORT "QmlTuiTargets" ...) includes target ... not in any export set` | A static-build dependency is missing from the export set. QmlTui handles utf8proc and the QML module object libraries; if you add a private static dependency to Core, install it into `QmlTuiTargets` too |
| `-std=c99 not allowed with C++` while building utf8proc | Automoc leaked into a C target; QmlTui disables it on utf8proc. Check that your project does not force `AUTOMOC` on fetched targets |
| Blank terminal, console says `module "QmlTui" is not installed` | The QML plugin did not link. Check the link line for `libQmlTuiCoreplugin.a`; as a fallback add `Q_IMPORT_QML_PLUGIN(QmlTuiPlugin)` to `main.cpp` |
| Blank terminal, no error | `qmltuiOnOutput` was set after the factory call, or `qmltuiResize` was not called before `callMain` |
| Console: `Terminating since we don't have asyncify` then `Aborted()` | The app called `Qt.quit()`. Expected; handle `onAbort` |
| Out of memory at start | Set `-DQMLTUI_WASM_INITIAL_MEMORY=96MB` (Qt's default is 50 MB; growth is enabled) |
| Browser warns about the `.wasm` MIME type | Your server sends `application/octet-stream`; Emscripten falls back to a slower compile. Serve `.wasm` as `application/wasm` |

Debug builds add `--profiling-funcs` for readable stack traces; add
`-sASSERTIONS=1` to `target_link_options` for Emscripten's own diagnostics.
