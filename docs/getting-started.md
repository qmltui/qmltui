# Getting Started

This tutorial walks Qt 6 developers through building a standalone QmlTui terminal application with CMake, binding a C++ `QObject` class to QML with `QML_ELEMENT`, and verifying rendered terminal frames in CTest with `Runner`.

For `find_package(QmlTui)` installation, optional modules (`QmlTui.Image`, `QmlTui.PtyView`, `Runner`), CMake cache options, and runtime environment variables, see [Build Configuration and Installation](build-configuration.md).

## Prerequisites

- **CMake 3.21** or newer and **Ninja** (recommended)
- **C++17** compiler
- **Qt 6.8 or newer** (tested across Qt 6.8 through 6.12) with `Core` and `Qml` (plus `QmlPrivate` when using `Behavior`, and `Gui` + `Network` when enabling `QmlTui.Image` with `-DQMLTUI_ENABLE_IMAGE=ON`). See the [Qt 6 Getting Started Guide](https://doc.qt.io/qt-6/gettingstarted.html) to install Qt 6.8+.

## Step 1: Create a Standalone Application

A standalone QmlTui project uses `QCoreApplication` (rather than `QGuiApplication`), `QQmlApplicationEngine`, and Qt's standard `qt_add_qml_module()`. Create a new directory with the following three files (for a minimal `QmlTui::Core`-only starter template without `QmlTui.Controls`, see [`examples/helloworld/`](../examples/helloworld/)).

### 1. `CMakeLists.txt`

With `FetchContent`, CMake downloads and builds QmlTui alongside your executable target. Calling `include(CTest)` enables `BUILD_TESTING` by default so `QmlTui::Runner` is built and ready for Step 3 (`-DBUILD_TESTING=OFF` omits it for production builds):

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qmltui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)
include(CTest)

include(FetchContent)
FetchContent_Declare(
    QmlTui
    GIT_REPOSITORY https://github.com/qmltui/qmltui.git
    GIT_TAG        main # Pin a release tag (e.g. v0.6.0) once published
)
FetchContent_MakeAvailable(QmlTui)

set(QT_QML_GENERATE_QMLLS_INI ON)

qt_add_executable(hello_qmltui main.cpp)
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    QML_FILES Main.qml
    DEPENDENCIES
        TARGET QmlTui::Core
        TARGET QmlTui::Controls
)
target_link_libraries(hello_qmltui PRIVATE
    Qt6::Core
    Qt6::Qml
    QmlTui::Core
    QmlTui::Controls
)
```

`QT_QML_GENERATE_QMLLS_INI` writes `.qmlls.ini` for editor autocomplete, and `DEPENDENCIES TARGET` orders QmlTui's `.qmltypes` generation before `qmllint` in `FetchContent` builds. When consuming an installed QmlTui package with `find_package(QmlTui 0.6 REQUIRED)`, omit `DEPENDENCIES TARGET` (installed QML metadata is resolved automatically via `QT_QML_IMPORT_PATH`; see [Build Configuration and Installation](build-configuration.md)).

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

`pragma ComponentBehavior: Bound` is standard Qt 6 QML for strict lexical scoping and typed delegates, and `import QmlTui` replaces `import QtQuick` (never import `QtQuick` in a QmlTui file). Provide an explicit keyboard quit handler on the root `Terminal`:

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
            event.accepted = true;
            Qt.quit();
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
            text: "Press Enter, or Q to quit"
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

Configure with Ninja, build, and run `hello_qmltui` in an interactive terminal:

```sh
# System Qt 6.8+ on PATH:
cmake -S . -B build -G Ninja

# Or point CMake at a specific Qt 6.8+ kit (Qt Online Installer / macOS / Windows):
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.x/gcc_64

cmake --build build
./build/hello_qmltui
```

If you configure without `-G Ninja` on a multi-config generator such as Visual Studio, run `.\build\Debug\hello_qmltui.exe`.

The terminal switches to the alternate screen and renders (a focused `Button` draws a double-line border):

```text
Welcome to QmlTui

╔═══════════════════════════╗
║ Press Enter, or Q to quit ║
╚═══════════════════════════╝

Status: ready
```

Press **Enter** or **Space** to activate the focused button and update the status text. Press **Q** or **Escape** to exit and restore the normal terminal screen.

> [!TIP]
> **Windows DLL Lookup**: On Windows, `FetchContent` shared builds place `QmlTuiCore.dll` and `QmlTuiControls.dll` in subproject directories unless unified. Set `set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")` before `FetchContent_MakeAvailable(QmlTui)` and add your Qt kit's `bin` directory to `PATH` (or run `windeployqt`).

## Step 2: Bind a C++ `QObject` and Add Reusable QML Components

Expose C++ state and slots to QML using `QML_ELEMENT` and `SOURCES` in `qt_add_qml_module()`. Do not mark `QML_ELEMENT` classes `final` (Qt's QML type registration subclasses them internally).

### 1. `AppController.h`

Create `AppController.h` next to `main.cpp`:

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class AppController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString status READ status NOTIFY countChanged)

public:
    using QObject::QObject;

    int count() const { return m_count; }
    QString status() const {
        return m_count == 0 ? QStringLiteral("Status: ready")
                            : QStringLiteral("Status: clicked %1 time(s)").arg(m_count);
    }

    Q_INVOKABLE void increment() {
        ++m_count;
        emit countChanged();
    }

    Q_INVOKABLE void reset() {
        if (m_count == 0) return;
        m_count = 0;
        emit countChanged();
    }

signals:
    void countChanged();

private:
    int m_count = 0;
};
```

### 2. Register `SOURCES` in `CMakeLists.txt`

Add `SOURCES AppController.h` to `qt_add_qml_module()`:

```cmake
qt_add_qml_module(hello_qmltui
    URI HelloQmlTui
    SOURCES AppController.h
    QML_FILES Main.qml
    DEPENDENCIES
        TARGET QmlTui::Core
        TARGET QmlTui::Controls
)
```

### 3. Bind `AppController` in `Main.qml`

Types registered in `URI HelloQmlTui` are automatically visible to `.qml` files in the same module without a self-import:

<!-- check-docs: ignore -->
```qml no-playground
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root
    color: "#0d1117"

    AppController {
        id: controller
    }

    Shortcut {
        sequence: "Ctrl+R"
        onActivated: controller.reset()
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            event.accepted = true;
            Qt.quit();
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
            text: "Increment (+1)"
            focus: true
            onClicked: controller.increment()
        }

        Text {
            text: controller.status + "  (Ctrl+R to reset)"
            color: "magenta"
        }
    }
}
```

Rebuild with `cmake --build build` and run `./build/hello_qmltui`. Pressing **Enter** or **Space** calls `AppController::increment()` and updates the bound `Text` automatically; **Ctrl+R** resets the counter.

### Adding Reusable QML Components

To split UI elements into reusable components, create an uppercase `.qml` file (such as `StatusBar.qml`) in the project directory and list it in `QML_FILES Main.qml StatusBar.qml`. Files in the module root become directly usable QML types (`StatusBar { ... }`); if you organize components into subdirectories, set `QT_QML_SOURCE_TYPENAME` on those files (see [Multi-File QML Modules and Singletons](build-configuration.md#multi-file-qml-modules-and-singletons)).

For binding C++ `QAbstractItemModel` and `QSortFilterProxyModel` classes to virtualized `ListView`, `TableView`, and `TreeView` items, see [Models in QmlTui Views](guides/models.md), [`examples/recipes/table_view/`](../examples/recipes/table_view/), and [`examples/recipes/lazy_model/`](../examples/recipes/lazy_model/).

## Step 3: Add a Headless `Runner` Test

Verify your application's rendered terminal frames in CTest without needing an interactive TTY.

### 1. `test_hello.js`

Create `test_hello.js` next to `CMakeLists.txt`:

```javascript
/** @param {Runner} app */
function main(app) {
    app.expectText("Welcome to QmlTui");
    app.expectText("Status: ready");

    app.press("Enter");
    app.expectText("Status: clicked 1 time(s)");

    app.press("r", ["Ctrl"]);
    app.expectText("Status: ready");
}
```

### 2. Register the Scenario in `CMakeLists.txt`

Append the `Runner` block to the end of `CMakeLists.txt`:

```cmake
if(BUILD_TESTING AND TARGET QmlTui::Runner)
    target_link_libraries(hello_qmltui PRIVATE QmlTui::Runner)
    qmltui_scenario_editor_support()
    qmltui_add_scenario(
        NAME hello_qmltui_smoke
        TARGET hello_qmltui
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_hello.js"
        COLUMNS 64
        ROWS 16
        CLOCK VIRTUAL
    )
endif()
```

### 3. Run CTest

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

`Runner` launches `hello_qmltui` against an in-memory terminal backend, asserts each rendered frame, and exits cleanly. Normal `./build/hello_qmltui` startup remains unaffected when launched directly. See [Testing and Recording with Runner](guides/testing.md) for snapshots and Asciinema `.cast` recording.

## Explore the Component Gallery (Optional)

Browse all 45 built-in controls, data views, layouts, and canvas primitives directly in your browser in the [Component Gallery](https://qmltui.github.io/playground.html?mode=gallery) or edit QML live in the [WebAssembly Playground](https://qmltui.github.io/playground.html).

To build and run `qmltui_gallery` locally in your terminal:

```sh
git clone https://github.com/qmltui/qmltui.git
cd qmltui
cmake -S . -B build -G Ninja -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target qmltui_gallery
./build/examples/gallery/qmltui_gallery
```

## Troubleshooting First-Hour Issues

| Symptom | Cause and Fix |
| :--- | :--- |
| `Could not find a package configuration file provided by "Qt6" (requested version 6.8)` | CMake cannot find your Qt 6.8+ kit on `PATH`. Pass `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.x/<kit>` when configuring. |
| Type conflicts or `QQuickItem` errors when loading QML | `QtQuick` was imported. Remove `import QtQuick` and use `import QtQml`, `import QmlTui`, and `import QmlTui.Controls`. |
| Items clip off-screen (`width: 400`, `height: 300`) | QmlTui coordinates are integer character cells (`80x24` is a typical viewport), not pixels. |
| `Keys.onPressed` never fires on a custom `Item` or `Rectangle` | Plain `Item` and `Rectangle` default to `Qt.NoFocus`. Set `focusPolicy: Qt.StrongFocus` and `focus: true` (`Button` and `TextInput` enable strong focus by default). |
| `console.log()` output appears only after quitting | While `Terminal.AltScreen` owns a TTY `stderr`, QmlTui buffers logs until exit so they never corrupt rendered frames. Run `./build/hello_qmltui 2> debug.log` or set `QMLTUI_LOG_FILE=debug.log` and `tail -f debug.log` in another terminal. |

## Next Steps

- **[Layouts & Cell Geometry](guides/layouts.md)**, **[Input & Focus](guides/input-and-focus.md)**, **[Styling & Colors](guides/styling-and-colors.md)**, and **[Models in QmlTui Views](guides/models.md)**: Core UI primitives, focus chains, terminal colors, and C++ model views.
- **[Editor and IDE Setup](guides/editor-setup.md)**: `qmlls`, `qmllint`, `qmlformat`, and `Runner` scenario autocomplete.
- **[Build Configuration and Installation](build-configuration.md)**: `find_package(QmlTui)`, multi-file modules, optional modules, and CMake/env options.
- **[Guides](guides/index.md)** & **[Examples and Recipes](examples.md)**: Task guides, copyable recipes, and showcase apps in [qmltui/apps](https://github.com/qmltui/apps).
