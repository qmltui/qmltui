# Getting Started

Think of **QmlTui** as the **Qt Quick equivalent for terminal user interfaces**: you declare reactive layouts, controls, and property bindings in QML and build the backend in C++. This tutorial walks Qt 6 developers through building a standalone QmlTui terminal application with CMake, running it in the terminal, and binding a C++ `QObject` class to QML with `QML_ELEMENT`.

For `find_package(QmlTui)` installation, optional modules (`QmlTui.Image`, `QmlTui.PtyView`, `Runner`), CMake cache options, and runtime environment variables, see [Build Configuration and Installation](build-configuration.md).

## Prerequisites

- **CMake 3.21** or newer (`3.20+` when building QmlTui in-tree)
- **C++17** compiler
- **Qt 6.8 or newer** with `Core` and `Qml` (plus `QmlPrivate` when using `Behavior`, and `Gui` + `Network` when enabling `QmlTui.Image` with `-DQMLTUI_ENABLE_IMAGE=ON`). See the [Qt 6 Getting Started Guide](https://doc.qt.io/qt-6/gettingstarted.html) to install Qt 6.8+.

## Step 1: Create a Standalone Application

A standalone QmlTui project uses `QCoreApplication` (rather than `QGuiApplication`), `QQmlApplicationEngine`, and `qt_add_qml_module`. Create a new directory with the following three files (for a minimal `qmltui::core`-only starter template without `QmlTui.Controls`, see [`examples/helloworld/`](../examples/helloworld/)).

### 1. `CMakeLists.txt`

With `FetchContent`, CMake downloads and builds QmlTui alongside your executable target:

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

set(QMLTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QMLTUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QMLTUI_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
# Disable QmlTui.Image for a Core + Qml only build (remove to enable Image with Qt6::Gui and Qt6::Network):
set(QMLTUI_ENABLE_IMAGE OFF CACHE BOOL "" FORCE)
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

### 2. `main.cpp`

```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("HelloQmlTui", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    return app.exec();
}
```

### 3. `Main.qml`

Import `QtQml`, `QmlTui`, and `QmlTui.Controls`, and provide an explicit keyboard quit handler on the root `Terminal`:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root
    color: "#0d1117"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

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
}
```

### 4. Build and Run

Configure, build, and run `hello_qmltui` in an interactive terminal:

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

Press **Enter** or **Space** to activate the focused button and update the status text. Press **Q** or **Escape** to exit and restore the normal terminal screen.

## Step 2: C++ Integration (`QML_ELEMENT` and Models)

QmlTui uses standard `Qt6::Qml` C++ integration:

- Expose C++ `QObject` controllers and `QAbstractItemModel` subclasses with `QML_ELEMENT` and `SOURCES` in `qt_add_qml_module(...)`, or pass existing C++ instances via `QQmlApplicationEngine::setInitialProperties(...)`.
- For QML syntax and C++ type registration, see Qt's [QML Syntax Basics](https://doc.qt.io/qt-6/qtqml-syntax-basics.html), [Overview - QML and C++ Integration](https://doc.qt.io/qt-6/qtqml-cppintegration-overview.html), and [Defining QML Types from C++](https://doc.qt.io/qt-6/qtqml-cppintegration-definetypes.html).
- For binding C++ `QAbstractItemModel` and `QSortFilterProxyModel` classes to `ListView`, `TableView`, and `TreeView`, see [Models in QmlTui Views](guides/models.md), [`examples/recipes/table_view/`](../examples/recipes/table_view/), and [`examples/recipes/lazy_model/`](../examples/recipes/lazy_model/).

## Step 3: Explore the Component Gallery (Optional)

To inspect all built-in controls, data views, layouts, and canvas primitives locally, clone the repository and build `qmltui_gallery`:

```sh
git clone https://github.com/qmltui-dev/qmltui.git
cd qmltui
cmake -S . -B build -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

You can also run and edit the gallery examples in your browser using the [WebAssembly Playground](playground.md).

## Next Steps

1. **[QmlTui for Qt Quick and Widgets Developers](guides/qt-quick-to-qmltui.md)**: Learn how QmlTui's character-cell coordinates, box-drawing borders, colors, keyboard quit handling, and model roles differ from `QtQuick` and `QtWidgets`.
2. **[Build Configuration and Installation](build-configuration.md)**: Install QmlTui for `find_package(QmlTui)`, configure optional modules (`QmlTui.Image`, `QmlTui.PtyView`, `Runner`), and review all CMake options and runtime environment variables.
3. **[Qt Creator Setup and Tooling](guides/qt-creator.md)**: Configure project wizards (`qmltui-qtc`), offline `F1` documentation (`qmltui.qch`), `qmlls`, `qmllint`, and `Runner` scenario actions.
4. **[Guides](guides/index.md)**: Read in-depth guides on [Layouts & Cell Geometry](guides/layouts.md), [Input & Focus](guides/input-and-focus.md), [Models in QmlTui Views](guides/models.md), [Styling & Colors](guides/styling-and-colors.md), and [Testing with Runner](guides/testing.md).
5. **[Examples and Recipes](examples.md)**: Browse standalone QML and C++ recipes under `examples/recipes/`.
