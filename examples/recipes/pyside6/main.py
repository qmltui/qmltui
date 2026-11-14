#!/usr/bin/env python3
"""QmlTui + PySide6 recipe: Python QAbstractTableModel & QSortFilterProxyModel in QML."""

from __future__ import annotations

import os
import sys
from dataclasses import dataclass
from pathlib import Path

from PySide6.QtCore import (
    Property,
    QAbstractTableModel,
    QCoreApplication,
    QModelIndex,
    QSortFilterProxyModel,
    Qt,
    QUrl,
    Signal,
    Slot,
)
from PySide6.QtQml import QmlElement, QQmlApplicationEngine

QML_IMPORT_NAME = "PySideDemo"
QML_IMPORT_MAJOR_VERSION = 1


@dataclass
class ServiceEntry:
    pid: int
    name: str
    cpu: float
    status: str


class ServiceTableModel(QAbstractTableModel):
    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._services: list[ServiceEntry] = [
            ServiceEntry(1001, "systemd", 0.1, "RUNNING"),
            ServiceEntry(1042, "terminal_engine", 12.4, "RUNNING"),
            ServiceEntry(1089, "qml_renderer", 4.2, "RUNNING"),
            ServiceEntry(1120, "dbus-daemon", 0.0, "STOPPED"),
            ServiceEntry(1250, "pyside_worker", 6.8, "RUNNING"),
            ServiceEntry(1304, "posix_tty", 0.5, "DEGRADED"),
        ]

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        return 0 if parent.isValid() else len(self._services)

    def columnCount(self, parent: QModelIndex = QModelIndex()) -> int:
        return 0 if parent.isValid() else 4

    def data(self, index: QModelIndex, role: int = Qt.ItemDataRole.DisplayRole):
        if (
            not index.isValid()
            or not (0 <= index.row() < len(self._services))
            or not (0 <= index.column() < 4)
            or role not in (Qt.ItemDataRole.DisplayRole, Qt.ItemDataRole.UserRole)
        ):
            return None

        entry = self._services[index.row()]
        col = index.column()
        if col == 0:
            return entry.pid
        if col == 1:
            return entry.name
        if col == 2:
            return entry.cpu if role == Qt.ItemDataRole.UserRole else f"{entry.cpu:.1f}%"
        if col == 3:
            return entry.status
        return None

    def toggle_service(self, row: int) -> ServiceEntry | None:
        if not (0 <= row < len(self._services)):
            return None
        entry = self._services[row]
        if entry.status == "RUNNING":
            entry.status = "STOPPED"
            entry.cpu = 0.0
        else:
            entry.status = "RUNNING"
            entry.cpu = 3.5 if entry.cpu == 0.0 else entry.cpu
        top_left = self.index(row, 2)
        bottom_right = self.index(row, 3)
        self.dataChanged.emit(
            top_left,
            bottom_right,
            [Qt.ItemDataRole.DisplayRole, Qt.ItemDataRole.UserRole],
        )
        return entry


@QmlElement
class ServiceController(QSortFilterProxyModel):
    filterTextChanged = Signal()
    sortLabelChanged = Signal()
    statusMessageChanged = Signal()

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._source = ServiceTableModel(self)
        self._filter_text = ""
        self._status_message = "Press Enter on a row to toggle service state"
        self.setSourceModel(self._source)
        self.setSortRole(Qt.ItemDataRole.UserRole)
        self.setFilterKeyColumn(1)
        self.setFilterCaseSensitivity(Qt.CaseSensitivity.CaseInsensitive)
        self.sort(2, Qt.SortOrder.DescendingOrder)

    def _get_filter_text(self) -> str:
        return self._filter_text

    def _set_filter_text(self, text: str) -> None:
        if self._filter_text == text:
            return
        self._filter_text = text
        self.setFilterFixedString(text)
        self.filterTextChanged.emit()

    filterText = Property(str, _get_filter_text, _set_filter_text, notify=filterTextChanged)

    def _get_sort_label(self) -> str:
        names = ["PID", "SERVICE", "CPU%", "STATUS"]
        col = self.sortColumn()
        if not (0 <= col < len(names)):
            return ""
        direction = "ascending" if self.sortOrder() == Qt.SortOrder.AscendingOrder else "descending"
        return f"Sort: {names[col]} ({direction})"

    sortLabel = Property(str, _get_sort_label, notify=sortLabelChanged)

    def _get_status_message(self) -> str:
        return self._status_message

    statusMessage = Property(str, _get_status_message, notify=statusMessageChanged)

    @Slot(int)
    def headerClicked(self, column: int) -> None:
        if not (0 <= column < self.columnCount()):
            return
        next_order = (
            Qt.SortOrder.DescendingOrder
            if self.sortColumn() == column and self.sortOrder() == Qt.SortOrder.AscendingOrder
            else Qt.SortOrder.AscendingOrder
        )
        self.sort(column, next_order)
        self.sortLabelChanged.emit()

    @Slot(int)
    def toggleRow(self, proxy_row: int) -> None:
        source_index = self.mapToSource(self.index(proxy_row, 0))
        if not source_index.isValid():
            return
        updated = self._source.toggle_service(source_index.row())
        if updated is not None:
            self._status_message = f"Service {updated.name} (PID {updated.pid}) -> {updated.status}"
            self.statusMessageChanged.emit()


def main() -> int:
    recipe_dir = Path(__file__).resolve().parent
    repo_root = recipe_dir.parents[2]
    build_dir = Path(os.environ.get("QMLTUI_BUILD_DIR", repo_root / "build")).resolve()

    app = QCoreApplication(sys.argv)
    engine = QQmlApplicationEngine()

    qml_dir = build_dir / "qml"
    if not (qml_dir / "QmlTui").is_dir():
        print(
            f"Error: QmlTui QML plugins not found under '{qml_dir / 'QmlTui'}'.\n"
            "Build the QML plugin targets first (or set QMLTUI_BUILD_DIR):\n"
            "  cmake -S . -B build -DQMLTUI_ENABLE_PRIVATE_QT=OFF\n"
            "  cmake --build build --target QmlTuiCoreplugin QmlTuiControlsplugin",
            file=sys.stderr,
        )
        return 1
    engine.addImportPath(str(qml_dir))

    engine.load(QUrl.fromLocalFile(str(recipe_dir / "Main.qml")))
    if not engine.rootObjects():
        return -1

    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
