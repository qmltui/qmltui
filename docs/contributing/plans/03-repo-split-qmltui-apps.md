# Plan 03: Showcase Repository Split (`qmltui-apps`)

## 1. Goal

Move the eight showcase apps kept in Plan 02 out of `examples/apps/` into a
dedicated `qmltui-apps` repository that consumes QmlTui the same way a newcomer does. The
core repository keeps the engine, the 31 component examples, the 11 concept
examples, the quickstart, and a self-contained test suite.

Since nothing is published, there is no compatibility to preserve. Delete and
rewire freely, but do it in one PR per repository so each stays green.

Ordering: this plan runs after Plans 00 to 02 because the site and casts are
produced from the in-tree build first. The `v0.4.0` tag from Plan 00 must
exist before `qmltui-apps` can fetch it.

---

## 2. Core Repository Changes (`QmlTui`)

### A. Root `CMakeLists.txt`

Remove the `elseif(QMLTUI_BUILD_TESTS)` branch (currently lines 111 to 120) that
pulls `industrial_plc_hmi` and `pos_cashier` in for tests. Keep
`add_subdirectory(examples)` under `QMLTUI_BUILD_EXAMPLES`.

### B. `examples/CMakeLists.txt`

Remove `add_subdirectory(apps)`. Keep `components`, `concepts`, and
`quickstart`.

### C. `tests/`

The test tree mirrors `src/QmlTui` since 2026-09-13: one CMake file per module
under `tests/{Core,Controls,Image,TestRunner,TerminalHost}`, with the shared
helpers in `tests/CMakeLists.txt`. Two groups of tests currently depend on apps:

1. The capture scenario `test_runner_capture` in `tests/TestRunner/CMakeLists.txt`
   runs against `qmltui_pos_cashier` and asserts on the text "RapidPOS". Point it
   at `qmltui_test_runner_fixture` and assert on "TESTRUNNER FIXTURE" instead.
2. The per-app scenarios, which are the whole of `tests/Examples/CMakeLists.txt`.
   Delete that directory and its `add_subdirectory(Examples)` line. They are
   recreated in `qmltui-apps` (Section 3D).

Add an in-tree Behavior test so the Behavior gate no longer depends on
`industrial_plc_hmi`:

```cmake
if(QMLTUI_ENABLE_BEHAVIOR)
    add_executable(test_behavior Core/test_behavior.cpp)
    target_link_libraries(test_behavior PRIVATE Qt6::Core Qt6::Qml Qt6::Test qmltui::core qmltui::controls)
    add_test(NAME test_behavior COMMAND test_behavior)
endif()
```

`test_behavior.cpp` loads a QML snippet with `Behavior on x { NumberAnimation {} }`,
changes `x`, pumps the event loop, and asserts the value moves through
intermediate frames. `custom_cpp_item_smoke` already fails with "Behavior is not
a type" in Behavior-disabled trees; keep that as the negative check.

### D. `tests/Core/benchmark_views.cpp`

Remove the import path pointing at `examples/apps/container_fleet` (lines 174
to 175).

### E. Delete `examples/apps/`

The eight apps that were not kept (`process_monitor`, `system_dashboard`,
`service_monitor`, `container_fleet`, `din_gateway`, `device_cockpit`,
`spaceship_game`, and `starter_template` as a showcase) are deleted in the core
repository before the move. `service_monitor`'s two idle-budget scenarios move
to `tests/` against `qmltui_test_runner_fixture` since they measure the engine,
not the app. After `qmltui-apps` is green, `git rm -r examples/apps`. Update the references
that remain so the tree is clean, not because anything downstream needs them:

- `README.md` showcase section: link to `qmltui-apps`.
- `.agents/skills/qmltui-dev-loop/SKILL.md` and
  `.agents/skills/qmltui-qml/check-skills.py`: drop app paths, add the
  `qmltui-apps` checkout convention.
- `examples/apps/markdown_viewer/resources/qmltui_guide.md` moves with the app.

---

## 3. New Repository (`qmltui-apps`)

Create `/home/yosri/qmltui-apps`, remote `github.com/qmltui-dev/qmltui-apps`.

### A. Root `CMakeLists.txt`

The earlier draft used `option()` for a path (it is boolean only) and
`EXCLUDE_FROM_ALL` on the QmlTui subdirectory (which skips the QML plugin
targets so every app fails at runtime). Corrected:

```cmake
cmake_minimum_required(VERSION 3.21)
project(qmltuiApps VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

option(QMLTUI_APPS_BUILD_TESTS "Build TestRunner scenarios for the apps" ON)

# QmlTui is fetched at the release tag. For local development against a
# checkout, pass -DFETCHCONTENT_SOURCE_DIR_QMLTUI=/path/to/QmlTui (CMake standard).
set(QMLTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QMLTUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QMLTUI_ENABLE_TEST_RUNNER ${QMLTUI_APPS_BUILD_TESTS} CACHE BOOL "" FORCE)
if(NOT WIN32)
    set(QMLTUI_ENABLE_TERMINAL_HOST ON CACHE BOOL "" FORCE)  # file_manager shell pane
endif()
set(QMLTUI_ENABLE_BEHAVIOR ON CACHE BOOL "" FORCE)       # industrial_plc_hmi

include(FetchContent)
FetchContent_Declare(
    QmlTui
    GIT_REPOSITORY https://github.com/qmltui-dev/qmltui.git
    GIT_TAG        v0.4.0
)
FetchContent_MakeAvailable(QmlTui)

if(QMLTUI_APPS_BUILD_TESTS)
    include(CTest)
    include(${qmltui_SOURCE_DIR}/cmake/QmlTuiTestRunner.cmake)   # qmltui_add_scenario_test()
endif()

foreach(app IN ITEMS
        industrial_plc_hmi btop markdown_viewer file_manager
        plotting_dashboard pos_cashier snake_game twozero48)
    add_subdirectory(${app})
endforeach()
```

`FETCHCONTENT_SOURCE_DIR_QMLTUI` replaces the custom `QMLTUI_SOURCE_DIR` variable.
It is the documented CMake mechanism, works for CI and local checkouts alike,
and needs no code in this file.

### B. Per-app `CMakeLists.txt`

Copied from the current tree with two changes: remove the standalone
`project()` guard blocks (the root now owns that), and keep the
`add_dependencies(... QmlTuiCoreplugin ...)` loop that `starter_template` already
uses so a `--target qmltui_btop` build brings its plugins along.

### C. `README.md`

Generated by `scripts/inventory.py` from Plan 02: category table, SVG still per
app, and one run command per app:

```bash
cmake -S . -B build
cmake --build build --target qmltui_btop
./build/btop/qmltui_btop
```

### D. Tests

Each app directory keeps its `test_*.js` scenarios and registers them with
`qmltui_add_scenario_test()` from the fetched `QmlTuiTestRunner.cmake`. This
reproduces the app scenario tests deleted from the core tree, and adds one for
`plotting_dashboard`, which has none today. Known flake:
`test_runner_twozero48` under `ctest -j`, keep it serial.

### E. CI (`.github/workflows/ci.yml`)

Matrix on `ubuntu-24.04` and `macos-14` with Qt 6.8 via
`jurplel/install-qt-action`, two jobs:

- `release`: default configure, fetches the tag. Proves the newcomer path.
- `main`: adds `-DFETCHCONTENT_SOURCE_DIR_QMLTUI` pointing at a checkout of
  `QmlTui@main`. Catches breakage before the next tag.

### F. Template repository

`starter_template` becomes its own GitHub template repository named
`qmltui-starter`, not a showcase entry. Slim it first: today it is about 2200
lines with a Wi-Fi scanner and a signal meter, which is a product, not a
starting point. Target a few hundred lines: `Main.qml` with a `TabHeader`, two
pages, one C++ model, one TestRunner scenario, and CI for Linux, macOS, and
Windows. A newcomer clicks "Use this template" and has a buildable, tested app
in under a minute. This is the single highest-leverage onboarding feature in
this plan.

### G. License

Both new repositories are `BSD-3-Clause`, following Qt's convention for
examples and Plan 00 section 8. A newcomer who clicks "Use this template"
must have nothing to read before shipping. Concretely:

- One `LICENSE` file (BSD-3-Clause) at the root of `qmltui-apps` and of
  `qmltui-starter`; SPDX header `BSD-3-Clause` on every source file; `reuse lint`
  in each CI.
- No contributor agreement. BSD code does not need to be relicensable.
- One sentence in each `README.md`, after the run command: "These apps are
  BSD-3-Clause. QmlTui itself is LGPLv3, GPLv3, or commercial; see the QmlTui
  repository." The apps link QmlTui through `FetchContent`, so a reader must not
  infer QmlTui's terms from the apps' license.
- The `FetchContent` build of QmlTui inside `qmltui-apps` links the QmlTui libraries
  as shared objects (the default). The starter template must not switch to a
  static QmlTui build silently, since that changes the user's LGPL obligations
  without telling them. If a static option is offered, the README says what it
  means.

---

## 4. Verification

```bash
# Core: both Behavior trees green with no apps present
cmake -S . -B build -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON -DQMLTUI_ENABLE_BEHAVIOR=ON
cmake --build build && ctest --test-dir build --output-on-failure
cmake -S . -B build-nobehavior -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build-nobehavior && ctest --test-dir build-nobehavior --output-on-failure

# Apps: against the local checkout, then against the tag
cd /home/yosri/qmltui-apps
cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_QMLTUI=/home/yosri/QmlTui
cmake --build build && ctest --test-dir build --output-on-failure
cmake -S . -B build-release && cmake --build build-release
```
