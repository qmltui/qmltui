# Agentic Development Loop with Runner

Raw-mode and `Terminal.AltScreen` applications are hard for AI coding agents to verify directly: interactive binaries block waiting for a TTY or emit cursor-positioning ANSI sequences, while terminal screenshot loops require a display server and miss single-cell clipping.

`qmltui::Runner` and `qmltui-runner` give coding agents (Claude Code, Codex, Gemini CLI, Antigravity, Cursor, Zed) a deterministic feedback loop on `QCoreApplication` without a TTY or display server. Agents compile a target, capture plain UTF-8 frames (`capture.txt`) and `#RRGGBBAA`/SGR style grids (`capture_styled.txt`), step animations on a virtual clock, and inspect structured diagnostics (`result.json`).

For the complete `app.*` scenario API, CLI flags, `CMakeLists.txt` setup, and artifact format, see [Testing and Recording with Runner](testing.md) and the [Runner Reference](../cpp-classes/runner.md).

## 1. Locate `qmltui-runner` and Project Targets

Ensure your project links `qmltui::runner` under `if(BUILD_TESTING AND TARGET qmltui::runner)` and sets `set(QT_QML_GENERATE_QMLLS_INI ON)` alongside `qmltui_scenario_editor_support()` (see [CMake Integration](testing.md#3-cmake-and-ctest-integration) and [Editor Setup](editor-setup.md)).

| Project Layout | `qmltui-runner` Path |
| :--- | :--- |
| Installed package (`find_package(QmlTui)`) | `<install-prefix>/bin/qmltui-runner` (or `qmltui-runner` on `PATH`) |
| `FetchContent` consumer build (`-B build`) | `./build/_deps/qmltui-build/tools/qmltui-runner/qmltui-runner` |
| QmlTui repository build (`-B build`) | `./build/tools/qmltui-runner/qmltui-runner` |

## 2. Run the Four-Step Verification Loop

Run the narrowest check that proves each change before running the full suite:

1. **Build and lint QML (`<target>_qmllint`)**:
   ```bash
   cmake --build build --target ops_console_qmllint ops_console
   ```
   Catches invalid imports (such as `import QtQuick`), unknown properties, and unqualified lookups before execution.

2. **Inspect initial cell geometry (`--capture-after-ms 0`)**:
   ```bash
   qmltui-runner --capture-after-ms 0 --columns 80 --rows 24 --artifacts ./tmp/frame-80x24 -- ./build/ops_console
   ```
   Writes `capture.txt` and `capture_styled.txt` into a fresh `--artifacts` directory (or omit `--artifacts` to use an auto-allocated temp path printed to `stderr`). Repeat at wider dimensions (for example `--columns 110 --rows 34`) to verify responsive layouts.

3. **Probe interactions and animations inline (`--eval` + `--clock virtual`)**:
   ```bash
   qmltui-runner --clock virtual --columns 80 --rows 24 \
     --eval 'app.clickText("Start"); app.advanceTime(300); app.expectText("RUNNING"); app.captureText("after_start.txt");' \
     -- ./build/ops_console
   ```
   `--clock virtual` steps QmlTui animations and QML `Timer` items instantaneously via `app.advanceTime(ms)` (use `--clock real` only for C++ `QTimer`, worker threads, or `PtyView`). For `Terminal.Inline` CLIs, assert emitted history with `app.scrollbackRowCount()` and `app.expectScrollbackText(...)`. On failure (exit code `1`), read `result.json` (`message` and `operation`) and `failure.txt` (or `.diff` on snapshot mismatch); exit code `2` indicates CLI, handshake, or crash errors.

4. **Lock behavior into CTest (`qmltui_add_scenario`)**:
   ```bash
   QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 ctest --test-dir build -R "^ops_console_smoke$" --output-on-failure
   ctest --test-dir build -LE Demo --output-on-failure
   ```
   Review updated snapshot files with `git diff` so unintended layout shifts never become the baseline.

## 3. Walkthrough: Catching Cell-Clipping Bugs Static Tools Miss

Compilers and `qmllint` validate types, not character-cell widths. Consider a 27-column `Panel` with two bordered `Button` controls:

```qml no-playground
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Q) {
            event.accepted = true;
            Qt.quit();
        }
    }

    Panel {
        x: 1
        y: 1
        width: 27
        height: 5
        title: Text {
            text: "CONTROLS"
            bold: true
        }

        Row {
            x: 2
            y: 1
            spacing: 1

            Button {
                width: 11
                height: 3
                text: "[S] START"
            }

            Button {
                width: 11
                height: 3
                text: "[X] STOP"
            }
        }
    }
}
```

This passes `qmllint`, but `qmltui-runner --capture-after-ms 0` immediately exposes clipped labels in `capture.txt`:

```text
┌─ CONTROLS ──────────────┐
│ ┌─────────┐ ┌─────────┐ │
│ │ S] STAR │ │ [X] STO │ │
│ └─────────┘ └─────────┘ │
└─────────────────────────┘
```

`Button` defaults to `leftPadding: 2` and `rightPadding: 2` (1 border cell + 1 inner margin cell per side), leaving `11 - 4 = 7` cells for a 9-cell string. Setting `leftPadding: 1` and `rightPadding: 1` with a width-aware label fixes the frame:

```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Q) {
            event.accepted = true;
            Qt.quit();
        }
    }

    Panel {
        x: 1
        y: 1
        width: 27
        height: 5
        title: Text {
            text: "CONTROLS"
            bold: true
        }

        Row {
            x: 2
            y: 1
            spacing: 1

            Button {
                id: startBtn
                width: 11
                height: 3
                leftPadding: 1
                rightPadding: 1
                text: startBtn.width >= 13 ? "[S] START" : "S START"
            }

            Button {
                id: stopBtn
                width: 11
                height: 3
                leftPadding: 1
                rightPadding: 1
                text: stopBtn.width >= 12 ? "[X] STOP" : "X STOP"
            }
        }
    }
}
```

Re-running `--capture-after-ms 0` confirms clean cell alignment:

```text
┌─ CONTROLS ──────────────┐
│ ┌─────────┐ ┌─────────┐ │
│ │ S START │ │ X STOP  │ │
│ └─────────┘ └─────────┘ │
└─────────────────────────┘
```

## 4. Configure `AGENTS.md` and Reusable Skills

Copy QmlTui's bundled agent skills from `.agents/skills/` into your repository's `.agents/skills/` (or `.claude/skills/`):

- **[`qmltui-qml`](../../.agents/skills/qmltui-qml/SKILL.md)**: Enforces QmlTui imports (no `QtQuick`), `pragma ComponentBehavior: Bound`, required-property delegates, `AltScreen` vs. `Inline` mode, integer cell geometry, borders, and quit paths.
- **[`qmltui-dev-loop`](../../.agents/skills/qmltui-dev-loop/SKILL.md)**: Guides target-scoped CMake builds, `qmltui-runner` frame captures, virtual-clock scenarios, and artifact inspection.

Pair the skills with a concise root `AGENTS.md` (see QmlTui's [`AGENTS.md`](../../AGENTS.md)):

```markdown
# Project Agent Instructions

- Build terminal UIs with `QtQml`, `QmlTui`, and `QmlTui.Controls`. Never import `QtQuick` or link `Qt6::Quick`.
- All item geometry (`x`, `y`, `width`, `height`, margins, padding, spacing) uses integer character cells.
- Provide a reachable keyboard quit path (`Q` or contextual `Escape` at the root `Terminal` after closing popups or clearing input focus).
- Build and lint before running scenarios: `cmake --build build --target ops_console_qmllint ops_console`
- Verify every visible UI change against a committed terminal frame via `qmltui-runner` (`capture.txt`, `final.txt`, or `expectSnapshot()`).
- Run `ctest --test-dir build -LE Demo --output-on-failure` and `git diff --check` before handoff.
```
