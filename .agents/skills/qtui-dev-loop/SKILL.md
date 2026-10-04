---
name: qtui-dev-loop
description: Develop, test, and visually verify Qtui retained-mode terminal applications using the current CMake, CTest, and Qtui::Testing/TestRunner workflows. Use when editing Qtui C++ or QML, adding JavaScript-driven application scenarios, inspecting terminal-frame artifacts, or validating Behavior-enabled examples. Do not use for generic Qt Quick applications.
---

# Qtui Development Loop

Use the repository's current TestRunner-based workflow:

```text
Edit C++/QML/JS -> Build -> Run focused tests or scenario -> Inspect live frame/artifacts -> Run regressions
```

All testing is powered by the integrated `Qtui::Testing` library and `TestRunner`.
Mouse interactions (`click`, `drag`, `scroll`), text matching, regex assertions, cell style
inspection, and screenshot capture are fully supported natively in `TestRunner`.

## Configure and Build

Qtui requires Qt 6.8 or newer. Configure ordinary development builds with
tests and examples:

```bash
rtk cmake -S . -B build \
  -DQTUI_BUILD_TESTS=ON \
  -DQTUI_BUILD_EXAMPLES=ON
rtk cmake --build build
```

Add `-DQTUI_ENABLE_BEHAVIOR=ON` when working on Behavior, animation, or the
Industrial PLC HMI. That option requires Qt's `QmlPrivate` component. The
Industrial PLC example is intentionally unavailable when Behavior is disabled.

`Qtui::Testing` is built automatically when `QTUI_BUILD_TESTS=ON`.
Use `-DQTUI_ENABLE_TEST_RUNNER=ON` when building Testing for installation
or consumption without the repository tests. Use
`-DQTUI_BUILD_TERMINAL_HOST=ON` only for work involving the optional PTY
`TerminalView` module.

After an edit, build the affected target. Run a full build after Core, public
API, QML-module, or CMake changes:

```bash
rtk cmake --build build --target <target>
rtk cmake --build build
```

## Test Loop

Run the narrowest relevant test first, then the complete configured suite:

```bash
rtk ctest --test-dir build -R '^test_name$' --output-on-failure
rtk ctest --test-dir build --output-on-failure
```

For Core, CMake, feature-gating, or Behavior-related changes, also validate a
Behavior-disabled tree:

```bash
rtk cmake -S . -B build-tests-only \
  -DQTUI_BUILD_TESTS=ON \
  -DQTUI_BUILD_EXAMPLES=OFF \
  -DQTUI_ENABLE_BEHAVIOR=OFF
rtk cmake --build build-tests-only
rtk ctest --test-dir build-tests-only --output-on-failure
```

Use `rtk git diff --check` before handoff. Preserve unrelated work in the
shared worktree.

## Automated Application Scenarios

Use `qtui::TestRunner` for application-level terminal tests. Read the
[TestRunner API](qtui/src/Qtui/Testing/doc/TestRunner.md) when adding
or changing a scenario.

Choose one integration style:

- **Embedded resource scenario**: preferred for a self-contained application test.
  Add the JavaScript file to the application's QML module `RESOURCES`, create
  a dedicated test executable (e.g. `main_test_runner.cpp`), construct `TestRunner` with the explicit
  `QQmlEngine`, `Terminal`, resource path, artifact directory, deterministic
  viewport, and bounded timeouts, then schedule `start()` with
  `QTimer::singleShot(0, ...)`.
- **Environment-driven scenario**: call `TestRunner::startFromEnvironment()` from
  a test-enabled executable and register it with `qtui_add_scenario_test()`.
  The installed CMake helper (`QtuiTesting.cmake`) supplies deterministic viewport and timeout
  defaults, validates `result.json`, and labels the test for agent runs.

## Closed-Loop Agent Development Workflow

`Qtui::Testing` is optimized for closed-loop AI agent iteration. Coding agents can modify QML/C++ code, build, and instantly visually inspect or test UI frames in one command.

Test sessions are quiet by default: frames still commit, but no ANSI terminal
output is emitted. Every run writes a structured `result.json` and a normalized
`final.txt` (or `failure.txt` / `inspect.txt`) beside requested captures. Use
`QTUI_TEST_LIVE=1` only when a human needs to watch the terminal render live.

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `QTUI_SCRIPT` | Path to JavaScript scenario file | None |
| `QTUI_EVAL` | Inline JavaScript code string to execute | None |
| `QTUI_INSPECT` | Set to `1` / `true` for one-shot frame inspection | Off |
| `QTUI_INSPECT_IMAGE` | Set to `1` to also capture PNG during inspection | Off |
| `QTUI_TEST_LIVE` | Set to `1` to output live ANSI terminal frames to stdout | Off |
| `QTUI_TEST_ARTIFACT_DIR` | Directory path for output artifacts | Temp dir |
| `QTUI_TEST_COLUMNS` | Test viewport columns | `80` |
| `QTUI_TEST_ROWS` | Test viewport rows | `24` |
| `QTUI_TEST_OPERATION_TIMEOUT_MS` | Per-operation wait/assertion timeout | `2000` |
| `QTUI_TEST_TIMEOUT_MS` | Overall scenario watchdog timeout | `10000` |

### 1. Instant One-Shot Inspection (`QTUI_INSPECT=1`)
To inspect the initial rendered frame of any app executable without writing a script:
```bash
QTUI_INSPECT=1 QTUI_TEST_ARTIFACT_DIR=/tmp/artifacts \
  ./build/examples/apps/pos_cashier/qtui_pos_cashier
cat /tmp/artifacts/inspect.txt
cat /tmp/artifacts/result.json
```

Inspection is text-first. Add `QTUI_INSPECT_IMAGE=1` only when a PNG is
required and the executable uses `QGuiApplication`.

### 2. Inline Action Evaluation (`QTUI_EVAL`)
To run a quick series of interactive actions and inspect the frame:
```bash
QTUI_EVAL="Test.clickText('Pay'); Test.press('Enter'); Test.captureText('after_click.txt');" \
  QTUI_TEST_ARTIFACT_DIR=/tmp/artifacts \
  ./build/examples/apps/pos_cashier/qtui_pos_cashier
cat /tmp/artifacts/after_click.txt
```

### 3. Rich Script & Assertion Capabilities
Scenario scripts support keys, mouse events, regex matching, region assertions, cell style assertions, locator functions, and baseline snapshot matching:

```javascript
// Mouse events
Test.click(10, 5, "left");
Test.doubleClick(15, 2);
Test.drag(5, 5, 25, 5);
Test.scroll(10, 10, 0, 120);

// Text/grid locators
const matches = Test.findText("Pay");
const cell = Test.cell(matches[0].x, matches[0].y);
Test.clickText("Pay");
const text = Test.frameText();

// Regex & Region matching
Test.waitForRegex("STATUS:\\s+READY");
Test.expectRegex("v\\d+\\.\\d+");
Test.expectTextInRegion(0, 0, 80, 2, "HEADER");

// Style & Snapshot verification
Test.expectStyle(0, 0, { bold: true, character: "A" });
Test.expectSnapshot("snapshots/baseline.txt");

// Auto-finish & Promise support (Test.finish() is optional)
return new Promise((resolve) => {
    Test.sleep(50);
    resolve();
});
```

Follow these invariants:

- Use `QTUI_INSPECT=1` or `QTUI_EVAL` during closed-loop development to visually verify layouts after edits.
- Use `typeText()` for Unicode text entry and `press()` for named keys and modifiers.
- Use `click()`, `doubleClick()`, `drag()`, and `scroll()` for mouse interactions.
- Assert rendered, committed frames with `expectText()`, `expectRegex()`, `expectTextInRegion()`, and `expectStyle()`.
- Prefer `findText()` and `clickText()` over hard-coded coordinates when the rendered label is stable; use `cell()` for exact grid/style diagnostics.
- Keep artifact paths relative to the configured artifact directory.
- `Test.finish()` is optional; synchronous scripts and Promises auto-finish on normal completion.

## Live Scenario Verification

CTest captures process output. To watch a scenario render live, run its
dedicated test executable directly in a real terminal. For Industrial PLC:

```bash
rtk cmake --build build --target qtui_industrial_plc_hmi_test_runner
QTUI_TEST_LIVE=1 \
  ./build/examples/apps/industrial_plc_hmi/qtui_industrial_plc_hmi_test_runner
```

The build must have `QTUI_ENABLE_BEHAVIOR=ON`. Qtui buffers Qt diagnostics
while the alternate screen is active, restores the terminal, and then prints
the TestRunner result. The live display should contain one deterministic test
viewport, not a physical-size frame overlaid by a second frame.

Run the equivalent automated contract with:

```bash
rtk ctest --test-dir build -R '^test_runner_plc$' --output-on-failure
```

Set a recoverable artifact location when useful:
```bash
QTUI_TEST_ARTIFACT_DIR=/tmp/qtui-test-artifacts \
  ./build/examples/apps/industrial_plc_hmi/qtui_industrial_plc_hmi_test_runner
```

## Qtui QML Rules

- Build terminal interfaces from Qtui types. Do not add a `QtQuick` import or
  Qt Quick dependency.
- A root `Terminal` must provide a reachable quit path for Q and Escape.
  Simple applications can use:

  ```qml
  Keys.onPressed: (event) => {
      if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
          Qt.quit()
          event.accepted = true
      }
  }
  ```

  If Escape first closes a modal, clears input state, or cancels an operation,
  preserve that behavior and quit only when nothing context-specific consumes
  the key.
- Keep test scripts outside production startup unless environment-driven tests
  are an intentional application feature. Prefer a separate test `main.cpp`
  for embedded scenarios.
- Verify what the committed terminal frame shows, not merely that QML
  properties changed.

## Handoff

Report the targets built, focused and full test results, live scenario result
when used, and artifact paths. Call out configurations not exercised.
