# Qt Creator Setup and Tooling

The companion `qmltui-qtc` repository integrates QmlTui with Qt Creator 20.0.2+ (and Qt 6.8+), providing project wizards, offline `F1` documentation (`qmltui.qch`), `qmlls` / `qmllint` configuration, `Runner` scenario actions, and a cell inspector.

## Installing `qmltui-qtc`

Clone and build `qmltui-qtc` alongside `qml-tui` (when placed as a sibling directory `../qml-tui`, `CMakeLists.txt` reuses the local checkout for `FETCHCONTENT_SOURCE_DIR_QMLTUI`):

```bash
cmake -S /path/to/qmltui-qtc -B /path/to/qmltui-qtc/build -GNinja
cmake --build /path/to/qmltui-qtc/build
python3 /path/to/qmltui-qtc/scripts/install.py --build-dir /path/to/qmltui-qtc/build
python3 /path/to/qmltui-qtc/scripts/install.py --verify
```

To inspect the installed files in a staging directory first:

```bash
python3 /path/to/qmltui-qtc/scripts/install.py \
  --staging-dir /tmp/qmltui-qtc-stage \
  --build-dir /path/to/qmltui-qtc/build
```

To remove all files managed by `qmltui-qtc`:

```bash
python3 /path/to/qmltui-qtc/scripts/install.py --uninstall
```

## QML Language Server (`qmlls`)

1. Open **Edit → Preferences → Qt Quick → QML/JS Editing**.
2. Enable **Use qmlls**.
3. Set `set(QT_QML_GENERATE_QMLLS_INI ON)` in `CMakeLists.txt` (included by default in the QmlTui project wizards) so CMake writes `.qmlls.ini` pointing `qmlls` at `build/qml`.

## Project and File Wizards

Under **File → New Project** and **File → New File**:

| Category | Wizard | Description |
| :--- | :--- | :--- |
| **Application (Qt)** | **QmlTui Terminal Application (Full-Screen)** | Creates a `Terminal.AltScreen` CMake project with `FetchContent`, a keyboard quit handler, and a `Runner` smoke test. |
| **Application (Qt)** | **QmlTui Inline Application (CLI & Scrollback)** | Creates a `Terminal.Inline` CMake project with `Scrollback`, a prompt input, and a scrollback `Runner` test. |
| **Qt** | **QmlTui Component (`.qml`)** | Creates a QmlTui component (`Panel`, `Rectangle`, `Item`, `Column`, or `Row`) with `pragma ComponentBehavior: Bound`. |
| **Qt** | **QmlTui Runner Scenario (`.mjs`)** | Creates an ES module scenario (`export function main(terminal)`) with `@param {Terminal} terminal` typing. |

When running a QmlTui executable interactively from Qt Creator, open **Projects → Run** for the active kit and check **Run in terminal** so `Terminal` attaches to a TTY.

## Offline Documentation (`qmltui.qch`)

Building `qmltui-qtc` produces `build/qch/qmltui.qch` from this documentation tree. To register it manually:

1. Open **Edit → Preferences → Help → Documentation**.
2. Click **Add...** and select `~/.local/share/data/QtProject/qtcreator/doc/qmltui.qch` (or `build/qch/qmltui.qch`).
3. Click **Apply**.

Pressing **`F1`** with the cursor on a QmlTui QML type, property, or C++ class opens its reference page in the Help sidebar.

## Tools → QmlTui Menu Actions

The `qmltui` Lua extension adds the following actions under **Tools → QmlTui**:

- **Lint Active QML File (`Ctrl+Shift+L`)**: Runs `qmllint --json - -I <import_dir>` on the current file and reports diagnostics in the **Issues** pane.
- **Preview & Cell Inspector (`Ctrl+Shift+P`)**: Launches the QmlTui Cell Inspector using `qml6 -a core`, displaying cell coordinates, grapheme widths, foreground/background colors, and the item stack covering the selected `(X, Y)` cell.
- **Preview Active QML in Live Terminal**: Launches `qml6 -a core` with `PreviewHost.qml` inside Qt Creator's terminal for interactive testing of a `Terminal` root or standalone component.
- **Run Registered Scenarios (CTest)**: Runs registered `Runner` scenarios in the active CMake build directory via `ctest`.
- **Run Scenario with Configured Application**: Runs the active `.mjs`/`.js` scenario against the configured application executable (`Edit → Preferences → QmlTui`) and reports failures in the **Issues** pane.
- **Run Scenario Live in Terminal**: Runs the active scenario with `QMLTUI_RUNNER_LIVE=1` in a terminal window.
- **Update Scenario Snapshots**: Runs with `QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1` and logs rewritten snapshot paths from `result.json`.
- **Cancel Active QmlTui Task**: Stops any running `Runner`, `qmllint`, or preview process.

## Scenario Autocomplete (`qmltui-runner.d.ts`)

Add `qmltui_scenario_editor_support()` to `CMakeLists.txt` to copy `.qmltui/qmltui-runner.d.ts` and generate `jsconfig.json`:

```cmake
if(COMMAND qmltui_scenario_editor_support)
    qmltui_scenario_editor_support()
endif()
```

Annotate the scenario entry point with JSDoc so Qt Creator provides completion and hover help on `terminal`:

```javascript
/** @param {Terminal} terminal */
export function main(terminal) {
    terminal.waitForText("Ready");
    terminal.expectSnapshot("snapshots/initial.txt");
}
```
