# Plan 06: From Solid to Excellent (post-launch quality roadmap)

## 1. Goal

Plans 00 to 05 make Qtui presentable. This plan records what separates a
presentable framework from one people build on, based on an assessment of the
tree on 2026-09-10. It is ordered by impact. Sections 2, 4, and 6 are the three
to do first if only three can be done: they address correctness, the input
experience, and trust in the documentation.

## 2. Assessment Baseline (2026-09-10)

Measured on the Release tree (`build/`, Qt 6.8.6, Behavior ON, WSL2):

| Check | Result |
| :--- | :--- |
| Full CTest suite | 87/87 passed in 17 s |
| Startup to first frame, `service_monitor` (offscreen) | 30 ms |
| Peak RSS, `service_monitor` / `btop` | 30 MB / 34 MB |
| Release `libQtuiCore.so` | 2.6 MB; links Qt6Core, Qt6Qml, ICU |
| `benchmark_render`, worst scene at 240x70 | paint 137 us, diff 196 us |
| Source size | 26k lines C++/QML, 9.4k tests, 35k examples |

Defects found during the assessment, all reproducible from the tree:

- **TableView truncates right-aligned cells.** The README flagship example
  (`examples/apps/service_monitor`) renders `LATENC|` and `12 m|` instead of
  `LATENCY` and `12 ms`. Right alignment appears to be computed against the
  full column width while the separator occupies the last cell.
- **`benchmark_render` composite scene fails to load** with
  `Cannot assign to non-existent property "maxVal"`, so the composite numbers
  in `docs/benchmarks/0.4-acceptance/` cannot be reproduced from the current
  tree.
- **`Column`/`Row` order Repeater delegates after static children** regardless
  of source order (verified 2026-09-09 on `cyber_audio`).
- **`Behavior on <prop>` never applies** when the QML-declared property has a
  binding (verified 2026-09-09).
- **`TestRunner.typeText()` delivers `event.key == 0`**, so handlers matching
  `Qt.Key_*` for printable keys ignore scripted input.

Gaps confirmed by grep, not bugs:

- No kitty keyboard protocol, no focus in/out reporting (`?1004`), no
  capability probing (DA1, XTVERSION, DECRQM); colour depth comes from
  environment heuristics only.
- No `SIGTSTP`/`SIGCONT` handling and no crash handler; Ctrl+Z or a segfault
  leaves the terminal in raw alternate-screen mode.
- CI builds Qt `6.8.*` only, while Behavior depends on `Qt6::QmlPrivate`.
- No ComboBox, Menu/MenuBar, context menu, ScrollBar, SpinBox, public
  `hovered` state, or wrapping Flow layout. (Layouts with `fillWidth`,
  stretch, and span attached properties do exist.)
- Whole-grid diff every frame; at 240x70 the diff costs as much as the paint.

---

## 3. Qt Quick Semantic Fidelity (highest impact)

The promise is "QML for the terminal". Every divergence from Qt Quick that a
Qt developer hits in the first afternoon destroys trust in the whole framework.

1. **Conformance suite.** A `tests/Conformance` target that loads the same QML
   (restricted to the shared subset: Item, Rectangle-like, Text, Row, Column,
   Grid, Repeater, Loader, anchors, layouts) through `QQuickView` offscreen and
   through Qtui, then compares item geometry after layout. Column ordering,
   anchor edge cases, implicit-size propagation, and focus-chain order fall out
   of it automatically. Requires `Qt6::Quick` as a test-only dependency.
2. **Fix the three verified divergences** listed in Section 2 (Repeater order,
   Behavior on bound property, `typeText` key code) with conformance tests.
3. **Tooling parity.** Verify `qmlls` completion and `qmllint` against the
   installed qmltypes; add a `qmllint` pass over `examples/` to CI. Qt
   developers judge a QML module by whether Qt Creator understands it.

Exit criterion: the conformance suite runs in CI and every documented Qtui
type in the shared subset passes it.

---

## 4. Behavior: Own It or Design Around It

Behavior is what makes QML animation declarative. Today it is optional, gated
on a private Qt header, and half-broken (Section 2). Pick one path:

| Path | Work | Result |
| :--- | :--- | :--- |
| **Own it** | Make `QTUI_ENABLE_BEHAVIOR` default ON; add Qt 6.9, 6.10, 6.11 rows to CI; document the supported Qt matrix | Private-header breaks are caught the day they happen |
| **Design around it** | Ship animated value types (`AnimatedColor`, `AnimatedInt`) that need no interceptor; document plain `Behavior` as unsupported | No private dependency; different syntax from Qt Quick |

Half-on, half-off is the worst option for users. The recommendation is to own
it: the CI matrix is cheap and the feature is core to the value proposition.

---

## 5. Keyboard and Terminal as First-Class Devices

1. **Kitty keyboard protocol** (`CSI > flags u`) with graceful fallback to the
   legacy sequences already decoded. Unlocks Ctrl+Shift combinations,
   Ctrl+Enter, key release, and unambiguous Escape.
2. **Focus reporting** (`?1004`) surfaced as `Terminal.active` and used to pause
   cursor blink and animations.
3. **Capability probing at startup**: DA1, XTVERSION, and DECRQM for `?2026`,
   replacing `COLORTERM`/`TERM` guesses. Keep `QTUI_COLOR_DEPTH` as the
   override.
4. **Signal safety** in `PosixTerminalBackend`: `SIGTSTP` restores the terminal
   and re-enters raw mode on `SIGCONT`; an async-signal-safe crash handler for
   `SIGSEGV`/`SIGABRT`/`SIGTERM` writes the reset sequence
   (`\033[?1049l\033[?25h\033[0m\033[?1000l...`) before re-raising.

Exit criterion: `test_input_decoder` covers kitty sequences; a test sends
`SIGTSTP` to a child running `quickstart_test` and asserts the terminal mode
is restored.

---

## 6. Documentation That Cannot Go Stale

The README example renders a truncated column today and nothing caught it.
`TestRunner` already writes normalized frames.

1. Every documentation screenshot is the `final.txt` (or SVG derived from it,
   Plan 02) of a scenario test. No hand-captured images.
2. CI fails when a committed frame differs from the generated one, the same
   way snapshot tests work. Intentional changes update the committed frame in
   the same PR.
3. Every code block in `docs/` and `README.md` is extracted and compiled
   (Plan 01 already requires this for the quickstart; extend it to all).

Exit criterion: `grep -rL` finds no image or frame in `docs/` without a
generating test.

---

## 7. Control Set Every Dashboard Needs

Admit under the existing `Qtui.Controls` rule (needs engine knowledge or is
universal): ComboBox, Menu, MenuBar, context menu, ScrollBar, SpinBox, and a
public `hovered` property with `entered`/`exited` signals on `MouseArea`.

Reconsider the no-palette stance. Qt Quick Controls has an inherited
`palette`; an inherited `palette` property on `Item` is Qt Quick naming, not a
foreign theme engine, and it is the single feature Textual users cite most.
This keeps "no framework theme singleton" while giving applications one place
to set colours.

---

## 8. Performance Under a Regression Gate

1. Fix the composite benchmark scene (Section 2).
2. Run `benchmark_render --json` in CI on a fixed runner, trend the medians,
   and fail on a regression above a documented threshold (start at 30%,
   tighten once variance is known).
3. **Dirty-row diff.** Record the row range touched during paint and diff only
   those rows. At 240x70 this removes most of the 100 to 200 us diff cost for
   small updates.
4. Publish footprint honestly: RSS floor, shared-library set, startup time.
   Offer a static-Qt build recipe for deployments where 30 MB matters.

---

## 9. Ship and Get Users

1. Tag `v0.4.0` (Plan 00); add vcpkg and Conan ports so `find_package(Qtui)`
   works without cloning.
2. Publish the quickstart the CI already compiles.
3. Find three real applications outside `examples/`. Excellent frameworks are
   shaped by users who are not the author; Qtui has none yet.

---

## 10. Comparison Context

Where Qtui stands against the field, for prioritising the above:

| Library | Qtui wins on | Qtui loses on |
| :--- | :--- | :--- |
| ncurses, Notcurses (C) | layout, Unicode, model/view, tests | maturity, footprint, graphics protocols |
| FTXUI (C++17) | models, anchors, animation, tests | simplicity, zero dependencies |
| Ratatui (Rust) | retained mode, model/view | ecosystem, community, 3 MB static binary |
| Bubble Tea (Go) | rich widgets, testing | ecosystem, styling (Lip Gloss) |
| Textual (Python) | startup, native speed | CSS theming, widget count, docs |
| Ink (JS) | speed, memory | flexbox, React familiarity |

Textual is the closest in spirit. Sections 3, 6, and 7 close most of the gap
to it; nothing else on the list offers QML, Qt models, or native C++.
