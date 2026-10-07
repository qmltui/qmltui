---
name: qtui-dev-loop
description: Develop and verify changes to Qtui retained-mode terminal applications using focused Qt Test suites, TestRunner scenarios, and committed-frame artifacts. Use when implementing or testing Qtui C++, QML, examples, Testing integration, or Behavior-enabled applications; do not use for read-only QML review or generic Qt Quick projects.
---

# Qtui Development Loop

Use the smallest feedback loop that proves the requested behavior:

```text
Inspect context -> Edit -> Build affected target -> Run focused test/scenario
                -> Inspect committed-frame artifacts -> Run relevant regressions
```

Qtui has two complementary test layers:

- Use ordinary Qt Test executables for C++ units, model/layout/input contracts,
  and QML components that do not need a whole application session.
- Use `qtui::TestRunner` from `Qtui::Testing` for application-level input,
  rendering, asynchronous state, and terminal-grid assertions.

Do not describe all repository tests as TestRunner tests. Before changing a
scenario or Testing integration, read the authoritative
[TestRunner API](../../../src/Qtui/Testing/doc/TestRunner.md),
[`QtuiTesting.cmake`](../../../cmake/QtuiTesting.cmake), and one nearby working
example.

## Configure and Build

Qtui requires Qt 6.8 or newer. A normal in-tree development configuration is:

```bash
rtk cmake -S . -B build \
  -DQTUI_BUILD_TESTS=ON \
  -DQTUI_BUILD_EXAMPLES=ON
rtk cmake --build build
```

`QTUI_BUILD_TESTS=ON` builds `Qtui::Testing` and the repository test suites.
Use `-DQTUI_ENABLE_TEST_RUNNER=ON` only when Testing must be built and installed
without the repository tests. Use `-DQTUI_BUILD_TERMINAL_HOST=ON` only for the
optional PTY-backed `TerminalView` module.

Behavior and the Industrial PLC HMI require `-DQTUI_ENABLE_BEHAVIOR=ON` and
Qt's private QML component. For changes to Behavior or its feature gating,
validate both an enabled build and a separate disabled build. Do not assume the
PLC target exists in a Behavior-disabled tree.

After an edit, build the affected target first. Rebuild the configured tree
after Core, public API, QML-module, or CMake changes:

```bash
rtk cmake --build build --target <target>
rtk cmake --build build
```

## Select and Run Tests

Discover exact CTest names and labels instead of guessing:

```bash
rtk ctest --test-dir build -N
rtk ctest --test-dir build -N -L AgentSmoke
```

Run a focused test, then the relevant label or configured suite:

```bash
rtk ctest --test-dir build -R '^exact_test_name$' --output-on-failure
rtk ctest --test-dir build -L AgentSmoke --output-on-failure
rtk ctest --test-dir build --output-on-failure
```

`AgentFull` scenarios may be deliberately slow or live-rendered. Run them when
the changed area or handoff confidence warrants it; do not substitute them for
a focused first pass.

## Integrate Application Scenarios

For new applications, link `Qtui::Testing` only into a dedicated test
executable. Some existing examples conditionally link Testing into their main
executable; treat these as legacy integration patterns and do not copy them
into production applications. Linking Testing installs the Terminal auto-hook
that honors `QTUI_SCRIPT`, `QTUI_EVAL`, and `QTUI_INSPECT`.

Choose one integration pattern:

- **External script:** the executable links `Qtui::Testing`; register it with
  `qtui_add_scenario_test(NAME ... TARGET ... SCRIPT ...)`. The helper sets a
  deterministic viewport and bounded timeouts, configures QML import paths,
  clears the scenario artifact directory, validates `result.json`, and adds the
  `TestRunner` label. Add `LIVE` only for a scenario that must render to a
  real terminal; automated suites should stay quiet.
- **Embedded resource:** add the script and snapshots to a QML/resource module,
  construct `TestRunner` with the explicit `QQmlEngine`, root `Terminal`,
  resource path, artifact directory, viewport, and timeouts, then schedule
  `start()` with `QTimer::singleShot(0, ...)`. Register the dedicated executable
  with `qtui_add_scenario_test(... EMBEDDED ...)` so automated runs are quiet
  and receive the same result validation.

`startFromEnvironment()` returns `nullptr` when no scenario mode is requested
and schedules `start()` itself when it creates a runner. A directly constructed
runner never starts automatically. The Testing auto-hook normally handles
environment-driven startup; call `startFromEnvironment()` explicitly only when
the executable needs to validate that a request was present or otherwise own
that startup check.

Minimal external-script registration:

```cmake
target_link_libraries(my_app_test PRIVATE Qtui::Core Qtui::Testing)

qtui_add_scenario_test(
    NAME my_app_smoke
    TARGET my_app_test
    SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
    LABELS AgentSmoke
)
```

Use
[`examples/apps/markdown_viewer/main_test_runner.cpp`](../../../examples/apps/markdown_viewer/main_test_runner.cpp)
for explicitly constructed embedded runners. Use
[`examples/apps/service_monitor/main_test.cpp`](../../../examples/apps/service_monitor/main_test.cpp)
or
[`examples/apps/starter_template/main_test.cpp`](../../../examples/apps/starter_template/main_test.cpp)
for dedicated test-only executables driven through the environment. Use
[`tests/Testing`](../../../tests/Testing) for framework scenarios and failure
contracts.

## Closed-Loop Inspection

Environment modes are mutually exclusive: set exactly one of `QTUI_SCRIPT`,
`QTUI_EVAL`, or `QTUI_INSPECT`. They work only when the executable contains
`Qtui::Testing`. For reproducible direct runs, set both viewport dimensions and
an explicit artifact directory:

```bash
rtk env \
  QTUI_INSPECT=1 \
  QTUI_TEST_COLUMNS=80 \
  QTUI_TEST_ROWS=24 \
  QTUI_TEST_ARTIFACT_DIR=/tmp/qtui-inspect \
  ./build/path/to/testing-linked-app
rtk read /tmp/qtui-inspect/inspect.txt
rtk json /tmp/qtui-inspect/result.json
```

Use inline evaluation for a short exploratory action sequence:

```bash
rtk env \
  QTUI_EVAL="terminal.clickText('Pay'); terminal.captureText('after-pay.txt');" \
  QTUI_TEST_COLUMNS=80 \
  QTUI_TEST_ROWS=24 \
  QTUI_TEST_ARTIFACT_DIR=/tmp/qtui-eval \
  ./build/path/to/testing-linked-app
rtk read /tmp/qtui-eval/after-pay.txt
```

Runs requested through the environment are quiet by default. Add
`QTUI_TEST_LIVE=1` only for a human watching a real terminal; automated checks
should normally inspect artifacts. For a directly constructed embedded runner,
also set `QTUI_TEST_EMBEDDED_SCENARIO=1` to select quiet/live terminal behavior:

```bash
rtk env QTUI_TEST_EMBEDDED_SCENARIO=1 QTUI_TEST_LIVE=1 \
  ./build/path/to/embedded-test-runner
```

`QTUI_INSPECT_IMAGE=1` and `terminal.captureImage()` rasterize the committed cell
grid and require a `QGuiApplication`; normal text capture works with
`QCoreApplication`. Prefer text artifacts unless an image materially helps.

## Write Reliable JavaScript Scenarios

Every test scenario script must define an entry point: `function main(terminal) { ... }`.
Use the API documentation for exact signatures. Choose operations by intent:

- `press()` and `typeText()` inject input and wait for the resulting committed
  frame. Mouse operations include `clickText()`, `click()`, `doubleClick()`,
  `drag()`, and `scroll()`.
- Prefer `findText()` and `clickText()` when a rendered label is stable. Use
  coordinates and `cell()` for grid geometry, wide-character, or style checks.
- Use `waitForText()`, `waitForRegex()`, or `waitForCondition()` for state that
  appears asynchronously. Use `expectText()`, `expectNoText()`,
  `expectRegex()`, `expectTextInRegion()`, and `expectStyle()` for assertions on
  committed frames. `expectSnapshot()` compares the frame against a stored
  snapshot file and writes a unified diff on mismatch. `fail()` aborts the
  scenario with a message when a check cannot be expressed as an assertion.
- `waitForFrame()` requests rendering. To prove that the application scheduled
  its own repaint, use `injectKey()` or `injectText()` followed by
  `expectRepaint()`; the regular input methods would consume that repaint.
  `frameCount()` can diagnose whether the committed-frame sequence advanced.
- Use `frameText()` for the current normalized frame and `rootObject()` only
  when application state inspection is genuinely needed. Keep at least one
  rendered-frame assertion for visible behavior.
- Keep capture paths relative to the artifact directory. Snapshot paths are
  resolved relative to the scenario; mismatches produce an actual frame and a
  unified diff beneath the artifact directory.

Synchronous scenarios pass when `main` returns normally, so `terminal.finish()`
is optional. For an asynchronous scenario, `main` can return a Promise or be
declared `async`:

```javascript
function main(terminal) {
    terminal.expectText("READY");
    return new Promise((resolve) => {
        terminal.sleep(50);
        resolve();
    });
}
```

Do not use a top-level `return`. Promise rejection, uncaught JavaScript errors,
assertion failures, render failures, operation timeouts, and the overall
watchdog fail the scenario. Wait operations are bounded and may not be nested;
callbacks passed to `waitForCondition()` must be synchronous and quick.

## Read Results and Hand Off

Every requested runner session writes schema-v1 `result.json` atomically. A
passing script writes `final.txt`, a failure writes `failure.txt`, and inspection
uses `inspect.txt`; requested captures are listed in the result. Treat the
normalized text frame as the primary visual oracle and inspect the operation
trace and failure message before changing code after a failure.

Qtui QML should use Qtui types rather than adding a `QtQuick` import. Test the
committed terminal output, not only a QML property change.

Before handoff, run `rtk git diff --check`. Report targets built, focused and
regression tests run, artifact paths inspected, live rendering when used, and
relevant configurations not exercised. Preserve unrelated work in a dirty
worktree.

After changing either repository skill, run
`python3 .agents/skills/qtui-qml/check-skills.py --build-dir build` to verify links,
named example targets, the Common Type Map, and whitespace.
