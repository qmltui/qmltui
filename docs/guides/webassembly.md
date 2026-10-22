# QmlTui in the browser (Qt for WebAssembly)

A QmlTui app compiles to WebAssembly and runs inside [xterm.js](https://xtermjs.org/)
on a web page. Nothing in the app changes: the same `main.cpp`, the same QML.
What changes is the terminal backend, which talks to the page instead of a tty.

**License notes.** Qt for WebAssembly is distributed by The Qt Company under
the GPLv3 or a commercial Qt license, and wasm builds link Qt statically. While
QmlTui source code is licensed under `LGPL-3.0-only`, combining QmlTui with
open-source Qt for WebAssembly results in a combined binary governed by the
GPLv3 unless you hold a commercial license for Qt and separate commercial terms
for QmlTui. See [LICENSE.md](../../LICENSE.md). Desktop builds are not
affected; they link Qt dynamically under the LGPLv3.

## Prerequisites

| Requirement | Version used here | Notes |
| :--- | :--- | :--- |
| Qt for WebAssembly kit | 6.11.1 `wasm_singlethread` | Installed with the Qt installer next to a host kit of the same version (`gcc_64`), which provides `qmlimportscanner` and `qmlcachegen` |
| Emscripten | **exactly 4.0.7** | Each Qt minor pins one Emscripten version; Qt's CMake aborts on any other. 6.8 pairs with 3.1.56, 6.9 with 3.1.70 |
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

## Build and run the gallery page

`examples/wasm/` is a small wrapper project that builds the library, one
generic runtime (`playground/`), and the gallery page, and lays them out in one
directory:

```bash
source ~/emsdk/emsdk_env.sh
~/Qt/6.11.1/wasm_singlethread/bin/qt-cmake -S examples/wasm -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-wasm --parallel
cmake --build build-wasm --target serve       # http://localhost:8000/
```

`build-wasm/www/` then holds `qmltui_playground.js`, `qmltui_playground.wasm`,
`gallery.qml`, `types.json`, `index.html`, and the two page scripts. Any static
web server can serve it.

Measured on 2026-09-14 with Qt 6.11.1, Emscripten 4.0.7, `MinSizeRel`:

| File | Raw | Brotli (q11) | gzip (-9) |
| :--- | ---: | ---: | ---: |
| `qmltui_playground.wasm` | 8.5 MB | 1.86 MB | 2.7 MB |
| `qmltui_playground.js` | 0.16 MB | 0.04 MB | 0.04 MB |

Serve the `.wasm` with Brotli or gzip content encoding; both compress it by a
factor of four.

## The page: gallery plus live editor

`index.html` is the whole site. On the right, xterm.js runs the component
gallery launcher (`examples/gallery/Main.qml`, copied to
`www/gallery.qml`) with the gallery example files (`examples/gallery/examples/*.qml`,
copied to `www/examples/` with an `index.json`: `38` total across all 4 modules,
`37` in non-PTY builds such as WebAssembly, or `34` when `Behavior`, `Image`,
and `PtyView` are disabled). On the left, a CodeMirror
editor with QML completion starts out with a hint. Open a tile in the gallery
(arrows and Enter, or a click) and that tile's file appears in the editor: an
`Item`-rooted component instantiated dynamically by `Main.qml` via `Qt.createQmlObject`. Edit it: about 400 ms after the last keystroke the file is
compiled (`qmltuiCheckQml`; errors show up with line and column and the old
version keeps running), then handed back and the gallery re-runs with the
same tile open. Escape in the gallery returns to the grid and empties the
editor. "launcher" edits `gallery.qml` itself, "Share" puts every source into
the URL hash, and "hide editor" gives the terminal the whole width (the app
is resized live). Tab indents inside the editor; Escape then Tab leaves it,
as CodeMirror documents.

The link between the two panes needs nothing from the page beyond xterm.js:

- App to page: the launcher sets `Terminal.title` to `QmlTui Gallery · <file>`
  while a tile is open. Title changes are written as an OSC sequence at once,
  and xterm.js raises `onTitleChange`. `qmltui-playground.js` looks for
  `· <name>` at the end of the title; the convention is generic, any app can
  use it.
- Page to app: on startup (and after edits), the page passes the example files
  via `qmltuiWriteFile("examples/" + file, code)`, which populates the
  `qmltuiExampleSources` context map read by `Main.qml`, sets
  `qmltuiSetContextProperty("qmltuiAvailableExamples", jsonArray)` and
  `qmltuiSetContextProperty("qmltuiGalleryPage", file)` (`""` for the grid),
  and loads `gallery.qml`.
- Inside `Main.qml`, the active component source is looked up from
  `qmltuiExampleSources[current.file]` (or embedded resources on desktop) and
  instantiated into the preview stage with `Qt.createQmlObject`.
- `Image.source` with an `http(s)` URL works in the browser too: Qt Network
  goes through the browser's `fetch`, so the picture's server must allow the
  page's origin (CORS). The gallery's own picture and sprite sheet stay
  resources for that reason.

How the runtime differs from an app binary (`examples/wasm/playground/main.cpp`):

- `main()` creates a plain `QQmlEngine` and nothing else. The page calls the
  embind function `qmltuiLoadQml(source, fileName)`, which deletes the previous
  root, clears the component cache, and creates the new root from a
  `QQmlComponent`. It
  returns `""` or a JSON array of `{line, column, message, file}`. The root
  must be a `Terminal`; anything else is reported. `qmltuiCheckQml(source,
  fileName)` compiles without creating and returns the same shape, and
  `qmltuiSetContextProperty(name, value)` sets a string context property for
  roots created afterwards.
- A generated `Imports.qml` that is never instantiated imports `QtQml`,
  `QtQml.Models`, `QmlTui`, and `QmlTui.Controls`, so `qmlimportscanner` links
  every plugin the editor may need. `QmlTui.Image` (and with it Qt Gui and Qt
  Network) is on in the gallery build because of its Image and AnimatedSprite
  tiles; `-DQMLTUI_WASM_PLAYGROUND_IMAGE=OFF` drops it and both tiles for a lighter
  runtime: 8.5 MB raw and 1.86 MB Brotli without it, about twice that with.
- `Qt.quit()` does not abort. With our own `QQmlEngine` the `quit` and `exit`
  signals are ours: the root is released with `deleteLater()` (quit arrives
  from inside one of the root's own handlers, and a synchronous delete there
  aborts) and the page is told through `Module.qmltuiOnQuit`. `qmltuiLoadQml`
  flushes the pending deletion before creating the next root so the old
  Terminal's teardown never lands on top of the new frame.
- QML warnings reach the page through `Module.qmltuiOnWarning` instead of stderr.
- CodeMirror comes from esm.sh through an import map in `index.html` that
  pins one exact version per package and uses esm.sh's external mode (the `*`
  prefix). Without the map two copies of a CodeMirror package load and their
  extensions silently stop composing. Bump all versions together.
- **Completion.** `examples/wasm/www/types.json` contains every exported type of
  `QmlTui`, `QmlTui.Controls`, `QML`, `QtQml`, and `QtQml.Models` with what it
  inherits and its properties, signals, methods, enums, and attached members.
  `qmltui-completion.js` adds cursor context with a brace
  scan, and offers the enclosing type's members and `on<Signal>` handlers at
  object scope, members after `id.` or `property.`, attached handlers after
  `Keys.` or `Layout.`, and enum values, colors, booleans, and ids after
  `property:`. It does not infer types through expressions; that would need
  the QML language server, whose libraries the wasm kit only partly ships.
- The base URL of the loaded source is `file:///playground/<fileName>`, which
  is Emscripten's in-memory filesystem. A page can write sibling files there
  with `instance.FS.writeFile` so relative imports resolve. The page does not
  do this yet.

Any app can still be built as its own fixed wasm binary: call
`qmltui_wasm_app(<target>)` in its `CMakeLists.txt` (the games and the gallery
already do) and add it to a wrapper project like `examples/wasm`. The wiring
for a fixed binary is `noInitialRun`, `qmltuiResize` before `callMain`,
`TextEncoder` on xterm's `onData`, and `onAbort` treated as exit; see
`startRuntime()` in `qmltui-playground.js`.

## How it works

`WasmTerminalBackend` (`src/QmlTui/Core/Primitives/WasmTerminalBackend.cpp`) is
the third implementation of `TerminalBackend`, selected by
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
runs. The gallery page does that for its runtime.

## Embedding in your own page

`startRuntime()` in `examples/wasm/www/qmltui-playground.js` is the reference.
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
add_subdirectory(path/to/QmlTui)          # or FetchContent, as examples/quickstart does
qt_add_executable(my_app main.cpp)
qt_add_qml_module(my_app URI MyApp VERSION 1.0 QML_FILES Main.qml DEPENDENCIES TARGET qmltui::core)
target_link_libraries(my_app PRIVATE Qt6::Core Qt6::Qml qmltui::core)
if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(my_app)               # no Qt shell page, output under ${QMLTUI_WASM_WWW_DIR}
endif()
```

`qmltui_wasm_app()` comes from `cmake/QmlTuiWasm.cmake`, which the QmlTui root
includes automatically under Emscripten. Static Qt handles the QML plugins:
`qmlimportscanner` finds the `QmlTui` module in the build tree and links
`QmlTuiCoreplugin` into the executable. Do not pass `-sMODULARIZE`,
`-sEXPORT_NAME`, or `-sEXPORTED_RUNTIME_METHODS` yourself; Qt's finalizer
sets them. Do not add `-sASYNCIFY`; it is not needed.

Force these off in a wasm configure: `QMLTUI_BUILD_TESTS`, `QMLTUI_BUILD_BENCHMARKS`,
`QMLTUI_ENABLE_RUNNER` (TestRunner owns a `std::thread`), and
`QMLTUI_ENABLE_PTY_VIEW` (PTYs). `QMLTUI_ENABLE_PRIVATE_QT` works when the kit
ships `Qt6QmlPrivate`, which the official 6.11 kit does (pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` otherwise).

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
