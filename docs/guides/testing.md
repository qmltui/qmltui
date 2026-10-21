# Testing and Recording with Runner

`qmltui::Runner` drives a QmlTui `Terminal` application headlessly for automated tests and Asciinema v2 `.cast` recordings. It feeds keyboard and mouse input through `InputDecoder`, advances animations and timers on a virtual clock, asserts committed terminal frames (`TextGridBuffer`), and writes structured artifacts (`result.json`, `final.txt`, `failure.txt`).

## CMake Integration (`qmltui_add_scenario`)

Applications and examples do not need a separate test binary. Link `qmltui::runner` conditionally into the normal executable when `QMLTUI_BUILD_TESTS=ON` or `QMLTUI_ENABLE_RUNNER=ON` is active, and register JavaScript scenarios with `qmltui_add_scenario()`:

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

Linking `qmltui::runner` installs a startup hook for QML-created `Terminal` roots. Normal interactive startup is unchanged unless a Runner environment variable or `qmltui-runner` CLI session is present. If an executable constructs `qmltui::Terminal` in C++, call `qmltui::Runner::startFromEnvironment(&engine, terminal)` after creating the root.

## Writing a JavaScript Scenario

A scenario script (`*.js`) defines `function main(terminal)` (or `export function main(terminal)` in an `.mjs` module). Add `/** @param {Terminal} terminal */` above `main` for editor autocomplete:

```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectSnapshot("snapshots/initial.txt");

    terminal.press("Space");
    terminal.expectText("Logging enabled");

    terminal.expectStyle(2, 1, { foreground: "cyan", bold: true });

    terminal.advanceTime(250);
    terminal.captureText("after_toggle.txt");
}
```

### Virtual Clock (`CLOCK VIRTUAL`)

When registered with `CLOCK VIRTUAL` (or `--clock virtual`), QmlTui timers, `NumberAnimation`, `ColorAnimation`, cursor blink, `Toast` timeouts, `AnimatedSprite`, and QML `Timer` items run on a synthetic clock. Calling `terminal.advanceTime(durationMs)` (or `terminal.sleep(durationMs)` under the virtual clock) steps QmlTui time forward in 16 ms increments without wall-clock delays.

### Frame Assertions and Snapshots

Assert visible behavior against the committed terminal frame:

- **Frame text**: `terminal.expectText("Ready")`, `terminal.expectNoText("Error")`, `terminal.expectRegex("CPU:\\s+\\d+%")`, `terminal.waitForText("Connected", 3000)`.
- **Scoped regions**: `terminal.region(0, 0, 80, 3).expectText("Header")` or `terminal.item("sidebar").clickText("Logs")`.
- **Snapshots**:
  - `terminal.expectSnapshot("snapshots/initial.txt")` compares normalized UTF-8 frame text.
  - `terminal.expectStyled("snapshots/initial_styled.txt")` compares both text and per-run `#RRGGBBAA` colors and SGR attributes (`bold`, `dim`, `italic`, `underline`, `reverse`).
- **Inline scrollback (`Terminal.Inline`)**:
  - `terminal.expectScrollbackText("Build succeeded")`, `terminal.waitForScrollbackText(...)`, and `terminal.expectScrollbackSnapshot("snapshots/history.txt", { fromRow: mark })` verify rows committed via `Scrollback`.

To create or update snapshot baselines after an intentional UI change, run CTest with `QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1`:

```bash
QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest --test-dir build -R my_app_smoke --output-on-failure
```

Rewritten snapshot files are listed in the test summary and in `result.json`.

## Recording Asciinema `.cast` Demos

Pass `RECORD <filename>.cast` to `qmltui_add_scenario()` with `CLOCK VIRTUAL` to record ANSI terminal output into an [asciicast v2](https://docs.asciinema.org/manual/asciicast/v2/) file:

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

Inside the recording script, pace steps with `terminal.sleep(ms)` / `terminal.advanceTime(ms)` and mark chapters with `terminal.marker(label)`:

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

## Scenario Editor Support (`qmltui-runner.d.ts`)

QmlTui installs TypeScript declarations (`qmltui-runner.d.ts`) for the `Terminal` test facade and `RegionLocator`. Call `qmltui_scenario_editor_support()` after `find_package(QmlTui)`, `FetchContent_MakeAvailable(QmlTui)`, or `add_subdirectory()`:

```cmake
if(COMMAND qmltui_scenario_editor_support)
    qmltui_scenario_editor_support()
endif()
```

This copies `.qmltui/qmltui-runner.d.ts` into the project root and generates a `jsconfig.json` when none exists.

## Running Scenarios from the Shell (`qmltui-runner`)

The `qmltui-runner` CLI launcher (built when `QMLTUI_BUILD_TESTS=ON` or `QMLTUI_ENABLE_RUNNER=ON`) runs scripts, one-line evaluations, or frame captures against any Runner-linked executable:

```bash
# Run a scenario script and write artifacts (result.json, final.txt) to ./run-artifacts
qmltui-runner --script test_smoke.js --artifacts ./run-artifacts -- ./build/my_app

# Capture the first committed frame (capture.txt and capture_styled.txt)
qmltui-runner --capture-after-ms 0 --columns 80 --rows 24 -- ./build/my_app

# Record an asciicast from a virtual-clock script
qmltui-runner --script test_record.js --clock virtual --record demo.cast -- ./build/my_app
```

See the [Runner C++ Reference](../cpp-classes/runner.md) for all `terminal.*` methods, `QMLTUI_RUNNER_*` environment variables, and the `result.json` schema.
