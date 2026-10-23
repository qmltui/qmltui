# Getting Started

This guide covers system requirements, running the component gallery, and setting up a standalone QmlTui application with CMake.

## Requirements

- **CMake 3.21** or newer (`3.20+` when building QmlTui in-tree)
- **C++17** compiler
- **Qt 6.8 or newer** with the `Core` and `Qml` components

### Optional Dependencies

| Feature / Module | CMake Option | Default | Requirement |
| :--- | :--- | :--- | :--- |
| **`QmlTui.Image`** (`Image`, `AnimatedSprite`) | `-DQMLTUI_ENABLE_IMAGE=ON` | `ON` | `Qt6::Gui` and `Qt6::Network` (set `OFF` for headless builds without GUI/network packages) |
| **`Behavior` & `StateGroup`** | `-DQMLTUI_ENABLE_PRIVATE_QT=ON` | `ON` (auto) | `Qt6::QmlPrivate` headers (used automatically when available; warns and falls back if missing unless set to `OFF`) |
| **`QmlTui.PtyView`** (`PtyView`) | `-DQMLTUI_ENABLE_PTY_VIEW=ON` | `OFF` | Linux or macOS (`libvterm` is fetched automatically unless `QMLTUI_USE_SYSTEM_LIBVTERM=ON`) |

## Build and Run the Gallery

Clone the repository and build `qmltui_gallery` to inspect the built-in controls, views, layouts, and canvas primitives:

```sh
git clone https://github.com/qmltui-dev/qmltui.git
cd qmltui
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

To build the full repository and run the test suite:

```sh
cmake -S . -B build -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Standalone Application (`FetchContent`)

A minimal standalone project requires three files (also available in [`examples/quickstart/`](../examples/quickstart/)). With `FetchContent`, CMake downloads and builds QmlTui alongside your target.

### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

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

The terminal switches to the alternate screen and renders:

```text
Welcome to QmlTui

╔═══════════════════════════════╗
║ [ Press Enter, or Q to quit ] ║
╚═══════════════════════════════╝

Status: ready
```

Press **Enter** or **Space** to activate the focused button, or **Q** / **Escape** to exit.

## Using an Installed Build (`find_package(QmlTui)`)

To install QmlTui once and consume it across multiple projects:

```sh
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=OFF -DQMLTUI_BUILD_TESTS=OFF
cmake --build build
cmake --install build --prefix /your/prefix
```

Then locate it in `CMakeLists.txt` with `find_package(QmlTui 0.5 REQUIRED)`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
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

- **In-tree (`add_subdirectory` or `FetchContent`)**: Pass `DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls` to `qt_add_qml_module(...)` so CMake orders QML module generation and `qmllint` resolves the build-tree modules under `QT_QML_OUTPUT_DIRECTORY`.
- **Installed (`find_package(QmlTui)`)**: Qt's `qt_add_qml_module(DEPENDENCIES TARGET ...)` does not accept `IMPORTED` targets. Link `qmltui::core`, `qmltui::controls`, and (if needed) `qmltui::image` via `target_link_libraries()`, and append `QmlTui_QML_IMPORT_PATH` to `QT_QML_IMPORT_PATH` (or `QML_IMPORT_PATH` at runtime). Installed QML modules default to `<prefix>/lib/qml` (configurable via `QMLTUI_INSTALL_QMLDIR`).

| QML Import | CMake Target | Description |
| :--- | :--- | :--- |
| `import QmlTui` | `qmltui::core` | Core engine (`Terminal`, `Item`, `Rectangle`, `Text`, `TextInput`, `TextArea`, layouts, data views, `Canvas`, input, animation; requires `Qt6::Core` and `Qt6::Qml`) |
| `import QmlTui.Controls` | `qmltui::controls` | Interactive controls (`Button`, `CheckBox`, `Dialog`, `Panel`, `ProgressBar`, `Slider`, `TabBar`, etc.) |
| `import QmlTui.Image` | `qmltui::image` | Terminal graphics (`Image`, `AnimatedSprite`; requires `QMLTUI_ENABLE_IMAGE=ON`) |
| `import QmlTui.PtyView` | `qmltui::pty_view` | Embedded Unix PTY (`PtyView`; requires `QMLTUI_ENABLE_PTY_VIEW=ON`) |

See [QML Modules and CMake Targets](guides/architecture.md#qml-modules-and-cmake-targets) for details on what each module provides.

## Optional Modules and Tooling

### Embedded PTY Shells (`QmlTui.PtyView`)

`PtyView` is opt-in on Linux and macOS:

```sh
cmake -S . -B build -DQMLTUI_ENABLE_PTY_VIEW=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery --example PtyView
```

See [Embedded PTY Shells](guides/pty-view.md) and the [PtyView QML Reference](qml-types/ptyview.md). Note the distinction between inline output and hosted PTYs:

- `qmltui_inline_repl_demo` ([`examples/recipes/inline_repl`](../examples/recipes/inline_repl/)) uses `Terminal.Inline` and `Scrollback` and does not require `PtyView`.
- `qmltui_gallery --example PtyView` hosts a child process inside a pseudo-terminal (`QMLTUI_ENABLE_PTY_VIEW=ON`).

### Headless Testing and Recording (`Runner`)

Enable `-DQMLTUI_ENABLE_RUNNER=ON` (or `-DQMLTUI_BUILD_TESTS=ON` in-tree) to build `qmltui::runner` and the `qmltui-runner` CLI. See [Testing and Recording with Runner](guides/testing.md) and the [Runner C++ reference](cpp-classes/runner.md).

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
| `QMLTUI_ENABLE_PRIVATE_QT` | `ON` (auto) | Use `Qt6::QmlPrivate` when available to enable `Behavior`, `StateGroup`, and advanced states (warns if missing unless set to `OFF`) |
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

## Next Steps

- **[Guides](guides/index.md)**: [Layouts & Cell Geometry](guides/layouts.md), [Input & Focus](guides/input-and-focus.md), [Models in QmlTui Views](guides/models.md), [Styling & Colors](guides/styling-and-colors.md), and [Testing with Runner](guides/testing.md).
- **[Qt Creator Setup](guides/qt-creator.md)**: Project wizards, offline `F1` documentation, `qmlls`, `qmllint`, and `qmltui-runner.d.ts`.
- **[QmlTui for Qt Quick Developers](guides/qt-quick-to-qmltui.md)**: Differences from `QtQuick`.
- **[Examples and Recipes](examples.md)**: Gallery components and standalone recipes.
