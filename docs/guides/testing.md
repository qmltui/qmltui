# Testing and Recording with Runner

`qmltui::Runner` drives a QmlTui `Terminal` application headlessly for automated tests, interactive debugging, and Asciinema v2 `.cast` recordings. It feeds keyboard and mouse input through `InputDecoder`, advances animations and timers on a virtual clock, asserts committed terminal frames (`TextGridBuffer`), and writes structured artifacts (`result.json`, `final.txt`, `failure.txt`).

When an executable links `qmltui::runner`, QML-created `Terminal` roots automatically install a startup hook. Normal interactive startup is unchanged unless you launch the binary through `qmltui-runner` or set `QMLTUI_RUNNER_*` environment variables. (If you construct `qmltui::Terminal` in C++ without QML, call `qmltui::Runner::startFromEnvironment(&engine, terminal)` after creating the root.)

## 1. Write a JavaScript Scenario

A scenario script defines `function main(app)` in a `.js` file, or `export function main(app)` in an `.mjs` ES module (which can `import` shared helper modules using relative paths). Add `/** @param {Runner} app */` above `main` to enable IDE autocomplete and hover help:

```javascript
/** @param {Runner} app */
function main(app) {
    app.expectSnapshot("snapshots/initial.txt");

    app.press("Tab");
    app.typeText("api-gateway");
    app.press("Enter");

    app.expectText("Filter: api-gateway");
    app.expectStyle(2, 1, { foreground: "cyan", bold: true });

    app.advanceTime(250);
    app.captureText("after_filter.txt");
}
```

Every `app.*` method is synchronous and blocks inside a nested Qt event loop until the resulting terminal frame is committed. If a scenario returns from `main(app)` without failed assertions or uncaught errors, it passes and exits with code `0`.

### Simulate Keyboard, Clipboard, and Mouse Input

Input methods route through the same `InputDecoder` as a live terminal and wait for the next committed frame:

| Operation | Example | Behavior |
| :--- | :--- | :--- |
| **Single key or shortcut** | `app.press("Enter")`<br>`app.press("c", ["Ctrl"])`<br>`app.press("Backtab")` | Sends a named key (`Enter`, `Escape`, `Tab`, `Backtab`, `Space`, `Backspace`, `Up`, `Down`, `Left`, `Right`, `Home`, `End`, `PageUp`, `PageDown`, `F1`..`F35`) or a single character with optional `["Shift", "Ctrl", "Alt", "Meta"]` modifiers. |
| **Typed text** | `app.typeText("hello 世界")` | Decodes UTF-8 characters into per-character key events and waits once for the committed frame. |
| **Bracketed paste** | `app.paste("line 1\nline 2")` | Delivers multi-line text as a single bracketed paste so `\n` and `\t` insert text instead of triggering `Return` or `Tab` navigation. |
| **Click rendered text** | `app.clickText("Apply")` | Finds the visible text match on screen, clicks its center cell, and waits for the committed frame. |
| **Coordinate mouse actions** | `app.click(10, 5)`<br>`app.doubleClick(10, 5)`<br>`app.drag(5, 5, 25, 5)`<br>`app.scroll(15, 10, 0, 120)` | Sends SGR mouse press/release, double-click, button-motion drag, hover (`app.hover(x, y)`), or wheel scroll events at cell coordinates `(x, y)`. |
| **Viewport resize** | `app.resize(100, 30)` | Resizes the terminal viewport to `columns` × `rows` and waits for the reflowed frame. |

### Assert Committed Frames, Regions, and Styles

Visible-behavior tests should assert the committed terminal frame rather than internal QML properties:

- **Immediate frame text assertions**:
  - `app.expectText("Ready")` and `app.expectNoText("Error")` check the next committed frame for a substring.
  - `app.expectRegex("CPU:\\s+\\d+%")` checks the frame against a regular expression.
- **Asynchronous waits**:
  - `app.waitForText("Connected", 3000)` and `app.waitForRegex("DONE \\(\\d+ ms\\)", 3000)` observe committed frames until the text appears or the timeout expires.
- **Scoped region and item locators**:
  - `app.region(0, 0, 80, 3).expectText("Header")` restricts text checks, snapshots, and clicks to a cell rectangle `[x, y, width, height]`.
  - `app.item("sidebar").clickText("Logs")` and `app.item("sidebar").expectText("Logs")` scope operations to the viewport bounds of the visible QML item whose `objectName` is `"sidebar"`.
- **Cell style assertions**:
  - `app.expectStyle(2, 1, { character: "●", foreground: "green", bold: true })` checks the glyph, resolved `#RRGGBBAA` or named color (`"default"`, `"cyan"`, etc.), and SGR attributes (`bold`, `dim`, `italic`, `underline`, `reversed`, `strikethrough`) at cell `(x, y)`.
- **Golden snapshots**:
  - `app.expectSnapshot("snapshots/initial.txt")` compares normalized UTF-8 frame text against a baseline file resolved relative to the scenario script.
  - `app.expectStyled("snapshots/initial_styled.txt")` compares both frame text and per-run `#RRGGBBAA` colors and SGR attributes. On mismatch, Runner writes `snapshots/<name>.actual.txt` and a unified `snapshots/<name>.diff` into the artifact directory.

### Step Animations and Timers with the Virtual Clock

When you run a scenario with `--clock virtual` (or `CLOCK VIRTUAL` in CMake), QmlTui freezes application time. `NumberAnimation`, `ColorAnimation`, `PropertyAnimation`, cursor blink, `Toast` and `ToolTip` delays, `AnimatedSprite`, and QML `Timer` items advance only when your script steps the clock:

```javascript
/** @param {Runner} app */
function main(app) {
    app.press("Space"); // starts a 300 ms NumberAnimation

    app.advanceTime(150);
    app.captureText("mid_animation.txt");

    app.advanceTime(150);
    app.expectText("Animation complete");
}
```

`app.advanceTime(durationMs)` (and `app.sleep(durationMs)` when running on the virtual clock) steps time in at most 16 ms increments, firing timers at their exact virtual deadlines without wall-clock delay or CI flakiness.

### Assert Inline Scrollback (`Terminal.Inline`)

In `Terminal.Inline` applications, completed output is committed above the live viewport through `Scrollback`. Use the scrollback methods to verify emitted history:

```javascript
/** @param {Runner} app */
function main(app) {
    const mark = app.scrollbackRowCount();

    app.typeText("build --release");
    app.press("Enter");

    app.expectScrollbackText("Build succeeded");
    app.expectScrollbackSnapshot("snapshots/build_log.txt", { fromRow: mark });
}
```

## 2. Run Scenarios with the `qmltui-runner` CLI

The `qmltui-runner` command-line tool (built by default in top-level builds or when `-DQMLTUI_ENABLE_RUNNER=ON` is set) launches any `qmltui::runner`-linked executable, manages the activation handshake and timeouts, and verifies the resulting `result.json`.

### Common CLI Workflows

1. **Run a scenario script headlessly**:
   ```bash
   qmltui-runner --script test_smoke.js --artifacts ./run-artifacts -- ./build/my_app
   ```
   Anything after `--` is the target executable followed by its own command-line arguments.

2. **Run a one-line inline check (`--eval`)**:
   ```bash
   qmltui-runner --eval 'app.expectText("Welcome"); app.press("Enter"); app.expectText("Clicked!");' -- ./build/my_app
   ```

3. **Capture the rendered frame without a script (`--capture-after-ms`)**:
   ```bash
   qmltui-runner --capture-after-ms 0 --columns 80 --rows 24 --artifacts ./capture-out -- ./build/my_app
   ```
   `--capture-after-ms 0` waits for the first committed frame and writes `capture.txt` (plain UTF-8) and `capture_styled.txt` (text plus SGR style runs) into the artifact directory.

4. **Watch a scenario run live in your terminal (`--live`)**:
   ```bash
   qmltui-runner --script test_smoke.js --live -- ./build/my_app
   ```

5. **Create or update snapshot baselines (`--update-snapshots`)**:
   ```bash
   qmltui-runner --script test_smoke.js --update-snapshots -- ./build/my_app
   ```
   Every `expectSnapshot()`, `expectStyled()`, `expectRegionSnapshot()`, and `expectScrollbackSnapshot()` call writes the current committed frame to its snapshot file and logs the updated path in `result.json`.

6. **Record an Asciinema v2 `.cast` file (`--record`)**:
   ```bash
   qmltui-runner --script test_record.js --clock virtual --record demo.cast --artifacts ./cast-out -- ./build/my_app
   ```
   Inside `test_record.js`, pace frames with `app.sleep(ms)` / `app.advanceTime(ms)` and add chapter markers with `app.marker("Chapter title")`. Because `--clock virtual` uses synthetic timestamps, the generated `.cast` file is deterministic across runs.

### `qmltui-runner` Options Reference

Specify exactly one mode (`--script`, `--eval`, or `--capture-after-ms`):

| Option | Default | Description |
| :--- | :--- | :--- |
| `--script <path>` | None | Run a `.js`, `.mjs`, or Qt resource (`:/...`) scenario file. |
| `--eval <code>` | None | Evaluate an inline JavaScript snippet inside `function main(app)`. |
| `--capture-after-ms <ms>` | None | Wait `0..30000` ms (`0` = first committed frame), then write `capture.txt` and `capture_styled.txt`. |
| `--columns <n>`, `--rows <n>` | `80`, `24` | Initial terminal viewport dimensions (`1..1000` cells). |
| `--clock <real\|virtual>` | `real` | Select wall-clock time (`real`) or synthetic stepped time (`virtual`). |
| `--artifacts <dir>` | Retained temp dir | Output directory for `result.json`, `final.txt`, `failure.txt`, and captures (must be new or empty). |
| `--live` | Off | Forward live ANSI rendering to the terminal during the scenario. |
| `--update-snapshots` | Off | Rewrite snapshot files from the committed frames instead of failing on mismatch. |
| `--record <relative-path>` | None | Record terminal output as an asciicast v2 `.cast` file under the artifact directory (incompatible with `--live`). |
| `--operation-timeout-ms <ms>` | `2000` | Per-operation timeout (`1..30000` ms). |
| `--timeout-ms <ms>` | `30000` | Overall scenario watchdog timeout (`1..300000` ms). |
| `--startup-timeout-ms <ms>` | `5000` | Deadline for the application to start and complete the Runner handshake (`1..300000` ms). |

`qmltui-runner` exits with `0` when the scenario passes, `1` when an assertion or script error fails the scenario, and `2` on invalid CLI arguments, startup/handshake timeouts, or process crashes.

### Inspecting Artifacts

At the end of every run, `qmltui-runner` prints the artifact directory path to `stderr` and writes:

- `result.json`: Schema v1 summary containing `status` (`"passed"` or `"failed"`), `exitCode`, `clock`, `viewport`, `finalViewport`, `operations` trace, `artifacts` paths, and `updatedSnapshots`.
- `final.txt`: Normalized text of the final committed frame on success.
- `failure.txt`: Normalized text of the last committed frame when a scenario fails.
- `snapshots/<name>.actual.txt` and `snapshots/<name>.diff`: Written when a snapshot assertion mismatches.

## 3. CMake and CTest Integration

To run scenarios automatically in CTest, link `qmltui::runner` conditionally into your application target and register each scenario with `qmltui_add_scenario()`. You do not need a separate test executable.

### Register Scenarios in `CMakeLists.txt`

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
    enable_testing()

    # Enable IDE autocomplete (.qmltui/qmltui-runner.d.ts and jsconfig.json):
    qmltui_scenario_editor_support()

    # Register a headless CTest scenario:
    qmltui_add_scenario(
        NAME my_app_smoke
        TARGET my_app
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 80
        ROWS 24
        CLOCK VIRTUAL
    )

    # Optional: register a deterministic asciicast recording (labeled 'Runner' and 'Cast'):
    qmltui_add_scenario(
        NAME my_app_demo_cast
        TARGET my_app
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_record.js"
        COLUMNS 80
        ROWS 24
        CLOCK VIRTUAL
        RECORD my_demo.cast
    )
endif()
```

For production builds, configure CMake with `-DQMLTUI_BUILD_TESTS=OFF -DQMLTUI_ENABLE_RUNNER=OFF` so `qmltui::runner` is omitted completely.

### Run and Update Scenarios with CTest

Every test registered with `qmltui_add_scenario()` receives the `Runner` CTest label (and `Cast` when `RECORD` is set), and writes its artifacts to `<build>/RunnerArtifacts/<NAME>/` by default:

```bash
# Run all Runner scenarios in the build tree:
ctest --test-dir build -L Runner --output-on-failure

# Run a single scenario by name:
ctest --test-dir build -R "^my_app_smoke$" --output-on-failure

# Update golden snapshots for a scenario after an intentional UI change:
QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest --test-dir build -R "^my_app_smoke$" --output-on-failure
```

See the [Runner Reference](../cpp-classes/runner.md) for the complete list of `app.*` methods, `qmltui_add_scenario()` parameters, `QMLTUI_RUNNER_*` environment variables, and C++ `qmltui::Runner` constructors.
