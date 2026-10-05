# Embedded Development, Cross-Compilation & Automated Testing Workflow

This guide details the complete development, cross-compilation, live hardware deployment, and automated testing lifecycle for **Qtui** terminal applications on Embedded Linux.

---

## 1. Architecture & Portability

Qtui is a C++20 and Qt 6.8+ retained-mode terminal UI framework. Applications are written entirely in standard QML and C++ with **no hardcoded dependencies on any specific build tree or hardware architecture**.

```
                   ┌──────────────────────────────────────────────┐
                   │           Your Qtui Application              │
                   │        (main.cpp, Main.qml, C++ models)      │
                   └──────────────────────┬───────────────────────┘
                                          │
                  ┌───────────────────────┴───────────────────────┐
                  ▼                                               ▼
    ┌───────────────────────────┐                   ┌───────────────────────────┐
    │    Host / CI Execution    │                   │  Any Embedded Yocto Target│
    │  (Linux / macOS Desktop)  │                   │ (ARM64 / ARM32 / RISC-V)  │
    ├───────────────────────────┤                   ├───────────────────────────┤
    │ • Native GCC / Clang      │                   │ • Standard Yocto SDK      │
    │ • Desktop Qt 6.8+         │                   │ • Target Sysroot & Libs   │
    │ • Local TestRunner / CTest│                   │ • Remote SSH Test Runner  │
    └───────────────────────────┘                   └───────────────────────────┘
```

---

## 2. Standalone Application Template

You can create a Qtui application in any directory on your filesystem.

### Directory Structure
```
my_qtui_app/
├── CMakeLists.txt
├── main.cpp
├── Main.qml
└── tests/
    └── smoke_test.js
```

### `CMakeLists.txt`
```cmake
cmake_minimum_required(VERSION 3.20)
project(my_qtui_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 1. Locate Qt6 and Qtui packages
find_package(Qt6 REQUIRED COMPONENTS Core Qml)
find_package(Qtui REQUIRED)

qt_add_executable(my_qtui_app main.cpp)
qt_add_qml_module(my_qtui_app
    URI MyQtuiApp
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES Qtui
)

target_link_libraries(my_qtui_app PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Controls
)

# 2. Optional: Enable Headless Automated Scenario Testing
if(TARGET Qtui::Testing)
    target_link_libraries(my_qtui_app PRIVATE Qtui::Testing)
endif()

enable_testing()
find_file(QTUI_TESTING_CMAKE "QtuiTesting.cmake" PATHS ${Qtui_DIR} ${CMAKE_MODULE_PATH})
if(QTUI_TESTING_CMAKE)
    include("${QTUI_TESTING_CMAKE}")
    qtui_add_scenario_test(
        NAME smoke_test
        TARGET my_qtui_app
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_test.js"
        COLUMNS 100
        ROWS 30
    )
endif()
```

### `main.cpp`
```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("my_qtui_app");

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    engine.addImportPath(QStringLiteral("/usr/lib/qml"));

    engine.loadFromModule("MyQtuiApp", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
```

### `Main.qml`
```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root
    fillTerminal: true
    color: "#000000"

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        border.style: "single"
        border.color: "#225c38"

        Column {
            anchors.centerIn: parent
            spacing: 1

            Text {
                text: qsTr("STANDALONE EMBEDDED QTUI APP")
                color: "#00ffcc"
                bold: true
            }

            Text {
                text: qsTr("Press [Q] or [ESC] to quit")
                color: "#888888"
            }
        }
    }
}
```

---

## 3. Cross-Compiling with Any Yocto Toolchain

### Option A: Using a Standard Yocto Relocatable SDK (Recommended for CI / Production)

1. **Generate the SDK Installer** (run in your Yocto environment):
   ```bash
   bitbake -c populate_sdk <image-name>
   # or with kas:
   kas build meta-qtui/kas/imx93-evk.yml -c populate_sdk
   ```

2. **Install the SDK** on any build server or workstation:
   ```bash
   ./build/tmp/deploy/sdk/poky-*-toolchain-*.sh -d /opt/qtui-sdk
   ```

3. **Source and Build**:
   ```bash
   source /opt/qtui-sdk/environment-setup-cortexa55-poky-linux

   cd /path/to/my_qtui_app
   cmake -B build-arm64 -G Ninja
   cmake --build build-arm64
   ```

### Option B: Using the Workspace Toolchain Helper (Direct Yocto Workspace)

If working directly from a checked-out Yocto build directory:
```bash
# Source the helper environment
source meta-qtui/scripts/environment-setup-arm64.sh

# Or use the one-line cross-CMake wrapper
meta-qtui/scripts/qtui-cross-cmake.sh -S /path/to/my_qtui_app -B /path/to/my_qtui_app/build-arm64
ninja -C /path/to/my_qtui_app/build-arm64
```

---

## 4. Live Fast Dev-Loop Tool (`qtui-dev.sh`)

To eliminate slow SD-card flashing, use `meta-qtui/scripts/qtui-dev.sh` for sub-second cross-compilation, deployment, and live execution on physical terminals.

### Common Commands

```bash
# Build, deploy, and run an external application on the physical HDMI/DSI screen (/dev/tty1)
./meta-qtui/scripts/qtui-dev.sh dev /path/to/my_qtui_app 192.168.1.35

# Build, deploy, and run any in-tree example (e.g. snake_game, system_dashboard)
./meta-qtui/scripts/qtui-dev.sh dev snake_game 192.168.1.35

# Deploy and run a pre-compiled binary
./meta-qtui/scripts/qtui-dev.sh dev /path/to/binary 192.168.1.35

# Run interactively inside an SSH session (for development & terminal debugging)
./meta-qtui/scripts/qtui-dev.sh run-ssh snake_game 192.168.1.35

# Restore the default production autostart service on /dev/tty1
./meta-qtui/scripts/qtui-dev.sh restart-tty1 192.168.1.35

# Check target status, IP, and active services
./meta-qtui/scripts/qtui-dev.sh status 192.168.1.35
```

---

## 5. Automated Testing Strategy

Qtui provides automated testing across three tiers:

```
                      ┌─────────────────────────────────┐
                      │    Tier 1: Unit & Logic Tests   │  Fast (< 1s), Host/CI
                      │         (QTest / CTest)         │  No UI / No hardware
                      └────────────────┬────────────────┘
                                       │
                      ┌────────────────▼────────────────┐
                      │ Tier 2: Headless Scenario Tests │  Interactive PTY simulation
                      │  (Qtui::Testing / TestRunner)   │  Frame & snapshot assertions
                      └────────────────┬────────────────┘
                                       │
                      ┌────────────────▼────────────────┐
                      │  Tier 3: On-Target Integration  │  Live hardware over SSH
                      │   (Frame Latency & Benchmarks)  │  Physical console verification
                      └─────────────────────────────────┘
```

### Writing a Headless Scenario Test (`tests/smoke_test.js`)

`Qtui::Testing` / `TestRunner` provides a headless JavaScript execution engine that inspects committed terminal frames:

```javascript
// 1. Assert initial frame content
Test.expectText("STANDALONE EMBEDDED QTUI APP");

// 2. Simulate user input
Test.press("Space");
Test.expectText("CLICKS: 1");

// 3. Navigate tabs
Test.press("2");
Test.expectText("DIAGNOSTICS");

// 4. Capture text snapshot artifact for CI review
Test.captureText("artifacts/diagnostics-frame.txt");

// 5. Exit application cleanly
Test.press("q");
```

### Running Tests Locally on Host / CI (No Hardware Required)
```bash
cmake -B build -DQTUI_BUILD_TESTS=ON -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

### Running Automated Scenario Tests on Physical Target Hardware over SSH
Execute scenarios in headless PTY mode on the real hardware:
```bash
ssh root@192.168.1.35 \
  "QTUI_EVAL='Test.expectText(\"STANDALONE EMBEDDED QTUI APP\"); \
              Test.press(\"Space\"); \
              Test.expectText(\"CLICKS: 1\"); \
              Test.captureText(\"frame.txt\"); \
              Test.press(\"q\");' \
   QTUI_TEST_COLUMNS=100 QTUI_TEST_ROWS=30 \
   /usr/bin/my_qtui_app"
```

### Running the End-to-End Latency & Performance Benchmark
Measure key-to-frame render times, cold startup, and SSH packet latency:
```bash
python3 meta-qtui/scripts/measure-ssh-tui-latency.py root@192.168.1.35
```
* Asserts ARM Cortex-A55 key-to-frame local render latency (< 50 ms)
* Asserts Wi-Fi / SSH settle latency (< 75 ms)
* Produces machine-readable JSON reports (`result.json`) for CI performance tracking
