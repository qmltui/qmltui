# Build Configuration and Installation

This reference covers QmlTui module targets, consuming an installed package with `find_package(QmlTui)`, `FetchContent` rules, CMake options, and runtime environment variables. To build your first standalone application, start with [Getting Started](getting-started.md).

## QML Modules and CMake Targets

`QmlTui::Core` and `QmlTui::Controls` require only `Qt6::Core` and `Qt6::Qml`. Use CMake cache options to enable optional modules and tooling:

| QML Import / Tool | CMake Target | CMake Option & Default | Dependencies | Guide & Reference |
| :--- | :--- | :--- | :--- | :--- |
| `import QmlTui` | `QmlTui::Core` | Always built (`QMLTUI_ENABLE_PRIVATE_QT=ON` auto-enables `Behavior` when `Qt6::QmlPrivate` is present) | `Qt6::Core`, `Qt6::Qml`, `utf8proc` | [Architecture](guides/architecture.md), [Animation & States](guides/animation-and-states.md) |
| `import QmlTui.Controls` | `QmlTui::Controls` | Always built | `QmlTui::Core` | [QML Types](qml-types/index.md) |
| `import QmlTui.Image` | `QmlTui::Image` | `-DQMLTUI_ENABLE_IMAGE=ON` (default `OFF`) | `QmlTui::Core`, `Qt6::Gui`, `Qt6::Network` | [Image](qml-types/image.md), [AnimatedSprite](qml-types/animatedsprite.md) |
| `import QmlTui.PtyView` | `QmlTui::PtyView` | `-DQMLTUI_ENABLE_PTYVIEW=ON` (default `OFF`, Linux/macOS) | `QmlTui::Core`, `libvterm` | [Embedded PTY Shells](guides/pty-view.md), [PtyView](qml-types/ptyview.md) |
| `Runner` & `qmltui-runner` | `QmlTui::Runner` | `-DQMLTUI_ENABLE_RUNNER=ON` (default `ON` top-level or when `BUILD_TESTING=ON`) | `QmlTui::Core` | [Testing with Runner](guides/testing.md), [Runner](cpp-classes/runner.md) |
| `qmltui_wasm_app(<target>)` | *(CMake helper)* | Automatic under Emscripten (`EMSCRIPTEN`) | Qt for WebAssembly `wasm_singlethread` | [Qt for WebAssembly](guides/webassembly.md) |

### Qt Version Compatibility and Private Qt ABI

QmlTui requires **CMake 3.21+**, a **C++17** compiler, and **Qt 6.8+** (tested across **Qt 6.8 through 6.12**):

- **Public Qt builds (`-DQMLTUI_ENABLE_PRIVATE_QT=OFF`)**: Link strictly against public `Qt6::Core` and `Qt6::Qml` headers (omitting `Behavior` and direct-assignment `PropertyChanges`). Shared binaries preserve Qt 6 forward binary compatibility across minor versions (`6.8` through `6.12`).
- **Private Qt builds (`-DQMLTUI_ENABLE_PRIVATE_QT=ON`, default when `Qt6::QmlPrivate` is installed)**: Link `Qt6::QmlPrivate` inside `QmlPrivateBridge` to enable `Behavior` and binding-aware `PropertyChanges`. Because Qt does not guarantee binary compatibility for `*Private` modules across minor releases, shared `QmlTuiCore` binaries built with `QMLTUI_ENABLE_PRIVATE_QT=ON` must run against the same `Qt 6.x` minor series they were compiled against. Rebuild QmlTui when upgrading Qt minor versions, or pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` when distributing shared binaries across multiple system Qt minor versions.
- **Pre-1.0 versioning (`0.x`) and shared library `SOVERSION`**: During the `0.x` series, `QmlTuiConfigVersion.cmake` enforces `SameMinorVersion` compatibility (`0.6.x` patch releases preserve QML and C++ source compatibility; breaking API refinements or C++ layout changes increment the minor version until `1.0.0`). Shared libraries (`QmlTuiCore`, `QmlTuiControls`, `QmlTuiImage`, `QmlTuiPtyView`) set `VERSION ${PROJECT_VERSION}` and `SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}` (`0.6` for `0.6.x`), switching to `SOVERSION ${PROJECT_VERSION_MAJOR}` at `1.0.0`.

## Using an Installed Build (`find_package(QmlTui)`)

Build and install QmlTui from a source checkout (pass `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/<kit>` when Qt 6.8+ is not on `PATH`, and add `-DQMLTUI_ENABLE_IMAGE=ON`, `-DQMLTUI_ENABLE_PTYVIEW=ON`, or `-DQMLTUI_ENABLE_RUNNER=OFF` as needed):

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQMLTUI_BUILD_EXAMPLES=OFF \
  -DQMLTUI_BUILD_TESTS=OFF \
  -DQMLTUI_BUILD_BENCHMARKS=OFF
cmake --build build
cmake --install build --prefix /your/prefix
```

Reuse `main.cpp` and `Main.qml` from [Getting Started](getting-started.md) and consume the package with `find_package(QmlTui 0.6 REQUIRED)`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)
find_package(QmlTui 0.6 REQUIRED)

set(QT_QML_GENERATE_QMLLS_INI ON)

qt_add_executable(hello_qmltui main.cpp)
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    QML_FILES Main.qml
)
target_link_libraries(hello_qmltui PRIVATE
    Qt6::Core
    Qt6::Qml
    QmlTui::Core
    QmlTui::Controls
)
```

Configure with the installation prefix (and Qt prefix if needed):

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="/your/prefix;/path/to/Qt/6.x/gcc_64"
cmake --build build
```

When installed to a non-standard prefix, expose the QML module directory at runtime (use `QmlTui_QML_IMPORT_PATH` if your platform installs under `lib64` or a custom `QMLTUI_INSTALL_QMLDIR`):

```sh
QML_IMPORT_PATH=/your/prefix/lib/qml ./build/hello_qmltui
```

### In-Tree (`FetchContent` / `add_subdirectory`) vs. Installed (`find_package`)

- **`DEPENDENCIES TARGET` ordering**: In `FetchContent` and `add_subdirectory` builds, pass `DEPENDENCIES TARGET QmlTui::Core TARGET QmlTui::Controls` (plus `TARGET QmlTui::Image` or `TARGET QmlTui::PtyView` when used) to `qt_add_qml_module()` so QmlTui's `.qmltypes` and `qmldir` build before `qmlcachegen` and `qmllint`. Omit `DEPENDENCIES TARGET` in `find_package(QmlTui)` builds because Qt rejects `IMPORTED` targets there; `find_package(QmlTui)` appends `QmlTui_QML_IMPORT_PATH` to `QT_QML_IMPORT_PATH` and `QML_IMPORT_PATH` automatically. To support both `FetchContent` and `find_package(QmlTui)` in a single `CMakeLists.txt` (for example with `FIND_PACKAGE_ARGS 0.6` on `FetchContent_Declare(QmlTui ...)` in CMake 3.24+, or as shown in [`examples/helloworld/`](../examples/helloworld/)), check `IMPORTED` before adding `DEPENDENCIES`:

  ```cmake
  set(app_qml_dependencies)
  get_target_property(qmltui_imported QmlTui::Core IMPORTED)
  if(NOT qmltui_imported)
      list(APPEND app_qml_dependencies
          DEPENDENCIES
              TARGET QmlTui::Core
              TARGET QmlTui::Controls
      )
  endif()

  qt_add_qml_module(hello_qmltui
      URI HelloQmlTui
      QML_FILES Main.qml
      ${app_qml_dependencies}
  )
  ```

- **Exported targets and `COMPONENTS` isolation**: Both source and installed builds provide `QmlTui::Core`, `QmlTui::Controls`, `QmlTui::Image`, `QmlTui::PtyView`, and `QmlTui::Runner` (plus the `QmlTui::qmltui-runner` CLI target when `Runner` is enabled), with `find_package(QmlTui COMPONENTS Core Controls ...)` matching component names directly. Installed packages split export sets per module (`QmlTuiCoreTargets`, `QmlTuiControlsTargets`, `QmlTuiImageTargets`, `QmlTuiPtyViewTargets`, `QmlTuiRunnerTargets`), so requesting `find_package(QmlTui 0.6 REQUIRED COMPONENTS Core Controls)` never pulls `Qt6::Gui`, `Qt6::Network`, or `libvterm`—even when `QmlTui.Image` and `QmlTui.PtyView` are installed into the same prefix. Calling `find_package(QmlTui 0.6 REQUIRED)` without `COMPONENTS` loads all installed modules.
- **Windows DLL lookup**: For `FetchContent` shared builds, set `set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")` before `FetchContent_MakeAvailable(QmlTui)` so `QmlTui*.dll` land next to your `.exe`. For installed `find_package(QmlTui)` shared builds, add `<prefix>\bin` (alongside your Qt kit's `bin` directory) to `PATH` and set `QML_IMPORT_PATH=<prefix>\lib\qml` at runtime, or bundle dependencies with `windeployqt`.

### Static Builds (`BUILD_SHARED_LIBS=OFF`)

Pass `-DBUILD_SHARED_LIBS=OFF` to build `QmlTuiCore`, `QmlTuiControls`, and optional modules as static libraries and static QML modules (WebAssembly always forces static QML modules automatically). Static QML plugins are linked automatically through `QmlTui::Core` / `QmlTui::Controls` via Qt's `qt_add_qml_module(... STATIC)` integration, so your application links the same `QmlTui::*` targets without manually referencing `*plugin` targets or setting `QML_IMPORT_PATH` at runtime. When installing a static build with `QMLTUI_USE_SYSTEM_UTF8PROC=ON` or `QMLTUI_USE_SYSTEM_LIBVTERM=ON`, ensure `pkg-config` can resolve `libutf8proc` (and `vterm` when requesting `COMPONENTS PtyView`) in the consumer environment.

### Multi-File QML Modules and Singletons

Register additional `.qml` components and singletons on the same `qt_add_qml_module()` target:

- **Root-directory components**: Uppercase `.qml` files next to `CMakeLists.txt` (`QML_FILES Main.qml StatusBar.qml`) become directly instantiable types (`StatusBar { ... }`) across the module.
- **Subdirectory components**: Under Qt's `QTP0001` policy (`qt_standard_project_setup(REQUIRES 6.8)`), set `QT_QML_SOURCE_TYPENAME` on files in subdirectories so unqualified lookup works in `Main.qml`:

  ```cmake
  set_source_files_properties(ui/StatusBar.qml PROPERTIES
      QT_QML_SOURCE_TYPENAME StatusBar
  )
  ```

- **Application singletons**: QmlTui has no global framework theme singleton. Start `Theme.qml` with `pragma Singleton` and mark `QT_QML_SINGLETON_TYPE TRUE` before `qt_add_qml_module()`:

  ```cmake
  set_source_files_properties(Theme.qml PROPERTIES
      QT_QML_SINGLETON_TYPE TRUE
  )
  qt_add_qml_module(my_app
      URI MyApp
      QML_FILES Main.qml Theme.qml
      DEPENDENCIES TARGET QmlTui::Core TARGET QmlTui::Controls
  )
  ```

### Offline / Air-Gapped CI and Reproducible Builds

- **System vs. fetched libraries**: By default, CMake downloads a SHA-256-verified `utf8proc` release tarball (and pinned `libvterm` commit when `PtyView` is enabled) via `FetchContent`. On offline runners, install system packages (resolved via `pkg-config` package names `libutf8proc` and `vterm`) and pass `-DQMLTUI_USE_SYSTEM_UTF8PROC=ON` / `-DQMLTUI_USE_SYSTEM_LIBVTERM=ON`, or point CMake to pre-staged source trees with `-DFETCHCONTENT_SOURCE_DIR_UTF8PROC=/path/to/utf8proc` and `-DFETCHCONTENT_SOURCE_DIR_LIBVTERM=/path/to/libvterm`.
- **Deterministic `Qt6::QmlPrivate`**: Because `QMLTUI_ENABLE_PRIVATE_QT=ON` falls back to `OFF` with a warning when private headers are absent, pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` explicitly in CI if unused, or install `qt6-declarative-private-dev` everywhere.

## Repository Test and Benchmark Builds

To build tests and examples from a QmlTui source checkout (see [Examples and Recipes](examples.md) for `qmltui_gallery` options):

```sh
cmake -S . -B build -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build -LE Demo --output-on-failure
```

To build Release benchmarks (`build-release/tests/Core/benchmark_views`, `benchmark_view_phases`, `benchmark_view_compare`, `benchmark_render`; see [Model/View Comparison](contributing/benchmarks/2026-10-27-view-comparison.md) and [0.4 Acceptance](contributing/benchmarks/0.4-acceptance/README.md)):

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
  -DQMLTUI_BUILD_TESTS=OFF -DQMLTUI_BUILD_EXAMPLES=OFF -DQMLTUI_BUILD_BENCHMARKS=ON
cmake --build build-release
```

## CMake Options and Environment Variables

### Top-Level CMake Options

| Option | Default | Description |
| :--- | :--- | :--- |
| `QMLTUI_BUILD_EXAMPLES` | `ON` (top-level, non-Emscripten) | Build gallery and recipe executables (`OFF` by default under `EMSCRIPTEN` or when consumed via `FetchContent`) |
| `QMLTUI_BUILD_TESTS` | `ON` (top-level, non-Emscripten) | Build unit tests (`Qt6::Test`) and enable `Runner` scenarios (`OFF` under `EMSCRIPTEN` / `FetchContent`) |
| `QMLTUI_BUILD_BENCHMARKS` | `ON` (top-level, non-Emscripten) | Build performance benchmark executables (`OFF` under `EMSCRIPTEN` / `FetchContent`) |
| `QMLTUI_ENABLE_PRIVATE_QT` | `ON` (auto) | Enable `Behavior` and binding-aware `PropertyChanges` via `Qt6::QmlPrivate` (warns if missing unless `OFF`) |
| `QMLTUI_ENABLE_IMAGE` | `OFF` | Build `QmlTui.Image` (`Qt6::Gui`, `Qt6::Network`) |
| `QMLTUI_ENABLE_PTYVIEW` | `OFF` | Build Unix `QmlTui.PtyView` (Linux/macOS) |
| `QMLTUI_ENABLE_RUNNER` | `ON` (top-level or `BUILD_TESTING=ON`) | Build and install `QmlTui::Runner` and `qmltui-runner` (pass `-DQMLTUI_ENABLE_RUNNER=OFF` on top-level release installs if unneeded) |
| `QMLTUI_ENABLE_COVERAGE` | `OFF` | Instrument targets for Clang source-based coverage |
| `QMLTUI_INSTALL_QMLDIR` | `${CMAKE_INSTALL_LIBDIR}/qml` | Install path for QmlTui QML modules |
| `QMLTUI_USE_SYSTEM_UTF8PROC` | `OFF` | Link system `libutf8proc` via `pkg-config` instead of `FetchContent` |
| `QMLTUI_USE_SYSTEM_LIBVTERM` | `OFF` | Link system `vterm` via `pkg-config` instead of `FetchContent` (`PtyView`) |
| `QMLTUI_WASM_INITIAL_MEMORY` | `""` (Emscripten default) | Optional Emscripten `INITIAL_MEMORY` override (for example `67108864`) applied by `qmltui_wasm_app()` |
| `QMLTUI_BUILD_WIDGETS_REFERENCE` | `ON` (when examples enabled & `Qt6::Widgets` found) | Build `qmltui_standard_item_delegate_widgets` side-by-side Qt Widgets comparison target |

### Runtime Environment Variables

| Variable | Values / Units | Description |
| :--- | :--- | :--- |
| `NO_COLOR` | non-empty | Disable ANSI color output (`ColorDepth::Mono`) |
| `QMLTUI_COLOR_DEPTH` | `mono`/`none`/`0`, `16`/`basic`/`ansi`, `256`/`indexed`, `truecolor`/`24bit`/`rgb` | Override detected terminal color depth |
| `QMLTUI_MAX_FPS` | integer `>= 0` (`0` = uncapped) | Maximum render frame rate (default `60`; uncapped `0` during `Runner` sessions) |
| `QMLTUI_LOG_FILE` | file path | Stream `qDebug` / `qWarning` / `console.log()` lines unbuffered to `<path>` alongside normal presentation (including during `Terminal.AltScreen`) |
| `QMLTUI_FRAME_STATS` | `1` / non-empty | Log per-frame render timing and byte statistics via `qmltui.frames` `qCDebug` (routed through `QtMessageRouting`; pair with `QMLTUI_LOG_FILE` or redirected `stderr` in `AltScreen`) |
| `QMLTUI_DEBUG_PAINT_DIRTY` | `1` / non-empty | Warn when an `Item` calls `markDirty()` inside `paint()` |
| `QMLTUI_OUTPUT_QUEUE_LIMIT` | bytes (`>= 0`, default `262144`) | Max queued async terminal output before backpressure defers rendering |
| `QMLTUI_SCROLLBACK_QUEUE_LIMIT` | bytes (`>= 0`, default `1048576`) | Max pending `Scrollback` ANSI/UTF-8 bytes queued in the session before submissions are rejected |
| `QMLTUI_SCROLLBACK_HISTORY_ROWS` | integer `>= 0` (default `5000`) | Max committed `Inline` scrollback rows retained in memory during `Runner` test sessions (interactive sessions do not retain history in memory) |
| `QMLTUI_ESCAPE_TIMEOUT_MS` | ms (default `50`) | Timeout before a lone `ESC` byte dispatches as `Qt::Key_Escape` |
| `QMLTUI_QUERY_TIMEOUT_MS` | ms (default `100`, `0` = disable queries) | Timeout for terminal capability and OSC color queries (`0` suppresses startup/resize escape queries) |
| `QMLTUI_SYNC_OUTPUT` | `0`, `1` | Force-disable (`0`) or force-enable (`1`) DEC mode 2026 synchronized output |
| `QMLTUI_KITTY_KEYBOARD` | `0`/`false`/`off` | Opt out of Kitty keyboard protocol negotiation (opt-out only; truthy values do not force negotiation on unsupported terminals) |
| `QMLTUI_KITTY_GRAPHICS` | `0`/`false`/`off` | Opt out of Kitty graphics protocol detection (opt-out only; truthy values do not force Kitty graphics on unsupported terminals) |
