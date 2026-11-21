# Runner Facade, CLI, and Artifact Reference

Complete quick-reference for `qmltui::Runner` JavaScript scenarios (`app` and `Region`), `qmltui-runner` CLI options, runtime environment variables, and `result.json` diagnostics.

## Scenario Entry Point & Options Contract

- **Classic script (`.js`)**: `function main(app) { ... }` (isolated function scope; line numbers preserved).
- **ES module (`.mjs`)**: `export function main(app) { ... }` (supports relative `import` from filesystem or `:/...` Qt resources).
- **Autocomplete**: Place `/** @param {Runner} app */` directly above `main`. TypeScript syntax inside `.js`/`.mjs` is invalid under `QJSEngine`.
- **Completion**: Synchronous scenarios pass automatically when `main(app)` returns. Returning a `Promise` (`async function main(app)`) is supported, though all `app.*` calls are synchronous. Do not use a top-level `return`. Trigger application `Qt.quit()` keys only as the final action of `main(app)` (calling `expect*` after `Qt.quit()` exits the event loop mid-wait).
- **Trailing options object**: Every optional parameter can be passed positionally or as a single trailing options object (for example `app.click(10, 5, { button: "right", modifiers: ["Shift"] })` or `app.waitForText("READY", { timeout: 3000 })`). Mixing positional optional arguments with a trailing options object, or passing an unknown option key, fails the scenario immediately.

---

## `app` (`Runner`) Methods

### Keyboard & Clipboard Input

| Signature | Behavior |
| :--- | :--- |
| `press(key, modifiers?)` / `press(key, { modifiers? })` | Sends one key event and waits for the committed frame. `key`: `Enter`/`Return`, `Esc`/`Escape`, `Tab`, `Backtab` (auto-applies `Shift`), `Backspace`, `Space`, `Up`, `Down`, `Left`, `Right`, `Home`, `End`, `Insert`, `Delete`, `PageUp`, `PageDown`, `F1`..`F35`, one ASCII char, or one non-ASCII Unicode grapheme (no modifiers). `modifiers`: `["Shift", "Ctrl"\|"Control", "Alt", "Meta"]`. |
| `typeText(text)` | Decodes non-empty UTF-8 `text` through `InputDecoder` into per-character key events (`\n` -> `Return`, `\t` -> `Tab`) and waits once for the committed frame. Use `press("Escape")` for Escape. |
| `paste(text)` | Delivers non-empty `text` as one bracketed paste (`\n` and `\t` arrive as literal text, not navigation keys) and waits for the committed frame. |
| `injectKey(key, modifiers?)` | Dispatches `key` **without** requesting rendering or waiting for a frame. Pair with `expectRepaint()`. |
| `injectText(text)` | Dispatches `text` **without** requesting rendering or waiting for a frame. Pair with `expectRepaint()`. |
| `injectPaste(text)` | Dispatches bracketed paste **without** requesting rendering or waiting for a frame. Pair with `expectRepaint()`. |

### Mouse Input

Mouse buttons accept `"left"` (default), `"middle"`, or `"right"`. Coordinate methods use 0-based terminal screen coordinates `(x, y)`; text helpers (`clickText`, `hoverText`) translate frame matches through the renderer's screen origin automatically (including `Terminal.Inline` bottom viewports).

| Signature | Behavior |
| :--- | :--- |
| `clickText(text, occurrence?, button?, modifiers?)` / `clickText(text, { occurrence?, button?, modifiers? })` | Clicks the center cell of the 0-based `occurrence` of rendered single-row `text` and waits for the frame. |
| `hoverText(text, occurrence?, modifiers?)` / `hoverText(text, { occurrence?, modifiers? })` | Moves pointer over the center of rendered `text` (delivered when `MouseArea.hoverEnabled` is active) and waits for the frame. |
| `click(x, y, button?, modifiers?)` / `click(x, y, { button?, modifiers? })` | Sends press + release at `(x, y)` and waits for the frame. |
| `doubleClick(x, y, button?, modifiers?)` / `doubleClick(x, y, { button?, modifiers? })` | Sends two consecutive press/release cycles at `(x, y)`. |
| `mouseDown(x, y, button?, modifiers?)` / `mouseDown(x, y, { button?, modifiers? })` | Presses button at `(x, y)` without releasing. |
| `mouseUp(x, y, button?, modifiers?)` / `mouseUp(x, y, { button?, modifiers? })` | Releases button at `(x, y)`. |
| `hover(x, y, modifiers?)` / `hover(x, y, { modifiers? })` | Moves pointer to `(x, y)` with no button held (active only when hover reporting is enabled). |
| `drag(startX, startY, endX, endY, button?, modifiers?)` / `drag(..., { button?, modifiers? })` | Presses at `(startX, startY)`, emits cell-by-cell motion, and releases at `(endX, endY)`. |
| `scroll(x, y, deltaX, deltaY, modifiers?)` / `scroll(..., { modifiers? })` | Sends mouse wheel scroll at `(x, y)` (e.g. `deltaY: 120` or `-120`). |

### Clock, Timing & Frame Synchronization

| Signature | Behavior |
| :--- | :--- |
| `advanceTime(durationMs)` | **Virtual clock only** (`--clock virtual` / `CLOCK VIRTUAL`). Steps QmlTui timers, animations, cursor blink, `ToolTip`, `AnimatedSprite`, and QML `Timer` items in `<= 16 ms` increments and commits invalidated frames. Fails on the real clock. |
| `sleep(durationMs)` | Waits `1..30000` ms. Delegates to `advanceTime(durationMs)` instantaneously on the virtual clock; processes the Qt event loop in wall time on the real clock. Nested waits fail. |
| `now()` | Elapsed milliseconds on the active QmlTui clock (virtual ms under `CLOCK VIRTUAL`, wall ms under `CLOCK REAL`). |
| `waitForFrame(timeoutMs?)` / `waitForFrame({ timeout? })` | Requests rendering and waits for a committed frame newer than the current `frameCount()` (default `2000` ms). |
| `expectRepaint(timeoutMs?)` / `expectRepaint({ timeout? })` | Waits for a committed frame **without** requesting rendering, proving the app invalidated the scene itself. |
| `frameCount()` | Sequence number of the latest committed frame (`-1` before attachment). Compare across an idle window to prove zero unscripted repaints. |
| `frameStats()` | Returns `{ frame, paintMicros, diffMicros, writeMicros, diffBytes, bytesWritten, dirtyDuringPaint, queuedBytes, frames, paintMicrosAvg, paintMicrosMax, diffMicrosAvg, bytesTotal, dirtyDuringPaintTotal, skippedForBackpressure }`. |

### Waits & Assertions

`expect*` waits for the next committed frame (`timeoutMs` defaults to `--operation-timeout-ms`, `2000` ms). `waitFor*` observes incoming frames until matched or `timeoutMs` (`1..30000` ms) expires.

| Signature | Behavior |
| :--- | :--- |
| `waitForText(text, timeoutMs?)` / `waitForText(text, { timeout? })` | Waits until a committed frame contains `text`. |
| `waitForRegex(pattern, timeoutMs?)` / `waitForRegex(pattern, { timeout? })` | Waits until a committed frame matches regex `pattern`. |
| `waitForCondition(fn, timeoutMs?)` / `waitForCondition(fn, { timeout? })` | Polls synchronous `fn(frameText, elapsedMs)` (~10 ms) until truthy. Do not nest `app.*` waits inside `fn`. |
| `waitForItem(objectName, timeoutMs?)` / `waitForItem(objectName, { timeout? })` | Waits until a QML `Item` with `objectName` is visible inside the viewport. |
| `expectText(text, timeoutMs?)` / `expectText(text, { timeout? })` | Fails unless the next committed frame contains `text`. |
| `expectNoText(text, timeoutMs?)` / `expectNoText(text, { timeout? })` | Fails if the next committed frame contains non-empty `text`. |
| `expectRegex(pattern, timeoutMs?)` / `expectRegex(pattern, { timeout? })` | Fails unless the next committed frame matches `pattern`. |
| `expectTextInRegion(x, y, width, height, text, timeoutMs?)` | Fails unless sub-rectangle `[x, y, width, height]` contains `text`. |
| `expectStyle(x, y, expectedStyle, timeoutMs?)` | Asserts cell `(x, y)` properties: `character`, `foreground`, `background` (`"#RRGGBBAA"`, CSS name, or `"default"`), `bold`, `dim`, `italic`, `underline`, `reversed`, `blink`, `conceal`, `strikethrough`. Unknown keys fail immediately. |

### Snapshots & Captures

Snapshot paths resolve relative to the scenario file; mismatches write `snapshots/<name>.actual.txt` and `snapshots/<name>.diff` under the artifact directory. Capture paths resolve inside the artifact directory (no absolute paths or `..`).

| Signature | Behavior |
| :--- | :--- |
| `expectSnapshot(snapshotRelativePath, timeoutMs?)` | Compares normalized UTF-8 frame text against a baseline file. |
| `expectStyled(snapshotRelativePath, timeoutMs?)` | Compares `styledText()` (text + `--- styles` `#RRGGBBAA`/SGR runs) against a baseline file. |
| `expectRegionSnapshot(x, y, width, height, snapshotRelativePath, timeoutMs?)` | Compares normalized text in `[x, y, width, height]` against a baseline file. |
| `captureText(relativePath)` | Writes normalized UTF-8 text of the next committed frame to `<artifacts>/<relativePath>`. |
| `captureStyled(relativePath)` | Writes `styledText()` of the next committed frame to `<artifacts>/<relativePath>`. |

### Inspection, Viewport & Scoped Locators (`Region`)

| Signature | Behavior |
| :--- | :--- |
| `frameText()` | Current committed frame's normalized UTF-8 text (no render requested). |
| `styledText()` | Current `frameText()` plus `--- styles` SGR/color runs (no render requested). |
| `regionText(x, y, width, height)` | Normalized text inside `[x, y, width, height]` of the current frame. |
| `cell(x, y)` | Returns `{ x, y, character, continuation, foreground, background, attributes }` at `(x, y)`. |
| `findText(text)` | Returns visible single-row matches in row-major order as `{ x, y, width }[]` in frame coordinates. |
| `itemRect(objectName)` | Returns `{ x, y, width, height }` in cells for the first visible `Item` with `objectName` (clipped to viewport; fails if missing/hidden/off-screen). |
| `size()` | Returns `{ columns, rows }` of the current test viewport. |
| `resize(columns, rows)` | Resizes viewport (`1..1000` each) and waits for a frame committed at the new size (in `Terminal.Inline`, frame height stays content-sized up to `rows - 1`). |
| `region(x, y, width, height)` | Returns a scoped `Region` locator for `[x, y, width, height]`. |
| `item(objectName)` | Returns a scoped `Region` locator for `itemRect(objectName)` (with `objectName` set). |
| `rootObject()` | Returns the root `Terminal` QObject (use only to read config/model inputs when computing expected frame text). |

**`Region` methods** (returned by `app.region(...)` and `app.item(...)`):
- `text()`: Normalized text inside the rectangle.
- `findText(text)`: Visible `{ x, y, width }[]` matches fully inside the rectangle.
- `expectText(text, { timeout? })`, `expectNoText(text, { timeout? })`, `waitForText(text, { timeout? })`.
- `expectSnapshot(snapshotRelativePath, { timeout? })`.
- `clickText(text, { occurrence?, button?, modifiers? })`, `click({ button?, modifiers? })` (clicks rectangle center).

### Inline Scrollback (`Terminal.Inline` Only)

Calling scrollback methods on an `AltScreen` terminal fails the scenario. Up to `QMLTUI_SCROLLBACK_HISTORY_ROWS` (`5000` by default) committed rows are retained in memory during a Runner session.

- `scrollbackRowCount()`: Total committed scrollback rows so far (use as `fromRow` checkpoint).
- `scrollbackText(fromRow?)` / `scrollbackText({ fromRow? })`: Normalized text from `fromRow` onwards without waiting.
- `waitForScrollbackText(text, timeoutMs?)`, `expectScrollbackText(text, timeoutMs?)`, `expectNoScrollbackText(text, timeoutMs?)`.
- `captureScrollback(relativePath, fromRow?)`, `captureScrollbackStyled(relativePath, fromRow?)`.
- `expectScrollbackSnapshot(snapshotRelativePath, fromRow?, timeoutMs?)` / `expectScrollbackSnapshot(path, { fromRow?, timeout? })`.

### Asciicast Recording & Lifecycle

Requires `--record <rel.cast>`, `QMLTUI_RUNNER_RECORD=<path>`, or `qmltui_add_scenario(... RECORD <rel.cast>)` (incompatible with `--live` / `LIVE`).

- `startRecording(relativePath)`: Closes any active `.cast` and opens a new asciicast v2 file at `<artifacts>/<relativePath>` starting with a full repaint.
- `stopRecording()`: Flushes and closes the active `.cast` recording.
- `marker(label)`: Emits an asciicast `"m"` chapter marker at the current timestamp.
- `finish()`: Marks scenario complete (optional when synchronous `main(app)` returns).
- `fail(message)`: Immediately aborts the scenario with `message`, writes `failure.txt`, and exits with code `1`.

---

## `qmltui-runner` CLI & Environment Variables

Choose `--script`, `--eval`, `--capture-after-ms`, or probe options (`--sizes` / `--sweep` / `--idle-ms`):

| CLI Flag | Env Equivalent | Default | Notes |
| :--- | :--- | :--- | :--- |
| `--script <path>` | `QMLTUI_RUNNER_SCRIPT` | — | `.js`, `.mjs`, or `:/...` resource path |
| `--eval <code>` | `QMLTUI_RUNNER_EVAL` | — | Inline JS body inside `function main(app)` |
| `--capture-after-ms <ms>` | `QMLTUI_RUNNER_CAPTURE_AFTER_MS` | — | `0..30000` ms (`0` = first committed frame); writes `capture.txt` and `capture_styled.txt` |
| `--sizes <list>` / `--sweep` | *(synthesized `EVAL`)* | — | Sweep `<cols>x<rows>` list (`--sweep` = `60x18,80x24,120x36`); incompatible with `--script` |
| `--idle-ms <ms>` | *(synthesized `EVAL`)* | — | Verify zero unscripted repaints across `1..30000` ms; incompatible with `--script` |
| `--columns <n>`, `--rows <n>` | `QMLTUI_RUNNER_COLUMNS`, `QMLTUI_RUNNER_ROWS` | `80`, `24` | `1..1000` cells (uses terminal size under `--live` if omitted) |
| `--clock <real\|virtual>` | `QMLTUI_RUNNER_CLOCK` | `real` | Prefer `virtual` for deterministic QML timers & animations |
| `--artifacts <dir>` | `QMLTUI_RUNNER_ARTIFACT_DIR` | Temp dir | **Must be new or empty** unless `--clean-artifacts` is set |
| `--clean-artifacts` | *(launcher flag)* | Off | Clear an existing Runner artifact directory (containing `result.json`) before running |
| `--print`, `--ruler` | *(launcher flag)* | Off | Print `frameStats` and captured frames (`--ruler` adds 0-based `(x, y)` rulers) to `stdout` |
| `--styled`, `--scrollback` | *(launcher flag)* | Off | Include `--- styles` SGR runs and/or `Terminal.Inline` scrollback when probing/printing |
| `--update-snapshots` | `QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1` | Off | Rewrites `expect*Snapshot` / `expectStyled` baseline files |
| `--record <rel.cast>` | `QMLTUI_RUNNER_RECORD` | Off | Relative path inside `--artifacts`; incompatible with `--live` |
| `--live` | `QMLTUI_RUNNER_LIVE=1` | Off | Stream ANSI to real terminal (human inspection only) |
| `--operation-timeout-ms <ms>` | `QMLTUI_RUNNER_OPERATION_TIMEOUT_MS` | `2000` | Per-operation bound (`1..30000` ms) |
| `--timeout-ms <ms>` | `QMLTUI_RUNNER_TIMEOUT_MS` | `300000` | Watchdog (`30000` ms default in `qmltui_add_scenario`) |
| `--startup-timeout-ms <ms>` | *(launcher handshake)* | `5000` | Deadline for `Runner::start()` handshake |

**Flag composition notes**:
- `--ruler` implies `--print` (both incompatible with `--live`), works with `--script`, enables `QMLTUI_DEBUG_PAINT_DIRTY=1`, and decorates `stdout` only (saved artifact files remain plain UTF-8).
- `--styled` alone only writes `*_styled.txt` artifacts; pair with `--print` or `--ruler` to print `--- Styles ---` to `stdout`.
- Probe modifiers (`--sweep` / `--sizes` / `--idle-ms` / `--scrollback`) enable `QMLTUI_DEBUG_PAINT_DIRTY=1` and exit `1` if `dirtyDuringPaintTotal > 0`.

**Exit codes**:
- `0`: Scenario passed (`result.json` `status: "passed"`).
- `1`: Assertion failure, uncaught JS/render error (`status: "failed"`, `failure.txt` written), or `dirtyDuringPaintTotal > 0` during a probe run.
- `2`: Invalid CLI args (including non-empty `--artifacts`), handshake timeout (target missing `QmlTui::Runner` link or C++ `startFromEnvironment` call), or process crash.

**Engine diagnostic environment variables**:
- `QMLTUI_DEBUG_PAINT_DIRTY=1`: Warns on `stderr` whenever an `Item` calls `markDirty()` inside `paint()`.
- `QMLTUI_LOG_FILE=<path>`: Streams `qDebug` / `qWarning` / `console.log()` unbuffered to `<path>` (essential during `--live` or interactive `AltScreen` runs where TTY `stderr` is buffered until exit).
- `QMLTUI_FRAME_STATS=1`: Logs periodic render timing and byte counters to `stderr`.
- `NO_COLOR=1` / `QMLTUI_COLOR_DEPTH=mono|16|256|truecolor`: Overrides `TerminalCapabilities.colorDepth` and ANSI color encoding in non-recording probes and `--live` runs (`--record` forces 24-bit `truecolor`).

## `result.json` (Schema v1)

Written atomically to `<artifacts>/result.json` on every run:
- `status` (`"passed"` | `"failed"`), `exitCode` (`0` | `1`), `mode` (`"script"` | `"eval"` | `"capture"` | `"embedded"`).
- `message` (failure reason), `operation` (`{ index, name, status }` of active/failing call), `operations` trace.
- `clock` (`"real"` | `"virtual"`), `elapsedMs`, `virtualTimeMs`.
- `viewport`, `finalViewport`, `frameStats`.
- `artifacts`: `{ directory, result, captures, updatedSnapshots, casts, finalFrame }` (`finalFrame` is `"final.txt"`, `"failure.txt"`, or `"capture.txt"`).
