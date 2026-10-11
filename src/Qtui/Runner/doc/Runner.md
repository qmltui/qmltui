# Runner

## Class Overview

`qtui::Runner` turns a running Qtui `Terminal` application into a bounded JavaScript-driven automation and test oracle. It loads a scenario from a filesystem or Qt resource path, invokes its `function main(terminal)` entry point with a driver facade, drives input, verifies committed frames, and writes machine-readable diagnostics. A synchronous scenario succeeds when `main` returns normally; `terminal.finish()` is optional.

The class is intended for application-level automated scenarios, agent harnesses, and tests. It operates on an explicitly supplied `Terminal`, so applications with multiple terminals do not rely on global target selection.

## Project Structure and Dependencies

The declaration is in `Runner/Runner.h` and the implementation is in `Runner/Runner.cpp`. `QtuiRunner` is available when either `QTUI_BUILD_TESTS` or `QTUI_ENABLE_RUNNER` is enabled. Consumers link against `Qtui::Runner`.

Its only build dependencies are Qt 6 QML and `Qtui::Core`; a `QCoreApplication` is sufficient, and no Qt GUI module is needed. The driver collaborates with these project types:

- `Terminal` identifies the UI under test.
- `TerminalEngine` injects input, requests rendering, publishes committed-frame notifications, and provides the current frame.
- `TextGridBuffer` contains the committed terminal cells used by text assertions and artifacts.

## Class Hierarchy and Role

`Runner` derives directly from `QObject`. `QObject` provides parent-based ownership, signal delivery, timers, and the meta-object data required to expose `Q_INVOKABLE` methods to JavaScript. The class is `final`, and copying and moving are disabled.

## Signals

#### finished(int exitCode)

Emitted once when the scenario has passed or failed and process exit has been scheduled. An exit code of `0` indicates success; `1` indicates a scenario, configuration, JavaScript, timeout, rendering, assertion, or artifact failure. Consumers may connect this signal to test-harness reporting, but the driver also schedules `QCoreApplication::exit(exitCode)`.

## Public Slots and Q_INVOKABLE Methods

The methods in this section are callable from JavaScript through the `terminal` argument passed to `function main(terminal)`. They must be called while the scenario is running. Unless stated otherwise, invalid arguments or failed operations fail the complete scenario, produce a failure artifact when a frame is available, and schedule a non-zero process exit.

Every optional parameter may also be given as one trailing options object: `terminal.click(10, 5, { button: "right" })`, `terminal.waitForText("READY", { timeout: 3000 })`. An unknown option key fails the scenario, so a typo can never pass silently. Mixing positional optional arguments with an options object in the same call is an error.

#### void press(const QString &key, const QStringList &modifiers = {})

Sends one key event to the target terminal and waits for the resulting committed frame. `key` accepts Enter/Return, Esc/Escape, Tab, Backtab, Backspace, Space, the arrow keys, Home, End, Insert, Delete, PageUp, PageDown, F1 through F35, one ASCII character, or any single Unicode grapheme, which is routed through the input decoder exactly like typed text (modifiers are invalid with such a text key). Modifier names are case-insensitive and accept Shift, Ctrl/Control, Alt, and Meta. Backtab automatically applies Shift. Control, Alt, and Meta combinations do not carry text. JavaScript usage: `terminal.press("Enter")`, `terminal.press("a", ["Shift"])`, or `terminal.press("é")`.

#### void typeText(const QString &text)

Types non-empty Unicode text and waits once for the resulting committed frame. The UTF-8 bytes go through the same input decoder as a real keyboard, so each character becomes the key event a terminal keystroke produces: letters map to `Qt.Key_A` through `Qt.Key_Z` whatever their case, digits and punctuation to their own codes, newline to `Qt.Key_Return`, tab to `Qt.Key_Tab`, and control characters to their Ctrl combinations. `Keys.onPressed` handlers that compare `event.key` therefore react to scripted typing exactly as they react to a user. Because the decoder is the real one, a lone Escape byte waits for a following byte exactly as it does on a terminal; use `press("Escape")` for that key. Use this for text entry rather than issuing one `press()` call per character; use `press()` when a modifier is needed. JavaScript usage: `terminal.typeText("café 世界")`.

#### void sleep(int durationMs)

Waits for the requested duration while continuing to process Qt events, QML timers, rendering, and the scenario watchdog. `durationMs` must be between 1 and 30,000 milliseconds. The overall scenario watchdog can fail the test before the sleep completes. Nested wait operations are rejected. Under the virtual clock, `sleep()` is `advanceTime()`: it passes application time instantly instead of waiting. JavaScript usage: `terminal.sleep(250)`.

#### void advanceTime(int durationMs)

Virtual clock only. Advances Qtui time by `durationMs` in steps of at most 16 ms — or to the next Qtui timer deadline when that is sooner — firing Qtui timers, animations, and QML `Timer`s at their exact virtual instants and committing frames for whatever they invalidated. A step that scheduled nothing commits no frame, so `frameCount()` comparisons remain meaningful. On the real clock the call fails with the variable that selects virtual mode. JavaScript usage: `terminal.advanceTime(500)`.

#### double now()

Milliseconds on the Qtui clock: virtual time under the virtual clock, wall time since process start otherwise. JavaScript usage: `const t = terminal.now();`

#### void waitForFrame(int timeoutMs = 2000)

Requests rendering and waits for a committed frame whose sequence is newer than the frame visible when the call began. `timeoutMs` must be between 1 and 30,000 milliseconds. The scenario fails if no new frame is committed before the timeout. JavaScript usage: `terminal.waitForFrame()` or `terminal.waitForFrame(5000)`.

#### void expectRepaint(int timeoutMs = 2000)

Waits for a committed frame **without** requesting one, so it verifies that the application itself invalidated the scene, and fails if no frame arrives before the timeout. Use it to catch missing invalidation — a bug class every other assertion hides, because `waitForFrame()` and the `expect*` family request a render themselves and therefore pass even when a change scheduled no repaint. Pair it with `injectKey()`/`injectText()`: the regular input methods wait for a frame and would consume the very repaint being asserted. JavaScript usage: `terminal.injectKey("k"); Test.expectRepaint();`

#### void injectKey(const QString &key, const QStringList &modifiers = {})

`press()` without the implicit frame wait. Same key and modifier names, no rendering requested and no frame awaited. JavaScript usage: `terminal.injectKey("Down", ["Shift"])`.

#### void injectText(const QString &text)

`typeText()` without the implicit frame wait: the same per-character key events, no rendering requested and no frame awaited. JavaScript usage: `terminal.injectText("abc")`.

#### void paste(const QString &text)

Sends non-empty text as one bracketed paste through the input decoder and waits for the resulting committed frame. Newlines and tabs inside the paste arrive as data, exactly as a terminal in bracketed-paste mode delivers them, instead of acting as the Return and Tab keys. JavaScript usage: `terminal.paste("line1\nline2")`.

#### void injectPaste(const QString &text)

`paste()` without the implicit frame wait, for use with `expectRepaint()`.

#### void resize(int columns, int rows)

Resizes the test viewport to `columns` x `rows` (each 1 through 1,000) and waits until a frame with the new dimensions is committed, bounded by the operation timeout. Use it to exercise reflow: content bound to the root size must move within the same scenario. JavaScript usage: `terminal.resize(60, 20)`.

#### object size()

Returns `{columns, rows}` of the current committed frame. JavaScript usage: `const size = terminal.size();`

#### int frameCount()

Sequence number of the most recently committed frame, or `-1` when no engine is attached. Compare it across an interaction to assert that nothing repainted. JavaScript usage: `const before = terminal.frameCount();`

#### object frameStats()

Cost of the most recently committed frame and totals for the session: `frame`, `paintMicros`, `diffMicros`, `writeMicros`, `diffBytes`, `bytesWritten`, `dirtyDuringPaint`, `queuedBytes`, `frames`, `paintMicrosAvg`, `paintMicrosMax`, `diffMicrosAvg`, `bytesTotal`, `dirtyDuringPaintTotal`, and `skippedForBackpressure`. `bytesTotal` sums encoded diff bytes, so it is meaningful in quiet sessions where nothing reaches a terminal. Use it for idle-budget assertions: capture the stats, interact or wait, and compare. JavaScript usage: `const stats = terminal.frameStats();`

#### void waitForCondition(const QJSValue &condition, int timeoutMs = 2000)

Evaluates a synchronous JavaScript callback immediately and then approximately every 10 milliseconds until it returns a truthy value. The callback receives two arguments: the normalized text of the current committed frame and the number of milliseconds elapsed since waiting began. `timeoutMs` must be between 1 and 30,000 milliseconds.

The callback must be callable and synchronous. A thrown JavaScript error fails the scenario. Promises are not awaited, and nested calls to `sleep()`, `waitForFrame()`, `waitForText()`, or `waitForCondition()` are rejected. JavaScript usage: `terminal.waitForCondition((frameText, elapsedMs) => elapsedMs >= 100 && frameText.includes("READY"), 2000)`.

#### void waitForText(const QString &text, int timeoutMs = 2000)

Waits until a committed terminal frame contains the specified non-empty substring. It observes committed frames rather than sleeping for an assumed render delay. `timeoutMs` must be between 1 and 30,000 milliseconds. JavaScript usage: `terminal.waitForText("SYSTEM READY", 3000)`.

#### void expectText(const QString &text, int timeoutMs = -1)

Waits for the next committed frame and fails unless its normalized text contains the specified non-empty substring. A negative timeout means the configured operation timeout; every other `expect*` assertion takes the same optional timeout. Use `waitForText()` when the expected content may appear asynchronously; use `expectText()` as an immediate assertion after an operation.

#### void expectNoText(const QString &text, int timeoutMs = -1)

Waits for the next committed frame and fails if its normalized text contains the specified substring. An empty substring is invalid because every string contains it.

#### void captureText(const QString &relativePath)

Waits for the next committed frame and writes its normalized UTF-8 text beneath the configured artifact directory. Trailing spaces are removed from each row. `relativePath` must be a non-empty relative path contained by the artifact directory; absolute paths and parent traversal are rejected. Missing parent directories inside the artifact directory are created automatically.

#### void captureStyled(const QString &relativePath)

Waits for the next committed frame and writes its styled text (the `styledText()` format) beneath the configured artifact directory, with the same path rules as `captureText()`. Run it once to record the baseline for `expectStyled()`, then copy the artifact next to the scenario.

#### QString frameText()

Returns normalized text for the current committed frame without requesting another render.

#### QString styledText()

Returns the normalized frame text followed by a `--- styles` section that records colors and attributes. Each line describes one run of adjacent cells on a row that share the same foreground, background, and attribute set; runs that use the terminal defaults for all three are omitted:

```
 STATUS: ASYNC READY
--- styles
y=3 x=0-0 fg=default bg=#101820FF
y=3 x=1-19 fg=#7FDBCAFF bg=#101820FF bold
y=3 x=20-79 fg=default bg=#101820FF
```

Colors are `#RRGGBBAA` after resolving ANSI palette entries to xterm's default RGB, or `default` for the terminal's own color. Attributes are a comma-separated subset of `bold`, `dim`, `italic`, `underline`, `reverse`, `blink`, `conceal`, and `strikethrough`. The output depends only on the committed cell grid, never on fonts or a display, so it is identical across platforms and CI.

#### object cell(int x, int y)

Returns the current cell's character, continuation state, colors, and attributes. Colors are `#RRGGBBAA`, or `"default"` for the terminal's own color, exactly as in `styledText()`. The `attributes` object carries `bold`, `dim`, `italic`, `underline`, `reversed`, `blink`, `conceal`, and `strikethrough`. Invalid coordinates fail the scenario.

#### array findText(const QString &text)

Returns all visible, single-row matches in row-major order as `{x, y, width}` objects. Coordinates remain correct for wide characters and continuation cells.

#### QString regionText(int x, int y, int width, int height)

Returns normalized text for the sub-grid `[x, y, width, height]` of the current committed frame, without waiting. Trailing spaces are trimmed from each row. Bounds outside the frame fail the scenario.

#### object itemRect(const QString &objectName)

Returns `{x, y, width, height}` in terminal cells of the first visible item with the given `objectName`, clipped to the viewport. A missing, hidden, or fully off-screen item fails the scenario. Like `rootObject()`, this locates and computes; the assertion itself belongs on the rendered frame.

#### void waitForItem(const QString &objectName, int timeoutMs = 2000)

Waits until an item with the given `objectName` is visible inside the committed viewport.

#### object region(int x, int y, int width, int height)

Returns a locator scoped to the rectangle, with `text()`, `findText()`, `expectText()`, `expectNoText()`, `waitForText()`, `expectSnapshot()`, `clickText()`, and `click()` operating only inside it. `expectTextInRegion()` remains as the one-shot shorthand. JavaScript usage: `terminal.region(0, 0, 80, 2).expectText("HEADER")`.

#### object item(const QString &objectName)

`region()` for the rectangle of `itemRect(objectName)`, so assertions and clicks can target one component without whole-frame substring matching. `click()` clicks the rectangle's center. JavaScript usage: `terminal.item("sidebar").expectText("Services")`.

#### void expectRegionSnapshot(int x, int y, int width, int height, const QString &snapshotRelativePath, int timeoutMs = -1)

`expectSnapshot()` for one region's normalized text, with the same mismatch artifacts and update mode. Also reachable as `region(...).expectSnapshot(path)`.

#### void clickText(const QString &text, int occurrence = 0, const QString &button = "left", const QStringList &modifiers = {})

Clicks the center of the selected rendered-text match and waits for the resulting committed frame.

#### void click(int x, int y, const QString &button = "left", const QStringList &modifiers = {})

Sends a mouse press and release event at terminal coordinates `(x, y)`. Accepts button `"left"`, `"middle"`, or `"right"`, and optional modifiers `["Shift", "Ctrl", "Alt", "Meta"]`. JavaScript usage: `terminal.click(10, 5)` or `terminal.click(10, 5, "right")`.

#### void doubleClick(int x, int y, const QString &button = "left", const QStringList &modifiers = {})

Sends two consecutive mouse press and release cycles at terminal coordinates `(x, y)`. JavaScript usage: `terminal.doubleClick(10, 5)`.

#### void hover(int x, int y, const QStringList &modifiers = {})

Moves the mouse to `(x, y)` with no button held. The event is delivered only while the application has hover (any-motion) reporting enabled — a `MouseArea` with `hoverEnabled` — exactly as a real terminal sends it; otherwise it is dropped and only a frame is awaited. JavaScript usage: `terminal.hover(10, 5)`.

#### void mouseDown(int x, int y, const QString &button = "left", const QStringList &modifiers = {})

Presses a mouse button at `(x, y)` without releasing it, for press-and-hold interactions and custom drags. JavaScript usage: `terminal.mouseDown(10, 5)`.

#### void mouseUp(int x, int y, const QString &button = "left", const QStringList &modifiers = {})

Releases a mouse button at `(x, y)`. JavaScript usage: `terminal.mouseUp(10, 5)`.

#### void drag(int startX, int startY, int endX, int endY, const QString &button = "left", const QStringList &modifiers = {})

Presses at `(startX, startY)`, reports button-held motion cell by cell along the path — as a terminal in button-motion mode does — and releases at `(endX, endY)`. JavaScript usage: `terminal.drag(5, 5, 25, 5)`.

#### void scroll(int x, int y, int deltaX, int deltaY, const QStringList &modifiers = {})

Sends a mouse wheel scroll event at terminal coordinates `(x, y)` with horizontal `deltaX` and vertical `deltaY`. JavaScript usage: `terminal.scroll(15, 10, 0, 120)`.

#### void waitForRegex(const QString &pattern, int timeoutMs = 2000)

Waits until a committed terminal frame matches the regular expression `pattern`. JavaScript usage: `terminal.waitForRegex("STATUS:\\s+READY", 3000)`.

#### void expectRegex(const QString &pattern)

Fails the scenario unless the next committed frame matches the regular expression `pattern`. JavaScript usage: `terminal.expectRegex("System\\s+v\\d+\\.\\d+")`.

#### void expectTextInRegion(int x, int y, int width, int height, const QString &text)

Fails the scenario unless the bounding box `[x, y, width, height]` contains `text`. JavaScript usage: `terminal.expectTextInRegion(0, 0, 80, 2, "HEADER")`.

#### void expectStyle(int x, int y, const QJSValue &expectedStyle)

Fails the scenario unless cell attributes at `(x, y)` match the properties specified in `expectedStyle` (e.g. `{ bold: true, character: "A" }`). JavaScript usage: `terminal.expectStyle(0, 0, { bold: true })`. The `dim` attribute can be checked with `{ dim: true }`. `foreground` and `background` accept a color string or `"default"` for the terminal's own color.

#### void expectSnapshot(const QString &snapshotRelativePath, int timeoutMs = -1)

Compares the committed frame's normalized text against a snapshot resolved relative to the scenario. On mismatch, writes `snapshots/<name>.actual.txt` and a unified `snapshots/<name>.diff` under the artifact directory. JavaScript usage: `terminal.expectSnapshot("snapshots/baseline.txt")`.

#### void expectStyled(const QString &snapshotRelativePath, int timeoutMs = -1)

Like `expectSnapshot()`, but compares `styledText()`, so a color or attribute regression fails the scenario even when the characters are unchanged. The mismatch artifacts are the same, and the diff names the exact rows and columns whose style changed. JavaScript usage: `terminal.expectStyled("snapshots/baseline_styled.txt")`.

## Snapshot Update Mode

`QTUI_RUNNER_UPDATE_SNAPSHOTS=1` makes `expectSnapshot()`, `expectStyled()`, and `expectRegionSnapshot()` rewrite their baseline from the committed frame and pass, so recording and refreshing golden files is one command: `QTUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest -R <test>`. The scenario runner forwards the variable to the launched process. An update is never silent: every rewritten path is listed in the final stderr summary and under `artifacts.updatedSnapshots` in `result.json`. A missing baseline in normal mode fails with a message naming the variable. A baseline inside a Qt resource cannot be written back; update mode then records the actual frame under the artifact directory and fails with both paths. In CMake, `qtui_add_scenario(... UPDATE_SNAPSHOTS)` forces the mode on for one test and `NO_UPDATE_SNAPSHOTS` pins it off so a verifying or negative test stays deterministic under a global update run.

#### object rootObject()

Returns the root `Terminal` QML object. Use it to compute expectations — reading `root.width` to derive an expected label, or a property that feeds a calculation — never as the assertion itself: a visible-behavior test asserts the rendered frame, not a QML property. JavaScript usage: `const columns = terminal.rootObject().width;`

#### void fail(const QString &message)

Fails the scenario immediately with `message`. An empty message is replaced with `Scenario failed`. The method writes `failure.txt` when a committed frame is available, stops the watchdog, emits `finished(1)`, raises a JavaScript error while a script is being evaluated, and schedules process exit with code `1`.

#### void finish()

Marks the scenario as finishing. Calling `terminal.finish()` explicitly is optional: if a script or Promise returns normally without uncaught errors or failed assertions, `Runner` will automatically auto-finish the scenario successfully.

## Promises

A `main` that returns a Promise keeps the scenario active until the Promise resolves (pass) or rejects (fail), still bounded by the scenario watchdog. This is a compatibility affordance, not a programming model: every driver operation is synchronous and none returns a Promise, so an `async main` gains nothing over the straight-line form. Prefer synchronous scenarios.

## Public Methods

#### Runner(QQmlEngine *qmlEngine, Terminal *terminal, QString scriptPath, QString artifactDirectory, QSize viewport, int operationTimeoutMs, int scenarioTimeoutMs, QString configurationError)

Constructs the driver with an explicit QML engine and terminal. `scriptPath` may be a filesystem path or a Qt resource path such as `:/qt/qml/MyApplication/test_scenario.js`. `artifactDirectory` is the root for captures and failure diagnostics. `viewport` is applied immediately so the terminal's first queued frame already uses the deterministic test size. The operation timeout must be between 1 and 30,000 milliseconds; the scenario watchdog accepts up to 300,000. A non-empty `configurationError` makes `start()` fail before loading the scenario.

Construction does not start execution. Schedule `start()` after the application has loaded its root `Terminal`.

#### static Runner *startFromEnvironment(QQmlEngine *qmlEngine, Terminal *terminal)

Creates and schedules a runner from environment variables. It returns `nullptr` when no request is present. `QTUI_RUNNER_SCRIPT`, `QTUI_RUNNER_EVAL`, and `QTUI_RUNNER_CAPTURE_AFTER_MS` are mutually exclusive.

| Variable | Default | Description |
|----------|---------|-------------|
| `QTUI_RUNNER_SCRIPT` | None | Required filesystem scenario path. |
| `QTUI_RUNNER_EVAL` | None | Inline JavaScript scenario; its temporary file is removed at completion. |
| `QTUI_RUNNER_CAPTURE_AFTER_MS` | None | Wait this many milliseconds (0 through 30,000; `0` means the first committed frame), then write `capture.txt` and `capture_styled.txt`. The delay plus the operation timeout must fit within `QTUI_RUNNER_TIMEOUT_MS`. |
| `QTUI_RUNNER_ARTIFACT_DIR` | Per-process temporary directory | Root directory for artifacts. |
| `QTUI_RUNNER_COLUMNS` | `80` | Viewport columns, from 1 through 1,000. |
| `QTUI_RUNNER_ROWS` | `24` | Viewport rows, from 1 through 1,000. |
| `QTUI_RUNNER_OPERATION_TIMEOUT_MS` | `2000` | Per-operation timeout, from 1 through 30,000 milliseconds. |
| `QTUI_RUNNER_TIMEOUT_MS` | `10000` | Overall scenario watchdog, from 1 through 300,000 milliseconds. |
| `QTUI_RUNNER_LIVE` | Off | Emit live ANSI rendering instead of quiet frame commits. C++: `useEmbeddedScenario(true)` or `setLiveOutput()`. |
| `QTUI_RUNNER_EMBEDDED_SCENARIO` | Off | Marks a run driven by a directly constructed `Runner`, so the engine applies the same quiet-by-default terminal behavior as an environment-requested scenario. C++: `useEmbeddedScenario()`. |
| `QTUI_RUNNER_UPDATE_SNAPSHOTS` | Off | Rewrite snapshot baselines instead of failing on mismatch; see Snapshot Update Mode. C++: `setUpdateSnapshots()`. |
| `QTUI_RUNNER_CLOCK` | `real` | `virtual` freezes Qtui time; see The Virtual Clock. C++: `useVirtualClock()`. |

Every Runner setting uses the `QTUI_RUNNER_` prefix. There are no alternate or legacy spellings. The environment is read in `startFromEnvironment()` only, with two deliberate exceptions: the terminal engine reads the session and live variables when the root `Terminal` is created, and `start()` falls back to `QTUI_RUNNER_UPDATE_SNAPSHOTS` when `setUpdateSnapshots()` was not called, so `QTUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest` also refreshes the baselines of embedded runs. A directly constructed `Runner` takes the constructor arguments plus the three methods below instead of the variables.

#### static void useEmbeddedScenario(bool liveOutput = false)

Marks the process as a scenario run driven by a directly constructed `Runner`, as `QTUI_RUNNER_EMBEDDED_SCENARIO=1` does: every `Terminal` created afterwards stays quiet — no ANSI output, no frame pacing — unless `liveOutput` is true or `QTUI_RUNNER_LIVE` is set. Call it before loading QML, like `useVirtualClock()`. A `Runner` constructed without it still switches its engine into the quiet test session from its constructor on, but the terminal's startup sequences (raw mode, cursor, bracketed paste) have reached the terminal by then.

#### void setLiveOutput(bool live)

Renders the scenario to the real terminal instead of committing frames quietly, as `QTUI_RUNNER_LIVE=1` does. Takes effect immediately and can be toggled during a run; outside a test session output is on regardless.

#### void setUpdateSnapshots(bool update)

Selects snapshot update mode for this run, as `QTUI_RUNNER_UPDATE_SNAPSHOTS=1` does. An explicit value wins over the environment in both directions, so a harness can pin verification off under a global update run or record baselines without touching the environment. Call it before `start()`.

## The Virtual Clock

`QTUI_RUNNER_CLOCK=virtual` — or `qtui_add_scenario(... CLOCK VIRTUAL)`, or `Runner::useVirtualClock()` before QML loads — runs the scenario on a frozen timeline: Qtui timers and animations, cursor blink, tooltip delays, toast lifetimes, `AnimatedSprite`, and QML `Timer` advance only through `advanceTime()` and `sleep()`. Timers fire at their exact virtual instants, an animation holds any mid-state for inspection, and a 10-second walkthrough of timed behavior executes in milliseconds without flakiness under load.

The mode is chosen once, before QML loads, and never changes mid-run. `waitFor*` timeouts stay on wall time and do not advance the virtual clock; a wait that depends on paused time fails quickly with a hint naming `advanceTime()`. Limits: application `QTimer`s, threads, image providers, network, and `Date.now()` stay on wall time, and a QML `Timer` shorter than the 16 ms step fires at most once per step. Scenarios mixing wall-time sources should use `waitFor*` for those parts. `result.json` records `clock` and, in virtual mode, the final `virtualTimeMs`.

## Structured results

Every requested run atomically writes `result.json` with schema version `1`, status, exit code, mode, scenario, elapsed time, viewport (initial) and finalViewport (after any `resize()` calls), current operation, argument-free operation trace, artifact paths, and any updated snapshot baselines. Successful scenarios write `final.txt`, failures write `failure.txt`, and capture mode uses `capture.txt`. Standard error prints one final summary with the absolute artifact directory.

## Scenario Files

A scenario ending in `.js` is a classic script defining `function main(terminal)`. It is evaluated inside a function scope, so its top-level declarations stay out of the application's JavaScript globals; reported line numbers are unchanged.

The argument is conventionally named `terminal` — the facade is the terminal under test, as Playwright's handle is `page` — and short scenarios often abbreviate it to `t`. Scenarios are plain JavaScript, so TypeScript annotations are a syntax error; editor completion comes from the shipped declarations instead. `Runner/qtui-runner.d.ts` (made ambient by the repository's `jsconfig.json`, and installed next to the headers for consumers) types the whole facade, so one JSDoc line enables autocomplete and hover documentation in any editor with a TypeScript language service:

```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.press("Enter");
}
```

`test_runner_declarations` fails when a facade method is missing from the declarations, so completion cannot silently fall behind the API.

Consumer projects get the same completion from one call after bringing Qtui in — whether through `find_package(Qtui)`, `FetchContent`, or `add_subdirectory`:

```cmake
FetchContent_MakeAvailable(qtui)
qtui_scenario_editor_support()   # optional: DIRECTORY <path>
```

The helper copies the declarations to `<project>/.qtui/qtui-runner.d.ts` (with a `.gitignore` so the directory stays out of version control) and generates a `jsconfig.json` when the project has none; an existing `jsconfig.json` is never modified — a status message names the one include line to add. From there the same `@param {Terminal}` JSDoc line lights up completion in the consumer's own scenario files.

A scenario ending in `.mjs` is an ECMAScript module: it must `export function main(terminal)` and may `import` helper modules with relative paths, so applications can share navigation and setup code between scenarios. Module scenarios load from the filesystem or from Qt resources, including their relative imports. `QTUI_RUNNER_EVAL` stays classic.

## CMake integration

The helpers live in `cmake/QtuiRunner.cmake` and are defined wherever the `Qtui::Runner` target is: `find_package(Qtui)` includes the module from the installed package, and an `add_subdirectory` or `FetchContent` build defines it while configuring the Runner library, before any consumer directory is processed. Nothing needs to `include()` the file by hand. Consumers register scenarios with `qtui_add_scenario(NAME ... TARGET ... SCRIPT ...)` or its `EMBEDDED` form. The helper supports deterministic dimensions, bounded timeouts, artifact directories, labels, explicit `LIVE` output, `CLOCK VIRTUAL|REAL`, and the snapshot-update controls `UPDATE_SNAPSHOTS`/`NO_UPDATE_SNAPSHOTS` with `SNAPSHOT_SEED_SOURCE`/`SNAPSHOT_SEED_DEST`. Automated runs are quiet by default. `qtui_scenario_editor_support()` materializes editor completion for the consumer's scenario files (see Scenario Files).

#### void start()

Validates configuration and object lifetimes, loads the JavaScript scenario, creates the artifact directory, waits for the initial committed frame, and invokes its `function main(terminal)` entry point. Call it once, normally through `QTimer::singleShot(0, driver, &Runner::start)`.

## Ownership and Lifecycle

Every instance uses `QCoreApplication::instance()` as its QObject parent and is deleted with the application. `QQmlEngine`, `Terminal`, and `TerminalEngine` references are guarded pointers and are not owned by the driver. Destroying any required object during a scenario fails the test.

`startFromEnvironment()` allocates the driver and schedules `start()` automatically. A directly constructed driver must have `start()` scheduled explicitly. Scenario completion emits `finished()` and schedules application exit; it does not synchronously destroy the driver.

## Thread Safety

`Runner` is GUI-thread only. It enters bounded nested Qt event loops to keep QML timers, rendering, and signals active during waits. Calls from worker threads or concurrent calls are unsupported. Nested wait operations are explicitly rejected.

## JavaScript Exposure

`Runner` is not registered as a constructible QML type. At scenario startup, the driver wraps itself in a JavaScript facade and passes it as the `terminal` argument to `function main(terminal)`. The global namespace remains clean, and the script can call all `Q_INVOKABLE` methods on `terminal` without owning the C++ object.

Scenario callbacks passed to `waitForCondition()` run in the same JavaScript engine and may close over ordinary scenario variables. They should be fast and side-effect-free so rendering and timers continue to make progress.

## Inter-Class Interactions

The driver sends decoded key and text input directly to `TerminalEngine`, subscribes to committed-frame and render-failure signals, and reads the engine's current `TextGridBuffer`. It watches the `QQmlEngine`, `Terminal`, and `TerminalEngine` lifetimes. It uses the QML engine both to evaluate the scenario and to propagate driver failures as JavaScript errors.

## Usage Example

```cpp
#include <QTimer>
#include <QQmlApplicationEngine>
#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Runner/Runner.h>

qtui::Runner::useEmbeddedScenario();   // before QML loads: quiet terminal
QQmlApplicationEngine engine;
engine.loadFromModule("MyApplication", "Main");
auto *terminal = qobject_cast<qtui::Terminal *>(engine.rootObjects().constFirst());
auto *driver = new qtui::Runner(
    &engine, terminal,
    QStringLiteral(":/qt/qml/MyApplication/test_scenario.js"),
    QStringLiteral("/tmp/test-artifacts"), QSize(80, 24),
    1500, 8000, QString());
driver->setUpdateSnapshots(recordBaselines);   // optional; defaults to the environment
QTimer::singleShot(0, driver, &qtui::Runner::start);
```
