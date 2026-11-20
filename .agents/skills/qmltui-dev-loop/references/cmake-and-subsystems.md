# CMake Integration, Demo Recording, and Optional Subsystems

Reference patterns for wiring `qmltui::runner` across consumer and in-tree CMake projects, recording deterministic Asciinema `.cast` / `.gif` demos, and verifying specialized QmlTui subsystems.

## 1. Consumer vs. In-Repo CMake Setup

Link `qmltui::runner` conditionally on the **existing** executable under `if(BUILD_TESTING AND TARGET qmltui::runner)`—never duplicate the executable or QML module target for tests. Production builds (`-DBUILD_TESTING=OFF`) omit Runner automatically.

In consumer projects supporting both `find_package(QmlTui)` (`IMPORTED` targets) and `FetchContent` (`DEPENDENCIES TARGET`), call `include(CTest)` **before** `FetchContent_MakeAvailable(QmlTui)` (so `BUILD_TESTING=ON` enables `QMLTUI_ENABLE_RUNNER` automatically) and branch on `IMPORTED` because Qt rejects `IMPORTED` targets inside `qt_add_qml_module(... DEPENDENCIES)`:

```cmake
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

# Must precede FetchContent_MakeAvailable(QmlTui) so qmltui::runner builds by default
# (passing -DBUILD_TESTING=OFF omits Runner cleanly for production builds).
include(CTest)
include(FetchContent)
FetchContent_Declare(QmlTui
    GIT_REPOSITORY https://github.com/qmltui/qmltui.git
    GIT_TAG        v0.6.0
)
FetchContent_MakeAvailable(QmlTui)
set(QT_QML_GENERATE_QMLLS_INI ON)

set(app_qml_module_args)
get_target_property(qmltui_is_imported qmltui::core IMPORTED)
if(qmltui_is_imported)
    if(QmlTui_QML_IMPORT_PATH)
        list(APPEND app_qml_module_args IMPORT_PATH "${QmlTui_QML_IMPORT_PATH}")
    endif()
else()
    list(APPEND app_qml_module_args
        DEPENDENCIES
            TARGET qmltui::core
            TARGET qmltui::controls
    )
endif()

# Optional QML singleton tokens file (`pragma Singleton`):
# set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(my_app main.cpp)
qt_add_qml_module(my_app
    URI MyApp
    # SOURCES MyModel.h MyModel.cpp
    QML_FILES Main.qml
    ${app_qml_module_args}
)
target_link_libraries(my_app PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(BUILD_TESTING AND TARGET qmltui::runner)
    target_link_libraries(my_app PRIVATE qmltui::runner)
    qmltui_scenario_editor_support()
    qmltui_add_scenario(
        NAME my_app_smoke
        TARGET my_app
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 80
        ROWS 24
        CLOCK VIRTUAL
    )
endif()
```

Minimal consumer `main.cpp` (uses `QCoreApplication`, never `QGuiApplication`):

```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("MyApp", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    return app.exec();
}
```

### `qmltui_add_scenario` Options

| Keyword | Default | Purpose |
| :--- | :--- | :--- |
| `NAME <name>` | Required | CTest name (artifacts write to `<binary-dir>/RunnerArtifacts/<NAME>` unless `ARTIFACT_DIR` is set). |
| `TARGET <target>` | Required | Executable target linked with `qmltui::runner`. |
| `SCRIPT <path>` | Required unless `EMBEDDED` | `.js`, `.mjs`, or `:/...` scenario script. |
| `EMBEDDED` | Off | Target constructs `qmltui::Runner` directly in C++ (mutually exclusive with `SCRIPT`). |
| `COLUMNS <n>`, `ROWS <n>` | `80`, `24` | Initial viewport size (`1..1000`). |
| `CLOCK <REAL\|VIRTUAL>` | `REAL` | Prefer `VIRTUAL` for deterministic QML timers, animations, and `.cast` pacing. |
| `RECORD <rel.cast>` | None | Records asciicast v2 into `${QMLTUI_RECORD_DIR}` (or `ARTIFACT_DIR`) and defaults CTest label to `Demo`. |
| `POST_COMMAND <cmd>...` | `${QMLTUI_RECORD_POST_COMMAND}` when `RECORD` set | Post-processing command; expands `<CAST>`, `<CAST_STEM>`, `<ARTIFACT_DIR>`, `<NAME>`. |
| `ARGUMENTS <arg>...` | None | Extra CLI arguments forwarded to `TARGET`. |
| `OPERATION_TIMEOUT_MS <ms>` | `2000` | Per-operation timeout (`1..30000` ms). |
| `SCENARIO_TIMEOUT_MS <ms>` | `30000` | Overall watchdog timeout (`1..300000` ms). |
| `UPDATE_SNAPSHOTS` / `NO_UPDATE_SNAPSHOTS` | Env-inherited | Pin snapshot baseline updating on or off (for negative mismatch tests). |
| `LIVE` | Off | Render ANSI to real terminal (incompatible with `RECORD`; avoid in CI). |
| `LABELS <label>...` | `Demo` (`RECORD`) else `Runner` | CTest labels. |

### C++-Created `Terminal` & `EMBEDDED` Runners

- **C++-constructed `Terminal` (no QML root)**: QML-created `Terminal` items auto-register the environment hook when `qmltui::runner` is linked. If `main.cpp` constructs `qmltui::Terminal` directly in C++, call `qmltui::Runner::startFromEnvironment(&engine, terminal)` after creating the root (see `examples/recipes/cpp_only/main.cpp` in the QmlTui repository).
- **Directly constructed `EMBEDDED` runner**: Call `qmltui::Runner::useEmbeddedScenario()` before QML loads (keeps terminal startup quiet unless `QMLTUI_RUNNER_LIVE=1`), construct `qmltui::Runner` with explicit `QQmlEngine*`, `Terminal*`, `:/...` resource script path, artifact dir, `QSize(cols, rows)`, and timeouts, then schedule `QTimer::singleShot(0, driver, &qmltui::Runner::start)` and register with `qmltui_add_scenario(... EMBEDDED)` (see `tests/Runner/main.cpp` in the QmlTui repository).

---

## 2. Deterministic Asciinema `.cast` & GIF Demo Recording

`Runner` captures real emitted 24-bit ANSI diff bytes in memory as [asciicast v2](https://docs.asciinema.org/manual/asciicast/v2/) without a TTY or desktop screen recorder:

1. **Ad-hoc CLI recording**:
   ```bash
   qmltui-runner --script tour.js --clock virtual --columns 90 --rows 28 \
     --record tour.cast --artifacts /tmp/demo-cast -- ./build/my_app
   ```
   Under `--clock virtual`, timestamps advance strictly via `app.sleep(ms)` / `app.advanceTime(ms)`, producing byte-identical `.cast` files across machines. Use `app.marker("Step title")` for chapter markers, and `app.stopRecording()` / `app.startRecording("part2.cast")` when splitting clips.
2. **GIF rendering with `agg`**:
   Convert `.cast` to `.gif` with Asciinema's `agg` (`agg --renderer resvg --font-size 24 --line-height 1.08 tour.cast tour.gif`) or register `RECORD tour.cast` in CMake and run `ctest --test-dir build -L Demo --output-on-failure`.

---

## 3. Subsystem-Specific Verification Rules

### Custom C++ `Item` Subclasses (`paint()` & Layout)
- Implement integer-cell rendering in `Item::paint(TextGridBuffer &buffer, ...)` without calling `markDirty()` inside `paint()`.
- **Unit level**: In the QmlTui repository, use `qmltui::testing::paintItem(item, buffer, 0, 0)` from `tests/TestPaint.h` inside a Qt Test executable (`add_qmltui_test`) to assert cell characters, colors, and clip bounds directly. In consumer projects, instantiate the custom QML item in a minimal fixture and assert with `app.cell(x, y)`, `app.expectStyle()`, or `app.expectStyled()`.

### Private Qt Feature Gating (`Behavior` & Advanced `State`s)
- `QMLTUI_ENABLE_PRIVATE_QT=ON` (default when `Qt6::QmlPrivate` headers exist) enables `Behavior on <prop>` and binding-aware `PropertyChanges`.
- Whenever touching `QmlPrivateBridge`, `Behavior`, `State`, `PropertyChanges`, or CMake feature gating, configure and test **two separate build trees**:
  1. `build` (`-DQMLTUI_ENABLE_PRIVATE_QT=ON`)
  2. `build-no-private` (`-DQMLTUI_ENABLE_PRIVATE_QT=OFF` — note `qmltui_independent_states_demo` is intentionally omitted here).

### `QmlTui.Image` (`-DQMLTUI_ENABLE_IMAGE=ON`)
- Requires `Qt6::Gui` and `Qt6::Network`.
- Headless `TextGridBuffer` captures verify fallback cell rendering and layout geometry; Kitty graphics protocol transmission is covered by unit tests (`test_kitty_graphics` in-repo) and `QMLTUI_KITTY_GRAPHICS`.
- `AnimatedSprite` frame stepping honors `CLOCK VIRTUAL` (`advanceTime()`), whereas asynchronous `ImageProvider` / network loads run on worker threads and require `CLOCK REAL` + `waitForCondition()` / `waitForText()`.

### `QmlTui.PtyView` (`-DQMLTUI_ENABLE_PTYVIEW=ON`)
- Supported on **Linux and macOS only** (never add unguarded Unix PTY dependencies to cross-platform targets).
- Because `PtyView` spawns a real OS pseudo-terminal child process, scenarios exercising `PtyView` must run on `CLOCK REAL` and synchronize on shell output with `app.waitForText(...)`, never `CLOCK VIRTUAL` `advanceTime()` alone.

### Browser / Qt for WebAssembly (`qmltui_wasm_app`)
- Configure with Qt 6.8+ **`wasm_singlethread`** (`qt-cmake`) and the exact Emscripten version pinned by that Qt minor release (`4.0.7` for Qt 6.11, `5.0.5` for Qt 6.12). Do not use `wasm_multithread` or pass manual `-sASYNCIFY` / `-sMODULARIZE` flags.
- Under `EMSCRIPTEN`, `QMLTUI_ENABLE_RUNNER` and `QMLTUI_BUILD_TESTS` default to `OFF`, and `QmlTui.PtyView` is unavailable. Verify core UI logic and cell layout natively with `Runner` first, then build the Wasm target (`cmake --build build-wasm`) and smoke-test `<target>.html` over `python3 -m http.server` (ensuring `@xterm/addon-unicode-graphemes` `15-graphemes` width alignment and clean `Qt.quit()` reset).
