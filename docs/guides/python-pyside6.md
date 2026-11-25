# Python and PySide6

QmlTui applications can be hosted directly from Python using **PySide6** (`QCoreApplication` and `QQmlApplicationEngine`) with no custom C++ wrapper code or Shiboken bindings. You write your models and controllers in Python (`QAbstractListModel`, `QAbstractTableModel`, `QSortFilterProxyModel`, `@QmlElement`) and declare the terminal UI in standard QmlTui QML.

Because QmlTui's scene graph and QML types are compiled C++ plugins, you still build (or install) the QmlTui C++ QML plugins once and point `QQmlApplicationEngine` at their `qml/` directory.

## Prerequisites and Qt Version Alignment

- **Python 3.10+** and **`PySide6`** matching the Qt 6 minor version used to build QmlTui (use a virtual environment on PEP 668 distros such as Ubuntu 24.04, Debian, and Fedora):
  ```bash
  python3 -m venv .venv
  source .venv/bin/activate   # Windows: .venv\Scripts\activate
  pip install "PySide6==6.<minor>.*"
  ```
- **Build QmlTui with `-DQMLTUI_ENABLE_PRIVATE_QT=OFF`**: PySide6 wheels bundle their own Qt 6 libraries. Building QmlTui strictly against public Qt APIs avoids private-ABI mismatches between the system/SDK Qt headers used to compile QmlTui and the Qt libraries shipped inside the PySide6 wheel.

## Quickstart: Run the PySide6 Recipe

Build only the Core and Controls QML plugins (disabling tests and benchmarks so `Qt6::Test` is not required):

```bash
cmake -S . -B build -G Ninja \
  -DQMLTUI_ENABLE_PRIVATE_QT=OFF \
  -DQMLTUI_BUILD_TESTS=OFF \
  -DQMLTUI_BUILD_BENCHMARKS=OFF
cmake --build build --target QmlTuiCoreplugin QmlTuiControlsplugin

python3 examples/recipes/pyside6/main.py
```

If your build directory is not `./build`, set `QMLTUI_BUILD_DIR`:

```bash
QMLTUI_BUILD_DIR=/path/to/qmltui-build python3 examples/recipes/pyside6/main.py
```

## Using an Installed QmlTui Prefix in a Standalone Python Project

To keep your Python application separate from the QmlTui source tree, install the public-Qt build to a prefix:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQMLTUI_ENABLE_PRIVATE_QT=OFF \
  -DQMLTUI_BUILD_EXAMPLES=OFF \
  -DQMLTUI_BUILD_TESTS=OFF \
  -DQMLTUI_BUILD_BENCHMARKS=OFF \
  -DQMLTUI_ENABLE_RUNNER=OFF
cmake --build build
cmake --install build --prefix /your/prefix
```

Then register the installed QML import path on `QQmlApplicationEngine` before loading your `Main.qml` (note that Fedora/RHEL/openSUSE install under `lib64/qml` rather than `lib/qml`; `QQmlApplicationEngine` reads `QML_IMPORT_PATH` automatically, so only pass a fallback prefix when `QML_IMPORT_PATH` is unset, or split multi-entry strings on `os.pathsep`):

```python
import os
import sys
from pathlib import Path
from PySide6.QtCore import QCoreApplication, QUrl
from PySide6.QtQml import QQmlApplicationEngine

# Import any module declaring @QmlElement classes BEFORE engine.load(...)
# so PySide6 registers those QML types in time.
import service_controller  # noqa: F401

def main() -> int:
    app = QCoreApplication(sys.argv)
    engine = QQmlApplicationEngine()

    if not os.environ.get("QML_IMPORT_PATH"):
        engine.addImportPath("/your/prefix/lib/qml")  # or lib64/qml on lib64 distros

    engine.load(QUrl.fromLocalFile(str(Path(__file__).with_name("Main.qml"))))
    if not engine.rootObjects():
        return -1
    return app.exec()

if __name__ == "__main__":
    sys.exit(main())
```

### Shared Library Search Paths

- **Linux & macOS**: Standard CMake installs embed relative `$ORIGIN` / `@loader_path` `RPATH` entries on the QML plugins so `libQmlTuiCore` and `libQmlTuiControls` resolve automatically. On non-RPATH builds (`-DCMAKE_SKIP_RPATH=ON`) or relocated directories, add `<prefix>/lib` (or `<prefix>/lib64`) to `LD_LIBRARY_PATH` (Linux) or `DYLD_LIBRARY_PATH` (macOS).
- **Windows**: Call `os.add_dll_directory(r"<prefix>\bin")` before loading QML (see `examples/recipes/pyside6/main.py`), or add `<prefix>\bin` to `PATH`.

## Exposing Python Models and Controllers to QML

Use PySide6's `@QmlElement` decorator with module-level `QML_IMPORT_NAME` and `QML_IMPORT_MAJOR_VERSION` constants (always ensure the module containing `@QmlElement` classes is imported before calling `engine.load(...)`). Standard Qt model classes (`QAbstractListModel`, `QAbstractTableModel`, `QSortFilterProxyModel`) work identically to their C++ counterparts in QmlTui views (excerpted from [`examples/recipes/pyside6/main.py`](../../examples/recipes/pyside6/main.py)):

```python
from PySide6.QtCore import Property, QSortFilterProxyModel, Qt, Signal, Slot
from PySide6.QtQml import QmlElement

QML_IMPORT_NAME = "PySideDemo"
QML_IMPORT_MAJOR_VERSION = 1

@QmlElement
class ServiceController(QSortFilterProxyModel):
    filterTextChanged = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._filter_text = ""
        # Attach your QAbstractTableModel / QAbstractListModel source model here:
        # self.setSourceModel(ServiceTableModel(self))

    def _get_filter_text(self) -> str:
        return self._filter_text

    def _set_filter_text(self, text: str) -> None:
        if self._filter_text == text:
            return
        self._filter_text = text
        self.setFilterFixedString(text)
        self.filterTextChanged.emit()

    filterText = Property(str, _get_filter_text, _set_filter_text, notify=filterTextChanged)

    @Slot(int)
    def headerClicked(self, column: int) -> None:
        self.sort(column, Qt.SortOrder.AscendingOrder)
```

In `Main.qml`, import the Python module alongside `QtQml`, `QmlTui`, and `QmlTui.Controls` (providing an explicit contextual keyboard quit path):

```qml no-playground
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls
import PySideDemo

Terminal {
    id: root

    ServiceController {
        id: services
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            event.accepted = true;
            Qt.quit();
        }
    }

    TableView {
        anchors.fill: parent
        focus: true
        model: services
        onHeaderClicked: column => services.headerClicked(column)
    }
}
```

See the complete sortable and filterable table implementation in [`examples/recipes/pyside6/main.py`](../../examples/recipes/pyside6/main.py) and [`examples/recipes/pyside6/Main.qml`](../../examples/recipes/pyside6/Main.qml).

## Debugging and Testing

- **Live `AltScreen` diagnostics**: `console.log()` and Python `qWarning()` messages are buffered while `Terminal.AltScreen` owns an interactive TTY `stderr`. Run with `QMLTUI_LOG_FILE=debug.log python3 main.py` or `python3 main.py 2> debug.log` to stream diagnostics live.
- **Testing status**: `QmlTui::Runner` (`qmltui-runner`) links into compiled C++ executables (`target_link_libraries(... PRIVATE QmlTui::Runner)`) and does not currently have a standalone Python binding. Test Python `QAbstractItemModel` and `QSortFilterProxyModel` logic directly with `pytest` or `unittest` on `QCoreApplication`, and reserve `qmltui-runner` for C++ host binaries.

See [Models in QmlTui Views](models.md) for view/role contracts and [Build Configuration and Installation](../build-configuration.md) for install prefix options.
