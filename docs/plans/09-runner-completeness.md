# Plan 09: Runner Completeness (Resize, Input, Snapshots, Virtual Clock)

## 1. Goal

Make `Qtui::Runner` the tool a Qt developer reaches for first when testing a
terminal application, and the one an agent can drive without learning its
quirks. The core model stays: scenarios are blocking, straight-line JavaScript
against committed cell grids, with structured artifacts. This plan closes the
gaps found in the 2026-09-26 review:

1. Scenarios cannot change the terminal size, so resize and reflow are
   untested.
2. Paste, hover, press-and-hold and non-ASCII keys cannot be expressed.
3. Updating a golden snapshot is a manual capture-and-copy.
4. Scenarios cannot share helpers, and their top-level names leak into the
   application's JavaScript global scope.
5. The argument lists are positional and inconsistent, and there is no scoped
   (region or item) locator.
6. The scenario watchdog is capped at 30 s.
7. Time is wall-clock only: 66 `sleep()` calls across the scenarios make the
   suite slow and load-sensitive, and animation mid-states cannot be asserted.
8. The documentation has drifted from the code, and internal hooks are public.

Each work package below is a separately reviewable change. WP1 to WP7 are
independent of each other and small to medium. WP8 (virtual clock) is the large
one and touches Core. WP9 migrates the existing scenarios once WP8 exists.

## 2. Verified State (2026-09-26)

- `Runner` exposes 33 facade methods (`installJavaScriptFacade()` in
  `src/Qtui/Runner/Runner.cpp`). Every method blocks inside a bounded nested
  event loop. `main(terminal)` may return a Promise, but no operation returns
  one.
- Usage across the tree's scenarios: `press` 255, `expectText` 189,
  `waitForText` 167, `captureText` 99, `sleep` 66, `typeText` 58,
  `expectNoText` 40, `expectStyle` 35, `waitForCondition` 25. `click`,
  `doubleClick`, `drag`, `scroll`, `waitForFrame` and `rootObject` are used
  three times or fewer.
- The viewport is fixed at construction through
  `TerminalEngine::setTestViewportSize()`. That method already calls
  `handleResize()` and `requestRender()`, so it can be called again at runtime.
- `typeText`/`injectText` feed UTF-8 through `TerminalEngine::feedInput()` and
  the real `InputDecoder`. The decoder already frames bracketed paste
  (`\e[200~` ... `\e[201~`) and routes it with `paste = true` to
  `TextInput::paste()`.
- Mouse input bypasses the decoder:
  `TerminalEngine::handleMouseInput(button, x, y, release, motion, modifiers)`.
  Motion (hover) reporting exists on the engine
  (`setMouseMotionReportingEnabled`). Terminal focus reporting (`CSI I`/`CSI O`)
  does not exist in the engine.
- `minimumTimeoutMs = 1`, `maximumTimeoutMs = 30000` bound every operation and
  the scenario watchdog alike.
- `Runner.md` does not document `frameStats()` or `rootObject()`, still shows a
  `Test.press(...)` global that no longer exists, and the `typeText` comment in
  `Runner.h` says "one text-input event" (it is decoded per character).
  `onPromiseResolved`, `onPromiseRejected`, `beginOperation` and `endOperation`
  are public `Q_INVOKABLE`s on the exported class, and `Runner.h` includes
  `<thread>`, `<mutex>` and `<condition_variable>`.
- `cell()` reports five attributes (bold, dim, italic, underline, reversed);
  `styledText()` reports eight (adds blink, conceal, strikethrough). For the
  terminal default color, `cell()` formats the sentinel through
  `rgbaString()`, while `styledText()` writes `default`.
- Time sources that a virtual clock must cover, all on `QTimer`/`QElapsedTimer`
  today: `AnimationManager` and `PropertyAnimation` (`Core/engine/Animation.h`),
  `ToolTip` delay and timeout, `Toast`, `AnimatedSprite`, the `TextInput` cursor
  blink, and the engine's Escape flush timer and frame pacing
  (`m_paceTimer`, `m_lastCommit`). QML `Timer` (`QQmlTimer` in QtQmlMeta) is
  a `QPauseAnimationJob`, so it runs on `QUnifiedTimer` and follows an installed
  `QAnimationDriver`. The capability query timers are about real terminal
  replies and stay on wall time.
- Many example C++ models tick on their own `QTimer` (btop, cyber_audio,
  device_cockpit providers, container_fleet, din_gateway). A virtual clock
  cannot reach those; scenarios that depend on them keep wall time.
- `startFromEnvironment()` is called after `engine.load()` in every
  Runner-linked `main`, so anything that must exist before QML objects are
  created cannot be set up by the Runner itself.

## 3. Decisions

- **The blocking model stays.** Straight-line scenarios are the Runner's best
  property. `main` may still return a Promise (the mechanism is tested and
  costs nothing), but the documentation stops presenting it as a feature: no
  operation is awaitable, and none will become so.
- **Positional forms stay; options objects are added.** About 50 scenarios use
  positional arguments. The facade accepts a trailing plain object as options
  for every method that has optional parameters. Native C++ signatures do not
  change. No deprecations.
- **The Runner mirrors the terminal.** A new input method is added only when a
  real terminal can produce that input and the engine handles it. Hover events
  are delivered only when the application enabled motion reporting, exactly
  as a terminal would. Focus in/out waits until the engine supports focus
  reporting (a Plan 08 follow-up), and is out of scope here.
- **The virtual clock is opt-in per scenario, fixed for the process.** It is
  chosen before QML loads (environment variable or CMake option), never
  switched mid-run. In virtual mode, time advances only through
  `advanceTime()` or `sleep()`. `waitFor*` timeouts stay on wall time and do
  not advance the virtual clock, so a wait on a Timer-driven label fails
  quickly with a message that names the paused clock.
- **Rendered-frame assertions remain the contract.** `rootObject()` and the new
  `itemRect()` are for computing expectations and locating targets, and the
  documentation says so. AGENTS.md's rule (a visible-behavior test asserts the
  frame) is unchanged.

## 4. Work Packages

### WP1: Documentation and API hygiene

Small, first, and it unblocks honest documentation for everything after it.

- `Runner.md`: document `frameStats()` (field list from `Runner.h`) and
  `rootObject()` (with the "expectations, not assertions" rule); replace every
  `Test.` example with `terminal.`; fix the description of
  `QTUI_SCRIPT`; rewrite "Asynchronous & Promise Integration" per section 3.
- `Runner.h`: correct the `typeText` comment.
- Move the internal invokables off the exported class: a private
  `RunnerBridge` QObject in `Runner.cpp` carries `onPromiseResolved`,
  `onPromiseRejected`, `beginOperation` and `endOperation`, and the facade and
  the Promise handler receive the bridge instead of the `Runner`.
- Move the hard watchdog (`std::thread`, mutex, condition variable) into a
  private `HardWatchdog` class in `Runner.cpp` behind a `std::unique_ptr`, so
  `Runner.h` stops including `<thread>`, `<mutex>` and
  `<condition_variable>`.
- `cell()`: report all eight attributes (keep the key `reversed`; add `blink`,
  `conceal`, `strikethrough`) and return `"default"` for the default color, as
  `styledText()` does. `expectStyle()` accepts `"default"` as an expected
  color.

Tests: extend `extended_features.js` to assert the three new attributes and
`"default"` against a fixture cell; add a scenario checking that
`terminal.onPromiseResolved` and `terminal.beginOperation` are `undefined` on
the facade.

### WP2: Resize

- `resize(columns, rows)`: validates 1 to 1,000 like `QTUI_RUNNER_COLUMNS`,
  calls `setTestViewportSize()`, and waits for the committed frame at the new
  size. The frame's dimensions are checked, not just its sequence number.
- `size()`: returns `{columns, rows}` of the current committed frame.
- `result.json`: `viewport` keeps the initial size, and a new `finalViewport`
  records the size at completion. The operation trace already names each
  `resize`.

Tests: the fixture shows `SIZE: <w>x<h>` and anchors a right-aligned marker;
`resize.js` shrinks to 40x12, asserts the marker moved and the text reflowed
(`expectTextInRegion`), grows to 120x40, and asserts `size()`. Negative
scenario: `resize(0, 10)` fails with the range message. Add one application
scenario: device_cockpit at 80x24 and 60x20 (its narrow layout is the most
likely to break).

### WP3: Input completeness

- `paste(text)`: feeds `\e[200~` + UTF-8 + `\e[201~` through `feedInput()`
  and waits for a frame; `injectPaste(text)` is the no-wait variant. Newlines
  and tabs inside the paste reach `TextInput::paste()` untranslated, which is
  the point of bracketed paste.
- `hover(x, y, modifiers)`: `handleMouseInput(0, x, y, false, true, ...)`,
  then wait for a frame. When motion reporting is off, the engine drops the
  event exactly as a terminal that never sends it would; the doc says so.
- `mouseDown(x, y, button, modifiers)` / `mouseUp(...)`: press-and-hold and
  custom drags. `drag()` is reimplemented on top of them, emitting motion
  events along the path (today it jumps from start to end).
- `press()` accepts any single Unicode grapheme that a terminal sends as text
  (routed through the decoder as UTF-8), not just ASCII. Named keys are
  unchanged.

Tests: `paste.js` pastes `"line1\nline2\ttab"` into a fixture `TextArea`
and asserts both lines and the tab expansion in the frame, then asserts
a multi-line paste into a single-line `TextField` behaves the way
`TextInput::paste()` defines; `hover.js` against a fixture with motion
reporting on (highlight appears) and off (frame unchanged, via
`frameCount()`); `press("é")` and `press("世")` into a `TextField`.

### WP4: API shape and scoped locators

All of this lives in the JavaScript facade over a small set of native
additions, so the C++ surface grows by three methods.

- Options objects everywhere optional parameters exist:
  `click(x, y, {button, modifiers})`,
  `clickText(text, {occurrence, button, modifiers})`,
  `waitForText(text, {timeout})`, `expectText(text, {timeout})`,
  `drag(sx, sy, ex, ey, {button, modifiers})`, and so on. A trailing plain
  object is options; unknown keys fail the scenario (typos must not pass
  silently).
- `expect*` gain an optional `timeout` (today they silently use the
  operation timeout).
- Native `regionText(x, y, width, height)`: normalized text of a clipped
  sub-grid, wide characters handled like `findText()`.
- `region(x, y, width, height)` returns a locator object with `text()`,
  `findText()`, `expectText()`, `expectNoText()`, `waitForText()`,
  `expectSnapshot()` and `clickText()`, each scoped to the rectangle.
  `expectTextInRegion()` stays as a shorthand.
- Native `itemRect(objectName)`: finds the first visible `Item` with that
  `objectName` in the root's tree and returns `{x, y, width, height}` in
  terminal cells from `Item::absolutePosition()`, clipped to the viewport, or
  fails when absent or invisible. `item(objectName)` in the facade returns
  `region(...)` for that rectangle plus `click()`.
- Native `waitForItem(objectName, timeout)`: waits until such an item is
  visible in a committed frame.

Tests: one scenario per locator (`region.js`, `item_locator.js`), an unknown
option key failing with its name in the message, and a clipped item (partly
off-screen) returning the clipped rectangle.

### WP5: Snapshot update mode

- `QTUI_UPDATE_SNAPSHOTS=1`: `expectSnapshot()` and `expectStyled()` write the
  actual text to the resolved baseline path and pass. A missing baseline in
  normal mode fails with a message naming the variable. Resource-path
  scenarios cannot write back: the actual text goes to the artifact directory,
  and the scenario fails with the path it would have updated.
- `result.json` gains `updatedSnapshots` (paths written), and the stderr
  summary lists them so an update is never silent.
- `cmake/QtuiRunner.cmake` forwards `QTUI_UPDATE_SNAPSHOTS` from the
  environment to the launched process, so
  `QTUI_UPDATE_SNAPSHOTS=1 ctest -R <name>` works for both
  `qtui_add_scenario` and `qtui_add_scenario_test`.
- `captureStyled()` stays for exploration; `Runner.md` points to update mode
  for baselines.

Tests: a scenario run twice under CTest fixtures (update into a temporary
copy of a baseline, then verify against it), a resource-path scenario in
update mode failing with the path, and a missing baseline failing with the
hint.

### WP6: Shared helpers and clean globals

- Module scenarios: a script ending in `.mjs` is loaded with
  `QJSEngine::importModule()` and must `export function main(terminal)`. It may
  `import` sibling modules (`import { openServices } from "./lib/cockpit.mjs"`).
  Qt resource paths are supported when `importModule` resolves `:/` or
  `qrc:` there; verify this on Qt 6.8 first, and if it does not work,
  document module scenarios as filesystem-only.
- Classic `.js` scenarios are evaluated inside a function wrapper, so their
  top-level declarations no longer become properties of the application's
  global object. The wrapper is prepended on the first line without a
  newline, so reported line numbers do not move; a regression test pins that.
- `QTUI_EVAL` stays classic.
- The operation trace's file/line extraction is checked against module stack
  frames.

Tests: `module_main.mjs` importing a helper; a module missing the `main`
export fails with a clear message; a classic scenario defining
`function helper()` leaves `rootObject()`'s engine global object without
`helper`; an error thrown on line 7 is reported at line 7.

### WP7: Timeouts

- The scenario watchdog accepts up to 300,000 ms (5 minutes). The per-operation
  and per-wait limit stays at 30,000 ms: a single wait longer than that is a
  scenario bug. `qtui_add_scenario` derives its CTest timeout from the larger
  value as it does today, and defaults stay 2,000/10,000 ms.
- The hard watchdog keeps its "scenario timeout + 1 s" rule.

Tests: `QTUI_RUNNER_TIMEOUT_MS=300000` accepted, `300001` rejected with the
range message; existing timeout scenarios unchanged.

### WP8: Virtual clock

The large package. It makes animation, Timer, ToolTip, Toast and blink
behaviour deterministic and instant to test, without touching application C++
timers.

**8.1 Clock seam in Core.** A private `qtui::Clock` (`Core/engine/Clock_p.h`)
with two modes:

- `Clock::elapsed()`: milliseconds since process start (real: a
  `QElapsedTimer`; virtual: a counter).
- `qtui::ClockTimer`: the subset of `QTimer` Qtui uses (interval, single-shot,
  `start`, `stop`, `isActive`, `timeout`). In real mode it wraps a `QTimer`,
  so behaviour and idle cost are unchanged. In virtual mode it registers a
  deadline with the clock.

Migrate `AnimationManager`, `PropertyAnimation`, `ToolTip`, `Toast`,
`AnimatedSprite`, the `TextInput` blink and the engine's Escape flush timer to
`Clock`/`ClockTimer`. The capability query timers stay on `QTimer`. Frame
pacing is disabled in virtual mode: a render request renders on the next event
loop pass, so a paused clock never withholds a frame.

**8.2 Mode selection before QML loads.** A `Q_COREAPP_STARTUP_FUNCTION` in
Core reads `QTUI_RUNNER_CLOCK` (`real`, the default, or `virtual`) when the
`QCoreApplication` is constructed. That is before any `main` loads QML,
whether the Runner is started from the environment or embedded. Virtual mode
also installs a `VirtualAnimationDriver` (a `QAnimationDriver` whose
`elapsed()` returns `Clock::elapsed()`), which brings QML `Timer` and
anything else on `QUnifiedTimer` onto the same timeline. The public C++ entry
point for embedders is `qtui::Runner::useVirtualClock()`, documented as "call
before loading QML"; it fails loudly if a Qtui item already exists.

**8.3 Runner API.**

- `advanceTime(ms)`: advances in steps of at most 16 ms, or to the next
  `ClockTimer` deadline if that is sooner. After each step it advances the
  animation driver, delivers timers in deadline order, processes posted
  events, and finally waits for one committed frame if anything invalidated.
  Bounded by the operation limit in virtual milliseconds (30,000).
- `now()`: virtual milliseconds since start.
- `sleep(ms)` in virtual mode is `advanceTime(ms)`, so existing scenarios run
  faster without edits where they only wait on Qtui or QML time.
- `waitFor*` in virtual mode: wall-clock timeouts, clock stays paused. The
  timeout message appends "the virtual clock is paused; advance it with
  advanceTime()".
- `result.json` records `clock: "real" | "virtual"` and the final virtual
  time. `qtui_add_scenario(... CLOCK VIRTUAL)` sets the variable.

**8.4 Limits, documented.** Application `QTimer`s, threads, image providers,
network and `Date.now()` stay on wall time. A QML `Timer` shorter than the
16 ms step fires at most once per step (the same granularity as the default
driver). Mixed scenarios are allowed but must use `waitFor*` for the
wall-time parts.

Tests:

- New Qt Test `test_clock`: `ClockTimer` ordering at equal deadlines,
  single-shot vs repeating, restart and stop during delivery, real-mode parity
  with `QTimer`.
- Runner scenarios under `CLOCK VIRTUAL`: a QML `Timer` counter shows exactly
  `Tick 5` after `advanceTime(500)` at 100 ms; a 400 ms `NumberAnimation` on a
  `Rectangle.width` asserts the committed mid-state after `advanceTime(200)`
  with `expectStyled`; a `ToolTip` absent at 499 ms and present at 500 ms after
  hover (depends on WP3); a `Toast` gone after its timeout; `advanceTime()`
  with nothing scheduled commits no frame (`frameCount()` unchanged);
  a `waitForText` on a Timer label fails with the paused-clock hint.
- The idle-budget scenarios (`btop`, `service_monitor`) still pass in real
  mode, proving the real-mode `ClockTimer` added no idle wakeups.
- Build and run in both Behavior-enabled and Behavior-disabled trees (Behavior
  and Transition sit on `Animation`), and once on Windows from WSL. The wasm
  build compiles `Clock` in real mode only; the Runner is not in wasm.

### WP9: Scenario migration and authoring guidance

After WP8:

- Audit all 66 `sleep()` calls. Each gets one of four outcomes: removed
  (`press`/`typeText` already waited for the frame), replaced by
  `waitForText`/`waitForCondition`/`expectRepaint`, kept under a
  `CLOCK VIRTUAL` scenario (Qtui/QML time only), or kept with a comment naming
  the wall-time source (an application `QTimer`). The btop run of
  `press` + `sleep(200)` pairs and cyber_audio's layout sleeps are the first
  candidates.
- Move gallery and animation-heavy component scenarios to `CLOCK VIRTUAL`.
- Convert one application (device_cockpit) to a `.mjs` helper module as the
  reference for shared navigation code.
- Update `.agents/skills/qtui-dev-loop/SKILL.md`: prefer `waitFor*` over
  `sleep`, use update mode for baselines, use `region`/`item` locators, pick
  `CLOCK VIRTUAL` when the behavior under test is Qtui or QML time.

Record the suite's wall time (sum of Runner scenario durations from CTest)
before WP9 and after it in this file.

## 5. Order

| Order | Package | Size | Depends on |
| :---: | :--- | :---: | :--- |
| 1 | WP1 Docs and hygiene | S | - |
| 2 | WP2 Resize | S | - |
| 3 | WP5 Snapshot update mode | S | - |
| 4 | WP7 Timeouts | XS | - |
| 5 | WP4 API shape and locators | M | WP1 (bridge) |
| 6 | WP3 Input completeness | M | WP4 (options) |
| 7 | WP6 Modules and clean globals | M | - |
| 8 | WP8 Virtual clock | L | WP3 (hover, for the ToolTip test) |
| 9 | WP9 Migration and guidance | M | WP8, WP6 |

WP1, WP2, WP5 and WP7 can land in any order or together. WP8's Core half
(8.1, 8.2) can start in parallel with WP4 to WP6.

## 6. Verification per Package

- Build `qtui_runner_fixture` and run `ctest -R test_runner_fixture` in the
  development tree first; then the full configured suite, because the Runner
  is public API and CMake integration.
- WP2, WP3 and WP8 each add an application scenario that asserts committed
  frames, not only fixture scenarios.
- WP8 additionally: Behavior-enabled and Behavior-disabled trees, Windows
  (MSVC, Qt 6.8.3), and the idle-budget scenarios.
- Inspect the new artifacts (`result.json`, `failure.txt`, snapshot diffs) for
  each negative scenario, not just the exit code.
- `rtk git diff --check` before each handoff.

## 7. Out of Scope

- Terminal focus in/out events, until the engine gains focus reporting.
- Image or pixel capture: the Runner stays a cell-grid oracle.
- Awaitable (Promise-returning) operations.
- Recording scenarios from live input (a possible later plan, alongside the
  Plan 02 recording pipeline).
- Controlling application C++ timers from the virtual clock.

## 8. Status

All nine work packages were implemented on 2026-09-26.

| Package | State |
| :--- | :--- |
| WP1 | Done: docs refreshed, RunnerBridge + hard-watchdog moved out of the public header, cell() reports 8 attributes and "default" colors |
| WP2 | Done: resize()/size(), finalViewport in result.json, fixture + device_cockpit narrow-layout scenarios |
| WP3 | Done: paste()/injectPaste(), hover() gated on motion reporting, mouseDown()/mouseUp(), drag() walks the path, press() takes any single grapheme |
| WP4 | Done: trailing options objects with unknown-key rejection, regionText()/itemRect()/waitForItem()/expectRegionSnapshot() natives, region()/item() locators, expect* timeouts |
| WP5 | Done: QTUI_UPDATE_SNAPSHOTS with per-test UPDATE_SNAPSHOTS/NO_UPDATE_SNAPSHOTS and seeded-baseline tests; updates listed in result.json and the summary |
| WP6 | Done: .mjs module scenarios (filesystem and Qt resources, relative imports), classic scripts wrapped in a function scope with line numbers preserved |
| WP7 | Done: scenario watchdog accepts up to 300 s; per-operation limit stays 30 s |
| WP8 | Done: Clock/ClockTimer seam (AnimationManager, ToolTip, Toast, AnimatedSprite, cursor blink, escape flush; pacing off in virtual mode), QTUI_RUNNER_CLOCK=virtual with a permanently-running animation driver so QML Timer is virtual too, advanceTime()/now(), CLOCK VIRTUAL in CMake, test_clock_virtual/test_clock_real, tooltip-to-the-millisecond scenario |
| WP9 | Done: sleep audit (66 → 21 calls: btop -14, cyber_audio -16, snake and gallery moved to CLOCK VIRTUAL, remaining wall-time sleeps name their source), device_cockpit converted to .mjs with lib/cockpit.mjs, qtui-dev-loop skill updated. Two findings: btop's sleeps were masking Behavior-animated panel collapse — the scenario now waits for the settled state; industrial_plc_hmi stays on the real clock because its waitForCondition() polls simulation state that only develops with time, and waits deliberately do not advance the virtual clock |

Suite wall time (build-public-delegates, `ctest -j4`, warm): 16.4 s for 108
tests before WP1; 15.1 s for 132 tests after WP9. Runner-labeled scenarios:
30.7 s·proc across 64 tests before; 22.6 s·proc across 86 after. The
snake AI gameplay run went from wall-clock seconds to 0.06 s and the
37-page gallery tour from ~8 s to 0.5 s under the virtual clock.

Follow-up landed the same day: `src/Qtui/Runner/qtui-runner.d.ts` plus a
repository `jsconfig.json` give scenario authors editor autocomplete from one
JSDoc line (`/** @param {Terminal} terminal */`); the declarations are
installed next to the headers, verified against the facade by
`test_runner_declarations`, and validated with tsc (a method typo and an
option-key typo are both flagged). Scenarios stay plain JavaScript — inline
TypeScript annotations would be a QJSEngine syntax error.

Not exercised on 2026-09-26: the Windows (MSVC, Qt 6.8.3) and wasm builds,
and the Qt 6.11.1 tree.
