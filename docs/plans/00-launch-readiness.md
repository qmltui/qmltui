# Plan 00: Launch Readiness (API Polish, Compiled Quickstart, CI, v0.4.0 Tag)

## 1. Goal

Nothing has been published yet. That means every naming inconsistency, every
unverified snippet, and every missing tag can still be fixed for free. This plan
fixes them before the first public impression, so a newcomer's first ten minutes
never hit a broken step.

Exit criteria:

- Every public QML control uses Qt Quick naming where an equivalent exists.
- `Terminal` is the only root type. Its mode is derived from whether it
  contains a `Scrollback`, and a nested `Terminal` is the full-screen takeover.
  `rendererMode`, `useAlternateScreen`, `liveItem`, `StreamView`, and
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

## 3. Root Type Redesign: One `Terminal`, Mode by Content

### The problem today

`Terminal` inherits `Rectangle` and carries the union of two modes. `rendererMode`,
`useAlternateScreen`, `liveItem`, `print()`, and `append()` only mean something
in Stream mode; `border` means nothing on a root; `height()` branches on the
mode internally. `StreamView` is a `QObject` that is instantiable anywhere but
only works inside a Stream-mode root, and the QML skill documents it with
`anchors.fill`, which it does not have. `Terminal.print()` and
`StreamView.print()` do the same job. `examples/concepts/agent_stream` switches
modes by binding three root properties, two visibilities, and focus to one
boolean; forgetting any one of the five produces a half-broken screen.

### The design

Two types. No mode flag. No new vocabulary.

| Type | Role |
| :--- | :--- |
| `Terminal` | The only root. Inherits `Item`. Full-screen unless it contains a `Scrollback`. Nested inside another `Terminal`, it is a full-screen takeover with `open()` and `close()`. |
| `Scrollback` | Declares that the enclosing `Terminal` runs inline. Owns the stream: model, delegate, `append()`, `print()`. At most one per `Terminal`. |

**Rule in one sentence:** a `Terminal` with a `Scrollback` is inline, and every
other child is the live region; a `Terminal` without one owns the screen.

### `Terminal` properties

| Property or member | Notes |
| :--- | :--- |
| `title` | Window title, unchanged |
| `color` | Background, mirrors Qt Quick `Window.color`. `border` is removed. |
| `fillTerminal` | Default `true`. Setting `width` and `height` opts into a fixed size. Ignored when inline. |
| `terminalWidth`, `terminalHeight` | Read-only, unchanged |
| `mouseReporting`, `clipboardMode`, `copyToClipboard()` | Unchanged |
| `hoveredItem`, `activeFocusItem` | Read-only |
| `inline` | Read-only `bool`. True when a `Scrollback` is registered. |
| `open()`, `close()`, `visible`, `opened()`, `closed()` | Only on a nested `Terminal`. A root ignores them with a warning. |
| Removed | `rendererMode`, `useAlternateScreen`, `liveItem`, `print()`, `append()`, `border` |

### `Scrollback` properties

| Property or member | Notes |
| :--- | :--- |
| `model`, `delegate`, `spacing`, `separator`, `reuseItems` | Moved from `StreamView` |
| `append(data)`, `append(item)`, `print(text)`, `clear()` | Moved from `StreamView` and `Terminal` |
| Not an `Item` | It has no geometry. The live region is the sibling items. |

### How the mode is resolved (no tree scanning)

`Scrollback` and nested `Terminal` are `QQmlParserStatus` types. In
`componentComplete()` each walks its parent chain to the nearest `Terminal`
and registers, exactly as `Popup` does today with `registerPopup()`. QML
completes children before parents, so when the root `Terminal` completes it
already knows whether a `Scrollback` registered and installs `StreamRenderer` or
`FrameRenderer` once. A `Scrollback` created later through a `Loader` registers
on completion and the engine swaps renderers through the existing
`setRenderer()`; destroying it swaps back. A second `Scrollback` under one
`Terminal` warns and is ignored. Only the outermost `Terminal` owns the engine
and the device.

### Takeover semantics

A nested `Terminal` is excluded from the live region and keeps its state while
closed, so a shell session survives being hidden. `open()` swaps to the frame
renderer, enters the alternate screen, sizes the item to the terminal, hides the
live region, and moves focus into it. `close()` reverses all five steps and
restores focus to the previous item. This replaces the manual choreography in
`agent_stream`.

### Before and after

```qml
// Dashboard: nothing to declare
Terminal {
    title: "PLC Cockpit"
    color: "#0d1117"
    Row { anchors.fill: parent }
}
```

```qml
// Agent or REPL
Terminal {
    id: root
    title: "Qt AI Studio"

    Scrollback {
        id: history
        model: chatHistory
        delegate: MessageCard {}
    }

    PromptBar {                                     // live region
        onSubmitted: (text) => history.append({ type: "user", text })
        onShellRequested: shell.open()
    }

    Terminal {                                      // takeover
        id: shell
        ShellPage { onExitRequested: shell.close() }
    }
}
```

### Migration inside the tree

| Today | After |
| :--- | :--- |
| `Terminal { rendererMode: Terminal.Frame }` | `Terminal {}` |
| `rendererMode: Terminal.Stream; useAlternateScreen: false; liveItem: x` | `Scrollback {}` plus `X {}` as a sibling |
| `StreamView { model; delegate }` | `Scrollback { model; delegate }` |
| `Terminal.print()`, `StreamView.print()` | `Scrollback.print()` |
| `rendererMode: cond ? Frame : Stream` and visibility and focus juggling | nested `Terminal` with `open()` and `close()` |
| `Terminal.border` | Removed |
| `fillTerminal: true` on every full-screen root | Default; delete the line |

Files to update: `src/Qtui/items/Terminal.{h,cpp}`, `StreamView.{h,cpp}` (renamed to
`Scrollback`), `engine/TerminalEngine.cpp` (takeover entry and exit),
`examples/concepts/agent_stream/Main.qml`, every example root that sets
`fillTerminal: true` or a border, `docs/qml_api.md`, `docs/architecture.md`,
`.agents/skills/qtui-qml/SKILL.md` section 3, and the `TestRunner` hook if it
reads `rendererMode`.

### Acceptance

- `grep -rn 'rendererMode\|useAlternateScreen\|liveItem\|StreamView' src examples docs tests` returns nothing.
- `agent_stream` opens and closes the shell takeover through `open()` and
  `close()` and its TestRunner scenario passes in both Behavior trees.
- A new concept example `examples/concepts/inline_repl/` shows the minimal
  inline case: one `Scrollback`, one `TextInput` as the live region, under 40
  lines, with a scenario test.
- Both Behavior trees green; `test_runner_pos` and `test_runner_plc` unchanged.

## 4. The Quickstart as a Compiled Project

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

`Button.text` assumes Section 2 is done. `fillTerminal` is omitted because
Section 3 makes it the default. `color` on `Terminal` is the root background,
as on Qt Quick `Window`.

### In-tree build

`examples/CMakeLists.txt` adds `quickstart` like any other example so it is
covered by the normal build. The `FetchContent` block is skipped when
`CMAKE_SOURCE_DIR` is not the quickstart itself, following the pattern already
used by `examples/apps/starter_template/CMakeLists.txt`.

---

## 5. GitHub Actions CI (`.github/workflows/ci.yml`)

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

## 6. Terminal Backend Seam and Windows Terminal

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

## 7. Tag and Release

1. Confirm `project(Qtui VERSION 0.4.0 ...)` matches the tag.
2. `git tag -a v0.4.0 -m "Qtui 0.4.0"` and push the tag.
3. Create a GitHub release. Attach `qtui.qch` once Plan 04 exists.
4. Re-run the quickstart job without the `FETCHCONTENT_SOURCE_DIR_QTUI`
   override to prove the public path works.
5. Run the quickstart by hand once in Windows Terminal, in a Linux terminal, and
   in macOS Terminal.app before publishing the release.

---

## 8. README Rewrite

The README is the second thing a newcomer sees after the site. Replace the top
of `README.md` with, in this order: one hero cast (from Plan 02), a two-sentence
pitch, the three quickstart files, links to the site, the showcase, and the
component catalog. Everything else moves below the fold or onto the site.

---

## 9. Verification

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

# No stale root API remains
grep -rn 'rendererMode\|useAlternateScreen\|liveItem\|StreamView' src examples docs tests || echo "clean"

# No stale names remain
grep -rn 'label:' examples docs src/Qtui/*/doc | grep -v 'StatusBadge\|Panel' || echo "clean"
grep -rln 'import QtQml 2' examples src tests docs || echo "clean"
```
