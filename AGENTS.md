# QmlTui Repository Guidance

## Architecture Boundaries

- QmlTui is a Qt 6.8+ retained-mode terminal UI framework. Its runtime
  targets are Linux, macOS, and Windows Terminal (plus WebAssembly for the browser playground). Platform code lives only in
  `src/QmlTui/Core/Primitives/{Posix,Win32Console,Wasm,Memory}*Backend.*` behind `TerminalBackend`;
  do not add `#ifdef _WIN32` elsewhere or include `<windows.h>` from a header.
  Legacy `conhost.exe` gets whatever its VT support allows and is not a target.
- `QmlTui.Controls` admits a type only if it needs engine knowledge an
  application cannot reproduce in QML (cell geometry, Unicode width,
  virtualization, overlays) or is universal in terminal toolkits. Everything
  else is a recipe under `examples/recipes/`. Core items and Controls default to the terminal's foreground and background.
  Expose ordinary color properties where useful; explicit overrides must survive
  focus and disabled states. Applications own shared styling through QML bindings
  and reusable components; do not add a framework palette or theme singleton.
- Build terminal interfaces from `QtQml` and QmlTui modules. Do not add a
  `QtQuick` import or Qt Quick dependency to QmlTui applications or modules.
- Keep optional subsystems gated. `QMLTUI_ENABLE_PRIVATE_QT=ON` (default) uses
  `Qt6::QmlPrivate` when available (enabling Behavior and advanced
  states) and warns when falling back if not explicitly disabled;
  `QMLTUI_ENABLE_PTYVIEW=ON` is the Unix PTY module and is
  supported only on Linux and macOS; `QMLTUI_ENABLE_IMAGE=ON` requires `Qt6::Gui`
  and `Qt6::Network`.

## Build and Test Decisions

- Configure the normal development tree with tests and examples enabled:

  ```bash
  cmake -S . -B build \
    -DQMLTUI_BUILD_TESTS=ON \
    -DQMLTUI_BUILD_EXAMPLES=ON
  ```

- Build the affected target and run the narrowest relevant CTest first. Run the
  broader configured suite after changes to Core, public APIs, QML modules, or
  CMake integration.
- Use Qt Test executables for C++ units and component contracts. Use
  `qmltui::Runner`/`Runner` for whole-application input and committed terminal
  frames. A visible-behavior test must assert the rendered frame, not only a QML
  property.
- For Private Qt or feature-gating changes, validate separate Private-Qt-enabled
  and Private-Qt-disabled build trees. Do not expect the `qmltui_independent_states_demo` target in
  a disabled tree.
- For detailed Runner scenarios, artifacts, inspection, and live rendering,
  follow `.agents/skills/qmltui-dev-loop/SKILL.md` and the authoritative
  `docs/cpp-classes/runner.md`.

## QML Application Behavior

- For imports, pragmas, typing, renderer mode, geometry, and the type
  reference, follow `.agents/skills/qmltui-qml/SKILL.md`.
- A root `Terminal` does not automatically map Q or Escape to application exit.
  Applications and examples must provide a reachable keyboard quit path.
- A simple application may handle Q and Escape at the root and mark the event
  accepted. Preserve contextual Escape behavior: close a modal, cancel an
  operation, or clear input first, and quit only when nothing consumes Escape.
- Examples and applications conditionally link `QmlTui::Runner` into their normal
  executable with `if(BUILD_TESTING AND TARGET QmlTui::Runner)` and register
  scenarios against that target. Do not duplicate executable or QML-module
  targets for Runner. Normal startup must remain independent from scenarios
  unless requested through the Runner environment variables. Build with
  `-DBUILD_TESTING=OFF` (or without tests and Runner) for production.

## Code Formatting and Style Conventions

- Format C++ files with `clang-format` according to `.clang-format` (4 spaces,
  attached braces, pointer/reference left-aligned, column limit 100).
- Format QML files with `qmlformat` according to `.qmlformat.ini` (4 spaces,
  no tabs, semicolon required on JS statements).
- Run `./scripts/format.py` to automatically format staged files or `./scripts/format.py <path>`
  for specific files. Use `./scripts/format.py --check` to verify code style without modifications.
- In QML, follow the Qt property declaration order:
  `id` -> custom properties -> signals -> JavaScript functions -> object properties / geometry -> visual/text -> signal handlers -> child objects -> states/transitions.
- In QML signal handlers with parameters, always use arrow functions: `Keys.onPressed: event => { ... }`.
- In QML, prefer grouped property notation (e.g. `anchors { left: ...; top: ... }`, `border { color: ...; style: ... }`)
  over repetitive dot notation when specifying multiple grouped properties.
- In C++ headers and sources, write comments as plain `//` prose (no `/** @brief */`,
  `///`, `@param`, `@thread_safety`, or `\a` doc tags). Give every class or struct a
  concise purpose and contract comment, comment only non-obvious `Q_PROPERTY`
  declarations and methods (skipping self-explanatory getters/setters), and start
  `*_p.h` or internal headers with a one-line `// Internal: ...` note instead of
  ASCII warning banners.

## Handoff

- Run `git diff --check` before handoff.
- Report focused and regression tests run, relevant build configurations, and
  committed-frame artifacts inspected. Explicitly identify configurations that
  were not exercised.
