# Plan 06: From Solid to Excellent (post-launch quality roadmap)

## 1. Goal

Plans 00 to 05 make Qamui presentable. This plan records what separates a
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
| Release `libQamuiCore.so` | 2.6 MB; links Qt6Core, Qt6Qml, ICU |
| `benchmark_render`, worst scene at 240x70 | paint 137 us, diff 196 us |
| Source size | 26k lines C++/QML, 9.4k tests, 35k examples |

Defects found during the assessment, all reproducible from the tree at the
time. Status as of 2026-09-12: all five fixed.

- **TableView truncates right-aligned cells.** The README flagship example
  (`examples/apps/service_monitor`) rendered `LATENC|` and `12 m|` instead of
  `LATENCY` and `12 ms`: right alignment was computed against the full column
  width while the separator occupied the last cell. Fixed in `cb2b01c`
  (2026-09-11), covered by `test_table_view_hardening`.
- **`benchmark_render` composite scene failed to load** with
  `Cannot assign to non-existent property "maxVal"`. Fixed in `23ddb4b`
  (2026-09-11); the composite numbers are reproducible again.
- **`Column`/`Row` ordered Repeater delegates after static children** regardless
  of source order. Fixed in `cb2b01c` via `Item::reorderChildItemsAfter()`,
  covered by `test_repeater`.
- **`Behavior on <prop>` never applied** when the QML-declared property had a
  binding. Fixed in `cb2b01c` (interceptor bypass keeps the binding), covered
  by `test_behavior`.
- **`TestRunner.typeText()` delivered `event.key == 0`**, so handlers matching
  `Qt.Key_*` for printable keys ignored scripted input. Fixed 2026-09-12:
  `typeText()` and `injectText()` send one key event per character with the
  code `InputDecoder::keyForCharacter()` assigns to real input. Covered by
  `test_input_decoder::testPrintableKeyCodes` and the `KEY 2` / `KEY PLUS`
  steps in `tests/TestRunner/scenarios/success.js`.

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
- Model views, profiled 2026-09-11 (`docs/benchmarks/2026-09-11-view-phases.md`).
  Fixed the same day: TreeView now flattens shape only and writes position
  only on scroll (244 us to 98 us per step, creation nine times cheaper),
  GridView pools delegates (95 us to 58 us). Still open: every
  `QQmlPropertyMap` row builds its own dynamic meta-object (hidden by the
  pools), name-based `setProperty` in ListView/GridView rebinding, and
  `StandardItemDelegate` at about 100 us and 6 KB per instance.

---

## 3. Qt Quick Semantic Fidelity (highest impact)

The promise is "QML for the terminal". Every divergence from Qt Quick that a
Qt developer hits in the first afternoon destroys trust in the whole framework.

1. **Conformance suite.** A `tests/Conformance` target that loads the same QML
   (restricted to the shared subset: Item, Rectangle-like, Text, Row, Column,
   Grid, Repeater, Loader, anchors, layouts) through `QQuickView` offscreen and
   through Qamui, then compares item geometry after layout. Column ordering,
   anchor edge cases, implicit-size propagation, and focus-chain order fall out
   of it automatically. Requires `Qt6::Quick` as a test-only dependency.
2. **Fix the three verified divergences** listed in Section 2 (Repeater order,
   Behavior on bound property, `typeText` key code) with conformance tests.
3. **Tooling parity.** Verify `qmlls` completion and `qmllint` against the
   installed qmltypes; add a `qmllint` pass over `examples/` to CI. Qt
   developers judge a QML module by whether Qt Creator understands it.

Exit criterion: the conformance suite runs in CI and every documented Qamui
type in the shared subset passes it.

---

Done 2026-09-15: `AbstractButton` (`background`, `contentItem`, `down`,
`checkable`, `checked`, `autoExclusive`, `highlighted`, paddings) is the
template behind Button, CheckBox, RadioButton and Switch, replacing their
Rectangle roots; the default look is pinned by `test_button_look`. Slider and
Panel keep their Rectangle roots for now.

## 4. Behavior: Own It or Design Around It

Behavior is what makes QML animation declarative. Today it is optional, gated
on a private Qt header, and half-broken (Section 2). Pick one path:

| Path | Work | Result |
| :--- | :--- | :--- |
| **Own it** | Make `QAMUI_ENABLE_BEHAVIOR` default ON; add Qt 6.9, 6.10, 6.11 rows to CI; document the supported Qt matrix | Private-header breaks are caught the day they happen |
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
   replacing `COLORTERM`/`TERM` guesses. Keep `QAMUI_COLOR_DEPTH` as the
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

Admit under the existing `Qamui.Controls` rule (needs engine knowledge or is
universal): ComboBox, Menu, MenuBar, context menu, ScrollBar, SpinBox. Done
2026-09-15: `MouseArea.containsMouse` with `entered`/`exited` (the Qt Quick
names rather than `hovered`), and `Button.hoverEnabled`/`hovered` as the
first control to use it; other controls can opt in the same way.

Reconsider the no-palette stance. Qt Quick Controls has an inherited
`palette`; an inherited `palette` property on `Item` is Qt Quick naming, not a
foreign theme engine, and it is the single feature Textual users cite most.
This keeps "no framework theme singleton" while giving applications one place
to set colours.

Noted but not admitted yet, carried over from the early missing-features list:
drag and drop (no API for starting a drag, showing the dragged item, or drop
targets), drop shadows drawn with background colours and block characters, and
a wrapping or proportional layout in the spirit of CSS flexbox and grid beyond
`Row`, `Column`, `GridLayout` and anchors.

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

1. Tag `v0.4.0` (Plan 00); add vcpkg and Conan ports so `find_package(Qamui)`
   works without cloning.
2. Publish the quickstart the CI already compiles.
3. Find three real applications outside `examples/`. Excellent frameworks are
   shaped by users who are not the author; Qamui has none yet.

---

## 10. Comparison Context

Where Qamui stands against the field, for prioritising the above:

| Library | Qamui wins on | Qamui loses on |
| :--- | :--- | :--- |
| ncurses, Notcurses (C) | layout, Unicode, model/view, tests | maturity, footprint, graphics protocols |
| FTXUI (C++17) | models, anchors, animation, tests | simplicity, zero dependencies |
| Ratatui (Rust) | retained mode, model/view | ecosystem, community, 3 MB static binary |
| Bubble Tea (Go) | rich widgets, testing | ecosystem, styling (Lip Gloss) |
| Textual (Python) | startup, native speed | CSS theming, widget count, docs |
| Ink (JS) | speed, memory | flexbox, React familiarity |

Textual is the closest in spirit. Sections 3, 6, and 7 close most of the gap
to it; nothing else on the list offers QML, Qt models, or native C++.

---

## 11. Findings From the Reference Documentation Pass (2026-09-14)

Writing the `docs/components` pages for `Qamui.Controls` against the source
turned up the following. Items marked *fixed* were resolved the same day
(commit noted in git history as "fix(controls)"); each page documents the
current behaviour. Triage the rest into section 3 or 7 as they are taken.

Inputs:

- `Button.pressed` is set by the mouse only; keyboard activation never sets it.
- *fixed* The mouse path bypassed `enabled` in Button, CheckBox and Switch and
  `Slider` checked it on neither path; all five now guard both paths (the
  engine already withheld input from disabled items, so this was latent).
- *fixed* `Slider` mouse clicks now snap to `stepSize` and use the same
  guarded range as the track. Still open: `value` is not clamped when
  `from`/`to` change.
- *fixed* `RadioButton._uncheckGroup()` and `BusyIndicator._frameIndex/_frames/_currentSymbol`
  moved into private `QtObject`s and are no longer part of the type.
- `ButtonGroup` does not reset `buttonGroup` on items removed from `buttons`.
- `examples/components/slider` says "Mouse Drag" but only clicks are handled.

Overlays and navigation:

- *fixed* `Popup::close()` on a popup that is not open is now a no-op instead
  of clearing terminal focus (ToolTip called it unconditionally).
- `Popup.opened()` is not emitted when `open()` runs before the popup is in a
  Terminal, or when the popup is already visible.
- `Popup.parentPopup` notifies through `ownerChanged` only; reparenting emits nothing.
- `Popup.color` defaults to `"black"` while every other control uses `"default"`.
- *fixed* `StackView.depth` is now a counter maintained by `push`/`pop`/`clear`;
  the page array is internal; `pop()` and `clear()` destroy only pages the
  view created and hide pages pushed as existing Items.
- *fixed* `Item.children` now notifies through `childrenChanged()` (Qt Quick
  naming), so `StackLayout.count` follows and late children are laid out.
- *fixed* `TabBar.setCurrentIndex` keeps the index while `tabs` is empty and
  `setTabs` clamps it. Still open: hit-testing and painting use
  `QString::length()` not display width.
- `Toast` boxes are not clipped to the item; `Toast::onTick` is exported as a method.
- `Panel` title overruns the border when wider than the panel.
- `ToolTip` reserves multi-line height but `Text` does not soft-wrap.
- *fixed* (2026-09-15) Hover requests bypassed `Terminal.mouseReporting`: a
  hover-enabled area enabled any-motion reporting, and dropping the last
  request re-enabled button-held motion, even with the switch off. The engine
  now writes the whole mouse state from the switch plus the motion request in
  one place (`applyMouseModes()`), covered by `test_mouse_modes`.

Indicators:

- *fixed* `ProgressBar.style` presets now render as promised: `"segmented"`
  ▰▱, `"pill"` ▮▯, `"dot"` ●○, `"hash"` #-, and the default is named
  `"block"` (█░), which keeps the previous default look.
- `Sparkline` uses `to == 0.0` as the auto-scale sentinel, so a fixed range
  ending at 0 auto-scales; `to <= from` forces a range of 1 silently; left
  padding repeats the first value instead of leaving cells blank.
- `StandardItemDelegate`: `ListView` consumes Space and Return into
  `itemActivated`, so the delegate's own Space handling rarely runs;
  `docs/guide/models.md` says otherwise. `toggled` is emitted only from
  `toggle()`, not from external `setData()` changes.
- `docs/qml_api.md` lists `Sparkline` as having only `color`; it also has `backgroundColor`.
