# Adding Features to QmlTui

This guide provides step-by-step checklists for adding QML types, `QmlTui.Controls` components, and copyable recipes to QmlTui, plus local commands for AddressSanitizer/UBSan and Clang source-based code coverage.

Before writing code, review the architecture boundaries and style rules in [`AGENTS.md`](../../AGENTS.md) and [`CONTRIBUTING.md`](../../CONTRIBUTING.md).

## Checklist: Adding a Core C++ Item or QML Type (`QmlTui::Core`)

1. **Architecture checks**:
   - Never import `QtQuick` or link `Qt6::Quick` / `Qt6::Gui` into `QmlTui::Core`.
   - Keep platform-specific code strictly inside `src/QmlTui/Core/Primitives/{PosixTerminal,Win32Console,WasmTerminal}Backend.*` behind `TerminalBackend` (`MemoryTerminalBackend.*` lives in `src/QmlTui/Runner/` for headless testing; never add `#ifdef _WIN32` elsewhere or include `<windows.h>` from headers).
   - Do not mark `QML_ELEMENT` classes `final` (Qt's QML type registration subclasses them internally).
   - Keep public headers free of private/internal includes (`cmake/QmlTuiPublicHeaders.cmake` enforces this at CMake configure time) and keep `src/QmlTui/Core/Engine/*` isolated from concrete `Items/` and `<private/...>` headers (`cmake/CheckEngineIncludes.cmake` enforces this at configure time).
   - Use plain `//` prose comments (no Doxygen tags) and follow `.clang-format`.
2. **CMake registration (`src/QmlTui/Core/CMakeLists.txt`)**:
   - Add the new `.h` and `.cpp` files to `qt_add_qml_module(QmlTuiCore ... SOURCES ...)`.
   - If the header is part of the public C++ API, add it to `qmltui_install_headers(...)` in `src/QmlTui/Core/CMakeLists.txt`.
   - If the feature uses `Qt6::QmlPrivate`, gate it under `if(QMLTUI_ENABLE_PRIVATE_QT)` and validate both `QMLTUI_ENABLE_PRIVATE_QT=ON` and `OFF` trees.
3. **Documentation and agent skills**:
   - Add `docs/qml-types/<typename>.md` (with `cpp:` frontmatter if backed by a C++ class), add it to `docs/qml-types/index.md`, and register its link in `docs/nav.json`.
   - If the class is part of the public C++ API, document it under `docs/cpp-classes/` and register it in `docs/cpp-classes/index.md` and `docs/nav.json`.
   - Update the type table in [`.agents/skills/qmltui-qml/SKILL.md`](../../.agents/skills/qmltui-qml/SKILL.md) so `check-skills.py` stays green.
4. **Tests and gallery demo**:
   - Add C++ unit tests under `tests/Core/` (registered in `tests/Core/CMakeLists.txt`).
   - Add an `Item`-rooted demo under `examples/gallery/examples/<Name>.qml`, register a tile entry in `allEntries` inside `examples/gallery/Main.qml`, add a row to [`docs/examples.md`](../examples.md) (enforced by `check_docs.py`), and add a `Runner` scenario asserting the rendered terminal frame (not just QML properties).

## Checklist: Adding a `QmlTui.Controls` Type

`QmlTui.Controls` only admits a type if it needs engine knowledge an application cannot reproduce in QML (cell geometry, Unicode width, virtualization, overlay z-stacking) or is universal across terminal toolkits. Domain-specific widgets belong under `examples/recipes/`.

1. **QML conventions**:
   - Start with `pragma ComponentBehavior: Bound`.
   - Default to terminal foreground and background (`"default"`); expose ordinary color properties whose explicit overrides survive focus and disabled states.
   - Follow `.qmlformat.ini`, Qt property ordering, arrow-function handlers (`event => { ... }`), and grouped property notation (`border { ... }`, `anchors { ... }`).
2. **Dual CMake registration (`src/QmlTui/Controls/CMakeLists.txt`)**:
   - List every new `.qml` file in **both** `qt_add_qml_module(QmlTuiControls ... QML_FILES ...)` **and** `install(FILES ...)` so installed `find_package(QmlTui)` trees ship the QML file.
   - If the control includes C++ headers/sources, add them to `SOURCES` and `qmltui_install_headers(QmlTui/Controls FILES ...)`.
3. **Documentation, skills, gallery, and tests**:
   - Add `docs/qml-types/<control>.md`, update `docs/qml-types/index.md`, `docs/nav.json`, and [`.agents/skills/qmltui-qml/SKILL.md`](../../.agents/skills/qmltui-qml/SKILL.md).
   - Add `examples/gallery/examples/<Control>.qml`, add an entry to `allEntries` in `examples/gallery/Main.qml`, list it in [`docs/examples.md`](../examples.md), and register unit / `Runner` tests in `tests/Controls/CMakeLists.txt`.

## Checklist: Adding a Standalone Recipe (`examples/recipes/`)

Recipes under `examples/recipes/` are automatically licensed `BSD-3-Clause` via [`REUSE.toml`](../../REUSE.toml) (do not add license headers).

1. Create `examples/recipes/<name>/` with `CMakeLists.txt`, `main.cpp`, `Main.qml` (with a reachable keyboard quit path), and a short `README.md`.
2. Register `add_subdirectory(<name>)` in `examples/recipes/CMakeLists.txt` (gate optional module recipes behind `if(QMLTUI_ENABLE_IMAGE)` or `if(QMLTUI_ENABLE_PRIVATE_QT)`).
3. Link `QmlTui::Runner` conditionally under `if(BUILD_TESTING AND TARGET QmlTui::Runner)` and register a `Runner` smoke test via `qmltui_add_scenario(...)` (prefer `CLOCK VIRTUAL`).
4. Guard WebAssembly packaging with `if(COMMAND qmltui_wasm_app)` (`cmake/QmlTuiWasm.cmake` is included only under `EMSCRIPTEN`) if the recipe has no desktop-only dependencies (`Qt6::Widgets`):
   ```cmake
   if(COMMAND qmltui_wasm_app)
       qmltui_wasm_app(qmltui_<name>_demo)
   endif()
   ```
5. Add a row to [`docs/examples.md`](../examples.md).

## Updating the Changelog

For user-visible features, behavioral changes, or breaking API updates, add a concise bullet under [`docs/changes/changelog.md`](../changes/changelog.md) before opening your pull request.

## Local Sanitizers (ASan + UBSan)

Run the same AddressSanitizer + UndefinedBehaviorSanitizer build used by the `sanitizers` CI job:

```bash
cmake -S . -B build-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DQMLTUI_BUILD_TESTS=ON \
  -DQMLTUI_ENABLE_RUNNER=ON \
  -DQMLTUI_ENABLE_PRIVATE_QT=ON \
  -DQMLTUI_ENABLE_IMAGE=ON
cmake --build build-asan --parallel
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0 \
  UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir build-asan -LE Demo --output-on-failure
```

## Local Code Coverage (`clang++` + `llvm-cov`)

Generate an HTML and summary coverage report locally with Clang's source-based coverage (`-DQMLTUI_ENABLE_COVERAGE=ON`):

```bash
cmake -S . -B build-coverage -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug \
  -DQMLTUI_BUILD_TESTS=ON \
  -DQMLTUI_BUILD_EXAMPLES=ON \
  -DQMLTUI_ENABLE_RUNNER=ON \
  -DQMLTUI_ENABLE_PRIVATE_QT=ON \
  -DQMLTUI_ENABLE_IMAGE=ON \
  -DQMLTUI_ENABLE_COVERAGE=ON
cmake --build build-coverage --parallel
QT_QPA_PLATFORM=offscreen cmake --build build-coverage --target coverage
```

Reports are written to `build-coverage/coverage/` (`html/index.html`, `summary.md`, `summary.txt`, `summary.json`, and `lcov.info`).
