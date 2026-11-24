# PySide6 — Python Models and Controllers with QmlTui

Demonstrates hosting a QmlTui application from Python using `PySide6` (`QCoreApplication` + `QQmlApplicationEngine`) without custom C++ wrapper code or Shiboken bindings.

- **Python `QAbstractTableModel` + `QSortFilterProxyModel`**: [`main.py`](main.py) defines `ServiceTableModel` and exposes `ServiceController` to QML via `@QmlElement` (`import PySideDemo`).
- **Standard QmlTui QML**: [`Main.qml`](Main.qml) imports `QmlTui`, `QmlTui.Controls`, and `PySideDemo`, rendering a sortable (`S` or header click) and filterable `TableView` inside a `Panel`.
- **Contextual quit path**: `Escape` clears `filterInput.text` first, returns focus to `serviceTable` when `filterInput` is empty and focused, and exits otherwise; `Q` exits when `filterInput` does not hold active focus.
- **Shared library search paths**: Standard ELF/macOS CMake builds embed RPATHs on QML plugins so `libQmlTuiCore` and `libQmlTuiControls` resolve automatically. On Windows, [`main.py`](main.py) registers existing `build/bin`, `build/src/QmlTui/Core`, and `build/src/QmlTui/Controls` directories via `os.add_dll_directory(...)` (multi-config or custom prefixes should also add the DLL directory to `PATH`). On non-RPATH Unix builds (`-DCMAKE_SKIP_RPATH=ON` or relocated prefixes), add the library directory to `LD_LIBRARY_PATH` (Linux) or `DYLD_LIBRARY_PATH` (macOS).

```bash
# Build QmlTui QML plugins with public Qt APIs only (avoids private-API version
# mismatches when PySide6 wheels bundle their own Qt libraries)
cmake -S . -B build -G Ninja \
  -DQMLTUI_ENABLE_PRIVATE_QT=OFF \
  -DQMLTUI_BUILD_TESTS=OFF \
  -DQMLTUI_BUILD_BENCHMARKS=OFF
cmake --build build --target QmlTuiCoreplugin QmlTuiControlsplugin

# Install PySide6 matching your Qt 6 minor version (e.g. 6.8.*), then run:
pip install "PySide6==6.<minor>.*"
python3 examples/recipes/pyside6/main.py
# Or specify a custom build tree:
QMLTUI_BUILD_DIR=/path/to/build python3 examples/recipes/pyside6/main.py
```

For standalone installed-prefix setups and testing guidance, see the [Python and PySide6 Guide](../../../docs/guides/python-pyside6.md).
