# Getting Started

This guide walks you through checking system requirements, running the interactive component gallery, and creating your first standalone QmlTui application.

---

## 1. Requirements

To build QmlTui or a QmlTui application, you need:

- **CMake 3.21** or newer (`3.20+` when building QmlTui in-tree)
- **A C++20 compiler** (GCC 11+, Clang 14+, Apple Clang, or MSVC 2022)
- **Qt 6.8 or newer** with the `Core` and `Qml` development packages

### Optional Qt & System Dependencies

| Feature / Module | CMake Option | Default | Additional Requirement |
| :--- | :--- | :--- | :--- |
| **`QmlTui.Image`** (`Image`, `AnimatedSprite`) | `-DQMLTUI_ENABLE_IMAGE=ON` | `ON` | `Qt6::Gui` and `Qt6::Network` (set `OFF` for a headless server build without GUI/network packages) |
| **`Behavior` & `StateGroup`** | `-DQMLTUI_ENABLE_PRIVATE_QT=ON` | `ON` | `Qt6::QmlPrivate` headers (couples build to the exact Qt minor version; set `OFF` for strict public-Qt builds) |
| **`QmlTui.PtyView`** (`PtyView`) | `-DQMLTUI_ENABLE_PTY_VIEW=ON` | `OFF` | Linux or macOS (`libvterm` is fetched automatically unless `QMLTUI_USE_SYSTEM_LIBVTERM=ON`) |

---

## 2. Try the Interactive Gallery in 30 Seconds

Clone the repository and build `qmltui_gallery` to explore over 35 interactive controls, virtualized views, canvas demos, and animations in your terminal:

```sh
git clone https://github.com/qmltui-dev/qmltui.git
cd qmltui
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

To build the entire repository and run the automated test suite:

```sh
cmake -S . -B build -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## 3. Create Your First Standalone Application (3 Files)

The fastest way to start a new project is with CMake's `FetchContent` (also available ready-to-build in [`examples/quickstart/`](../examples/quickstart/)). You only need Qt 6.8+ installed—CMake fetches and builds QmlTui automatically.

Create a new directory with the following three files:

### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

# Fetch QmlTui at a release tag: nothing to install besides Qt.
set(QMLTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QMLTUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(
    QmlTui
    GIT_REPOSITORY https://github.com/qmltui-dev/qmltui.git
    GIT_TAG        v0.5.0
)
FetchContent_MakeAvailable(QmlTui)

qt_add_executable(hello_qmltui main.cpp)
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)
target_link_libraries(hello_qmltui PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
```

### `main.cpp`

```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("HelloQmlTui", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
```

### `Main.qml`

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root
    color: "#0d1117"

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "Welcome to QmlTui"
            color: "cyan"
            bold: true
        }

        Button {
            text: "[ Press Enter, or Q to quit ]"
            focus: true
            onClicked: status.text = "Status: Clicked!"
        }

        Text {
            id: status
            text: "Status: ready"
            color: "magenta"
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }
}
```

### Build and Run

```sh
cmake -S . -B build
cmake --build build
./build/hello_qmltui
```

Your terminal switches to the alternate screen and renders:

```text
Welcome to QmlTui

╔═══════════════════════════════╗
║ [ Press Enter, or Q to quit ] ║
╚═══════════════════════════════╝

Status: ready
```

Press **Enter** or **Space** to activate the focused button (updating the label to `Status: Clicked!`), or press **Q** / **Escape** to exit cleanly.

---

## 4. Consuming an Installed QmlTui (`find_package(QmlTui)`)

If you prefer installing QmlTui once onto your system or sysroot instead of fetching it per project:

```sh
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=OFF -DQMLTUI_BUILD_TESTS=OFF
cmake --build build
cmake --install build --prefix /your/prefix
```

Then consume it in your project's `CMakeLists.txt` via `find_package(QmlTui 0.5 REQUIRED)`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
find_package(QmlTui 0.5 REQUIRED)
qt_standard_project_setup(REQUIRES 6.8)

# Point qmllint / qmlls at the installed QmlTui QML modules:
list(APPEND QT_QML_IMPORT_PATH "${QmlTui_QML_IMPORT_PATH}")

qt_add_executable(hello_qmltui main.cpp)
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    VERSION 1.0
    QML_FILES Main.qml
)
target_link_libraries(hello_qmltui PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)
```

### In-Tree (`FetchContent` / `add_subdirectory`) vs. Installed (`find_package`)

- **In-tree (`add_subdirectory` or `FetchContent`)**: Pass `DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls` to `qt_add_qml_module(...)` so CMake orders QML module generation and `qmllint` finds the build-tree modules under `QT_QML_OUTPUT_DIRECTORY`.
- **Installed (`find_package(QmlTui)`)**: Qt's `qt_add_qml_module(DEPENDENCIES TARGET ...)` does not accept `IMPORTED` targets. Instead, link `qmltui::core`, `qmltui::controls`, and (if needed) `qmltui::image` via `target_link_libraries()`, and append the exported `QmlTui_QML_IMPORT_PATH` variable to `QT_QML_IMPORT_PATH` (or `QML_IMPORT_PATH` at runtime). Installed QML modules live under `<prefix>/lib/qml` by default (configurable via `QMLTUI_INSTALL_QMLDIR`).

The available QML modules and CMake targets are:

| QML Import | CMake Target | Description |
| :--- | :--- | :--- |
| `import QmlTui` | `qmltui::core` | Standalone retained-mode TUI engine (`Terminal`, `Item`, `Rectangle`, `Text`, `TextInput`, `TextArea`, layouts, virtualized views, `Canvas`, input, animation; requires only `Qt6::Core` and `Qt6::Qml`) |
| `import QmlTui.Controls` | `qmltui::controls` | Interactive controls (`Button`, `CheckBox`, `Dialog`, `Panel`, `ProgressBar`, `Slider`, etc.) |
| `import QmlTui.Image` | `qmltui::image` | Terminal graphics (`Image`, `AnimatedSprite`; requires `QMLTUI_ENABLE_IMAGE=ON`) |
| `import QmlTui.PtyView` | `qmltui::pty_view` | Embedded Unix PTY (`PtyView`; requires `QMLTUI_ENABLE_PTY_VIEW=ON`) |

See [Module Boundaries & What `QmlTui.Core` Provides Alone](guides/architecture.md#module-boundaries-what-qmltui-core-provides-alone) for a breakdown of building applications using only `qmltui::core` (`import QmlTui`).

---

## 5. Optional Modules & Features

### Embedded PTY Shells (`QmlTui.PtyView`)

The Unix PTY view (`PtyView`) is opt-in on Linux and macOS:

```sh
cmake -S . -B build -DQMLTUI_ENABLE_PTY_VIEW=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery --example PtyView
```

See [Embedded PTY Shells](guides/pty-view.md) and the [PtyView QML Reference](qml-types/ptyview.md) for its API, security defaults, offline `libvterm` setup, and compatibility notes. Note that QmlTui separates inline streaming output from hosted PTY shells:

- `qmltui_inline_repl_demo` ([`examples/recipes/inline_repl`](../examples/recipes/inline_repl/)) and `qmltui_gallery --example InlineMode` ([`examples/gallery/examples/InlineMode.qml`](../examples/gallery/examples/InlineMode.qml)) demonstrate inline sessions with scrollback and prompt input using `Terminal.Inline`—they do **not** require `PtyView`.
- `qmltui_gallery --example PtyView` embeds an interactive PTY shell inside a full-screen layout (`QMLTUI_ENABLE_PTY_VIEW=ON`).

### Headless Testing & `.cast` Recording (`Runner`)

Enable `-DQMLTUI_ENABLE_RUNNER=ON` (or `-DQMLTUI_BUILD_TESTS=ON` in-tree) to build `qmltui::runner` and the `qmltui-runner` CLI launcher for deterministic headless testing and Asciinema recording. See [Testing and Recording with Runner](guides/testing.md) and the [Runner C++ reference](cpp-classes/runner.md).

### Performance Benchmarks

Benchmarks are built when `-DQMLTUI_BUILD_BENCHMARKS=ON`:

- `benchmark_views` records 1,000- and 10,000-row view creation time.
- `benchmark_render` records per-frame paint and diff cost for representative scenes at 80×24 and 240×70 (`--json <path>` for trending, `--frames <count>` to adjust sample count from the default 200 after 5 warm-up frames). JSON output also includes setter time (`updateMedianUs`) and median combined setter/paint cost (`updatePaintMedianUs`) so text-cache rebuilds are accounted for in changing-text workloads.

Run `benchmark_render` before and after rendering changes when contributing. See the [0.4 acceptance report](contributing/benchmarks/0.4-acceptance/README.md) for methodology and baseline numbers.

---

## 6. CMake Options & Runtime Environment Variables

### Top-Level CMake Options

| Option | Default | Description |
| :--- | :--- | :--- |
| `QMLTUI_BUILD_EXAMPLES` | `ON` (top-level) | Build example and guide executables |
| `QMLTUI_BUILD_TESTS` | `ON` (top-level) | Build unit tests and enable Runner scenarios |
| `QMLTUI_BUILD_BENCHMARKS` | `ON` (top-level) | Build performance benchmark executables |
| `QMLTUI_ENABLE_PRIVATE_QT` | `ON` | Enable `Behavior`, `StateGroup`, and advanced states (`Qt6::QmlPrivate`) |
| `QMLTUI_ENABLE_IMAGE` | `ON` | Build the `QmlTui.Image` module (`Qt6::Gui`, `Qt6::Network`) |
| `QMLTUI_ENABLE_PTY_VIEW` | `OFF` | Build the Unix PTY `QmlTui.PtyView` module (Linux/macOS) |
| `QMLTUI_ENABLE_RUNNER` | `OFF` | Build and install `qmltui::runner` and the `qmltui-runner` CLI |
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

---

## 7. Next Steps

- **Read the [Guides](guides/index.md)**: Explore [Layouts & Cell Geometry](guides/layouts.md), [Input & Focus](guides/input-and-focus.md), [Models in QmlTui Views](guides/models.md), [Styling & Colors](guides/styling-and-colors.md), and [Testing with Runner](guides/testing.md).
- **Set up your editor**: Read the [IDE setup guide](guides/ide-setup.md) to configure `qmlls`, `qmllint`, and `qmltui-runner.d.ts`.
- **Coming from Qt Quick?** Read [QmlTui for Qt Quick developers](guides/qt-quick-to-qmltui.md).
- **Explore components and recipes**: Browse the [Examples & Component Cookbook](examples.md).
