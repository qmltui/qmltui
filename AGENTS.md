# Qtui Repository Guidance

## Architecture Boundaries

- Qtui is a C++20, Qt 6.8+ retained-mode terminal UI framework. Its runtime
  targets are Linux, macOS, and Windows Terminal. Platform code lives only in
  `src/Qtui/primitives/{Posix,Win32Console}*Backend.*` behind `TerminalBackend`;
  do not add `#ifdef _WIN32` elsewhere or include `<windows.h>` from a header.
  Legacy `conhost.exe` gets whatever its VT support allows and is not a target.
- Build terminal interfaces from `QtQml` and Qtui modules. Do not add a
  `QtQuick` import or Qt Quick dependency to Qtui applications or modules.
- Keep optional subsystems gated. `QTUI_ENABLE_BEHAVIOR=ON` requires
  `Qt6::QmlPrivate`; `QTUI_BUILD_TERMINAL_HOST=ON` is the Unix PTY module and is
  supported only on Linux and macOS.

## Build and Test Decisions

- Configure the normal development tree with tests and examples enabled:

  ```bash
  rtk cmake -S . -B build \
    -DQTUI_BUILD_TESTS=ON \
    -DQTUI_BUILD_EXAMPLES=ON
  ```

- Build the affected target and run the narrowest relevant CTest first. Run the
  broader configured suite after changes to Core, public APIs, QML modules, or
  CMake integration.
- Use Qt Test executables for C++ units and component contracts. Use
  `Qtui::Testing`/`TestRunner` for whole-application input and committed terminal
  frames. A visible-behavior test must assert the rendered frame, not only a QML
  property.
- For Behavior or feature-gating changes, validate separate Behavior-enabled
  and Behavior-disabled build trees. Do not expect the Industrial PLC target in
  a disabled tree.
- For detailed TestRunner scenarios, artifacts, inspection, and live rendering,
  follow `.agents/skills/qtui-dev-loop/SKILL.md` and the authoritative
  `src/Qtui/Testing/doc/TestRunner.md`.

## QML Application Behavior

- For imports, pragmas, typing, renderer mode, geometry, and the type
  reference, follow `.agents/skills/qtui-qml/SKILL.md`.
- A root `Terminal` does not automatically map Q or Escape to application exit.
  Applications and examples must provide a reachable keyboard quit path.
- A simple application may handle Q and Escape at the root and mark the event
  accepted. Preserve contextual Escape behavior: close a modal, cancel an
  operation, or clear input first, and quit only when nothing consumes Escape.
- Keep production startup independent from test scenarios unless an
  environment-driven Testing hook is an intentional feature. Prefer a separate
  Testing-linked executable for application scenarios.

## Handoff

- Run `rtk git diff --check` before handoff.
- Report focused and regression tests run, relevant build configurations, and
  committed-frame artifacts inspected. Explicitly identify configurations that
  were not exercised.
