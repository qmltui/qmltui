# Qamui Repository Guidance

## Architecture Boundaries

- Qamui is a C++20, Qt 6.8+ retained-mode terminal UI framework. Its runtime
  targets are Linux, macOS, and Windows Terminal. Platform code lives only in
  `src/Qamui/Core/primitives/{Posix,Win32Console}*Backend.*` behind `TerminalBackend`;
  do not add `#ifdef _WIN32` elsewhere or include `<windows.h>` from a header.
  Legacy `conhost.exe` gets whatever its VT support allows and is not a target.
- `Qamui.Controls` admits a type only if it needs engine knowledge an
  application cannot reproduce in QML (cell geometry, Unicode width,
  virtualization, overlays) or is universal in terminal toolkits. Everything
  else is a recipe under `examples/components/`. Core items and Controls default to the terminal's foreground and background.
  Expose ordinary color properties where useful; explicit overrides must survive
  focus and disabled states. Applications own shared styling through QML bindings
  and reusable components; do not add a framework palette or theme singleton.
- Build terminal interfaces from `QtQml` and Qamui modules. Do not add a
  `QtQuick` import or Qt Quick dependency to Qamui applications or modules.
- Keep optional subsystems gated. `QAMUI_ENABLE_BEHAVIOR=ON` requires
  `Qt6::QmlPrivate`; `QAMUI_ENABLE_TERMINAL_HOST=ON` is the Unix PTY module and is
  supported only on Linux and macOS; `QAMUI_ENABLE_IMAGE=ON` requires `Qt6::Gui`
  and `Qt6::Network`.

## Build and Test Decisions

- Configure the normal development tree with tests and examples enabled:

  ```bash
  rtk cmake -S . -B build \
    -DQAMUI_BUILD_TESTS=ON \
    -DQAMUI_BUILD_EXAMPLES=ON
  ```

- Build the affected target and run the narrowest relevant CTest first. Run the
  broader configured suite after changes to Core, public APIs, QML modules, or
  CMake integration.
- Use Qt Test executables for C++ units and component contracts. Use
  `Qamui::Runner`/`Runner` for whole-application input and committed terminal
  frames. A visible-behavior test must assert the rendered frame, not only a QML
  property.
- For Behavior or feature-gating changes, validate separate Behavior-enabled
  and Behavior-disabled build trees. Do not expect the Industrial PLC target in
  a disabled tree.
- For detailed Runner scenarios, artifacts, inspection, and live rendering,
  follow `.agents/skills/qamui-dev-loop/SKILL.md` and the authoritative
  `docs/cpp-classes/runner.md`.

## QML Application Behavior

- For imports, pragmas, typing, renderer mode, geometry, and the type
  reference, follow `.agents/skills/qamui-qml/SKILL.md`.
- A root `Terminal` does not automatically map Q or Escape to application exit.
  Applications and examples must provide a reachable keyboard quit path.
- A simple application may handle Q and Escape at the root and mark the event
  accepted. Preserve contextual Escape behavior: close a modal, cancel an
  operation, or clear input first, and quit only when nothing consumes Escape.
- Examples and applications conditionally link `Qamui::Runner` into their normal
  executable with `if(TARGET Qamui::Runner)` and register scenarios against that
  target. Do not duplicate executable or QML-module targets for Runner. Normal
  startup must remain independent from scenarios unless requested through the
  Runner environment variables. Build without tests and Runner for production.

## Handoff

- Run `rtk git diff --check` before handoff.
- Report focused and regression tests run, relevant build configurations, and
  committed-frame artifacts inspected. Explicitly identify configurations that
  were not exercised.
