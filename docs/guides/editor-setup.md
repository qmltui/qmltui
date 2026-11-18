# Editor and IDE Setup

QmlTui uses standard Qt 6 QML tooling (`qmlls`, `qmllint`, `qmlformat`) and TypeScript declaration files (`qmltui-runner.d.ts`) so autocomplete, hover documentation, diagnostics, and formatting work out of the box across VS Code, Neovim, Zed, Helix, CLion, and Qt Creator.

## QML Autocomplete, Linting, and Formatting

### 1. Enable `.qmlls.ini` Generation in CMake

Add `set(QT_QML_GENERATE_QMLLS_INI ON)` before `qt_add_executable()` in `CMakeLists.txt`:

```cmake
set(QT_QML_GENERATE_QMLLS_INI ON)

qt_add_executable(my_app main.cpp)
qt_add_qml_module(my_app
    URI MyApp
    QML_FILES Main.qml
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)
```

Configuring and building your project writes `.qmlls.ini` in the source directory, pointing Qt's QML Language Server (`qmlls`) at `build/qml` (for `FetchContent` / `add_subdirectory` builds) or `QmlTui_QML_IMPORT_PATH` (when consuming an installed package via `find_package(QmlTui)`).

### 2. Configure Your Editor or IDE

- **VS Code**: Install the official **Qt QML** extension (`theqtcompany.qt-qml`) and point it at your Qt 6.8+ installation (or `<Qt>/bin/qmlls`). Once `cmake --build build` has generated `.qmlls.ini` and QmlTui's `.qmltypes` metadata, completion and diagnostics resolve `QmlTui` and `QmlTui.Controls` types automatically. See [`examples/.vscode/settings.json`](../../examples/.vscode/settings.json) for a reference workspace configuration.
- **Neovim / Zed / Helix**: Configure your LSP client to launch `<Qt>/bin/qmlls -E` from your Qt 6.8+ `bin/` directory (`-E` tells `qmlls` to read `.qmlls.ini` and build-directory import paths).
- **CLion**: Configure `<Qt>/bin/qmlls` under QML Language Server settings after running an initial CMake build so `.qmlls.ini` and `build/qml` exist.
- **Qt Creator**:
  1. Open **Edit -> Preferences -> Qt Quick -> QML/JS Editing** and enable **Use qmlls**.
  2. To run a QmlTui executable interactively from Qt Creator, open **Projects -> Run** for the active kit and check **Run in terminal** so `Terminal` attaches to a real TTY rather than Qt Creator's non-interactive Application Output pane.

### 3. Command-Line Linting and Formatting (`qmllint` & `qmlformat`)

Every `qt_add_qml_module(<target> ...)` automatically creates a `<target>_qmllint` CMake build target:

```sh
# Lint all QML files registered in the module:
cmake --build build --target my_app_qmllint

# Or lint an individual QML file directly against build/qml:
/path/to/Qt/6.8.x/gcc_64/bin/qmllint -I build/qml Main.qml

# Format QML files in place (4 spaces, no tabs):
/path/to/Qt/6.8.x/gcc_64/bin/qmlformat -i Main.qml
```

## `Runner` Scenario Autocomplete (`qmltui-runner.d.ts`)

When writing JavaScript or ES module scenario tests for [`qmltui::Runner`](testing.md), call `qmltui_scenario_editor_support()` inside your `if(BUILD_TESTING AND TARGET qmltui::runner)` block:

```cmake
if(BUILD_TESTING AND TARGET qmltui::runner)
    target_link_libraries(my_app PRIVATE qmltui::runner)
    qmltui_scenario_editor_support()
endif()
```

At configure time, CMake writes `.qmltui/qmltui-runner.d.ts` in your project directory and generates a `jsconfig.json` referencing it if neither `jsconfig.json` nor `tsconfig.json` already exists (if your project already maintains a custom `tsconfig.json`, add `".qmltui/qmltui-runner.d.ts"` to its `"include"` array).

Annotate scenario entry points with JSDoc so VS Code, Neovim (`ts_ls` / `vtsls`), Zed, WebStorm/CLion, and Qt Creator provide method completion and parameter types on `app`:

```javascript
/** @param {Runner} app */
export function main(app) {
    app.waitForText("Ready");
    app.press("Enter");
    app.expectSnapshot("snapshots/after_enter.txt");
}
```
