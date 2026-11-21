# Runner

- **Header**: `<QmlTui/Runner/Runner.h>`
- **Inherits**: `QObject`
- **Target**: `QmlTui::Runner`

## Overview

`qmltui::Runner` turns a running QmlTui `Terminal` application into a bounded, JavaScript-driven test harness and automation oracle. It loads a scenario from a filesystem or Qt resource path, invokes its `function main(app)` entry point with a driver facade, injects keyboard and mouse input, verifies committed terminal frames (`TextGridBuffer`), and writes machine-readable diagnostics (`result.json`, `final.txt`, `failure.txt`). A synchronous scenario succeeds when `main` returns normally; calling `app.finish()` is optional.

The class is declared in `src/QmlTui/Runner/Runner.h` (installed as `<QmlTui/Runner/Runner.h>`) and implemented in `src/QmlTui/Runner/Runner.cpp`. Build and link `QmlTui::Runner` when `QMLTUI_ENABLE_RUNNER=ON` (default for top-level builds) or `QMLTUI_BUILD_TESTS=ON`. Its only build dependencies are `Qt6::Core`, `Qt6::Qml`, and `QmlTui::Core` (no `QtGui` required).

`Runner` collaborates with three core types:

- `Terminal`: Identifies the UI root under test.
- `TerminalEngine`: Injects decoded input, schedules rendering, publishes committed-frame notifications, and exposes the current frame.
- `TextGridBuffer`: Holds the committed character-cell grid inspected by assertions and snapshots.

For a step-by-step walkthrough, see [Testing and Recording with Runner](../guides/testing.md).

## Scenario Files and Editor Autocomplete

### `.js` Scripts and `.mjs` Modules

- **Classic scripts (`.js`)**: Define `function main(app)`. Evaluated inside an isolated function scope so top-level declarations do not pollute application globals; reported line numbers remain unchanged.
- **ECMAScript modules (`.mjs`)**: Define `export function main(app)` and can `import` shared helper modules using relative paths (from either the filesystem or Qt resources). `QMLTUI_RUNNER_EVAL` uses classic script mode.

### TypeScript Declarations (`qmltui-runner.d.ts`)

Scenarios are plain JavaScript files. Editor autocomplete and hover documentation come from `src/QmlTui/Runner/qmltui-runner.d.ts` (installed alongside the Runner CMake support). Add one JSDoc annotation above `main`:

```javascript
/** @param {Runner} app */
function main(app) {
    app.press("Enter");
}
```

In consumer CMake projects (using `find_package(QmlTui)`, `FetchContent`, or `add_subdirectory`), call `qmltui_scenario_editor_support()` to copy `.qmltui/qmltui-runner.d.ts` into the project directory and generate `jsconfig.json` when none exists:

```cmake
if(COMMAND qmltui_scenario_editor_support)
    qmltui_scenario_editor_support() # optional: DIRECTORY <path>
endif()
```

An existing `jsconfig.json` is never overwritten; CMake prints a status message showing the `include` entry to add.

## JavaScript `app` (`Runner`) Facade Reference

The methods below are callable on the `app` argument passed to `function main(app)`. Unless stated otherwise, invalid arguments or failed assertions fail the scenario immediately, write `failure.txt` when a frame is available, and schedule process exit with code `1`.

Every optional parameter can also be passed as a trailing options object:

```javascript
app.click(10, 5, { button: "right" });
app.waitForText("READY", { timeout: 3000 });
```

Unknown option keys fail the scenario so typos never pass silently. Mixing positional optional arguments with a trailing options object in the same call is an error.

### Summary by Category

| Category | Methods |
| :--- | :--- |
| **Keyboard & Paste Input** | `press`, `typeText`, `paste`, `injectKey`, `injectText`, `injectPaste` |
| **Mouse Input** | `clickText`, `hoverText`, `click`, `doubleClick`, `hover`, `mouseDown`, `mouseUp`, `drag`, `scroll` |
| **Clock & Frame Sync** | `sleep`, `advanceTime`, `now`, `waitForFrame`, `expectRepaint`, `waitForCondition`, `frameCount`, `frameStats` |
| **Text, Regex & Style Assertions** | `waitForText`, `expectText`, `expectNoText`, `waitForRegex`, `expectRegex`, `expectTextInRegion`, `expectStyle` |
| **Snapshots & Captures** | `expectSnapshot`, `expectStyled`, `expectRegionSnapshot`, `captureText`, `captureStyled` |
| **Inspection & Locators** | `frameText`, `styledText`, `cell`, `findText`, `regionText`, `size`, `resize`, `itemRect`, `waitForItem`, `region`, `item`, `rootObject` |
| **Inline Scrollback** | `scrollbackRowCount`, `scrollbackText`, `waitForScrollbackText`, `expectScrollbackText`, `expectNoScrollbackText`, `captureScrollback`, `captureScrollbackStyled`, `expectScrollbackSnapshot` |
| **Asciicast Recording** | `startRecording`, `stopRecording`, `marker` |
| **Lifecycle** | `finish`, `fail` |

### Keyboard and Clipboard Input

#### `void press(const QString &key, const QStringList &modifiers = {})`

Sends one key event to the target terminal and waits for the resulting committed frame. `key` accepts `Enter`/`Return`, `Esc`/`Escape`, `Tab`, `Backtab`, `Backspace`, `Space`, arrow keys (`Up`, `Down`, `Left`, `Right`), `Home`, `End`, `Insert`, `Delete`, `PageUp`, `PageDown`, `F1` through `F35`, one ASCII character, or any single Unicode grapheme (routed through `InputDecoder` as typed text; modifiers are invalid with a non-ASCII grapheme key). Modifier names are case-insensitive and accept `Shift`, `Ctrl`/`Control`, `Alt`, and `Meta`. `Backtab` automatically applies `Shift`. `Control`, `Alt`, and `Meta` combinations do not carry text.

```javascript
app.press("Enter");
app.press("a", ["Shift"]);
app.press("é");
```

#### `void typeText(const QString &text)`

Types non-empty Unicode text and waits once for the resulting committed frame. The UTF-8 bytes pass through `InputDecoder` just like physical keystrokes: letters map to `Qt.Key_A` through `Qt.Key_Z`, digits and punctuation to their key codes, newline to `Qt.Key_Return`, tab to `Qt.Key_Tab`, and control characters to `Ctrl` combinations. Because `InputDecoder` buffers a lone `Escape` byte waiting for an ANSI sequence, use `press("Escape")` for the Escape key. Use `typeText()` for text entry and `press()` when modifiers are required.

```javascript
app.typeText("café 世界");
```

#### `void paste(const QString &text)`

Sends non-empty text as a single bracketed paste through `InputDecoder` and waits for the resulting committed frame. Newlines and tabs inside the paste arrive as text data instead of acting as the `Return` and `Tab` keys.

```javascript
app.paste("line1\nline2");
```

#### `void injectKey(const QString &key, const QStringList &modifiers = {})`

Same as `press()`, but without requesting rendering or waiting for a frame. Pair with `expectRepaint()`.

```javascript
app.injectKey("Down", ["Shift"]);
```

#### `void injectText(const QString &text)`

Same as `typeText()`, but without requesting rendering or waiting for a frame. Pair with `expectRepaint()`.

#### `void injectPaste(const QString &text)`

Same as `paste()`, but without requesting rendering or waiting for a frame. Pair with `expectRepaint()`.

### Mouse Input

#### `void clickText(const QString &text, int occurrence = 0, const QString &button = "left", const QStringList &modifiers = {})`

Clicks the center of the selected rendered-text match and waits for the resulting committed frame. Text-based mouse helpers translate frame rows through the renderer's screen origin (quiet inline sessions place the frame immediately above the terminal's last row). Coordinate-based mouse methods use screen coordinates, whereas `findText()` returns frame coordinates.

```javascript
app.clickText("Apply");
app.clickText("Delete", 1, "right");
```

#### `void hoverText(const QString &text, int occurrence = 0, const QStringList &modifiers = {})`

Moves the mouse to the center of the selected rendered-text match with no button held and waits for the resulting committed frame. Delivered only while any-motion reporting (`MouseArea.hoverEnabled`) is active.

#### `void click(int x, int y, const QString &button = "left", const QStringList &modifiers = {})`

Sends a mouse press and release event at terminal coordinates `(x, y)` and waits for the committed frame. Accepts button `"left"`, `"middle"`, or `"right"` and optional modifiers `["Shift", "Ctrl", "Alt", "Meta"]`.

```javascript
app.click(10, 5);
app.click(10, 5, "right");
```

#### `void doubleClick(int x, int y, const QString &button = "left", const QStringList &modifiers = {})`

Sends two consecutive mouse press and release cycles at terminal coordinates `(x, y)`.

#### `void hover(int x, int y, const QStringList &modifiers = {})`

Moves the mouse to `(x, y)` with no button held. Delivered only while the application has hover (`MouseArea.hoverEnabled`) enabled; otherwise the move is dropped and only a frame is awaited.

#### `void mouseDown(int x, int y, const QString &button = "left", const QStringList &modifiers = {})`

Presses a mouse button at `(x, y)` without releasing it, for press-and-hold interactions and custom drags.

#### `void mouseUp(int x, int y, const QString &button = "left", const QStringList &modifiers = {})`

Releases a mouse button at `(x, y)`.

#### `void drag(int startX, int startY, int endX, int endY, const QString &button = "left", const QStringList &modifiers = {})`

Presses at `(startX, startY)`, reports button-held motion cell by cell along the path, and releases at `(endX, endY)`.

```javascript
app.drag(5, 5, 25, 5);
```

#### `void scroll(int x, int y, int deltaX, int deltaY, const QStringList &modifiers = {})`

Sends a mouse wheel scroll event at terminal coordinates `(x, y)` with horizontal `deltaX` and vertical `deltaY`.

```javascript
app.scroll(15, 10, 0, 120);
```

### Clock, Timing, and Frame Synchronization

#### `void sleep(int durationMs)`

Waits for `durationMs` (`1..30000` ms) while continuing to process Qt events, QML timers, rendering, and the scenario watchdog. Nested wait operations are rejected. Under the virtual clock, `sleep()` delegates to `advanceTime()` and advances application time immediately without wall-clock waiting.

#### `void advanceTime(int durationMs)`

Virtual clock only. Advances QmlTui time by `durationMs` in steps of at most 16 ms (or to the next QmlTui timer deadline when sooner), firing QmlTui timers, animations, and QML `Timer` items at their exact virtual instants and committing frames for invalidated items. A step that invalidates nothing commits no frame, preserving `frameCount()` comparisons. Calling `advanceTime()` on the real clock fails the scenario with a message naming `QMLTUI_RUNNER_CLOCK=virtual`.

```javascript
app.advanceTime(500);
```

#### `double now()`

Returns elapsed milliseconds on the active QmlTui clock (virtual time under `CLOCK VIRTUAL`, wall time since process start under `CLOCK REAL`).

#### `void waitForFrame(int timeoutMs = 2000)`

Requests rendering and waits for a committed frame whose sequence number is newer than the frame visible when the call began (`timeoutMs` in `1..30000` ms). Fails if no new frame is committed before the timeout.

#### `void expectRepaint(int timeoutMs = 2000)`

Waits for a committed frame **without** requesting one, verifying that the application itself invalidated the scene. Fails if no frame arrives before `timeoutMs`. Pair with `injectKey()`, `injectText()`, or `injectPaste()`, because `press()` and `expect*` request a render themselves and would hide missing invalidation bugs:

```javascript
app.injectKey("k");
app.expectRepaint();
```

#### `void waitForCondition(const QJSValue &condition, int timeoutMs = 2000)`

Evaluates a synchronous JavaScript callback immediately and then approximately every 10 ms until it returns a truthy value (`timeoutMs` in `1..30000` ms). The callback receives `(frameText, elapsedMs)`. Promises are not awaited, and nested wait calls inside the callback are rejected.

```javascript
app.waitForCondition((frameText, elapsedMs) => elapsedMs >= 100 && frameText.includes("READY"), 2000);
```

#### `int frameCount()`

Returns the sequence number of the most recently committed frame, or `-1` when no engine is attached. Compare across an interaction to assert that nothing repainted.

#### `object frameStats()`

Returns timing and byte metrics for the most recently committed frame and cumulative session totals: `frame`, `paintMicros`, `diffMicros`, `writeMicros`, `diffBytes`, `bytesWritten`, `dirtyDuringPaint`, `queuedBytes`, `frames`, `paintMicrosAvg`, `paintMicrosMax`, `diffMicrosAvg`, `bytesTotal`, `dirtyDuringPaintTotal`, and `skippedForBackpressure`. `bytesTotal` sums encoded diff bytes even in quiet headless sessions.

#### The Virtual Clock (`CLOCK VIRTUAL`)

Setting `QMLTUI_RUNNER_CLOCK=virtual`, `--clock virtual`, `qmltui_add_scenario(... CLOCK VIRTUAL)`, or `Runner::useVirtualClock()` before QML loads runs the session on a synthetic timeline. QmlTui timers, animations (`NumberAnimation`, `ColorAnimation`, `PropertyAnimation`), cursor blink, `ToolTip` delays, `AnimatedSprite`, and QML `Timer` items advance only through `advanceTime()` and `sleep()`.

Limits of virtual clock mode:
- `waitFor*` timeouts remain on wall time and do not advance the virtual clock.
- C++ `QTimer` instances, worker threads, async image providers, network I/O, and JavaScript `Date.now()` remain on wall time.
- A QML `Timer` with an interval shorter than 16 ms fires at most once per 16 ms step.

### Frame Text, Regex, and Style Assertions

#### `void waitForText(const QString &text, int timeoutMs = 2000)`

Waits until a committed terminal frame contains the non-empty substring `text` (`timeoutMs` in `1..30000` ms). Observes committed frames as they arrive without sleeping for a fixed delay.

#### `void expectText(const QString &text, int timeoutMs = -1)`

Waits for the next committed frame and fails unless its normalized text contains `text`. A negative `timeoutMs` uses the configured operation timeout (`2000` ms by default). Use `expectText()` immediately after a synchronous action, and `waitForText()` when waiting for asynchronous work.

#### `void expectNoText(const QString &text, int timeoutMs = -1)`

Waits for the next committed frame and fails if its normalized text contains `text`. An empty string is invalid.

#### `void waitForRegex(const QString &pattern, int timeoutMs = 2000)`

Waits until a committed terminal frame matches the regular expression `pattern`.

#### `void expectRegex(const QString &pattern, int timeoutMs = -1)`

Waits for the next committed frame and fails unless its normalized text matches `pattern`.

#### `void expectTextInRegion(int x, int y, int width, int height, const QString &text, int timeoutMs = -1)`

Fails unless the bounding box `[x, y, width, height]` in the next committed frame contains `text`.

#### `void expectStyle(int x, int y, const QJSValue &expectedStyle, int timeoutMs = -1)`

Fails unless the cell at `(x, y)` matches all properties specified in `expectedStyle`: `character`, `foreground`, `background` (color string or `"default"`), `bold`, `dim`, `italic`, `underline`, `reversed` (or `reverse`, matching `styledText()`), `blink`, `conceal`, and `strikethrough`. Unknown keys in `expectedStyle` fail the scenario immediately so misspelled attribute names never pass silently.

```javascript
app.expectStyle(0, 0, { character: "A", foreground: "cyan", bold: true, strikethrough: false });
```

### Frame Snapshots and Captures

#### `void expectSnapshot(const QString &snapshotRelativePath, int timeoutMs = -1)`

Compares the committed frame's normalized UTF-8 text against a snapshot file resolved relative to the scenario script. On mismatch, writes `snapshots/<name>.actual.txt` and a unified `snapshots/<name>.diff` under the artifact directory.

#### `void expectStyled(const QString &snapshotRelativePath, int timeoutMs = -1)`

Compares `styledText()` against `snapshotRelativePath` so color or SGR attribute regressions fail the scenario even when characters are unchanged. Writes the same `.actual.txt` and `.diff` artifacts on mismatch.

#### `void expectRegionSnapshot(int x, int y, int width, int height, const QString &snapshotRelativePath, int timeoutMs = -1)`

Compares normalized text inside `[x, y, width, height]` against `snapshotRelativePath`. Also available as `app.region(x, y, width, height).expectSnapshot(snapshotRelativePath)`.

#### `void captureText(const QString &relativePath)`

Waits for the next committed frame and writes its normalized UTF-8 text (with trailing spaces trimmed per row) to `relativePath` inside the artifact directory. Absolute paths and `..` traversal are rejected.

#### `void captureStyled(const QString &relativePath)`

Waits for the next committed frame and writes its `styledText()` representation to `relativePath` inside the artifact directory.

#### Snapshot Update Mode (`QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1`)

Running with `--update-snapshots` or `QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1` causes `expectSnapshot()`, `expectStyled()`, `expectRegionSnapshot()`, and `expectScrollbackSnapshot()` to write the current committed frame back to the baseline file and pass. Every rewritten path is logged to `stderr` and listed under `artifacts.updatedSnapshots` in `result.json`. In CMake, `qmltui_add_scenario(... UPDATE_SNAPSHOTS)` forces update mode on for a test, and `NO_UPDATE_SNAPSHOTS` pins it off even during a global update run.

### Frame Inspection and Scoped Locators (`region` and `item`)

#### `QString frameText()`

Returns normalized UTF-8 text for the current committed frame without requesting another render.

#### `QString styledText()`

Returns normalized frame text followed by a `--- styles` section describing runs of non-default colors and SGR attributes:

```text
 STATUS: ASYNC READY
--- styles
y=3 x=0-0 fg=default bg=#101820FF
y=3 x=1-19 fg=#7FDBCAFF bg=#101820FF bold
y=3 x=20-79 fg=default bg=#101820FF
```

Colors are `#RRGGBBAA` (with ANSI palette entries resolved to xterm default RGB) or `default`. Attributes are a comma-separated subset of `bold`, `dim`, `italic`, `underline`, `reverse`, `blink`, `conceal`, and `strikethrough`.

#### `object cell(int x, int y)`

Returns `{ x, y, character, continuation, foreground, background, attributes }` for cell `(x, y)` in the current committed frame. Out-of-bounds coordinates fail the scenario.

#### `array findText(const QString &text)`

Returns all visible, single-row matches in row-major order as `{ x, y, width }` objects in frame coordinates.

#### `QString regionText(int x, int y, int width, int height)`

Returns normalized text for the sub-grid `[x, y, width, height]` of the current committed frame without waiting.

#### `object size()` and `void resize(int columns, int rows)`

- `app.size()` returns `{ columns, rows }` of the current test viewport.
- `app.resize(columns, rows)` resizes the test viewport (`1..1000` each) and waits for a frame committed at the new dimensions (`columns × rows` in `AltScreen`, or `columns` wide with bounded content height `<= rows - 1` in `Terminal.Inline`).

#### `object itemRect(const QString &objectName)` and `void waitForItem(const QString &objectName, int timeoutMs = 2000)`

- `app.itemRect(objectName)` returns `{ x, y, width, height }` in terminal cells for the first visible item with `objectName`, clipped to the viewport. Fails if the item is missing, hidden, or off-screen.
- `app.waitForItem(objectName, timeoutMs)` waits until an item with `objectName` is visible inside the committed viewport.

#### `object region(int x, int y, int width, int height)`

Returns a `Region` (`RegionLocator`) scoped to `[x, y, width, height]` (typed as `Region` in `qmltui-runner.d.ts`) supporting `text()`, `findText()`, `expectText()`, `expectNoText()`, `waitForText()`, `expectSnapshot()`, `clickText()`, and `click()` (which clicks the region's center).

```javascript
app.region(0, 0, 80, 2).expectText("HEADER");
```

#### `object item(const QString &objectName)`

Returns a `Region` (`RegionLocator`) for `itemRect(objectName)` (with `objectName` attached) so assertions and clicks can target a specific QML component by `objectName`:

```javascript
app.item("sidebar").expectText("Services");
app.item("submitButton").click();
```

#### `object rootObject()`

Returns the root `Terminal` QML object. Use it only to read geometry or configuration values needed to compute expected frame text; always assert visible behavior on the rendered frame.

### Inline Scrollback Assertions (`Terminal.Inline`)

In `Terminal.Inline` mode, completed rows are committed through `Scrollback` and do not remain in the live bottom viewport. During a Runner session, `TerminalEngine` retains up to `QMLTUI_SCROLLBACK_HISTORY_ROWS` (default `5000`) committed scrollback rows as cells. Calling scrollback methods when `Terminal` is in `AltScreen` mode fails the scenario.

- `int scrollbackRowCount()`: Returns the total number of scrollback rows committed so far. Pass this value as `fromRow` to inspect only output appended after a checkpoint.
- `QString scrollbackText(int fromRow = 0)`: Returns normalized UTF-8 text of committed scrollback rows starting at `fromRow`.
- `void waitForScrollbackText(const QString &text, int timeoutMs = 2000)`: Waits until committed scrollback contains `text`.
- `void expectScrollbackText(const QString &text, int timeoutMs = -1)`: Waits for the next committed frame and fails unless committed scrollback contains `text`.
- `void expectNoScrollbackText(const QString &text, int timeoutMs = -1)`: Waits for the next committed frame and fails if committed scrollback contains `text`.
- `void captureScrollback(const QString &relativePath, int fromRow = 0)`: Writes committed scrollback rows from `fromRow` onwards to `relativePath` under the artifact directory.
- `void captureScrollbackStyled(const QString &relativePath, int fromRow = 0)`: Writes committed scrollback rows from `fromRow` onwards in `styledText()` format.
- `void expectScrollbackSnapshot(const QString &snapshotRelativePath, int fromRow = 0, int timeoutMs = -1)`: Compares committed scrollback rows from `fromRow` onwards against `snapshotRelativePath`.

### Asciicast Recording (`.cast`)

When armed with `--record <path>`, `qmltui_add_scenario(... RECORD <name>.cast)`, or `QMLTUI_RUNNER_RECORD=<path>`, Runner substitutes an in-memory 24-bit color backend and writes emitted ANSI bytes into an [asciicast v2](https://docs.asciinema.org/manual/asciicast/v2/) file. Under `CLOCK VIRTUAL`, frame timestamps advance only through `sleep()` and `advanceTime()`, producing byte-identical `.cast` recordings across runs. `RECORD` cannot be combined with `LIVE`.

- `void startRecording(const QString &relativePath)`: Closes any active recording and starts a new `.cast` file at `relativePath` under the artifact directory, beginning with a full repaint.
- `void stopRecording()`: Flushes pending bytes and closes the active recording. Emits a warning and does nothing if no recording is in progress.
- `void marker(const QString &label)`: Writes an asciicast `"m"` chapter marker event at the current timestamp. Emits a warning and does nothing if no recording is in progress.

### Scenario Lifecycle (`finish`, `fail`, and Promises)

- `void finish()`: Explicitly marks the scenario as finishing. Optional for synchronous scripts, which finish automatically when `main(app)` returns.
- `void fail(const QString &message)`: Immediately fails the scenario with `message`, writes `failure.txt` if a frame is available, emits `finished(1)`, and schedules exit code `1`.
- **Promises**: If `main(app)` returns a Promise, Runner keeps the scenario active until the Promise resolves or rejects (bounded by the scenario watchdog). Because all `app.*` operations are synchronous, prefer synchronous `main(app)` functions.

## `qmltui-runner` Command-Line Launcher

`qmltui-runner` launches a `QmlTui::Runner`-linked executable from the shell without requiring CMake test registration:

```bash
qmltui-runner --script test.js -- ./myApp --config demo.json
qmltui-runner --eval 'app.clickText("Save")' -- ./myApp
qmltui-runner --capture-after-ms 500 -- ./myApp
qmltui-runner --clock virtual --sweep --idle-ms 300 --ruler --styled -- ./myApp
qmltui-runner --script tour.js --clock virtual --record tour.cast -- ./myApp
```

Specify `--script`, `--eval`, `--capture-after-ms`, or probe options (`--sizes` / `--sweep` / `--idle-ms`). Launcher options precede the target executable; everything after `--` (or after the executable path) is forwarded as arguments to the application.

| Option | Default | Description |
| :--- | :--- | :--- |
| `--script <path>` | None | JavaScript scenario path (`.js`, `.mjs`, or Qt resource `:/...`). |
| `--eval <code>` | None | Inline JavaScript scenario code. |
| `--capture-after-ms <ms>` | None | Delay (`0..30000` ms) before writing `capture.txt` and `capture_styled.txt`. |
| `--sizes <list>` | None | Comma-separated `<cols>x<rows>` viewport sweep writing `frame_<WxH>.txt` (incompatible with `--script`). |
| `--sweep` | Off | Shorthand for `--sizes 60x18,80x24,120x36`. |
| `--idle-ms <ms>` | None | Verify zero unscripted repaints across `1..30000` ms after capture (incompatible with `--script`). |
| `--columns <n>`, `--rows <n>` | `80`, `24` (terminal size with `--live`) | Initial viewport dimensions (`1..1000` cells). When `--live` is set without `--columns` or `--rows`, the application uses the terminal emulator's dimensions. |
| `--artifacts <dir>` | Retained temp dir | Output directory (must be new or empty unless `--clean-artifacts` is set). |
| `--clean-artifacts` | Off | Clear an existing Runner artifact directory (containing `result.json`) before running. |
| `--print`, `--ruler` | Off | Print `frameStats` and captured frames to `stdout` (`--ruler` adds 0-based `(x, y)` coordinate rulers; incompatible with `--live`). |
| `--styled` | Off | Include `--- styles` SGR/color runs when probing or printing frames. |
| `--scrollback` | Off | Capture and print `Terminal.Inline` committed scrollback rows (`scrollback.txt`). |
| `--live` | Off | Forward live ANSI rendering to the terminal. |
| `--clock <real\|virtual>` | `real` | Select the Runner clock before application startup. |
| `--record <path>` | Off | Write an asciicast v2 `.cast` file (relative to the artifact directory; incompatible with `--live`). |
| `--update-snapshots` | Off | Rewrite snapshot baselines from committed frames. |
| `--operation-timeout-ms <ms>` | `2000` | Per-operation timeout (`1..30000` ms). |
| `--timeout-ms <ms>` | `300000` | Overall scenario watchdog timeout (`1..300000` ms). |
| `--startup-timeout-ms <ms>` | `5000` | Runner activation handshake deadline (`1..300000` ms). |

### How Probe and Output Flags Compose

Launcher flags fall into three groups:

- **Base modes** (`--script`, `--eval`, `--capture-after-ms`): specify at most one.
- **Probe modifiers** (`--sweep`, `--sizes`, `--idle-ms`, `--scrollback`): synthesize an inline `EVAL` scenario instead of requiring a script file. They cannot be combined with `--script` (exit `2`), but compose cleanly with `--eval` and `--capture-after-ms`. Any probe modifier also enables `QMLTUI_DEBUG_PAINT_DIRTY=1` (unless already set) and fails the run with exit code `1` if `frameStats.dirtyDuringPaintTotal > 0`.
- **Output flags** (`--print`, `--ruler`, `--styled`): not standalone modes—pair them with a base mode or probe modifier. `--ruler` implies `--print`, works with `--script`, and enables `QMLTUI_DEBUG_PAINT_DIRTY=1`; both `--print` and `--ruler` are incompatible with `--live`.
  - `--ruler` adds 0-based `(x, y)` column/row rulers to `stdout` only; saved files in `--artifacts` (`frame_<WxH>.txt`, `final.txt`, `capture.txt`, snapshots) remain plain un-ruled UTF-8 frames.
  - `--styled` writes `frame_<WxH>_styled.txt` (or `capture_styled.txt`, `final_styled.txt` / `failure_styled.txt` with `--script` or `--eval`, plus `scrollback_styled.txt` with `--scrollback`) containing logical `TextGridBuffer` `#RRGGBBAA`/SGR runs. By itself it does not print to `stdout`—pair it with `--print` or `--ruler` to print the `--- Styles ---` section.

When probe modifiers are active (for example `--sweep --idle-ms 300 --styled`), `qmltui-runner` launches the target at the first viewport (`60x18` for `--sweep`) and synthesizes:

1. `app.sleep(<capture-after-ms>)` (when `--capture-after-ms > 0`).
2. User `--eval` body (when `--eval` is also provided).
3. `app.captureText("frame_<W>x<H>.txt")` (plus `app.captureStyled(...)` when `--styled` is set) and `app.captureScrollback(...)` (when `--scrollback` is set).
4. For each remaining viewport in `--sizes`/`--sweep`: `app.resize(W, H)` followed by `captureText` (and `captureStyled`).
5. Idle-budget gate (when `--idle-ms` is set): compares `app.frameCount()` across `app.sleep(<idle-ms>)` and fails if any unscripted repaint occurred.

### Exit Codes and Handshake Protocol

- **Exit code `0`**: Scenario passed and produced a valid schema-v1 `result.json` with `status: "passed"`.
- **Exit code `1`**: Scenario failed a test assertion, threw a script error (`status: "failed"` in `result.json`), or reported `dirtyDuringPaintTotal > 0` during a probe run (`--sweep` / `--sizes` / `--idle-ms` / `--scrollback`).
- **Exit code `2`**: Invalid CLI arguments, startup/handshake timeout, child process crash, or missing/inconsistent `result.json`.

When `qmltui-runner` starts a child process, it clears inherited `QMLTUI_RUNNER_*` variables, sets `QMLTUI_RUNNER_HANDSHAKE_PATH` and `QMLTUI_RUNNER_HANDSHAKE_TOKEN`, and verifies that `Runner::start()` writes `{"protocolVersion":1,"token":"...","pid":123}` before `--startup-timeout-ms` expires. After activation, the child has `--timeout-ms` plus 5 seconds to exit before termination (`SIGTERM` followed by `SIGKILL` after 1 second).

## CMake Integration (`qmltui_add_scenario`)

`cmake/QmlTuiRunner.cmake` is included automatically when `QmlTui::Runner` is built or imported via `find_package(QmlTui)`. Guard linking and scenario registration with `BUILD_TESTING` (defined by `include(CTest)`) so production builds (`-DBUILD_TESTING=OFF`) do not force-load `QmlTui::Runner` into shipped binaries:

```cmake
include(CTest)

if(BUILD_TESTING AND TARGET QmlTui::Runner)
    target_link_libraries(my_app PRIVATE QmlTui::Runner)
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

### `qmltui_add_scenario` Parameters

`qmltui_add_scenario_test` is provided as an alias for `qmltui_add_scenario`.

| Parameter | Default | Description |
| :--- | :--- | :--- |
| `NAME <test-name>` | Required | CTest test name. |
| `TARGET <target>` | Required | Executable target linked against `QmlTui::Runner`. |
| `SCRIPT <path>` | Required (unless `EMBEDDED`) | Path to the `.js`, `.mjs`, or `:/...` scenario script. |
| `EMBEDDED` | Off | Run an executable that constructs `qmltui::Runner` directly in C++ instead of reading `QMLTUI_RUNNER_SCRIPT`. |
| `COLUMNS <n>`, `ROWS <n>` | `80`, `24` | Initial terminal viewport dimensions. |
| `CLOCK <REAL\|VIRTUAL>` | `REAL` | Run on the real clock or synthetic virtual clock. |
| `RECORD <relative-path.cast>` | None | Record an asciicast v2 file into `${QMLTUI_RECORD_DIR}` (or `ARTIFACT_DIR` when unset) and default the CTest label to `Demo`. |
| `LIVE` | Off | Emit live ANSI output during the test (incompatible with `RECORD`). |
| `UPDATE_SNAPSHOTS` | Inherited from env | Force snapshot baseline updating on for this test. |
| `NO_UPDATE_SNAPSHOTS` | Off | Pin snapshot updating off even when `QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1` is set in the environment. |
| `OPERATION_TIMEOUT_MS <ms>` | `2000` | Per-operation timeout (`1..30000` ms). |
| `SCENARIO_TIMEOUT_MS <ms>` | `30000` | Overall scenario watchdog timeout (`1..300000` ms). |
| `ARTIFACT_DIR <path>` | `<binary-dir>/RunnerArtifacts/<NAME>` | Directory for `result.json`, frame captures, and recordings. |
| `QML_IMPORT_PATH <path>` | `${QT_QML_OUTPUT_DIRECTORY}` or `${QmlTui_QML_IMPORT_PATH}` | QML module import path passed to the scenario driver. |
| `ARGUMENTS <arg>...` | None | Command-line arguments forwarded to `TARGET`. |
| `POST_COMMAND <cmd> [args...]` | `${QMLTUI_RECORD_POST_COMMAND}` when `RECORD` is set, else None | Post-processing command executed in `ARTIFACT_DIR` after the scenario succeeds. Expands `<CAST>`, `<CAST_STEM>` (`<CAST>` without `.cast`), `<ARTIFACT_DIR>`, and `<NAME>`. |
| `LABELS <label>...` | `Demo` when `RECORD` is set, else `Runner` | CTest labels attached to the test. |

## C++ API and Environment Variables (`qmltui::Runner`)

### Environment-Driven Startup (`Runner::startFromEnvironment`)

When `QmlTui::Runner` is linked into an application, QML-created `Terminal` roots invoke `Runner::startFromEnvironment()` automatically on startup. If you construct `qmltui::Terminal` in C++ without QML, call `Runner::startFromEnvironment(&engine, terminal)` after creating the root. It returns `nullptr` when no `QMLTUI_RUNNER_*` request is present in the environment.

`QMLTUI_RUNNER_SCRIPT`, `QMLTUI_RUNNER_EVAL`, and `QMLTUI_RUNNER_CAPTURE_AFTER_MS` are mutually exclusive:

| Environment Variable | Default | Description |
| :--- | :--- | :--- |
| `QMLTUI_RUNNER_SCRIPT` | None | Filesystem or Qt resource (`:/...`) scenario path. |
| `QMLTUI_RUNNER_EVAL` | None | Inline JavaScript scenario code. |
| `QMLTUI_RUNNER_CAPTURE_AFTER_MS` | None | Delay (`0..30000` ms; `0` = first committed frame) before writing `capture.txt` and `capture_styled.txt`. |
| `QMLTUI_RUNNER_ARTIFACT_DIR` | Per-process temp dir | Root directory for output artifacts. |
| `QMLTUI_RUNNER_COLUMNS` | `80` | Initial viewport columns (`1..1000`). |
| `QMLTUI_RUNNER_ROWS` | `24` | Initial viewport rows (`1..1000`). |
| `QMLTUI_RUNNER_OPERATION_TIMEOUT_MS` | `2000` | Per-operation timeout (`1..30000` ms). |
| `QMLTUI_RUNNER_TIMEOUT_MS` | `30000` | Overall scenario watchdog (`1..300000` ms). |
| `QMLTUI_RUNNER_CLOCK` | `real` | `real` or `virtual` clock selection. |
| `QMLTUI_RUNNER_LIVE` | Off | Emit live ANSI terminal output during the scenario. |
| `QMLTUI_RUNNER_EMBEDDED_SCENARIO` | Off | Mark a process driven by a directly constructed `Runner` as a quiet test session before `Terminal` creation. |
| `QMLTUI_RUNNER_UPDATE_SNAPSHOTS` | Off | Rewrite snapshot baselines on mismatch instead of failing. |
| `QMLTUI_RUNNER_RECORD` | None | Absolute path, or relative path under the artifact directory, for recording an asciicast v2 `.cast` file (incompatible with `QMLTUI_RUNNER_LIVE`). |
| `QMLTUI_RUNNER_HANDSHAKE_PATH` | None | File path where `Runner::start()` writes the activation handshake JSON when launched by `qmltui-runner`. |
| `QMLTUI_RUNNER_HANDSHAKE_TOKEN` | None | Token echoed into `QMLTUI_RUNNER_HANDSHAKE_PATH` to verify activation. |
| `QMLTUI_SCROLLBACK_HISTORY_ROWS` | `5000` | Maximum number of committed `Inline` scrollback rows retained by `TerminalEngine` during a Runner session. |

### Direct C++ Construction and Methods

#### `Runner(QQmlEngine *qmlEngine, Terminal *terminal, QString scriptPath, QString artifactDirectory, QSize viewport, int operationTimeoutMs, int scenarioTimeoutMs, QString configurationError)`

Constructs a `Runner` bound to an explicit `QQmlEngine` and `Terminal`. `viewport` is applied immediately so the first frame uses the test dimensions. Pass an empty `QString()` for `configurationError` in normal construction; when non-empty, `start()` fails immediately, records `configurationError` in `result.json`, and exits with code `1`. Schedule `start()` after the root `Terminal` is loaded.

#### `static void useVirtualClock()`

Switches the process-wide QmlTui clock into virtual mode before QML loads (equivalent to `QMLTUI_RUNNER_CLOCK=virtual`).

#### `static void useEmbeddedScenario(bool liveOutput = false)`

Marks the process as an embedded scenario run before QML loads (equivalent to `QMLTUI_RUNNER_EMBEDDED_SCENARIO=1`), keeping terminal startup sequences quiet unless `liveOutput` is `true`.

#### `void setLiveOutput(bool live)` and `void setUpdateSnapshots(bool update)`

- `setLiveOutput(live)` toggles live ANSI terminal rendering on or off.
- `setUpdateSnapshots(update)` overrides `QMLTUI_RUNNER_UPDATE_SNAPSHOTS` for this `Runner` instance when called before `start()`.

#### `void start()` and `void finished(int exitCode)`

- `start()` validates configuration, creates the artifact directory, waits for the initial committed frame, and invokes `main(app)`.
- `finished(int exitCode)` is emitted once when the scenario finishes (`0` = pass, `1` = failure) and `QCoreApplication::exit(exitCode)` is scheduled.

```cpp
#include <QTimer>
#include <QQmlApplicationEngine>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Runner/Runner.h>

qmltui::Runner::useEmbeddedScenario(); // before QML loads: quiet terminal
QQmlApplicationEngine engine;
engine.loadFromModule("MyApplication", "Main");
auto* terminal = qobject_cast<qmltui::Terminal*>(engine.rootObjects().constFirst());
auto* driver = new qmltui::Runner(
    &engine, terminal,
    QStringLiteral(":/qt/qml/MyApplication/test_scenario.js"),
    QStringLiteral("/tmp/test-artifacts"), QSize(80, 24),
    1500, 8000, QString());
driver->setUpdateSnapshots(recordBaselines);
QTimer::singleShot(0, driver, &qmltui::Runner::start);
```

### Ownership and Thread Safety

- **Ownership**: `Runner` uses `QCoreApplication::instance()` as its `QObject` parent and holds guarded pointers to `QQmlEngine`, `Terminal`, and `TerminalEngine`. Destroying any required target object mid-scenario fails the run.
- **Thread safety**: Main/GUI thread only. `Runner` enters bounded nested Qt event loops during waits; calls from worker threads or nested wait calls are rejected.

## Structured Results (`result.json`)

Every run atomically writes `result.json` (schema version `1`) into the artifact directory containing:

- `schemaVersion` (`1`), `status` (`"passed"` or `"failed"`), `exitCode` (`0` or `1`), and `mode` (`"script"`, `"eval"`, `"capture"`, or `"embedded"`).
- `scenario`, `message`, `elapsedMs`, `clock` (`"real"` or `"virtual"`), and `virtualTimeMs` (when `clock` is `"virtual"`).
- `viewport` (initial `{ columns, rows }`), `finalViewport` (dimensions after any `resize()` calls), and `frameStats`.
- `operation` (active or failing operation object, or `null`) and argument-free `operations` trace.
- `artifacts` (`directory`, `result`, `captures`, `updatedSnapshots`, `casts`, and `finalFrame`).
