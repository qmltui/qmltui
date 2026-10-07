# Plan 00: Launch Readiness (API Polish, Compiled Quickstart, CI, v0.4.0 Tag)

## 1. Goal

Nothing has been published yet. That means every naming inconsistency, every
unverified snippet, and every missing tag can still be fixed for free. This plan
fixes them before the first public impression, so a newcomer's first ten minutes
never hit a broken step.

Exit criteria:

- Every public QML control uses Qt Quick naming where an equivalent exists.
- `Terminal` is the only root type, with explicit `FullScreen` or `Inline`
  mode. Visible children form the UI; `Scrollback` owns committed output.
  `rendererMode`, `useAlternateScreen`, `fillTerminal`, `liveItem`, `StreamView`, and
  `Terminal.border` no longer exist.
- A quickstart project lives in the repo, is compiled in CI, and is the exact
  text the documentation site embeds.
- GitHub Actions builds and tests the library on every push.
- Terminal I/O sits behind a `TerminalBackend` seam with a POSIX and a Win32
  console implementation, and every example runs in Windows Terminal.
- `v0.4.0` is tagged so `FetchContent` and `find_package` have a real target.

---

## 2. API Consistency Pass (Qt Quick naming)

Newcomers are Qt developers. Every place Qtui deviates from Qt Quick naming
becomes a permanent papercut once published. Audit and fix now.

Known deviations:

| Where | Today | Change to | Reason |
| :--- | :--- | :--- | :--- |
| `Button`, `CheckBox`, `RadioButton`, `Switch` | `property string label` | `text` | Qt Quick `AbstractButton.text` |
| All examples and docs | `import QtQml 2.15` | `import QtQml` | Versionless imports are the Qt 6 convention |

Procedure:

1. Grep every `property` declaration in `src/Qtui/Controls/*.qml` and
   `src/Qtui/Extras/*.qml` and compare against the matching Qt Quick Controls
   type. Record each deviation in a table in the PR description.
2. Rename in the control, then update `examples/`, `tests/`, and every markdown
   under `src/Qtui/*/doc/` and `docs/` in the same commit. No deprecation
   aliases: there are no users yet.
3. Run the full suite in both Behavior trees (see `AGENTS.md`).

```bash
grep -n 'property ' src/Qtui/Controls/*.qml | grep -v 'readonly'
grep -rln 'import QtQml 2' examples src tests docs
```

---

## 3. Root Type Redesign: Explicit Mode, Ordinary Children, Separate Output

Design approved on 2026-09-08. This section describes the intended API, not
the currently implemented API.

### The problem today

`examples/concepts/agent_stream` coordinates `rendererMode`, `fillTerminal`,
and `useAlternateScreen` to change presentation. These overlapping controls
should become one property. Visibility and focus remain ordinary QML concerns.

The current `StreamRenderer` already paints the root's visible subtree;
`liveItem` contributes sizing and notifications but does not select the rendered
subtree. `StreamView` is a nonvisual `QObject`, despite its name, and duplicates
the root's printing API. Output is forwarded to the active renderer, whose
default `print()` implementation does nothing in frame mode.

### The design

Two types, with presentation independent from output and object creation.

| Type | Role |
| :--- | :--- |
| `Terminal` | The only root. Inherits `Item` and owns one engine and terminal session. Explicit `mode`, defaulting to `Terminal.FullScreen`. |
| `Scrollback` | Nonvisual output object, renamed from `StreamView`. Owns model/delegate output and stream submission. Its presence never changes mode. |

`mode` controls terminal presentation, `visible` controls displayed content,
and `focus` controls keyboard ownership. No nested terminals, `TerminalPage`,
or special takeover lifecycle is introduced.

### `Terminal` properties

| Property or member | Notes |
| :--- | :--- |
| `title` | Window title, unchanged |
| `color` | Background, mirrors Qt Quick `Window.color`. `border` is removed. |
| `mode` | `Terminal.FullScreen` (default) or `Terminal.Inline`; supports runtime changes. Selects renderer, screen buffer, and root sizing together. |
| `width`, `height` | Session-managed geometry, according to the sizing rules below. Fixed-size panels belong inside the root. |
| `terminalWidth`, `terminalHeight` | Read-only, unchanged |
| `mouseReporting`, `clipboardMode`, `copyToClipboard()` | Unchanged |
| Focus and hover accessors | Preserve the current contract; no takeover-specific focus API. |
| Removed | `rendererMode`, `useAlternateScreen`, `fillTerminal`, `liveItem`, `print()`, `append()`, `border` |

### `Scrollback` properties

| Property or member | Notes |
| :--- | :--- |
| `model`, `delegate`, `spacing`, `separator`, `reuseItems` | Moved from `StreamView` |
| `append(data)`, `append(item)`, `print(text)`, `clear()` | Moved from `StreamView` and `Terminal` |
| Not an `Item` | No geometry, anchors, or participation in layout. |

Scrollback is committed output, not an editable view. Specify append, model
reset, and `clear()` semantics before implementation; clearing logical state
must not promise to retract bytes already committed to the user's terminal.

### Visible children and sizing

All visible children participate in the same UI in both modes. The root does
not stack children or choose an active page. Children at the same coordinates
can overlap under normal painting order; use `Row`, `Column`, or other layouts
to arrange them. Creating or destroying a `Scrollback` has no presentation effect.

| Mode | Root size |
| :--- | :--- |
| `FullScreen` | Terminal width and height. |
| `Inline` | Terminal width; height derived from visible content bounds and capped to available rows. |

Inline children establish their height through explicit or implicit sizing.
Vertically filling the root cannot determine the root's own content-driven
height. Hidden content does not contribute to inline bounds.

### Presentation transitions and output ownership

One session owns terminal I/O, input, and screen transitions. Renderers draw
the visible UI; scrollback submission moves out of the renderer interface.
Changing `mode` preserves the session and QML objects, without hiding children
or moving focus automatically. Applications bind page visibility and focus to
their own state, including contextual Escape behavior.

Scrollback submissions made during full-screen mode wait until inline mode
resumes. Restore the normal screen, commit pending output in submission order,
then redraw the live region. Define bounded buffering and an observable overflow
policy before implementing this path; output must not silently disappear into
the frame renderer or accumulate without a limit.

### Before and after

```qml
pragma ComponentBehavior: Bound
import QtQml
import Qtui

// Dashboard: full-screen by default
Terminal {
    title: "PLC Cockpit"
    color: "#0d1117"
    Row { anchors.fill: parent }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            event.accepted = true
            Qt.quit()
        }
    }
}
```

The following composition sketch uses application-defined `PromptBar`,
`ShellPage`, and `MessageCard` components and an existing `chatHistory` model:

```qml
pragma ComponentBehavior: Bound
import QtQml
import Qtui

Terminal {
    id: root
    title: "Qt AI Studio"
    property bool shellMode: false
    mode: shellMode ? Terminal.FullScreen : Terminal.Inline

    Scrollback {
        id: history
        model: chatHistory
        delegate: MessageCard {}
    }

    PromptBar {
        id: prompt
        visible: !root.shellMode
        onSubmitted: (text) => chatHistory.append({ type: "user", text: text })
        onShellRequested: root.shellMode = true
    }

    ShellPage {
        id: shell
        anchors.fill: parent
        visible: root.shellMode
        onExitRequested: root.shellMode = false
    }

    onShellModeChanged: {
        if (shellMode)
            shell.focusShell()
        else
            prompt.focusPrompt()
    }

    Keys.onPressed: event => {
        if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Q) {
            event.accepted = true
            Qt.quit()
        }
    }
}
```

Both pages stay alive while hidden. If both are visible, both render. Shell
exit and contextual Escape are handled by the application components; changing
mode itself does not select a page or establish a focus-restoration policy.

### Migration inside the tree

| Today | After |
| :--- | :--- |
| `Terminal { rendererMode: Terminal.Frame }` | `Terminal {}` |
| `rendererMode: Terminal.Stream; useAlternateScreen: false; liveItem: x` | `mode: Terminal.Inline`; ordinary children form the live region |
| `StreamView { model; delegate }` | `Scrollback { model; delegate }` |
| `Terminal.print()`, `StreamView.print()` | `Scrollback.print()` |
| Coordinated renderer, screen-buffer, and fill properties | `mode: cond ? Terminal.FullScreen : Terminal.Inline`; retain application visibility and focus bindings |
| `Terminal.border` | Removed |
| `fillTerminal`, explicit root dimensions | Remove `fillTerminal`; move fixed-size geometry to an inner item |

Files to update: `src/Qtui/items/Terminal.{h,cpp}`, `StreamView.{h,cpp}` (renamed to
`Scrollback`), `engine/TerminalEngine.cpp` and renderers (mode transitions and
separate scrollback output),
`examples/concepts/agent_stream/Main.qml`, every example root that sets
`fillTerminal: true` or a border, `docs/qml_api.md`, `docs/architecture.md`,
`.agents/skills/qtui-qml/SKILL.md` section 3, and the `TestRunner` hook if it
reads `rendererMode`.

### Acceptance

- No removed public API remains in implementation, examples, tests, or current
  API documentation. Migration explanations in this roadmap may name old APIs.
- `agent_stream` uses one `mode` binding plus ordinary visibility and focus;
  entering and leaving its persistent shell passes committed-frame assertions.
- Multiple visible children render together in both modes; layout, overlap,
  hidden-child sizing, and content-height changes have committed-frame coverage.
- Adding or removing `Scrollback` never changes mode. Output submitted while
  full-screen is preserved and committed in order when inline resumes.
- Device-output tests verify alternate-screen entry/exit, output ordering,
  bounded-buffer behavior, and terminal restoration; headless frames alone
  cannot verify terminal I/O.
- A new concept example `examples/concepts/inline_repl/` shows the minimal
  inline case: one `Scrollback`, one `TextInput` as the live region, under 40
  lines, with a scenario test.
- Both Behavior trees green; POS and PLC frame results preserved in applicable
  configurations. PLC is exercised only with Behavior enabled.

## 4. Module Boundaries, Component Cuts, and Terminal Colors

Decided on 2026-09-08, colors revised on 2026-09-09. Three problems share one
fix window: which module a type lives in is not principled (`Qtui.Extras`
exists because nothing else fit), the library ships components that are an
application's business, and every control invents its own color properties
(Button alone has nine) while three example apps each wrote their own theme
singleton.

### Module rules

1. **`Qtui`** holds what cannot be built in QML from other Qtui types: the
   scene primitives, geometry and layouts, input areas, the model-driven views,
   text editing, Canvas and its shapes, animation and states, and the root
   types. `ListView` and `TableView` stay here for the same reason they live in
   `QtQuick` rather than `QtQuick.Controls`.
2. **`Qtui.Controls`** holds everything a user operates or reads as a finished
   widget with a consistent look and keyboard behaviour, regardless of whether
   it is implemented in QML or C++. `Popup` moves here (Dialog, ToolTip, and
   Toast build on it, and it is a Controls type in Qt).
3. **A separate module exists only for an optional dependency.** `Qtui.Image`
   (QtGui) and `Qtui.TerminalHost` (libvterm, Unix PTY) stay. `Qtui.Canvas`
   folds into `Qtui`. `Qtui.Extras` is dissolved.

| Module | Contents after the move |
| :--- | :--- |
| `Qtui` | Terminal, Scrollback, Item, Rectangle, Text, Paragraph, TextInput, TextArea, Row, Column, Layouts, Repeater, Loader, MouseArea, ScrollView, ListView, GridView, TableView, TreeView, LogView, Canvas and shapes, animations, states, Keys, Shortcut |
| `Qtui.Controls` | Button, ButtonGroup, CheckBox, RadioButton, Switch, Slider, ProgressBar, BusyIndicator, TabBar, StackView, StackLayout, Panel, Dialog, Popup, ToolTip, Toast, Sparkline |
| `Qtui.Image` | Image |
| `Qtui.TerminalHost` | TerminalView |

Renames to Qt Quick names in the same step: `TabHeader` to `TabBar`, `Spinner`
to `BusyIndicator`, `ToastManager` to `Toast`. Two overlaps get a decision, not
a footnote: `Paragraph` folds into `Text` with `wrapMode`; `LogView` and
`Scrollback` are different things and the docs say how.

### Component cuts

A type stays in `Qtui.Controls` only if it needs engine knowledge an app author
cannot reproduce in QML (cell geometry, Unicode width, virtualization,
overlays) or is universal enough that every TUI toolkit ships it. Everything
else is an application's concern and becomes a recipe under
`examples/components/`, where it teaches composition without an API promise.

| Cut | Reason | Becomes |
| :--- | :--- | :--- |
| `Gauge` | Duplicates ProgressBar; its gradient and bracket options fold into ProgressBar | Removed |
| `BigText` | Niche banner fonts with a large table to maintain | Custom C++ item recipe |
| `StatusBadge` | A Rectangle and a Text with four app-specific colors | QML composition recipe |
| `Calendar` | Heavy to document and test, rare in terminal apps, kept out of Controls by Qt too | QML recipe |

This rule goes into `AGENTS.md` so `Extras` cannot grow back.

### Terminal colors

Superseded on 2026-09-09. This section first specified a palette: named color
roles on a `Palette` singleton in `Qtui.Controls`, implemented in C++ so C++
controls read the same instance, with a `Terminal` preset (ANSI named colors)
and a `Dark` preset (the emerald-on-navy brand look). Later, if a real need
appeared, an inherited `palette` grouped property on `Item` as in Qt Quick.
The singleton was built (`0706d8f`), the inherited property was prototyped,
and both were removed. `v0.4.0` ships neither.

What made the singleton look necessary was the color-property zoo, and that
problem is solved without it. Every role a palette would have supplied turns
out to be either the terminal's own color or the application's decision:

- **Normal text and chrome** are `"default"`: a color name for alpha 0, which
  the encoder emits as SGR 39 for a foreground and SGR 49 for a background. The
  terminal's own foreground and background therefore decide how a control
  looks, on a light theme as much as a dark one. This is the part a palette
  role could not have done. A named entry such as `"white"` does follow the
  user's theme, and is still unreadable on a light background, which is how
  the reversal started.
- **Accents and dimmed states** are named ANSI entries (`"cyan"`, `"gray"`),
  themed by the user's sixteen-color palette.
- **Everything else** belongs to the application, set per instance.

Controls keep one override property each, where a color is genuinely
per-widget: `textColor` on Button, CheckBox, RadioButton, Slider, Switch and
Dialog, `titleColor` on Panel and Dialog, and `ProgressBar.color` because a
fill color is data, not theme. Button is down from nine color properties to
one. The three example apps that each wrote their own theme singleton lose the
duplication by having nothing to duplicate: shared styling across an
application is ordinary QML: a binding, or a component that wraps a control.

Focus and disabled states no longer substitute colors, so whatever an instance
configures survives both. Focus is carried by `border.style` (`double` when
focused, `single` otherwise) and by `bold` on the label.

The same rule applies below `Qtui.Controls`, which is what makes it hold: the
color defaults on `Text`, `TextArea`, `TextInput`, `Paragraph`, `Canvas` and
`Border`, and the text and branch roles on `TableView`, `TreeView` and
`ListView`, are all `"default"`; `Cell::fgColor` and the `TextGridBuffer`
drawing helpers default to the terminal foreground rather than literal white.
A control that painted a literal RGB value would silently ignore the user's
theme, so `tests/Controls/test_terminal_colors.cpp` asserts against it on
committed frames (`defaultInk`, `explicitColorsSurviveStates`,
`selectionColors`, `toastAndTooltipOverrides`).

Open, and not a launch blocker: a disabled control is now visually identical to
an enabled one apart from losing focus, because dimming used to come from the
`placeholderText` role. `Text.dim` (SGR 2) exists and is the natural
replacement, but no control uses it yet.

Order within this section: cuts and moves, then renames. All of it lands
before the README rewrite and the tag so `v0.4.0` carries the final module
shape and property lists.

## 5. The Quickstart as a Compiled Project

The quickstart must be real code that CI compiles, never prose that rots.
Create `examples/quickstart/` with exactly three files. The documentation site
imports these files verbatim (see Plan 01), so the docs and the compiled code
cannot diverge.

### `examples/quickstart/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_qtui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)

# Consume Qtui with zero install. CI overrides the source with
# FETCHCONTENT_SOURCE_DIR_QTUI to test the working tree.
set(QTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QTUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(
    Qtui
    GIT_REPOSITORY https://github.com/yosriayed/Qtui.git
    GIT_TAG        v0.4.0
)
FetchContent_MakeAvailable(Qtui)

qt_add_executable(hello_qtui main.cpp)
qt_add_qml_module(hello_qtui
    URI HelloQtui
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)
target_link_libraries(hello_qtui PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)
```

Notes:

- Target names are `Qtui::Core`, `Qtui::Controls`, `Qtui::Extras`. The old plan
  used `QtuiCore`, which does not exist.
- Do not add `EXCLUDE_FROM_ALL` to the Qtui subdirectory. The QML plugin targets
  (`QtuiCoreplugin`, `QtuiControlsplugin`) are not link dependencies, so with
  `EXCLUDE_FROM_ALL` they never build and the app fails at runtime with
  "module Qtui is not installed". `starter_template` works around this with an
  `add_dependencies` loop; the quickstart avoids the problem entirely.

### `examples/quickstart/main.cpp`

```cpp
#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("HelloQtui", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
```

### `examples/quickstart/Main.qml`

```qml
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root
    color: "#0d1117"

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 1

        Text {
            text: "Welcome to Qtui"
            color: "cyan"
            bold: true
        }

        Button {
            text: "[ Press Enter, or Q to quit ]"
            focus: true
            onClicked: status.text = "Clicked!"
        }

        Text {
            id: status
            text: "Status: ready"
            color: "magenta"
        }
    }
}
```

`Button.text` assumes Section 2 is done. `mode` is omitted because full-screen
is the default; Section 3 removes `fillTerminal`. `color` is the root background,
as on Qt Quick `Window`.

### In-tree build

`examples/CMakeLists.txt` adds `quickstart` like any other example so it is
covered by the normal build. The `FetchContent` block is skipped when
`CMAKE_SOURCE_DIR` is not the quickstart itself, following the pattern already
used by `examples/apps/starter_template/CMakeLists.txt`.

---

## 6. GitHub Actions CI (`.github/workflows/ci.yml`)

There is no `.github/workflows/` directory today. Add one job matrix and one
standalone quickstart job.

```yaml
name: CI
on:
  push:
    branches: [main]
  pull_request:

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-24.04, macos-14, windows-2022]
        behavior: [ON, OFF]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
      - uses: jurplel/install-qt-action@v4
        with:
          version: 6.8.*
          modules: qtshadertools
      - run: >
          cmake -S . -B build
          -DQTUI_BUILD_TESTS=ON -DQTUI_BUILD_EXAMPLES=ON
          -DQTUI_ENABLE_BEHAVIOR=${{ matrix.behavior }}
      - run: cmake --build build --parallel --config Release
      - run: ctest --test-dir build --output-on-failure -C Release

  quickstart:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - uses: jurplel/install-qt-action@v4
        with:
          version: 6.8.*
      - name: Build the quickstart exactly as a newcomer would, against this tree
        run: >
          cmake -S examples/quickstart -B build-qs
          -DFETCHCONTENT_SOURCE_DIR_QTUI=${{ github.workspace }}
      - run: cmake --build build-qs --parallel
      - name: Smoke-run headless
        run: QTUI_INSPECT=1 QTUI_TEST_ARTIFACT_DIR=qs-artifacts ./build-qs/hello_qtui
```

`FETCHCONTENT_SOURCE_DIR_QTUI` is CMake's standard override for a declared
dependency. It lets CI validate the quickstart against the working tree before
the tag exists, and the file a newcomer copies is unchanged.

---

## 7. Terminal Backend Seam and Windows Terminal

### Why now

Two goals need the same refactor: Windows Terminal support, which is part of
the launch, and the WebAssembly playground (Plan 05), which is not. The I/O
layer today is a chain of `#ifdef _WIN32` blocks, about a dozen in
`primitives/TerminalDevice.cpp` alone, plus Unix-only resize handling in
`engine/TerminalEngine.cpp`. Replacing that with one interface and one class per
platform is the cheapest it will ever be while nothing is published.

### What already works on Windows

`TerminalDevice::enableRawMode()` already disables echo, line input, and
processed input on the console input handle and enables
`ENABLE_VIRTUAL_TERMINAL_PROCESSING` on output. Output rendering through ANSI
sequences therefore works in Windows Terminal today.

### What is broken on Windows, and why it is "compile-tested only"

| Problem | Location | Effect |
| :--- | :--- | :--- |
| `QSocketNotifier` on `STDIN_FILENO` | `TerminalEngine::start()` | Qt on Windows only accepts sockets; the console handle never fires, so no input ever arrives. |
| `ENABLE_VIRTUAL_TERMINAL_INPUT` not set | `TerminalDevice::enableRawMode()` | Keys arrive as console input records, not the VT sequences `InputDecoder` parses. |
| Resize is `SIGWINCH` only | `TerminalResizeBridge`, `Q_OS_UNIX` | Resizing the window on Windows does nothing. |
| `stdoutIsTty` hard-coded `true` | `TerminalEngine::start()` | Capability detection cannot tell a redirected stdout from a console. |
| Code page not set | `TerminalDevice` | Box-drawing and non-ASCII glyphs depend on the user's code page. |

### The seam

```cpp
// primitives/TerminalBackend.h
class TerminalBackend : public QObject {
    Q_OBJECT
public:
    virtual bool enterRawMode() = 0;
    virtual void leaveRawMode() = 0;
    virtual void write(std::span<const char> bytes) = 0;   // unbuffered frame bytes
    virtual bool flush(int timeoutMs) = 0;
    virtual QSize size() const = 0;
    virtual bool isInteractive() const = 0;               // replaces isatty()
signals:
    void inputReceived(const QByteArray& bytes);          // raw VT bytes
    void resized(QSize cells);
};
```

Implementations:

- **`PosixTerminalBackend`**: the existing termios, `write()`, `poll()`,
  `QSocketNotifier` on stdin, and the `SIGWINCH` self-pipe, moved verbatim.
- **`Win32ConsoleBackend`**:
  - `SetConsoleMode(input, ... | ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT)`
    so keystrokes arrive as the same VT sequences the POSIX path produces, and
    resize arrives as `WINDOW_BUFFER_SIZE_EVENT` records. `InputDecoder` needs
    no changes.
  - `SetConsoleOutputCP(CP_UTF8)` and `SetConsoleCP(CP_UTF8)` on entry,
    restored on exit.
  - `QWinEventNotifier` on the console input handle, which is a waitable
    object, calling `ReadConsoleInputW` and forwarding key records' VT bytes
    through `inputReceived` and size events through `resized`.
  - `size()` from `GetConsoleScreenBufferInfo` (already present).
  - `isInteractive()` from `GetConsoleMode` succeeding on stdout.
- **`WasmTerminalBackend`**: Plan 05, later.

`TerminalDevice` and `TerminalEngine` keep their public API and own a backend
chosen at construction. The `TestRunner` continues to drive the engine without
any backend, which is the existing headless path.

### Application-provided backends

`TerminalBackend::setFactory()` lets an application replace the platform
backend process-wide: serial consoles on devices without a tty layer,
in-process hosts over sockets or ConPTY, the WebAssembly bridge of Plan 05,
and tests. `Qtui::Testing` ships `MemoryTerminalBackend` as the reference
implementation, and `tests/Core/test_terminal_backend.cpp` drives the engine
through it end to end. C++ only, no QML surface, documented as subject to
change before 1.0. Per-Terminal injection is deferred until a multi-session
host needs it.

### Scope limits for launch

- **Windows Terminal is the target.** Legacy `conhost.exe` gets whatever works
  for free; do not spend time on it.
- **Mouse reporting** works in Windows Terminal through the same SGR sequences
  once VT input is enabled. Verify, do not assume.
- **`Qtui.TerminalHost`** (the embedded PTY module) is Unix only and stays
  `QTUI_BUILD_TERMINAL_HOST=OFF` on Windows. ConPTY support is a separate,
  later plan.
- **Sixel images** are not supported by Windows Terminal; `Qtui.Image` falls
  back to half-block rendering, which it already does when capabilities say so.

### Windows CI and examples

- The `windows-2022` row in the CI matrix above builds with MSVC and runs
  `ctest`. Scenario tests are headless and need no console.
- Add one interactive smoke job that launches the quickstart inside Windows
  Terminal through `wt.exe` with `QTUI_SCRIPT` and checks the artifact, so the
  real console path is exercised, not only the headless one.
- The README and the site quickstart list Windows alongside Linux and macOS
  with the MSVC and MinGW commands that CI runs.

## 8. Tag and Release

1. Confirm `project(Qtui VERSION 0.4.0 ...)` matches the tag.
2. `git tag -a v0.4.0 -m "Qtui 0.4.0"` and push the tag.
3. Create a GitHub release. Attach `qtui.qch` once Plan 04 exists.
4. Re-run the quickstart job without the `FETCHCONTENT_SOURCE_DIR_QTUI`
   override to prove the public path works.
5. Run the quickstart by hand once in Windows Terminal, in a Linux terminal, and
   in macOS Terminal.app before publishing the release.

---

## 9. README Rewrite

The README is the second thing a newcomer sees after the site. Replace the top
of `README.md` with, in this order: one hero cast (from Plan 02), a two-sentence
pitch, the three quickstart files, links to the site, the showcase, and the
component catalog. Everything else moves below the fold or onto the site.

---

## 10. Verification

```bash
# Both Behavior trees green
cmake -S . -B build -DQTUI_BUILD_TESTS=ON -DQTUI_BUILD_EXAMPLES=ON -DQTUI_ENABLE_BEHAVIOR=ON
cmake --build build && ctest --test-dir build --output-on-failure
cmake -S . -B build-nobehavior -DQTUI_BUILD_TESTS=ON -DQTUI_BUILD_EXAMPLES=ON
cmake --build build-nobehavior && ctest --test-dir build-nobehavior --output-on-failure

# Quickstart standalone against the working tree
cmake -S examples/quickstart -B build-qs -DFETCHCONTENT_SOURCE_DIR_QTUI=$PWD
cmake --build build-qs && QTUI_INSPECT=1 ./build-qs/hello_qtui

# Windows (from a Developer PowerShell)
cmake -S . -B build-win -DQTUI_BUILD_TESTS=ON -DQTUI_BUILD_EXAMPLES=ON
cmake --build build-win --config Release
ctest --test-dir build-win -C Release --output-on-failure
.\build-win\examples\quickstart\Release\hello_qtui.exe   # inside Windows Terminal

# Audit stale public root API; migration explanations in plans are excluded
rg -n 'rendererMode|useAlternateScreen|fillTerminal|liveItem|StreamView' src examples docs tests -g '!docs/plans/**'

# No stale names remain
grep -rn 'label:' examples docs src/Qtui/*/doc | grep -v 'StatusBadge\|Panel' || echo "clean"
grep -rln 'import QtQml 2' examples src tests docs || echo "clean"
```
