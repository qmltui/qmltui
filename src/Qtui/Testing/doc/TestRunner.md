# TestRunner

## Class Overview

`qtui::TestRunner` turns a running Qtui `Terminal` application into a bounded JavaScript-driven test oracle. It loads a scenario from a filesystem or Qt resource path, invokes its `function main(terminal)` entry point with a test driver facade, drives input, verifies committed frames, and writes machine-readable diagnostics. A synchronous scenario succeeds when `main` returns normally; `terminal.finish()` is optional.

The class is intended for application-level automated tests. It operates on an explicitly supplied `Terminal`, so applications with multiple terminals do not rely on global target selection.

## Project Structure and Dependencies

The declaration is in `Testing/TestRunner.h` and the implementation is in `Testing/TestRunner.cpp`. `QtuiTesting` is available when either `QTUI_BUILD_TESTS` or `QTUI_ENABLE_TEST_RUNNER` is enabled. Consumers link against `Qtui::Testing`.

Its public build dependencies are Qt 6 QML and `Qtui::Core`. Qt 6 GUI is a private dependency used for image artifact generation. The driver collaborates with these project types:

- `Terminal` identifies the UI under test.
- `TerminalEngine` injects input, requests rendering, publishes committed-frame notifications, and provides the current frame.
- `TextGridBuffer` contains the committed terminal cells used by text assertions and artifacts.

## Class Hierarchy and Role

`TestRunner` derives directly from `QObject`. `QObject` provides parent-based ownership, signal delivery, timers, and the meta-object data required to expose `Q_INVOKABLE` methods to JavaScript. The class is `final`, and copying and moving are disabled.

## Signals

#### finished(int exitCode)

Emitted once when the scenario has passed or failed and process exit has been scheduled. An exit code of `0` indicates success; `1` indicates a scenario, configuration, JavaScript, timeout, rendering, assertion, or artifact failure. Consumers may connect this signal to test-harness reporting, but the driver also schedules `QCoreApplication::exit(exitCode)`.

## Public Slots and Q_INVOKABLE Methods

The methods in this section are callable from JavaScript through the `terminal` argument passed to `function main(terminal)`. They must be called while the scenario is running. Unless stated otherwise, invalid arguments or failed operations fail the complete scenario, produce a failure artifact when a frame is available, and schedule a non-zero process exit.

#### void press(const QString &key, const QStringList &modifiers = {})

Sends one key event to the target terminal and waits for the resulting committed frame. `key` accepts Enter/Return, Esc/Escape, Tab, Backtab, Backspace, Space, the arrow keys, Home, End, Insert, Delete, PageUp, PageDown, F1 through F35, or one ASCII character. Modifier names are case-insensitive and accept Shift, Ctrl/Control, Alt, and Meta. Backtab automatically applies Shift. Control, Alt, and Meta combinations do not carry text. JavaScript usage: `terminal.press("Enter")` or `Test.press("a", ["Shift"])`.

#### void typeText(const QString &text)

Sends non-empty Unicode text as one text-input event and waits for the resulting committed frame. Use this for text entry rather than issuing one `press()` call per character. JavaScript usage: `terminal.typeText("café 世界")`.

#### void sleep(int durationMs)

Waits for the requested duration while continuing to process Qt events, QML timers, rendering, and the scenario watchdog. `durationMs` must be between 1 and 30,000 milliseconds. The overall scenario watchdog can fail the test before the sleep completes. Nested wait operations are rejected. JavaScript usage: `terminal.sleep(250)`.

#### void waitForFrame(int timeoutMs = 2000)

Requests rendering and waits for a committed frame whose sequence is newer than the frame visible when the call began. `timeoutMs` must be between 1 and 30,000 milliseconds. The scenario fails if no new frame is committed before the timeout. JavaScript usage: `terminal.waitForFrame()` or `Test.waitForFrame(5000)`.

#### void expectRepaint(int timeoutMs = 2000)

Waits for a committed frame **without** requesting one, so it verifies that the application itself invalidated the scene, and fails if no frame arrives before the timeout. Use it to catch missing invalidation — a bug class every other assertion hides, because `waitForFrame()` and the `expect*` family request a render themselves and therefore pass even when a change scheduled no repaint. Pair it with `injectKey()`/`injectText()`: the regular input methods wait for a frame and would consume the very repaint being asserted. JavaScript usage: `terminal.injectKey("k"); Test.expectRepaint();`

#### void injectKey(const QString &key, const QStringList &modifiers = {})

`press()` without the implicit frame wait. Same key and modifier names, no rendering requested and no frame awaited. JavaScript usage: `terminal.injectKey("Down", ["Shift"])`.

#### void injectText(const QString &text)

`typeText()` without the implicit frame wait. JavaScript usage: `terminal.injectText("abc")`.

#### int frameCount()

Sequence number of the most recently committed frame, or `-1` when no engine is attached. Compare it across an interaction to assert that nothing repainted. JavaScript usage: `const before = terminal.frameCount();`

#### void waitForCondition(const QJSValue &condition, int timeoutMs = 2000)

Evaluates a synchronous JavaScript callback immediately and then approximately every 10 milliseconds until it returns a truthy value. The callback receives two arguments: the normalized text of the current committed frame and the number of milliseconds elapsed since waiting began. `timeoutMs` must be between 1 and 30,000 milliseconds.

The callback must be callable and synchronous. A thrown JavaScript error fails the scenario. Promises are not awaited, and nested calls to `sleep()`, `waitForFrame()`, `waitForText()`, or `waitForCondition()` are rejected. JavaScript usage: `terminal.waitForCondition((frameText, elapsedMs) => elapsedMs >= 100 && frameText.includes("READY"), 2000)`.

#### void waitForText(const QString &text, int timeoutMs = 2000)

Waits until a committed terminal frame contains the specified non-empty substring. It observes committed frames rather than sleeping for an assumed render delay. `timeoutMs` must be between 1 and 30,000 milliseconds. JavaScript usage: `terminal.waitForText("SYSTEM READY", 3000)`.

#### void expectText(const QString &text)

Waits for the next committed frame and fails unless its normalized text contains the specified non-empty substring. The frame wait uses the configured operation timeout. Use `waitForText()` when the expected content may appear asynchronously; use `expectText()` as an immediate assertion after an operation.

#### void expectNoText(const QString &text)

Waits for the next committed frame and fails if its normalized text contains the specified substring. An empty substring is invalid because every string contains it. The frame wait uses the configured operation timeout.

#### void captureText(const QString &relativePath)

Waits for the next committed frame and writes its normalized UTF-8 text beneath the configured artifact directory. Trailing spaces are removed from each row. `relativePath` must be a non-empty relative path contained by the artifact directory; absolute paths and parent traversal are rejected. Missing parent directories inside the artifact directory are created automatically.

#### void captureImage(const QString &relativePath)

Waits for the next committed frame and rasterizes it to an image beneath the configured artifact directory. The application must be a `QGuiApplication`; a `QCoreApplication` cannot capture images. The path containment rules are the same as for `captureText()`, and the image format is selected from the filename extension supported by Qt.

#### QString frameText()

Returns normalized text for the current committed frame without requesting another render.

#### object cell(int x, int y)

Returns the current cell's character, continuation state, `#RRGGBBAA` colors, and bold, dim, italic, underline, and reversed attributes. Invalid coordinates fail the scenario.

#### array findText(const QString &text)

Returns all visible, single-row matches in row-major order as `{x, y, width}` objects. Coordinates remain correct for wide characters and continuation cells.

#### void clickText(const QString &text, int occurrence = 0, const QString &button = "left", const QStringList &modifiers = {})

Clicks the center of the selected rendered-text match and waits for the resulting committed frame.

#### void click(int x, int y, const QString &button = "left", const QStringList &modifiers = {})

Sends a mouse press and release event at terminal coordinates `(x, y)`. Accepts button `"left"`, `"middle"`, or `"right"`, and optional modifiers `["Shift", "Ctrl", "Alt", "Meta"]`. JavaScript usage: `terminal.click(10, 5)` or `Test.click(10, 5, "right")`.

#### void doubleClick(int x, int y, const QString &button = "left", const QStringList &modifiers = {})

Sends two consecutive mouse press and release cycles at terminal coordinates `(x, y)`. JavaScript usage: `terminal.doubleClick(10, 5)`.

#### void drag(int startX, int startY, int endX, int endY, const QString &button = "left", const QStringList &modifiers = {})

Simulates a mouse drag operation starting at `(startX, startY)` and moving to `(endX, endY)`. JavaScript usage: `terminal.drag(5, 5, 25, 5)`.

#### void scroll(int x, int y, int deltaX, int deltaY, const QStringList &modifiers = {})

Sends a mouse wheel scroll event at terminal coordinates `(x, y)` with horizontal `deltaX` and vertical `deltaY`. JavaScript usage: `terminal.scroll(15, 10, 0, 120)`.

#### void waitForRegex(const QString &pattern, int timeoutMs = 2000)

Waits until a committed terminal frame matches the regular expression `pattern`. JavaScript usage: `terminal.waitForRegex("STATUS:\\s+READY", 3000)`.

#### void expectRegex(const QString &pattern)

Fails the scenario unless the next committed frame matches the regular expression `pattern`. JavaScript usage: `terminal.expectRegex("System\\s+v\\d+\\.\\d+")`.

#### void expectTextInRegion(int x, int y, int width, int height, const QString &text)

Fails the scenario unless the bounding box `[x, y, width, height]` contains `text`. JavaScript usage: `terminal.expectTextInRegion(0, 0, 80, 2, "HEADER")`.

#### void expectStyle(int x, int y, const QJSValue &expectedStyle)

Fails the scenario unless cell attributes at `(x, y)` match the properties specified in `expectedStyle` (e.g. `{ bold: true, character: "A" }`). JavaScript usage: `terminal.expectStyle(0, 0, { bold: true })`. The `dim` attribute can be checked with `{ dim: true }`.

#### void expectSnapshot(const QString &snapshotRelativePath)

Compares the committed frame against a snapshot resolved relative to the scenario. On mismatch, writes `snapshots/<name>.actual.txt` and a unified `snapshots/<name>.diff` under the artifact directory. JavaScript usage: `terminal.expectSnapshot("snapshots/baseline.txt")`.

#### void fail(const QString &message)

Fails the scenario immediately with `message`. An empty message is replaced with `Scenario failed`. The method writes `failure.txt` when a committed frame is available, stops the watchdog, emits `finished(1)`, raises a JavaScript error while a script is being evaluated, and schedules process exit with code `1`.

#### void finish()

Marks the scenario as finishing. Calling `terminal.finish()` explicitly is optional: if a script or Promise returns normally without uncaught errors or failed assertions, `TestRunner` will automatically auto-finish the scenario successfully.

## Asynchronous & Promise Integration

`TestRunner` supports asynchronous scenario scripts returning JavaScript `Promise` objects. When a scenario script evaluates to a Promise (or returns a Promise from its entry point), `TestRunner` registers resolution and rejection handlers. The scenario will remain active while Qt event loop processing continues, and will automatically pass upon Promise resolution or fail upon Promise rejection (or scenario watchdog timeout).

## Public Methods

#### TestRunner(QQmlEngine *qmlEngine, Terminal *terminal, QString scriptPath, QString artifactDirectory, QSize viewport, int operationTimeoutMs, int scenarioTimeoutMs, QString configurationError)

Constructs the driver with an explicit QML engine and terminal. `scriptPath` may be a filesystem path or a Qt resource path such as `:/qt/qml/MyApplication/test_scenario.js`. `artifactDirectory` is the root for captures and failure diagnostics. `viewport` is applied immediately so the terminal's first queued frame already uses the deterministic test size. Operation and scenario timeouts must be between 1 and 30,000 milliseconds. A non-empty `configurationError` makes `start()` fail before loading the scenario.

Construction does not start execution. Schedule `start()` after the application has loaded its root `Terminal`.

#### static TestRunner *startFromEnvironment(QQmlEngine *qmlEngine, Terminal *terminal)

Creates and schedules a runner from environment variables. It returns `nullptr` when no request is present. `QTUI_SCRIPT`, `QTUI_EVAL`, and `QTUI_INSPECT` are mutually exclusive.

| Variable | Default | Description |
|----------|---------|-------------|
| `QTUI_SCRIPT` | None | Required filesystem scenario path. |
| `QTUI_EVAL` | None | Inline JavaScript scenario; its temporary file is removed at completion. |
| `QTUI_INSPECT` | Off | Capture the initial committed frame as `inspect.txt`. |
| `QTUI_INSPECT_IMAGE` | Off | Explicitly request `inspect.png`; requires `QGuiApplication`. |
| `QTUI_TEST_ARTIFACT_DIR` | Per-process temporary directory | Root directory for artifacts. |
| `QTUI_TEST_COLUMNS` | `80` | Test viewport columns, from 1 through 1,000. |
| `QTUI_TEST_ROWS` | `24` | Test viewport rows, from 1 through 1,000. |
| `QTUI_TEST_OPERATION_TIMEOUT_MS` | `2000` | Per-operation timeout, from 1 through 30,000 milliseconds. |
| `QTUI_TEST_TIMEOUT_MS` | `10000` | Overall scenario watchdog, from 1 through 30,000 milliseconds. |
| `QTUI_TEST_LIVE` | Off | Emit live ANSI rendering instead of quiet frame commits. |

## Structured results

Every requested run atomically writes `result.json` with schema version `1`, status, exit code, mode, scenario, elapsed time, viewport, current operation, argument-free operation trace, and artifact paths. Successful scenarios write `final.txt`, failures write `failure.txt`, and inspection uses `inspect.txt`. Standard error prints one final summary with the absolute artifact directory.

## CMake integration

Installed consumers can register scenarios with `qtui_add_scenario_test(NAME ... TARGET ... SCRIPT ...)` or its `EMBEDDED` form. The helper supports deterministic dimensions, bounded timeouts, artifact directories, labels, and explicit `LIVE` output. Automated runs are quiet by default.

#### void start()

Validates configuration and object lifetimes, loads the JavaScript scenario, creates the artifact directory, waits for the initial committed frame, and invokes its `function main(terminal)` entry point. Call it once, normally through `QTimer::singleShot(0, driver, &TestRunner::start)`.

## Ownership and Lifecycle

Every instance uses `QCoreApplication::instance()` as its QObject parent and is deleted with the application. `QQmlEngine`, `Terminal`, and `TerminalEngine` references are guarded pointers and are not owned by the driver. Destroying any required object during a scenario fails the test.

`startFromEnvironment()` allocates the driver and schedules `start()` automatically. A directly constructed driver must have `start()` scheduled explicitly. Scenario completion emits `finished()` and schedules application exit; it does not synchronously destroy the driver.

## Thread Safety

`TestRunner` is GUI-thread only. It enters bounded nested Qt event loops to keep QML timers, rendering, and signals active during waits. Calls from worker threads or concurrent calls are unsupported. Nested wait operations are explicitly rejected.

## JavaScript Exposure

`TestRunner` is not registered as a constructible QML type. At scenario startup, the driver wraps itself in a JavaScript facade and passes it as the `terminal` argument to `function main(terminal)`. The global namespace remains clean, and the script can call all `Q_INVOKABLE` methods on `terminal` without owning the C++ object.

Scenario callbacks passed to `waitForCondition()` run in the same JavaScript engine and may close over ordinary scenario variables. They should be fast and side-effect-free so rendering and timers continue to make progress.

## Inter-Class Interactions

The driver sends decoded key and text input directly to `TerminalEngine`, subscribes to committed-frame and render-failure signals, and reads the engine's current `TextGridBuffer`. It watches the `QQmlEngine`, `Terminal`, and `TerminalEngine` lifetimes. It uses the QML engine both to evaluate the scenario and to propagate driver failures as JavaScript errors.

## Usage Example

```cpp
#include <QTimer>
#include <QQmlApplicationEngine>
#include <Terminal.h>
#include <Testing/TestRunner.h>

auto *terminal = qobject_cast<qtui::Terminal *>(engine.rootObjects().constFirst());
auto *driver = new qtui::TestRunner(
    &engine, terminal,
    QStringLiteral(":/qt/qml/MyApplication/test_scenario.js"),
    QStringLiteral("/tmp/test-artifacts"), QSize(80, 24),
    1500, 8000, QString());
QTimer::singleShot(0, driver, &qtui::TestRunner::start);
```

---
AI assistance has been used to create this output.
