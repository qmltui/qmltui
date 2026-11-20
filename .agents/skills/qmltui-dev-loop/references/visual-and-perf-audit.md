# Visual, Responsive, and Performance Audit Playbook

Compilers and `qmllint` verify types and bindings, not character-cell geometry. Before finishing any visible QmlTui change, inspect the committed frame, style runs, viewport reflow, and paint telemetry.

## 1. Integer-Cell Box Model & Clipping Traps

All QmlTui geometry (`x`, `y`, `width`, `height`, `spacing`, margins, padding) is in integer character cells. Watch for these recurring cell-budget miscalculations when reading `capture.txt` or `qmltui-runner --ruler` output:

- **Visible borders consume 2 cells of each axis**: A `Rectangle` or `Panel` with `border.style !== Border.None` automatically adds `1` cell to `leftPadding`, `rightPadding`, `topPadding`, and `bottomPadding` (insetting both `anchors.fill: parent` children and positioned layouts).
- **`Button` inner label width is `width - 4` by default**: `Button` sets `leftPadding: 2` and `rightPadding: 2` (1 border cell + 1 inner space per side). An 11-column `Button` fits only `7` label columns (`11 - 4 = 7`) unless `leftPadding: 1; rightPadding: 1` is set (`width - 2`) or `implicitWidth` sizes the control.
- **`TableView` widths vs. grid separators**: Column widths include cell `leftPadding` + `rightPadding`, while `showOuterBorder` and `showColumnGrid` add separator columns *outside* column widths. Remember `TableView`, `TreeView`, `Sparkline`, and `TabBar` do not expose `Rectangle.border`; wrap them in a `Rectangle`/`Panel` or use `TableView`'s `showOuterBorder`/`showColumnGrid`/`showRowGrid`.
- **Wide Unicode graphemes (CJK & emoji)**: 2-column graphemes occupy a leading cell plus a trailing `continuation: true` cell (`app.cell(x + 1, y).continuation`). Verify labels near right borders do not clip a 2-cell grapheme into a 1-cell gap.
- **Overlays, `Popup`, `Dialog`, and `ToolTip`**: Trigger popups/combos/tooltips (`CLOCK VIRTUAL` + `advanceTime()` for tooltip delay) and check that borders do not bleed through underlying text or clip off the bottom/right viewport edge.

## 2. Multi-Viewport Reflow Sweep

Never validate responsive layouts (`RowLayout`, `ColumnLayout`, `GridLayout`, `ScrollView`, `TableView`, `anchors`) at `80x24` alone. Sweep at least three aspect ratios with `qmltui-runner --sweep --ruler` or `app.resize(cols, rows)`:

1. **Cramped split pane (`60x18`)**: Does dynamic `Text` clip cleanly (`clip: true`), do `ScrollView` / virtualized views scroll instead of overflowing borders, and do status bars/shortcuts stay readable?
2. **Classic terminal (`80x24`)**: Default baseline for smoke snapshots.
3. **Wide desktop terminal (`120x36`)**: Do stretch columns (`Layout.fillWidth: true`) expand proportionally instead of leaving stranded fixed-width islands or unanchored gaps?

## 3. Styling, Focus Indicators & Monochrome Resilience

Inspect `capture_styled.txt` (`--- styles` section) or `qmltui-runner --ruler --styled`:

- **Note on `styledText()` vs. `NO_COLOR`**: `styledText()` and `app.expectStyle()` inspect the logical `TextGridBuffer` (`#RRGGBBAA` colors and SGR attributes). Setting `NO_COLOR=1` or `QMLTUI_COLOR_DEPTH=mono|16|256|truecolor` sets `TerminalCapabilities.colorDepth` and downsamples emitted ANSI bytes in normal headless probes (`frameStats().bytesTotal`, QML bindings reading `Terminal.capabilities.colorDepth`) and `--live` runs (non-TTY headless probes default to `mono` unless `QMLTUI_COLOR_DEPTH` is set).
- **Recording override**: `--record` / `RECORD` intentionally pins the virtual recording terminal to 24-bit `TrueColor` (`Runner::armRecording()`) so `.cast` demos stay byte-identical across environments; do not combine `--record` with `NO_COLOR=1` or `QMLTUI_COLOR_DEPTH=16` when testing color downsampling.
- **Accessible focus & selection**: Ensure focused controls and selected rows are distinguishable without relying on truecolor alone—combine color changes with `bold`, `reversed`, `underline`, border style changes (`Border.Double` / `Border.Rounded`), or glyph markers (`▸`, `[x]`) so 16-color and `NO_COLOR=1` terminals stay usable.
- **Explicit color overrides**: Verify custom controls preserve user-supplied color overrides across focused and disabled states.

## 4. `Terminal.Inline` & `Scrollback` Verification

Inline CLIs and REPLs (`mode: Terminal.Inline`) follow distinct layout and assertion rules:

- **Finite vertical height**: Never bind a root child to `height: parent.height` or `anchors.fill: parent` vertically in `Inline` mode; root height is derived from visible children (capped at `rows - 1`). Hidden (`visible: false`) items contribute `0` height.
- **Checkpoint scrollback rows**: Completed history leaves the live bottom frame and enters `Scrollback`. Capture `const mark = app.scrollbackRowCount()` before an action, then assert newly committed output with `app.expectScrollbackText("...", { timeout: 2000 })` or `app.expectScrollbackSnapshot("snapshots/step.txt", { fromRow: mark })`.

## 5. Retained-Mode Performance, Idle Budget & Paint Hygiene

QmlTui's retained-mode scene graph should consume **zero renders while idle** and paint only dirty subtrees:

1. **Zero `dirtyDuringPaint`**: Never mutate a QML property or call `markDirty()` from inside `Item::paint()`. Run checks with `QMLTUI_DEBUG_PAINT_DIRTY=1` (enabled automatically by `qmltui-runner --sweep` / `--ruler` / `--idle-ms`) and verify `app.frameStats().dirtyDuringPaintTotal === 0`.
2. **Idle repaint budget**: Guard animations, `BusyIndicator`, and repeating `Timer` items with `running: visible && ...` so hidden tabs or settled screens do not tick at 60 FPS. Prove idle stability in scenarios (or via `qmltui-runner --clock virtual --idle-ms 500`):
   ```javascript
   /** @param {Runner} app */
   function main(app) {
       app.expectText("Ready");
       app.sleep(300); // Let initial startup timers settle.
       const before = app.frameCount();
       app.sleep(500);
       if (app.frameCount() !== before) {
           app.fail("Unexpected idle repaints: " + (app.frameCount() - before));
       }
   }
   ```
   Run on `CLOCK VIRTUAL` to catch QML `Timer`/animation leaks instantaneously, and on `CLOCK REAL` when the app owns C++ `QTimer`s or worker threads.
3. **Diff byte & paint cost checks**: Inspect `app.frameStats()` (`paintMicrosAvg`, `paintMicrosMax`, `diffBytes`, `bytesTotal`, `skippedForBackpressure`). Small cursor moves or single-cell updates should emit tiny `diffBytes`, not full-screen redraws.
4. **Release benchmarks (QmlTui repository)**: When changing `TextGridBuffer`, `Renderer`, layout traversal, or view virtualization (`ListView`, `TableView`, `TreeView`, `GridView`), build Release benchmarks in a dedicated tree:
   ```bash
   cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
     -DQMLTUI_BUILD_TESTS=OFF -DQMLTUI_BUILD_EXAMPLES=OFF -DQMLTUI_BUILD_BENCHMARKS=ON
   cmake --build build-release
   ./build-release/tests/Core/benchmark_render
   ./build-release/tests/Core/benchmark_views
   ```
