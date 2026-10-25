# Build Configuration and Installation

This reference covers QmlTui module dependencies, consuming an installed build with `find_package(QmlTui)`, in-tree `FetchContent` rules, CMake options, and runtime environment variables.

To build your first standalone application, start with the [Getting Started tutorial](getting-started.md).

## QML Modules and CMake Targets

QmlTui is organized into a core library (`qmltui::core`) and optional companion modules:

| QML Import | CMake Target | Required Dependencies | Description |
| :--- | :--- | :--- | :--- |
| `import QmlTui` | `qmltui::core` | `Qt6::Core`, `Qt6::Qml`, `utf8proc` | Core engine (`Terminal`, `Scrollback`, `Item`, `Rectangle`, `Text`, `TextInput`, `TextArea`, layouts, virtualized data views, `Canvas`, input, states, and animations) |
| `import QmlTui.Controls` | `qmltui::controls` | `qmltui::core` | Interactive controls (`Button`, `CheckBox`, `RadioButton`, `Switch`, `Slider`, `ProgressBar`, `BusyIndicator`, `Sparkline`, `TabBar`, `StackView`, `Panel`, `Dialog`, `Toast`, `ToolTip`) |
| `import QmlTui.Image` | `qmltui::image` | `qmltui::core`, `Qt6::Gui`, `Qt6::Network` | Terminal graphics (`Image`, `AnimatedSprite`; requires `-DQMLTUI_ENABLE_IMAGE=ON`) |
| `import QmlTui.PtyView` | `qmltui::pty_view` | `qmltui::core`, `libvterm` | Embedded Unix PTY (`PtyView`; requires `-DQMLTUI_ENABLE_PTYVIEW=ON` on Linux or macOS) |

See [QML Modules and CMake Targets](guides/architecture.md#qml-modules-and-cmake-targets) in the Architecture guide for details on what each module provides.

## Choosing Optional Dependencies

`qmltui::core` and `qmltui::controls` require only `Qt6::Core` and `Qt6::Qml`. A default source build also enables `QmlTui.Image` and private-Qt features, which use the additional packages below:

| Feature / Module | CMake Option | Default | Requirement |
| :--- | :--- | :--- | :--- |
| **`QmlTui.Image`** (`Image`, `AnimatedSprite`) | `-DQMLTUI_ENABLE_IMAGE=ON` | `ON` | `Qt6::Gui` and `Qt6::Network` (set `OFF` for headless builds without GUI or network packages) |
| **`Behavior` & direct `PropertyChanges`** | `-DQMLTUI_ENABLE_PRIVATE_QT=ON` | `ON` (auto) | `Qt6::QmlPrivate` headers (used automatically when available; warns and falls back if missing unless set to `OFF`) |
| **`QmlTui.PtyView`** (`PtyView`) | `-DQMLTUI_ENABLE_PTYVIEW=ON` | `OFF` | Linux or macOS (`libvterm` is fetched automatically unless `QMLTUI_USE_SYSTEM_LIBVTERM=ON`) |
| **`qmltui::runner`** (`Runner` & `qmltui-runner`) | `-DQMLTUI_ENABLE_RUNNER=ON` | `ON` (top-level) / `OFF` (subproject) | Headless scenario test runner and CLI |

For an application that depends only on public `Qt6::Core` and `Qt6::Qml` APIs, configure with:

```sh
cmake -S . -B build -DQMLTUI_ENABLE_IMAGE=OFF -DQMLTUI_ENABLE_PRIVATE_QT=OFF
```

This keeps `qmltui::core` and `qmltui::controls` enabled while omitting `QmlTui.Image`, `Behavior`, and direct-assignment `PropertyChanges`.

## Using an Installed Build (`find_package(QmlTui)`)

To install QmlTui once and consume it across multiple projects, build and install from a QmlTui source checkout:

```sh
cmake -S . -B build \
  -DQMLTUI_BUILD_EXAMPLES=OFF \
  -DQMLTUI_BUILD_TESTS=OFF \
  -DQMLTUI_BUILD_BENCHMARKS=OFF
cmake --build build
cmake --install build --prefix /your/prefix
```

Reuse `main.cpp` and `Main.qml` from [Getting Started](getting-started.md) and locate QmlTui in `CMakeLists.txt` with `find_package(QmlTui 0.5 REQUIRED)`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)
find_package(QmlTui 0.5 REQUIRED)

# Point qmllint and qmlls at the installed QmlTui QML modules:
list(APPEND QT_QML_IMPORT_PATH "${QmlTui_QML_IMPORT_PATH}")

qt_add_executable(hello_qmltui main.cpp)
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    VERSION 1.0
    QML_FILES Main.qml
)
target_link_libraries(hello_qmltui PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
```

Configure the application with the installation prefix (and your Qt prefix if needed):

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="/your/prefix;/path/to/Qt/6.x/gcc_64"
cmake --build build
```

When installing to a non-standard prefix, make the installed QML modules discoverable at runtime:

```sh
QML_IMPORT_PATH=/your/prefix/lib/qml ./build/hello_qmltui
```

Use the actual `QmlTui_QML_IMPORT_PATH` value if your platform uses `lib64` or a custom `QMLTUI_INSTALL_QMLDIR`. In Qt Creator, add `QML_IMPORT_PATH` to the run environment. On Windows, add the directory containing the installed QmlTui DLLs to `PATH`.

### In-Tree (`FetchContent` / `add_subdirectory`) vs. Installed (`find_package`)

- **In-tree (`add_subdirectory` or `FetchContent`)**: Pass `DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls` to `qt_add_qml_module(...)` so CMake orders QML module generation and `qmllint` resolves the build-tree modules under `QT_QML_OUTPUT_DIRECTORY`.
- **Installed (`find_package(QmlTui)`)**: Qt's `qt_add_qml_module(DEPENDENCIES TARGET ...)` does not accept `IMPORTED` targets. Omit `DEPENDENCIES TARGET ...`, link `qmltui::core`, `qmltui::controls`, and (if needed) `qmltui::image` via `target_link_libraries()`, and append `QmlTui_QML_IMPORT_PATH` to `QT_QML_IMPORT_PATH` (or `QML_IMPORT_PATH` at runtime). Installed QML modules default to `<prefix>/lib/qml` (configurable via `QMLTUI_INSTALL_QMLDIR`).

## Optional Modules and Tooling

### Embedded PTY Shells (`QmlTui.PtyView`)

`PtyView` is opt-in on Linux and macOS:

```sh
cmake -S . -B build -DQMLTUI_ENABLE_PTYVIEW=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery --example PtyView
```

See [Embedded PTY Shells](guides/pty-view.md) and the [PtyView QML Reference](qml-types/ptyview.md). Note the distinction between inline output and hosted PTYs:

- `qmltui_inline_repl_demo` ([`examples/recipes/inline_repl`](../examples/recipes/inline_repl/)) uses `Terminal.Inline` and `Scrollback` and does not require `PtyView`.
- `qmltui_gallery --example PtyView` hosts a child process inside a pseudo-terminal (`QMLTUI_ENABLE_PTYVIEW=ON`).

### Headless Testing and Recording (`Runner`)

Enable `-DQMLTUI_ENABLE_RUNNER=ON` (enabled by default for top-level builds or when `-DQMLTUI_BUILD_TESTS=ON` is set) to build `qmltui::runner` and the `qmltui-runner` CLI. See [Testing and Recording with Runner](guides/testing.md) and the [Runner C++ reference](cpp-classes/runner.md).

## Development Builds

### Build and Run the Gallery and Test Suite

Clone the repository and build `qmltui_gallery` to inspect the built-in controls, views, layouts, and canvas primitives:

```sh
git clone https://github.com/qmltui-dev/qmltui.git
cd qmltui
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

To build the full repository and run the CTest suite:

```sh
cmake -S . -B build -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Benchmarks

Build benchmarks with `-DQMLTUI_BUILD_BENCHMARKS=ON`:

- `benchmark_views` measures 1,000- and 10,000-row view creation time.
- `benchmark_render` measures per-frame paint and diff cost at 80×24 and 240×70 (`--json <path>`, `--frames <count>`). JSON output includes `updateMedianUs` and `updatePaintMedianUs`.

See the [0.4 acceptance report](contributing/benchmarks/0.4-acceptance/README.md) for baseline numbers.

## CMake Options and Environment Variables

### Top-Level CMake Options

| Option | Default | Description |
| :--- | :--- | :--- |
| `QMLTUI_BUILD_EXAMPLES` | `ON` (top-level) | Build example and recipe executables |
| `QMLTUI_BUILD_TESTS` | `ON` (top-level) | Build unit tests and enable Runner scenarios |
| `QMLTUI_BUILD_BENCHMARKS` | `ON` (top-level) | Build benchmark executables |
| `QMLTUI_ENABLE_PRIVATE_QT` | `ON` (auto) | Use `Qt6::QmlPrivate` when available to enable `Behavior` and advanced states (warns if missing unless set to `OFF`) |
| `QMLTUI_ENABLE_IMAGE` | `ON` | Build the `QmlTui.Image` module (`Qt6::Gui`, `Qt6::Network`) |
| `QMLTUI_ENABLE_PTYVIEW` | `OFF` | Build the Unix PTY `QmlTui.PtyView` module (Linux/macOS) |
| `QMLTUI_ENABLE_RUNNER` | `ON` (top-level) | Build and install `qmltui::runner` and the `qmltui-runner` CLI |
| `QMLTUI_ENABLE_COVERAGE` | `OFF` | Instrument QmlTui targets for Clang source-based code coverage |
| `QMLTUI_INSTALL_QMLDIR` | `${CMAKE_INSTALL_LIBDIR}/qml` | Install directory for QmlTui QML modules (`<prefix>/lib/qml`) |
| `QMLTUI_USE_SYSTEM_UTF8PROC` | `OFF` | Link system `utf8proc` instead of the bundled copy |
| `QMLTUI_USE_SYSTEM_LIBVTERM` | `OFF` | Link system `libvterm` instead of the fetched source (`PtyView` only) |
| `QMLTUI_COMMERCIAL_LICENSE` | `OFF` | Acknowledge building under a commercial QmlTui license |

### Runtime Environment Variables

| Variable | Values / Units | Description |
| :--- | :--- | :--- |
| `NO_COLOR` | non-empty | Disable ANSI color output (`ColorDepth::Monochrome`) |
| `QMLTUI_COLOR_DEPTH` | `mono`/`0`, `16`, `256`, `truecolor`/`24bit` | Override auto-detected terminal color depth |
| `QMLTUI_MAX_FPS` | integer `1`–`240` | Override maximum render frame rate |
| `QMLTUI_FRAME_STATS` | `1` / non-empty | Log periodic render timing and byte statistics to stderr |
| `QMLTUI_OUTPUT_QUEUE_LIMIT` | bytes (`>= 1024`) | Maximum queued async terminal output before backpressure drops frames |
| `QMLTUI_SYNC_OUTPUT` | `0`, `1` | Force-disable (`0`) or force-enable (`1`) DEC mode 2026 synchronized output |
| `QMLTUI_KITTY_KEYBOARD` | `0`, `1` | Force-disable (`0`) or force-enable (`1`) Kitty keyboard protocol negotiation |
| `QMLTUI_KITTY_GRAPHICS` | `0`, `1` | Force-disable (`0`) or force-enable (`1`) Kitty graphics protocol detection |
