# Testing and Recording with Runner

`qmltui::Runner` turns any QmlTui `Terminal` application into a deterministic, headless test target and Asciinema v2 `.cast` recorder. It drives keyboard and mouse events through the real input decoder, advances animations and timers on a virtual clock, asserts committed terminal frames (`TextGridBuffer`), and writes structured diagnostics (`result.json`, `final.txt`, `failure.txt`).

---

## 1. Conditional CMake Integration (`qmltui_add_scenario`)

Applications and examples do not need a separate test executable. Link `qmltui::runner` conditionally into your normal executable when `QMLTUI_BUILD_TESTS=ON` or `QMLTUI_ENABLE_RUNNER=ON` is configured, and register JavaScript scenarios with `qmltui_add_scenario()`:

```cmake
qt_add_executable(my_app main.cpp)
qt_add_qml_module(my_app
    URI MyApp
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)
target_link_libraries(my_app PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(TARGET qmltui::runner)
    target_link_libraries(my_app PRIVATE qmltui::runner)

    qmltui_add_scenario(
        NAME my_app_smoke
        TARGET my_app
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 80 ROWS 24
        CLOCK VIRTUAL
        LABELS AgentSmoke
    )
endif()
```

Linking `qmltui::runner` installs a lightweight startup hook for QML-created `Terminal` roots. Normal interactive startup is completely unaffected unless a Runner environment variable or `qmltui-runner` CLI session is active. (If your application constructs `qmltui::Terminal` directly in C++, call `qmltui::Runner::startFromEnvironment(&engine, terminal)` after creating the root.)

---

## 2. Writing a JavaScript Scenario

A scenario script (`*.js`) defines `function main(terminal)` (or `export function main(terminal)` in an `.mjs` module). Annotate the function with `/** @param {Terminal} terminal */` to get full IDE autocompletion:

```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    // 1. Verify the initial committed frame against a checked-in snapshot
    terminal.expectSnapshot("snapshots/initial.txt");

    // 2. Drive keyboard or mouse input (waits for the resulting committed frame)
    terminal.press("Space");
    terminal.expectText("Logging enabled");

    // 3. Assert cell-level styling and attributes at (x, y)
    terminal.expectStyle(2, 1, { foreground: "cyan", bold: true });

    // 4. Advance virtual time for animations, timers, or Toast lifetimes
    terminal.advanceTime(250);

    // 5. Save an artifact under the scenario's artifact directory
    terminal.captureText("after_toggle.txt");
}
```

### Virtual Clock (`CLOCK VIRTUAL`)

When registered with `CLOCK VIRTUAL` (or `--clock virtual`), QmlTui timers, `NumberAnimation`, `ColorAnimation`, cursor blink, `Toast` timeouts, `AnimatedSprite`, and QML `Timer` items run on a frozen timeline:

- `terminal.advanceTime(durationMs)` (and `terminal.sleep(durationMs)` under the virtual clock) steps QmlTui time forward deterministically in 16 ms increments without wall-clock delays.
- Multi-second animations and timed transitions execute in milliseconds with zero CI flakiness.

### Assertions and Snapshot Updates

Always assert visible behavior against the committed terminal frame rather than internal QML properties:

- **Frame text**: `terminal.expectText("Ready")`, `terminal.expectNoText("Error")`, `terminal.expectRegex("CPU:\\s+\\d+%")`, `terminal.waitForText("Connected", 3000)`.
- **Scoped regions**: `terminal.region(0, 0, 80, 3).expectText("HEADER")` or `terminal.item("sidebar").clickText("Logs")`.
- **Golden snapshots**:
  - `terminal.expectSnapshot("snapshots/initial.txt")` compares normalized UTF-8 frame text.
  - `terminal.expectStyled("snapshots/initial_styled.txt")` compares both text and per-run `#RRGGBBAA` colors and SGR attributes (`bold`, `dim`, `italic`, `underline`, `reverse`).
- **Inline scrollback (`Terminal.Inline`)**:
  - `terminal.expectScrollbackText("Build succeeded")`, `terminal.waitForScrollbackText(...)`, and `terminal.expectScrollbackSnapshot("snapshots/history.txt", { fromRow: mark })` verify rows committed through `Scrollback`.

To create or refresh snapshot baselines after an intentional UI change, run CTest with `QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1`:

```bash
QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest --test-dir build -R my_app_smoke --output-on-failure
```

Every updated snapshot file is reported in the test summary and recorded in `result.json`. Inspect the diff with `git diff` before committing.

---

## 3. Recording Deterministic Asciinema `.cast` Demos

Pass `RECORD <filename>.cast` to `qmltui_add_scenario()` with `CLOCK VIRTUAL` to capture real ANSI terminal output into a byte-reproducible [asciicast v2](https://docs.asciinema.org/manual/asciicast/v2/) recording:

```cmake
qmltui_add_scenario(
    NAME my_app_cast
    TARGET my_app
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_record.js"
    COLUMNS 80 ROWS 24
    CLOCK VIRTUAL
    RECORD my_demo.cast
)
```

Inside the recording script, pace interactions with `terminal.sleep(ms)` / `terminal.advanceTime(ms)` and annotate chapters with `terminal.marker(label)`:

```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.marker("Initial view");
    terminal.sleep(600);

    terminal.marker("Toggle logging");
    terminal.press("Space");
    terminal.sleep(800);

    terminal.marker("Open confirmation dialog");
    terminal.clickText("Apply settings");
    terminal.sleep(1000);
}
```

Because virtual time advances only when requested, the generated `.cast` file has no wall-clock jitter and can be converted to GIF or SVG with `agg` or `svg-term`.

---

## 4. JavaScript Scenario Editor Support

QmlTui ships TypeScript declarations (`qmltui-runner.d.ts`) covering the entire `Terminal` test facade and `RegionLocator`. Enable autocomplete in consumer projects with a single CMake call after `find_package(QmlTui)`, `FetchContent_MakeAvailable(QmlTui)`, or `add_subdirectory()`:

```cmake
if(COMMAND qmltui_scenario_editor_support)
    qmltui_scenario_editor_support()
endif()
```

This copies `.qmltui/qmltui-runner.d.ts` into your project root and generates a `jsconfig.json` (when none exists) so VS Code, Qt Creator, Neovim (`ts_ls`), and Helix provide method completion and parameter docs whenever a scenario starts with `/** @param {Terminal} terminal */`.

---

## 5. Launching Scenarios from the Shell (`qmltui-runner`)

Use the `qmltui-runner` CLI launcher (built when `QMLTUI_BUILD_TESTS=ON` or `QMLTUI_ENABLE_RUNNER=ON`) to run ad-hoc scripts, one-line evaluations, or frame captures against any Runner-linked binary without registering a CTest target:

```bash
# Run a scenario script and write artifacts (result.json, final.txt) to ./run-artifacts
qmltui-runner --script test_smoke.js --artifacts ./run-artifacts -- ./build/my_app

# Capture the first committed frame (capture.txt and capture_styled.txt)
qmltui-runner --capture-after-ms 0 --columns 80 --rows 24 -- ./build/my_app

# Record a deterministic asciicast from a virtual-clock script
qmltui-runner --script test_record.js --clock virtual --record demo.cast -- ./build/my_app
```

See the [Runner C++ Reference](../cpp-classes/runner.md) for the complete list of `terminal.*` methods, `QMLTUI_RUNNER_*` environment variables, and `result.json` schema details.
