#!/usr/bin/env python3
"""Completion index for the QML playground, generated from the build.

Reads the same ``.qmltypes`` and ``qmldir`` files as ``component_docs.py`` and
writes one JSON file the browser editor loads: every exported type with what
it inherits, its properties, signals, methods, enums, and attached properties,
plus the Qt modules the playground binary links (QtQml, QtQml.Models).

    qml_type_index.py --build-dir build-wasm --qt-qml-dir ~/Qt/6.11.1/wasm_singlethread/qml \\
                      --output build-wasm/www/types.json
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import component_docs as cd  # noqa: E402

# Qt modules the playground links, and their qmltypes relative to Qt's qml dir.
QT_MODULES = {
    "QML": "QML/plugins.qmltypes",            # QtObject, Component, and the other builtins
    "QtQml": "QtQml/plugins.qmltypes",
    "QtQml.Models": "QtQml/Models/plugins.qmltypes",
}


def member(m: cd.Member) -> dict:
    out = {"name": m.name}
    if m.type:
        out["type"] = m.type
    if m.params:
        out["params"] = [[n, t] for n, t in m.params]
    if m.readonly:
        out["readonly"] = True
    if m.default:
        out["default"] = m.default
    return out


def type_entry(t: cd.QmlType) -> dict:
    return {
        "module": t.module,
        "inherits": t.inherits,
        "creatable": t.creatable and not t.value_type,
        "singleton": t.singleton,
        "properties": [member(m) for m in t.properties],
        "signals": [member(m) for m in t.signals],
        "methods": [member(m) for m in t.methods],
        "enums": {name: values for name, values in t.enums},
        "attached": [member(m) for m in t.attached],
    }


def build_index(build_dir: Path, qt_qml_dir: Path | None, modules: list[str]) -> dict:
    types: dict[str, dict] = {}
    by_module: dict[str, list[str]] = {}
    known: dict = {}

    def add(qtypes: list[cd.QmlType]) -> None:
        for t in qtypes:
            if t.name in types:      # first module wins; Qamui is loaded before Qt
                continue
            types[t.name] = type_entry(t)
            by_module.setdefault(t.module, []).append(t.name)

    for module in modules:
        if module in cd.MODULES:
            qmltypes_rel, qml_source = cd.MODULES[module]
            path = build_dir / "qml" / qmltypes_rel
            if not path.is_file():
                print(f"warning: {module}: {path} not found, skipped", file=sys.stderr)
                continue
            cpp_types, exported = cd.load_cpp_types(module, path)
            known.update(exported)
            add(cpp_types)
            if qml_source:
                add(cd.load_qml_file_types(module, path.parent / "qmldir", cd.REPO_ROOT / qml_source, known))
        elif module in QT_MODULES and qt_qml_dir:
            path = qt_qml_dir / QT_MODULES[module]
            if not path.is_file():
                print(f"warning: {module}: {path} not found, skipped", file=sys.stderr)
                continue
            cpp_types, exported = cd.load_cpp_types(module, path)
            known.update(exported)
            add(cpp_types)
        else:
            print(f"warning: unknown module {module}", file=sys.stderr)

    # Prototype chains may name a C++ class that has no export (QObject, QQuickItem
    # stand-ins); keep them as opaque roots so the editor stops walking there.
    for t in types.values():
        if t["inherits"] and t["inherits"] not in types:
            t["inherits"] = ""
    return {"modules": by_module, "types": types}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--qt-qml-dir", type=Path, help="Qt's qml/ directory for QtQml and QtQml.Models")
    parser.add_argument("--module", action="append", dest="modules",
                        help="module URI to include; default: Qamui, Qamui.Controls, QML, QtQml, QtQml.Models")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    modules = args.modules or ["Qamui", "Qamui.Controls", "QML", "QtQml", "QtQml.Models"]
    index = build_index(args.build_dir, args.qt_qml_dir, modules)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(index, separators=(",", ":")), encoding="utf-8")
    count = sum(len(v) for v in index["modules"].values())
    print(f"{args.output}: {count} types from {', '.join(index['modules'])}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
