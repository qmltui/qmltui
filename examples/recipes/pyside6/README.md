# PySide6 — Python Models and Controllers with QmlTui

Demonstrates hosting a QmlTui application from Python using `PySide6` (`QCoreApplication` + `QQmlApplicationEngine`) without custom C++ wrapper code or Shiboken bindings.

- **Python `QAbstractTableModel` + `QSortFilterProxyModel`**: [`main.py`](main.py) defines `ServiceTableModel` and exposes `ServiceController` to QML via `@QmlElement` (`import PySideDemo`).
- **Standard QmlTui QML**: [`Main.qml`](Main.qml) imports `QmlTui`, `QmlTui.Controls`, and `PySideDemo`, rendering a sortable and filterable `TableView` inside a `Panel`.
- **Contextual quit path**: `Escape` clears `filterInput.text` first and exits when the filter is empty; `Q` exits when `filterInput` does not hold active focus.

```bash
# Build QmlTui QML plugins with public Qt APIs only (avoids private-API version
# mismatches when PySide6 wheels bundle their own Qt libraries)
cmake -S . -B build -DQMLTUI_ENABLE_PRIVATE_QT=OFF
cmake --build build --target QmlTuiCoreplugin QmlTuiControlsplugin

# Run with a Python environment containing PySide6 (matching your Qt 6 minor version)
python3 examples/recipes/pyside6/main.py
```
