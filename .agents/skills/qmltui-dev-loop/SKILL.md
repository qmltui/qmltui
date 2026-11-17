---
name: qmltui-dev-loop
description: Develop, visually audit, profile, and verify QmlTui retained-mode terminal applications using qmllint, Qt Test suites, Runner virtual-clock scenarios, multi-viewport frame probes, frameStats/idle-budget telemetry, and Asciinema recordings. Use when implementing, debugging, or testing QmlTui C++, QML, controls, layouts, Inline scrollback, examples, Runner scenarios, or demos; do not use for read-only QML review or generic Qt Quick projects.
---

# QmlTui Development Loop

Run the tightest closed loop that proves compilation, cell-exact visual layout, interaction, and retained-mode paint hygiene:

```text
1. Edit & format      -> ./scripts/format.py (or clang-format / qmlformat)
2. Build & qmllint    -> cmake --build build --target <target>_qmllint <target>
3. Probe frames       -> qmltui-runner --clock virtual --sweep --idle-ms 300 --ruler
4. Audit grid & stats -> Check clipping, multi-viewport reflow, styles, and idle repaints
5. Lock into CTest    -> Register scenario / Qt Test, update snapshots if intended, run regressions
```

QmlTui uses two complementary test layers (do not describe all repository tests as Runner tests):

- **Qt Test executables (`-L Unit`)**: C++ units, model/layout/input contracts, and isolated `Item::paint()` or QML component checks (`tests/TestPaint.h` in the QmlTui repository) that do not need a full terminal session.
- **`qmltui::Runner` (`-L Runner`, `-L Demo`)**: Headless whole-application keyboard/mouse/paste routing, `AltScreen` and `Terminal.Inline` `Scrollback`, virtual-clock animations, text/styled snapshots, `frameStats()` telemetry, and `.cast` demo recordings on `QCoreApplication` (no TTY or display server required).

For detailed references inside this skill, read on demand:
- [runner-facade.md](./references/runner-facade.md) — Complete `app.*` / `Region` signatures, CLI flags, env vars, exit codes, and `result.json` schema.
- [visual-and-perf-audit.md](./references/visual-and-perf-audit.md) — Cell-padding traps, multi-viewport reflow, focus/monochrome checks, `Inline` scrollback, `frameStats()`, and idle budgets.
- [cmake-and-subsystems.md](./references/cmake-and-subsystems.md) — Consumer vs. in-tree CMake, `qmltui_add_scenario`, `.cast`/`agg` demo recording, `EMBEDDED` and C++ `Terminal` hooks, `Image`, `PtyView`, Private Qt, and WebAssembly.

## 1. Configure, Build, and Static-Lint

QmlTui requires CMake 3.21+, C++17, and Qt 6.8+. Always build `<target>_qmllint` alongside `<target>` to catch forbidden `import QtQuick`, unknown properties, and unqualified lookups before execution:

```bash
# QmlTui repository tree:
cmake -S . -B build -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON
cmake --build build --target <target>_qmllint <target>

# Standalone consumer application (FetchContent or find_package(QmlTui)):
cmake -S . -B build -G Ninja
cmake --build build --target my_app_qmllint my_app
```

Optional QmlTui flags: `-DQMLTUI_ENABLE_IMAGE=ON` (`QmlTui.Image`), `-DQMLTUI_ENABLE_PTYVIEW=ON` (`QmlTui.PtyView`, Linux/macOS only), and `-DQMLTUI_ENABLE_PRIVATE_QT=ON` (default when `Qt6::QmlPrivate` is installed, enabling `Behavior` and binding-aware `PropertyChanges`). In the QmlTui repository, validate both enabled and `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` build trees when touching private-Qt features or gating (`qmltui_independent_states_demo` is omitted when `OFF`), and rebuild the full tree after Core, public API, QML-module, or CMake changes.

Locate `qmltui-runner` by project layout:
- **QmlTui repository**: `./build/tools/qmltui-runner/qmltui-runner`
- **Consumer (`CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"`)**: `./build/bin/qmltui-runner` (`FetchContent`) or `qmltui-runner` on `PATH` (`find_package(QmlTui)`)
- **Consumer (`FetchContent` default layout)**: `./build/_deps/qmltui-build/tools/qmltui-runner/qmltui-runner`

## 2. Rapid Closed-Loop Probing (`qmltui-runner`)

Use `./build/tools/qmltui-runner/qmltui-runner` for multi-viewport sweeps, 0-based `(x, y)` coordinate rulers, idle-budget checks, and inline evaluations without manually setting environment variables. Pass `--clean-artifacts` when re-using a prior Runner `--artifacts` directory (without `--clean-artifacts`, `qmltui-runner` requires `--artifacts` to be new or empty and exits with code `2`):

```bash
# Multi-viewport sweep (60x18, 80x24, 120x36), 0-based rulers, style runs & idle check:
./build/tools/qmltui-runner/qmltui-runner \
  --clock virtual --sweep --idle-ms 300 --ruler --styled \
  --clean-artifacts --artifacts /tmp/qmltui-probe \
  -- ./build/path/to/testing-linked-app

# Deterministic interaction & animation check on the virtual clock:
./build/tools/qmltui-runner/qmltui-runner \
  --clock virtual --columns 80 --rows 24 --ruler \
  --eval 'app.clickText("Start"); app.advanceTime(300); app.expectText("RUNNING");' \
  -- ./build/path/to/testing-linked-app
```

Useful probe flags: `--sizes 60x18,80x24,120x36` (or `--sweep`), `--idle-ms <ms>`, `--print` (plain frames without rulers), `--ruler` (0-based `(x, y)` rulers), `--styled` (`--- styles`), `--scrollback` (`Terminal.Inline` history), and `--clock real` (for C++ `QTimer`, worker threads, or `PtyView`). Set `NO_COLOR=1` or `QMLTUI_COLOR_DEPTH=16` in the environment when checking monochrome/16-color fallback.

Runs are quiet/headless by default. Pass `--live` (`QMLTUI_RUNNER_LIVE=1`) only for a human watching a real terminal; when debugging live or interactive `AltScreen` sessions, set `QMLTUI_LOG_FILE=/tmp/qmltui.log` so `console.log()` and QML warnings stream immediately instead of buffering until exit.

## 3. Audit Visual Layout, Styles, and Paint Hygiene

Static tools cannot see character-cell overflow. On every visible UI change, verify (details in [visual-and-perf-audit.md](./references/visual-and-perf-audit.md)):

1. **No cell clipping or broken borders**: Visible `Rectangle`/`Panel` borders consume `1` cell per side; `Button` defaults to `leftPadding: 2; rightPadding: 2` (`width - 4` inner label budget). Confirm labels, corners (`┌─┐`), and 2-cell wide graphemes (CJK/emoji `continuation` cells) are intact.
2. **Responsive across viewports**: Verify cramped (`60x18`), standard (`80x24`), and wide (`120x36`) sizes via `--sweep` or `app.resize(cols, rows)`.
3. **Readable focus & monochrome fallback**: Check `capture_styled.txt` (`--- styles`) to confirm focus/selection pairs color with `bold`, `reversed`, border style, or glyph cues, and verify contextual `Escape` (close popup -> clear input -> quit; never quit on `Q` inside focused text input).
4. **Zero `dirtyDuringPaint` & zero idle repaints**: Confirm `dirtyDuringPaintTotal === 0` and `frameCount()` does not advance while idle (`--idle-ms 300` or an idle-budget scenario). Check `paintMicrosAvg` and `diffBytes` in `frameStats()`.

## 4. Write Reliable Scenarios & Snapshots

Define `function main(app) { ... }` in `.js` or `export function main(app) { ... }` in `.mjs` (ES modules may `import` relative helpers). Precede `main` with `/** @param {Runner} app */`—`qmltui_scenario_editor_support()` (and the QmlTui repo's `jsconfig.json` + `src/QmlTui/Runner/qmltui-runner.d.ts`) provides full `app.*` completion (plain JS only; TypeScript syntax is invalid). See [runner-facade.md](./references/runner-facade.md) for all 60 `app.*` methods.

- **Choose input & locators by intent**: `press()`, `typeText()`, `paste()`, `clickText()`, `hoverText()`, `click()`, `drag()`, `scroll()`, `resize()`. Prefer `app.item("objectName")` or `app.region(x, y, w, h)` over brittle full-frame coordinates or whole-screen substring checks.
- **Virtual clock first, never blind wall-clock `sleep()`**: Observe async state with `waitForText()`, `waitForRegex()`, `waitForItem()`, or `waitForCondition()`. For QML `Timer`s, animations, `Behavior`, cursor blink, `AnimatedSprite`, and `ToolTip` delays, register `CLOCK VIRTUAL` and step instantly with `advanceTime(ms)` (`sleep(ms)` delegates to `advanceTime()` on the virtual clock; note `QtQml.Timer` consumes one `<= 16 ms` `QUnifiedTimer` start step from idle, so step `+20 ms` across a `Timer` deadline). Any `CLOCK REAL` `sleep()` must name its wall-time source (C++ `QTimer`, worker thread, `PtyView`) in a comment.
- **Prove self-scheduled repaints**: `press()` and `expect*` request a render; to prove an app scheduled its own invalidation, pair `injectKey()` / `injectText()` / `injectPaste()` with `expectRepaint()`.
- **Assert `Terminal.Inline` history via `Scrollback`**: Completed rows leave the live bottom frame. Record `const mark = app.scrollbackRowCount()` and call `expectScrollbackText()`, `waitForScrollbackText()`, or `expectScrollbackSnapshot(..., { fromRow: mark })` (these fail in `AltScreen` mode).
- **Golden text & styled snapshots**: Use `expectSnapshot()` for UTF-8 frames, `expectStyled()` to lock in `#RRGGBBAA` colors and SGR attributes, and `expectRegionSnapshot()` for sub-rectangles. Record or refresh baselines with `--update-snapshots` or `QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest --test-dir build -R '^test_name$'` and inspect `git diff` (never hand-copy captures).
- **Record deterministic `.cast` demos when showcasing flows**: Run with `--clock virtual --record demo.cast` (or `qmltui_add_scenario(... RECORD demo.cast)`, labeled `Demo`), pacing holds with `advanceTime()` / `sleep()` and chapter markers with `app.marker("...")` (see [cmake-and-subsystems.md](./references/cmake-and-subsystems.md)).

## 5. Integrate Scenarios & Run CTest Suites

Link `qmltui::runner` into the normal application target (do not duplicate executable or QML-module targets; see `examples/recipes/status_badge/CMakeLists.txt` in the QmlTui repo, `examples/recipes/cpp_only/main.cpp` for C++-created `Terminal`s, and `tests/Runner/main.cpp` for `EMBEDDED` runners):

```cmake
if(BUILD_TESTING AND TARGET qmltui::runner)
    target_link_libraries(my_app PRIVATE qmltui::runner)
    qmltui_scenario_editor_support()
    qmltui_add_scenario(
        NAME my_app_smoke
        TARGET my_app
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        CLOCK VIRTUAL
    )
endif()
```

Discover exact CTest names and labels (`Unit`, `Runner`, `Demo`) before running:

```bash
ctest --test-dir build -N
ctest --test-dir build -R '^exact_test_name$' --output-on-failure
ctest --test-dir build -L Unit --output-on-failure
ctest --test-dir build -L Runner --output-on-failure
ctest --test-dir build -LE Demo --output-on-failure
```

## 6. Triage Failures & Hand Off

Every Runner session writes atomic schema-v1 `result.json` plus `final.txt` (pass), `failure.txt` (failure), `capture.txt` (capture mode), and `snapshots/<name>.diff` on snapshot mismatch. Exit code `1` means an assertion/JS failure (`result.json` `message` and `operation`); exit code `2` means CLI/argument error, non-empty `--artifacts`, missing Runner handshake, or crash. Always inspect `result.json` and `failure.txt` before editing code after a failure.

Before handoff:
1. Format modified C++/QML files (`./scripts/format.py <files>` in-repo, or `clang-format` / `qmlformat`) and run `git diff --check`.
2. Preserve unrelated modifications in a dirty worktree.
3. Report targets built (including `<target>_qmllint`), focused and regression CTests run, committed-frame artifacts and viewports inspected, `frameStats`/idle or `--live` checks performed, and configurations not exercised (such as `-DQMLTUI_ENABLE_PRIVATE_QT=OFF`, `PtyView`, `Image`, WebAssembly, or Windows).

After editing skills inside the QmlTui repository, run `python3 .agents/skills/qmltui-qml/check-skills.py --build-dir build`.
