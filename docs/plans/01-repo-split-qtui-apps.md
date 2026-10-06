# Implementation Plan 01: Showcase Repository Decoupling (`qtui-apps`)

## 1. Goal Description

Extract the 17 production-grade applications from [`examples/apps/`](file:///home/yosri/qtui/examples/apps) into a dedicated repository located at `/home/yosri/qtui-apps`.

### Key Motivations
1. **Dogfooding External Consumption**: The showcase apps must consume Qtui via `FetchContent` or `find_package(Qtui 0.4 REQUIRED)` rather than in-tree subdirectories.
2. **Core Leanliness & Focus**: Keeps the core `qtui` repository focused on C++20 engine primitives, controls (`examples/components/`), and self-contained unit tests.
3. **Decoupled CI**: The core test suite will no longer depend on full application smoke tests or `industrial_plc_hmi` for Behavior gating.

---

## 2. Core Repository Changes (`qtui`)

### A. Root CMake Decoupling
In [`CMakeLists.txt`](file:///home/yosri/qtui/CMakeLists.txt):
- Remove lines 113–120 where `industrial_plc_hmi` and `pos_cashier` were added under `elseif(QTUI_BUILD_TESTS)`.
- Keep `add_subdirectory(examples)` gated behind `if(QTUI_BUILD_EXAMPLES)`.

### B. Examples Directory Decoupling
In [`examples/CMakeLists.txt`](file:///home/yosri/qtui/examples/CMakeLists.txt):
- Remove `add_subdirectory(apps)`.
- Retain `add_subdirectory(components)` (31 primitive controls) and `add_subdirectory(concepts)` (11 core mechanics).

### C. Test Suite Decoupling & In-Tree Behavior Test
In [`tests/CMakeLists.txt`](file:///home/yosri/qtui/tests/CMakeLists.txt):
- Remove lines 297–356 (`if(TARGET qtui_pos_cashier)` through `if(TARGET qtui_twozero48)`).
- Add a lightweight in-tree `test_behavior` target when `QTUI_ENABLE_BEHAVIOR=ON`:

```cmake
if(QTUI_ENABLE_BEHAVIOR)
    add_executable(test_behavior Core/test_behavior.cpp)
    target_link_libraries(test_behavior PRIVATE Qt6::Core Qt6::Qml Qt6::Test Qtui::Core)
    add_test(NAME test_behavior COMMAND test_behavior)
endif()
```

### D. Benchmark Path Cleanup
In [`tests/Core/benchmark_views.cpp`](file:///home/yosri/qtui/tests/Core/benchmark_views.cpp):
- Remove unused line 174–175:
  ```cpp
  engine.addImportPath(QCoreApplication::applicationDirPath()
                       + QStringLiteral("/../examples/apps/container_fleet"));
  ```

### E. Deletion of `examples/apps/`
- Once all apps are verified in `qtui-apps`, remove `examples/apps/` from the `qtui` tree.

---

## 3. Dedicated Repository Setup (`qtui-apps`)

Create `/home/yosri/qtui-apps` with the following root files:

### A. Root `CMakeLists.txt`
```cmake
cmake_minimum_required(VERSION 3.20)
project(QtuiApps VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

# Allow local checkout override or FetchContent default
option(QTUI_SOURCE_DIR "Path to local qtui source checkout" "")

if(QTUI_SOURCE_DIR)
    message(STATUS "Using local Qtui source: ${QTUI_SOURCE_DIR}")
    add_subdirectory(${QTUI_SOURCE_DIR} _deps/qtui-build EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        Qtui
        GIT_REPOSITORY https://github.com/yosriayed/Qtui.git
        GIT_TAG        v0.4.0
    )
    FetchContent_MakeAvailable(Qtui)
endif()

include(CTest)
enable_testing()

add_subdirectory(btop)
add_subdirectory(container_fleet)
add_subdirectory(industrial_plc_hmi)
add_subdirectory(pos_cashier)
add_subdirectory(process_monitor)
add_subdirectory(service_monitor)
add_subdirectory(file_manager)
add_subdirectory(markdown_viewer)
add_subdirectory(plotting_dashboard)
add_subdirectory(snake_game)
add_subdirectory(spaceship_game)
add_subdirectory(system_dashboard)
add_subdirectory(twozero48)
add_subdirectory(starter_template)
add_subdirectory(din_gateway)
add_subdirectory(agent_demo)

find_package(Qt6 COMPONENTS DBus QUIET)
if(Qt6DBus_FOUND)
    add_subdirectory(device_cockpit)
endif()
```

### B. Showcase `README.md`
- Categorized table of apps with screenshots, descriptions, and one-liner run commands:
  ```bash
  cmake -B build
  cmake --build build --target qtui_btop
  ./build/btop/qtui_btop
  ```

### C. GitHub Actions CI (`.github/workflows/ci.yml`)
- Automated matrix build on Ubuntu 24.04 and macOS 14 testing all 17 apps against the latest `qtui` release.

---

## 4. Verification Plan

```bash
# In qtui: verify all in-tree tests pass
rtk cmake -S . -B build -DQTUI_BUILD_TESTS=ON -DQTUI_BUILD_EXAMPLES=ON
rtk cmake --build build
rtk ctest --test-dir build --output-on-failure

# In qtui: verify Behavior test passes when enabled
rtk cmake -S . -B build-behavior -DQTUI_BUILD_TESTS=ON -DQTUI_ENABLE_BEHAVIOR=ON
rtk cmake --build build-behavior
rtk ctest --test-dir build-behavior -R test_behavior --output-on-failure

# In qtui-apps: verify build and tests pass against local qtui
cd /home/yosri/qtui-apps
rtk cmake -S . -B build -DQTUI_SOURCE_DIR=/home/yosri/qtui
rtk cmake --build build
rtk ctest --test-dir build --output-on-failure
```
