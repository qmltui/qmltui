---
name: qtui-dev-loop
description: Develop, test, and visually verify Qtui retained-mode terminal applications using the current CMake, CTest, and Qtui::TestSupport/TestDriver workflows. Use when editing Qtui C++ or QML, adding JavaScript-driven application scenarios, inspecting terminal-frame artifacts, or validating Behavior-enabled examples. Do not use for generic Qt Quick applications.
---

# Qtui Development Loop

Use the repository's current TestDriver-based workflow:

```text
Edit C++/QML/JS -> Build -> Run focused tests or scenario -> Inspect live frame/artifacts -> Run regressions
```

The experimental `qtui-driver snapshot/daemon` target is not present in this
branch. Do not use an old `build/tools/qtui-driver` binary left by another
branch, and do not claim that daemon or mouse-driving support exists.

## Configure and build

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

`Qtui::TestSupport` is built automatically when `QTUI_BUILD_TESTS=ON`.
Use `-DQTUI_ENABLE_TEST_DRIVER=ON` when building TestSupport for installation
or consumption without the repository tests. Use
`-DQTUI_BUILD_TERMINAL_HOST=ON` only for work involving the optional PTY
`TerminalView` module.

After an edit, build the affected target. Run a full build after Core, public
API, QML-module, or CMake changes:

```bash
rtk cmake --build build --target <target>
rtk cmake --build build
```

## Test loop

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

## Automated application scenarios

Use `qtui::TestDriver` for application-level terminal tests. Read the
[TestDriver API](../../../src/Qtui/TestSupport/doc/TestDriver.md) when adding
or changing a scenario.

Choose one integration style:

- Embedded resource scenario: preferred for a self-contained application test.
  Add the JavaScript file to the application's QML module `RESOURCES`, create
  a dedicated test executable, construct `TestDriver` with the explicit
  `QQmlEngine`, `Terminal`, resource path, artifact directory, deterministic
  viewport, and bounded timeouts, then schedule `start()` with
  `QTimer::singleShot(0, ...)`.
- Environment scenario: use
  `TestDriver::startFromEnvironment(&engine, terminal)` when the application
  intentionally accepts external scripts through `QTUI_SCRIPT`. The supported
  viewport, artifact, and timeout environment variables are documented in the
  TestDriver API.

Scenario code uses the global `Test` object:

```javascript
Test.waitForText("[■ STANDBY]");
Test.press("S");
Test.waitForCondition((frameText, elapsedMs) =>
    elapsedMs >= 0 && frameText.includes("[● RUNNING]"), 3000);
Test.expectText("[● RUNNING]");
Test.captureText("scenario/running.txt");
Test.finish();
```

Follow these invariants:

- Prefer bounded condition or text waits over arbitrary sleeps. Use
  `Test.sleep()` only when elapsed time itself matters or a live animation
  should remain visible.
- Use `typeText()` for Unicode text entry and `press()` for named keys and
  modifiers.
- Assert rendered, committed frames with `expectText()` and
  `expectNoText()`; do not inspect transient object state as the oracle.
- Keep artifact paths relative to the configured artifact directory. Artifact
  write failures must fail the scenario.
- Make `Test.finish()` the final statement. Returning without it or throwing
  afterward fails the scenario.
- Every operation and the complete scenario must have finite timeouts.
- TestDriver currently drives keys and text, not mouse input.

`captureText()` works with `QCoreApplication`. `captureImage()` requires a
`QGuiApplication`; in display-less environments run that executable with
`QT_QPA_PLATFORM=offscreen`. Inspect text artifacts with `rtk read` and
local image artifacts with the available `view_image` tool.

## Live scenario verification

CTest captures process output. To watch a scenario render live, run its
dedicated test executable directly in a real terminal. For Industrial PLC:

```bash
rtk cmake --build build --target qtui_industrial_plc_hmi_test_driver
rtk proxy ./build/examples/apps/industrial_plc_hmi/qtui_industrial_plc_hmi_test_driver
```

The build must have `QTUI_ENABLE_BEHAVIOR=ON`. Qtui buffers Qt diagnostics
while the alternate screen is active, restores the terminal, and then prints
the TestDriver result. The live display should contain one deterministic test
viewport, not a physical-size frame overlaid by a second frame.

Run the equivalent automated contract with:

```bash
rtk ctest --test-dir build -R '^test_driver_plc$' --output-on-failure
```

Set a recoverable artifact location when useful:

```bash
rtk env QTUI_TEST_ARTIFACT_DIR=/tmp/qtui-test-artifacts \
  ./build/examples/apps/industrial_plc_hmi/qtui_industrial_plc_hmi_test_driver
```

## Qtui QML rules

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
when used, and artifact paths. Call out configurations not exercised. Never
report the removed `qtui-driver` workflow as available merely because a stale
binary exists in a local build directory.
