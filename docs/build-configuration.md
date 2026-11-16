# Build Configuration and Installation

This reference covers QmlTui module targets, consuming an installed package with `find_package(QmlTui)`, `FetchContent` rules, CMake options, and runtime environment variables. To build your first standalone application, start with [Getting Started](getting-started.md).

## QML Modules and CMake Targets

`qmltui::core` and `qmltui::controls` require only `Qt6::Core` and `Qt6::Qml`. Use CMake cache options to enable optional modules and tooling:

| QML Import / Tool | CMake Target | CMake Option & Default | Dependencies | Guide & Reference |
| :--- | :--- | :--- | :--- | :--- |
| `import QmlTui` | `qmltui::core` | Always built (`QMLTUI_ENABLE_PRIVATE_QT=ON` auto-enables `Behavior` when `Qt6::QmlPrivate` is present) | `Qt6::Core`, `Qt6::Qml`, `utf8proc` | [Architecture](guides/architecture.md), [Animation & States](guides/animation-and-states.md) |
| `import QmlTui.Controls` | `qmltui::controls` | Always built | `qmltui::core` | [QML Types](qml-types/index.md) |
| `import QmlTui.Image` | `qmltui::image` | `-DQMLTUI_ENABLE_IMAGE=ON` (default `OFF`) | `qmltui::core`, `Qt6::Gui`, `Qt6::Network` | [Image](qml-types/image.md), [AnimatedSprite](qml-types/animatedsprite.md) |
| `import QmlTui.PtyView` | `qmltui::pty_view` | `-DQMLTUI_ENABLE_PTYVIEW=ON` (default `OFF`, Linux/macOS) | `qmltui::core`, `libvterm` | [Embedded PTY Shells](guides/pty-view.md), [PtyView](qml-types/ptyview.md) |
| `Runner` & `qmltui-runner` | `qmltui::runner` | `-DQMLTUI_ENABLE_RUNNER=ON` (default `ON` top-level or when `BUILD_TESTING=ON`) | `qmltui::core` | [Testing with Runner](guides/testing.md), [Runner](cpp-classes/runner.md) |
| `qmltui_wasm_app(<target>)` | *(CMake helper)* | Automatic under Emscripten (`EMSCRIPTEN`) | Qt for WebAssembly `wasm_singlethread` | [Qt for WebAssembly](guides/webassembly.md) |

### Qt Version Compatibility and Private Qt ABI

QmlTui requires **CMake 3.21+**, a **C++17** compiler, and **Qt 6.8+** (tested across **Qt 6.8 through 6.12**):

- **Public Qt builds (`-DQMLTUI_ENABLE_PRIVATE_QT=OFF`)**: Link strictly against public `Qt6::Core` and `Qt6::Qml` headers (omitting `Behavior` and direct-assignment `PropertyChanges`). Shared binaries preserve Qt 6 forward binary compatibility across minor versions (`6.8` through `6.12`).
- **Private Qt builds (`-DQMLTUI_ENABLE_PRIVATE_QT=ON`, default when `Qt6::QmlPrivate` is installed)**: Link `Qt6::QmlPrivate` inside `QmlPrivateBridge` to enable `Behavior` and binding-aware `PropertyChanges`. Because Qt does not guarantee binary compatibility for `*Private` modules across minor releases, shared `QmlTuiCore` binaries built with `QMLTUI_ENABLE_PRIVATE_QT=ON` must run against the same `Qt 6.x` minor series they were compiled against. Rebuild QmlTui when upgrading Qt minor versions, or pass `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` when distributing shared binaries across multiple system Qt minor versions.

## Using an Installed Build (`find_package(QmlTui)`)

Build and install QmlTui from a source checkout:

```sh
cmake -S . -B build \
  -DQMLTUI_BUILD_EXAMPLES=OFF \
  -DQMLTUI_BUILD_TESTS=OFF \
  -DQMLTUI_BUILD_BENCHMARKS=OFF
cmake --build build
cmake --install build --prefix /your/prefix
```

Reuse `main.cpp` and `Main.qml` from [Getting Started](getting-started.md) and consume the package with `find_package(QmlTui 0.5 REQUIRED)`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)
find_package(QmlTui 0.5 REQUIRED)

set(QT_QML_GENERATE_QMLLS_INI ON)

qt_add_executable(hello_qmltui main.cpp)
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    VERSION 1.0
    QML_FILES Main.qml
)
target_link_libraries(hello_qmltui PRIVATE
    Qt6::Core
    Qt6::Qml
    qmltui::core
    qmltui::controls
)
```

Configure with the installation prefix (and Qt prefix if needed):

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="/your/prefix;/path/to/Qt/6.x/gcc_64"
cmake --build build
```

When installed to a non-standard prefix, expose the QML module directory at runtime (use `QmlTui_QML_IMPORT_PATH` if your platform installs under `lib64` or a custom `QMLTUI_INSTALL_QMLDIR`):

```sh
QML_IMPORT_PATH=/your/prefix/lib/qml ./build/hello_qmltui
```

### In-Tree (`FetchContent` / `add_subdirectory`) vs. Installed (`find_package`)

- **`DEPENDENCIES TARGET` ordering**: In `FetchContent` and `add_subdirectory` builds, pass `DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls` (plus `TARGET qmltui::image` or `TARGET qmltui::pty_view` when used) to `qt_add_qml_module()` so QmlTui's `.qmltypes` and `qmldir` build before `qmlcachegen` and `qmllint`. Omit `DEPENDENCIES TARGET` in `find_package(QmlTui)` builds because Qt rejects `IMPORTED` targets there; `find_package(QmlTui)` appends `QmlTui_QML_IMPORT_PATH` to `QT_QML_IMPORT_PATH` and `QML_IMPORT_PATH` automatically.
- **Target aliases**: Both source and installed builds export `qmltui::core` (`QmlTui::Core`), `qmltui::controls` (`QmlTui::Controls`), `qmltui::image` (`QmlTui::Image`), `qmltui::pty_view` (`QmlTui::PtyView`), and `qmltui::runner` (`QmlTui::Runner`).
- **Windows DLL lookup**: Set `set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")` before `FetchContent_MakeAvailable(QmlTui)` so `QmlTui*.dll` land next to your `.exe`, and add your Qt kit's `bin` directory to `PATH` (or run `windeployqt`).

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
      VERSION 1.0
      QML_FILES Main.qml Theme.qml
      DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
  )
  ```

### Offline / Air-Gapped CI and Reproducible Builds

- **System vs. fetched libraries**: By default, CMake downloads pinned `utf8proc` (and `libvterm` when `PtyView` is enabled) via `FetchContent`. On offline runners, install system packages and pass `-DQMLTUI_USE_SYSTEM_UTF8PROC=ON` / `-DQMLTUI_USE_SYSTEM_LIBVTERM=ON`, or pass `-DFETCHCONTENT_SOURCE_DIR_UTF8PROC=/path/to/utf8proc`.
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
| `QMLTUI_BUILD_EXAMPLES` | `ON` (top-level) | Build gallery and recipe executables |
| `QMLTUI_BUILD_TESTS` | `ON` (top-level) | Build unit tests and enable `Runner` scenarios |
| `QMLTUI_BUILD_BENCHMARKS` | `ON` (top-level) | Build benchmark executables |
| `QMLTUI_ENABLE_PRIVATE_QT` | `ON` (auto) | Enable `Behavior` and binding-aware `PropertyChanges` via `Qt6::QmlPrivate` (warns if missing unless `OFF`) |
| `QMLTUI_ENABLE_IMAGE` | `OFF` | Build `QmlTui.Image` (`Qt6::Gui`, `Qt6::Network`) |
| `QMLTUI_ENABLE_PTYVIEW` | `OFF` | Build Unix `QmlTui.PtyView` (Linux/macOS) |
| `QMLTUI_ENABLE_RUNNER` | `ON` (top-level or `BUILD_TESTING=ON`) | Build and install `qmltui::runner` and `qmltui-runner` |
| `QMLTUI_ENABLE_COVERAGE` | `OFF` | Instrument targets for Clang source-based coverage |
| `QMLTUI_INSTALL_QMLDIR` | `${CMAKE_INSTALL_LIBDIR}/qml` | Install path for QmlTui QML modules |
| `QMLTUI_USE_SYSTEM_UTF8PROC` | `OFF` | Link system `utf8proc` instead of `FetchContent` |
| `QMLTUI_USE_SYSTEM_LIBVTERM` | `OFF` | Link system `libvterm` instead of `FetchContent` (`PtyView`) |

### Runtime Environment Variables

| Variable | Values / Units | Description |
| :--- | :--- | :--- |
| `NO_COLOR` | non-empty | Disable ANSI color output (`ColorDepth::Mono`) |
| `QMLTUI_COLOR_DEPTH` | `mono`/`none`/`0`, `16`/`basic`/`ansi`, `256`/`indexed`, `truecolor`/`24bit`/`rgb` | Override detected terminal color depth |
| `QMLTUI_MAX_FPS` | integer `>= 0` (`0` = uncapped) | Maximum render frame rate (default `60`) |
| `QMLTUI_LOG_FILE` | file path | Stream `qDebug` / `qWarning` / `console.log()` lines unbuffered to `<path>` (including during `Terminal.AltScreen`) |
| `QMLTUI_FRAME_STATS` | `1` / non-empty | Log periodic render timing and byte statistics to `stderr` |
| `QMLTUI_DEBUG_PAINT_DIRTY` | `1` / non-empty | Warn when an `Item` calls `markDirty()` inside `paint()` |
| `QMLTUI_OUTPUT_QUEUE_LIMIT` | bytes (`>= 0`, default `262144`) | Max queued async terminal output before backpressure defers rendering |
| `QMLTUI_SCROLLBACK_HISTORY_ROWS` | integer `>= 0` (default `5000`) | Max committed `Inline` scrollback rows retained in memory |
| `QMLTUI_ESCAPE_TIMEOUT_MS` | ms (default `50`) | Timeout before a lone `ESC` byte dispatches as `Qt::Key_Escape` |
| `QMLTUI_QUERY_TIMEOUT_MS` | ms (default `100`) | Timeout for terminal capability and OSC color queries |
| `QMLTUI_SYNC_OUTPUT` | `0`, `1` | Force-disable (`0`) or force-enable (`1`) DEC mode 2026 synchronized output |
| `QMLTUI_KITTY_KEYBOARD` | `0`, `1` | Opt out (`0`/`false`/`off`) of Kitty keyboard protocol negotiation |
| `QMLTUI_KITTY_GRAPHICS` | `0`, `1` | Opt out (`0`/`false`/`off`) of Kitty graphics protocol detection |
