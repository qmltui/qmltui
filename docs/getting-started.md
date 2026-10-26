# Getting Started

Think of **QmlTui** as the **Qt Quick equivalent for terminal user interfaces**: you declare reactive layouts, controls, and property bindings in QML and build the backend in C++. This tutorial walks Qt 6 developers through building a standalone QmlTui terminal application with CMake, running it in the terminal, and binding a C++ `QObject` class to QML with `QML_ELEMENT`.

For `find_package(QmlTui)` installation, optional modules (`QmlTui.Image`, `QmlTui.PtyView`, `Runner`), CMake cache options, and runtime environment variables, see [Build Configuration and Installation](build-configuration.md).

## Prerequisites

- **CMake 3.21** or newer (`3.20+` when building QmlTui in-tree)
- **C++17** compiler
- **Qt 6.8 or newer** with `Core` and `Qml` (plus `Gui` and `Network` when enabling the optional `QmlTui.Image` module with `-DQMLTUI_ENABLE_IMAGE=ON`)

## Step 1: Create a Standalone Application

A standalone QmlTui project uses `QCoreApplication` (rather than `QGuiApplication`), `QQmlApplicationEngine`, and `qt_add_qml_module`. Create a new directory with the following three files (or copy [`examples/helloworld/`](../examples/helloworld/) from the repository).

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

## Step 2: Bind a C++ `QObject` with `QML_ELEMENT`

QmlTui uses standard Qt 6 QML type registration (`QML_ELEMENT`). You can expose C++ controllers and `QAbstractItemModel` subclasses to QML without changing `main.cpp`.

### 1. Add `CounterBackend.h`

Create `CounterBackend.h` alongside `main.cpp`:

```cpp
#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

// Exposes an integer counter to QML inside the HelloQmlTui module.
class CounterBackend : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    explicit CounterBackend(QObject* parent = nullptr) : QObject(parent) {}

    int count() const { return m_count; }

    Q_INVOKABLE void increment() {
        ++m_count;
        emit countChanged();
    }

signals:
    void countChanged();

private:
    int m_count = 0;
};
```

### 2. Register `CounterBackend.h` in `CMakeLists.txt`

Add `SOURCES CounterBackend.h` to `qt_add_qml_module` so `qmltyperegistrar` registers `CounterBackend` under the `HelloQmlTui` URI:

```cmake
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    VERSION 1.0
    QML_FILES Main.qml
    SOURCES CounterBackend.h
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)
```

### 3. Instantiate `CounterBackend` in `Main.qml`

Import `HelloQmlTui` (or use the type directly inside the same QML module) and bind the UI to `backend.count`:

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

    CounterBackend {
        id: backend
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
            text: "[ Increment Counter ]"
            focus: true
            onClicked: backend.increment()
        }

        Text {
            text: "Count: " + backend.count
            color: "magenta"
        }
    }
}
```

Rebuild and run `./build/hello_qmltui` to verify that pressing **Enter** or clicking the button increments the C++ counter and updates the terminal frame. To bind C++ `QAbstractItemModel` and `QSortFilterProxyModel` classes to `ListView`, `TableView`, and `TreeView`, see [Models in QmlTui Views](guides/models.md).

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
