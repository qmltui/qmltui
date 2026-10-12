#!/usr/bin/env python3
"""QML type reference pages, generated from and checked against the build.

Every exported QmlTui QML type has one page, ``docs/qml-types/<slug>.md``, in the
Plan 04 template. The build's ``.qmltypes`` files (C++ types) and ``qmldir``
files (types written in QML) are the source of what exists: type names, what a
type inherits, and every property, signal, method, enum and attached property.

    component_docs.py generate --build-dir build   # write pages that are missing
    component_docs.py check    --build-dir build   # fail on a type without a
                                                   # page or a page whose member
                                                   # tables drift from the build

``generate`` never overwrites a page. It fills structure and leaves prose as
``_TODO_`` placeholders. ``check`` is registered as ``test_component_docs`` and
reports the placeholders left, so the catalog cannot silently omit a type and
the tables cannot silently go stale. ``check --strict`` also fails on
placeholders once the prose is complete.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
PAGES_DIR = REPO_ROOT / "docs/qml-types"

# Module URI -> (qmltypes relative to the build's qml/ root, QML source dir)
MODULES = {
    "QmlTui": ("QmlTui/QmlTuiCore.qmltypes", None),
    "QmlTui.Controls": ("QmlTui/Controls/QmlTuiControls.qmltypes", "src/QmlTui/Controls"),
    "QmlTui.Image": ("QmlTui/Image/QmlTuiImage.qmltypes", None),
    "QmlTui.TerminalHost": ("QmlTui/TerminalHost/QmlTuiTerminalHost.qmltypes", None),
}

# Catalog category per type; anything unlisted lands in "other" and check()
# reports it so the table stays deliberate.
CATEGORIES = {
    "core": ["Item", "Terminal", "Rectangle", "Border", "Gradient", "GradientStop",
             "Scrollback", "Clipboard", "QmlTui", "rgba", "terminalCapabilities", "AnchorLine", "Anchors"],
    "layout": ["Row", "Column", "RowLayout", "ColumnLayout", "GridLayout", "Layout",
               "StackLayout", "Repeater", "Loader"],
    "data views": ["ListView", "GridView", "TableView", "TreeView", "ScrollView", "LogView",
                   "StandardItemDelegate"],
    "inputs": ["AbstractButton", "Button", "ButtonGroup", "CheckBox", "RadioButton", "Switch",
               "Slider", "TextInput", "TextArea"],
    "display": ["Text", "ProgressBar", "BusyIndicator", "Sparkline", "Image",
                "TerminalView"],
    "overlays and navigation": ["Dialog", "Popup", "Toast", "ToolTip", "TabBar", "StackView",
                                "Panel"],
    "canvas": ["Canvas", "CanvasShape", "CanvasRectangle", "Circle", "Line", "Point", "Polygon"],
    "input events": ["Keys", "KeyEvent", "MouseEvent", "MouseArea", "Shortcut"],
    "animation and state": ["Animation", "PropertyAnimation", "NumberAnimation",
                            "ColorAnimation", "Easing", "Behavior", "State", "Transition",
                            "PropertyChanges", "StateGroup"],
}
CATEGORY_OF = {name: category for category, names in CATEGORIES.items() for name in names}

# Types that exist only when a build option is on. Their pages are kept in
# every tree; check() reads the option from CMakeCache.txt and does not treat
# such a page as an orphan when the option is off.
OPTIONAL_TYPES = {
    "Behavior": "QMLTUI_ENABLE_PRIVATE_QT",
    "Image": "QMLTUI_ENABLE_IMAGE",
    "StateGroup": "QMLTUI_ENABLE_PRIVATE_QT",
}

# Members (properties, signals, methods, attached) that exist only when a
# build option is on.
OPTIONAL_MEMBERS = {
    ("PropertyChanges", "explicit"): "QMLTUI_ENABLE_PRIVATE_QT",
}

# Qt/C++ spellings shown as QML types in the tables.
QML_TYPE_NAMES = {
    "QString": "string", "QStringList": "list<string>", "bool": "bool", "int": "int",
    "double": "real", "qreal": "real", "float": "real", "QVariant": "var",
    "QVariantList": "list<var>", "QVariantMap": "var", "QJSValue": "var",
    "QSize": "size", "QPoint": "point", "QRect": "rect", "QUrl": "url",
    "QColor": "color", "qmltui::Color": "rgba", "qmltui::TerminalCapabilities": "terminalCapabilities",
    "QObject": "QtObject", "QAbstractItemModel": "model", "QQmlComponent": "Component",
    "void": "void",
}


# --------------------------------------------------------------------------
# qmltypes parsing (the QtQuick.tooling object notation)


@dataclass
class Node:
    kind: str
    props: dict = field(default_factory=dict)
    children: list["Node"] = field(default_factory=list)

    def child(self, kind: str) -> list["Node"]:
        return [c for c in self.children if c.kind == kind]


_TOKEN = re.compile(
    r'\s*(?://[^\n]*\n|(?P<str>"(?:[^"\\]|\\.)*")|(?P<num>-?\d+(?:\.\d+)?)'
    r'|(?P<ident>[A-Za-z_][A-Za-z0-9_.]*)|(?P<punct>[{}\[\]:;,]))'
)


def _tokens(text: str):
    pos = 0
    while pos < len(text):
        m = _TOKEN.match(text, pos)
        if not m:
            if text[pos:].strip() == "":
                return
            raise ValueError(f"qmltypes parse error near: {text[pos:pos + 40]!r}")
        pos = m.end()
        if m.lastgroup is None:
            continue  # comment
        yield m.lastgroup, m.group(m.lastgroup)


def parse_qmltypes(text: str) -> Node:
    tokens = list(_tokens(text))
    index = 0

    def parse_value():
        nonlocal index
        kind, value = tokens[index]
        index += 1
        if kind == "str":
            return value[1:-1]
        if kind == "num":
            return float(value) if "." in value else int(value)
        if kind == "ident":
            return {"true": True, "false": False}.get(value, value)
        if value == "[":
            items = []
            while tokens[index][1] != "]":
                items.append(parse_value())
                if tokens[index][1] == ",":
                    index += 1
            index += 1
            return items
        raise ValueError(f"unexpected token {value!r}")

    def parse_object(kind: str) -> Node:
        nonlocal index
        node = Node(kind)
        assert tokens[index][1] == "{"
        index += 1
        while tokens[index][1] != "}":
            tkind, tvalue = tokens[index]
            if tkind == "ident" and tokens[index + 1][1] == "{":
                index += 1
                node.children.append(parse_object(tvalue))
            elif tkind == "ident" and tokens[index + 1][1] == ":":
                index += 2
                node.props[tvalue] = parse_value()
            elif tvalue == ";":
                index += 1
            else:
                raise ValueError(f"unexpected token {tvalue!r} in {kind}")
        index += 1
        return node

    # skip "import QtQuick.tooling 1.2"
    while tokens[index][1] != "Module":
        index += 1
    index += 1
    return parse_object("Module")


# --------------------------------------------------------------------------
# The type model shared by generate and check


@dataclass
class Member:
    name: str
    type: str = ""
    params: list[tuple[str, str]] = field(default_factory=list)
    readonly: bool = False
    default: str = ""

    def signature(self) -> str:
        args = ", ".join(f"{t} {n}".strip() for n, t in self.params)
        return f"{self.name}({args})"


@dataclass
class QmlType:
    name: str
    module: str
    cpp: str
    inherits: str
    creatable: bool = True
    singleton: bool = False
    value_type: bool = False
    item: bool = False
    properties: list[Member] = field(default_factory=list)
    signals: list[Member] = field(default_factory=list)
    methods: list[Member] = field(default_factory=list)
    enums: list[tuple[str, list[str]]] = field(default_factory=list)
    attached: list[Member] = field(default_factory=list)

    @property
    def slug(self) -> str:
        return self.name.lower()

    @property
    def page(self) -> Path:
        return PAGES_DIR / f"{self.slug}.md"


def qml_type_name(cpp: str, pointer: bool = False, is_list: bool = False, known: dict = None) -> str:
    name = cpp
    if name in QML_TYPE_NAMES:
        name = QML_TYPE_NAMES[name]
    elif name.startswith("qmltui::"):
        short = name[len("qmltui::"):]
        name = known.get(short, short) if known else short
    if is_list:
        name = f"list<{name}>"
    return name


def _members(component: Node, known: dict) -> tuple[list[Member], list[Member], list[Member], list]:
    properties, signals, methods, enums = [], [], [], []
    for p in component.child("Property"):
        properties.append(Member(
            name=p.props["name"],
            type=qml_type_name(p.props.get("type", "var"), p.props.get("isPointer", False),
                               p.props.get("isList", False), known),
            readonly=bool(p.props.get("isReadonly", False)),
        ))
    for s in component.child("Signal"):
        params = [(a.props.get("name", ""), qml_type_name(a.props.get("type", "var"), known=known))
                  for a in s.child("Parameter")]
        signals.append(Member(name=s.props["name"], params=params))
    for m in component.child("Method"):
        if m.props.get("isConstructor"):
            continue
        params = [(a.props.get("name", ""), qml_type_name(a.props.get("type", "var"), known=known))
                  for a in m.child("Parameter")]
        methods.append(Member(name=m.props["name"], params=params,
                              type=qml_type_name(m.props.get("type", "void"), known=known)))
    for e in component.child("Enum"):
        enums.append((e.props["name"], list(e.props.get("values", []))))
    # Property-change signals carry no information a reader needs.
    notify_names = {p.props.get("notify") for p in component.child("Property")}
    signals = [s for s in signals if s.name not in notify_names]
    return properties, signals, methods, enums


def load_cpp_types(module: str, qmltypes_path: Path) -> tuple[list[QmlType], dict]:
    root = parse_qmltypes(qmltypes_path.read_text(encoding="utf-8"))
    components = {c.props["name"]: c for c in root.child("Component")}
    # cpp name -> exported QML name, for prototype links and type spellings
    exported = {}
    for cpp, c in components.items():
        for export in c.props.get("exports", []):
            exported[cpp[len("qmltui::"):] if cpp.startswith("qmltui::") else cpp] = export.split("/")[-1].split(" ")[0]
    types = []
    for cpp, c in components.items():
        exports = c.props.get("exports", [])
        if not exports:
            continue
        name = exports[0].split("/")[-1].split(" ")[0]
        proto = c.props.get("prototype", "")
        proto_short = proto[len("qmltui::"):] if proto.startswith("qmltui::") else proto
        inherits = exported.get(proto_short, "")
        if not inherits and proto_short in ("QObject", ""):
            inherits = ""
        elif not inherits:
            inherits = proto_short
        item = False
        walk = c
        while walk is not None:
            if walk.props["name"] == "qmltui::Item":
                item = True
                break
            walk = components.get(walk.props.get("prototype", ""))
        properties, signals, methods, enums = _members(c, exported)
        attached = []
        if "attachedType" in c.props and c.props["attachedType"] in components:
            attached_component = components[c.props["attachedType"]]
            if attached_component is not c:
                attached, _, _, _ = _members(attached_component, exported)
        qtype = QmlType(
            name=name, module=module, cpp=cpp, inherits=inherits,
            creatable=bool(c.props.get("isCreatable", True)) and not c.props.get("isSingleton", False)
            and c.props.get("accessSemantics", "reference") == "reference",
            singleton=bool(c.props.get("isSingleton", False)),
            value_type=c.props.get("accessSemantics") == "value",
            item=item, properties=properties, signals=signals, methods=methods, enums=enums,
            attached=attached,
        )
        types.append(qtype)
    return types, exported


_QML_PROPERTY = re.compile(
    r"^    (?:(?P<readonly>readonly) |(?:required) |(?:default) )?property (?P<type>[\w<>.]+) (?P<name>\w+)(?:\s*:\s*(?P<default>.+))?$")
_QML_SIGNAL = re.compile(r"^    signal (?P<name>\w+)(?:\((?P<params>[^)]*)\))?")
_QML_FUNCTION = re.compile(r"^    function (?P<name>\w+)\((?P<params>[^)]*)\)")
_QML_ROOT = re.compile(r"^(?P<type>[A-Z]\w*)\s*\{", re.MULTILINE)
_QML_ENUM = re.compile(r"^    enum (?P<name>\w+)\s*\{(?P<body>[^}]*)\}", re.MULTILINE)


def load_qml_file_types(module: str, qmldir_path: Path, source_dir: Path, known: dict) -> list[QmlType]:
    types = []
    for line in qmldir_path.read_text(encoding="utf-8").splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2].endswith(".qml"):
            name, _version, file = parts
            text = (source_dir / file).read_text(encoding="utf-8")
            root = _QML_ROOT.search(text)
            inherits = root.group("type") if root else ""
            properties, signals, methods = [], [], []
            for source_line in text.splitlines():
                if m := _QML_PROPERTY.match(source_line):
                    default = (m.group("default") or "").strip()
                    properties.append(Member(name=m.group("name"), type=m.group("type"),
                                             readonly=bool(m.group("readonly")),
                                             default=default if re.fullmatch(r'"[^"]*"|-?\d+(\.\d+)?|true|false|null', default) else ""))
                elif m := _QML_SIGNAL.match(source_line):
                    params = []
                    for param in filter(None, (p.strip() for p in (m.group("params") or "").split(","))):
                        bits = param.split()
                        params.append((bits[-1], bits[0] if len(bits) > 1 else "var"))
                    signals.append(Member(name=m.group("name"), params=params))
                elif m := _QML_FUNCTION.match(source_line):
                    params = [(p.strip().split(":")[0].strip(), "var")
                              for p in (m.group("params") or "").split(",") if p.strip()]
                    methods.append(Member(name=m.group("name"), params=params, type="var"))
            enums = [(m.group("name"), [v.strip().split("=")[0].strip()
                                        for v in m.group("body").split(",") if v.strip()])
                     for m in _QML_ENUM.finditer(text)]
            # A type written in QML has no class of its own; record the root
            # item's class so the page can say what it is underneath.
            cpp = f"qmltui::{inherits}" if inherits in known.values() else ""
            types.append(QmlType(name=name, module=module, cpp=cpp, inherits=inherits,
                                 item=True, properties=properties, signals=signals, methods=methods,
                                 enums=enums))
    return types


def load_types(build_dir: Path) -> tuple[list[QmlType], list[str]]:
    qml_root = build_dir / "qml"
    types: list[QmlType] = []
    skipped: list[str] = []
    known: dict = {}   # C++ short name -> QML name, across every module loaded so far
    for module, (qmltypes_rel, qml_source) in MODULES.items():
        qmltypes_path = qml_root / qmltypes_rel
        if not qmltypes_path.is_file():
            skipped.append(module)
            continue
        cpp_types, exported = load_cpp_types(module, qmltypes_path)
        known.update(exported)
        types.extend(cpp_types)
        if qml_source:
            types.extend(load_qml_file_types(module, qmltypes_path.parent / "qmldir",
                                             REPO_ROOT / qml_source, known))
    return types, skipped


# --------------------------------------------------------------------------
# generate


def _link(name: str, slugs: set[str]) -> str:
    return f"[{name}]({name.lower()}.md)" if name.lower() in slugs else (f"`{name}`" if name else "none")


def render_page(t: QmlType, slugs: set[str]) -> str:
    category = CATEGORY_OF.get(t.name, "other")
    kind_word = "singleton" if t.singleton else "value type" if t.value_type else \
        "attached type" if not t.creatable and t.attached else "QML type"
    lines = [
        "---",
        f"title: {t.name}",
        "kind: qml",
        f"import: {t.module}",
        'since: "0.4"',
        f"inherits: {t.inherits}",
        f"cpp: {t.cpp}",
        f"category: {category}",
        "summary: _TODO_",
        "---",
        "",
        f"# {t.name} {kind_word[0].upper() + kind_word[1:]}",
        "",
        "_TODO_ one sentence. [More...](#detailed-description)",
        "",
        "| | |",
        "| :--- | :--- |",
        f"| **Import Statement** | `import {t.module}` |",
        "| **Since** | QmlTui 0.4 |",
        f"| **Inherits** | {_link(t.inherits, slugs)} |",
    ]
    if t.singleton:
        lines.append("| **Instantiation** | Singleton, use by name |")
    elif t.value_type:
        lines.append("| **Instantiation** | Value type, not instantiable |")
    elif not t.creatable:
        lines.append("| **Instantiation** | Not instantiable; use as an attached property |")
    lines.append("")

    def table(title, header, rows):
        if not rows:
            return
        lines.append(f"## {title}")
        lines.append("")
        lines.append("| " + " | ".join(header) + " |")
        lines.append("|" + " :--- |" * len(header))
        lines.extend(rows)
        lines.append("")

    table("Properties", ["Property", "Type", "Default", "Description"],
          [f"| `{p.name}` | `{p.type}` | {('`' + p.default + '`') if p.default else ''} | "
           f"{'Read-only. ' if p.readonly else ''}_TODO_ |" for p in t.properties])
    table("Attached Properties", ["Property", "Type", "Description"],
          [f"| `{t.name}.{p.name}` | `{p.type}` | {'Read-only. ' if p.readonly else ''}_TODO_ |"
           for p in t.attached])
    table("Signals", ["Signal", "Description"],
          [f"| `{s.signature()}` | _TODO_ |" for s in t.signals])
    table("Methods", ["Method", "Description"],
          [f"| `{m.type} {m.signature()}` | _TODO_ |" for m in t.methods])
    for enum_name, values in t.enums:
        table(f"Enumeration {t.name}.{enum_name}", ["Value", "Description"],
              [f"| `{t.name}.{v}` | _TODO_ |" for v in values])

    lines += ["## Detailed Description", "", "_TODO_", "", "### Example", "", "```qml",
              "import QtQml", "import QmlTui"]
    if t.module != "QmlTui":
        lines.append(f"import {t.module}")
    lines.append("")
    if t.name == "Terminal":
        lines += ["Terminal {", "    Text { text: \"Hello\" }", "}"]
    elif t.creatable and t.item:
        lines += ["Terminal {", f"    {t.name} {{", "        anchors.centerIn: parent", "    }", "}"]
    else:
        lines += ["Terminal {", f"    // _TODO_ show {t.name} in use", "}"]
    lines += ["```", ""]
    return "\n".join(lines)


def generate(build_dir: Path, force: bool) -> int:
    types, skipped = load_types(build_dir)
    PAGES_DIR.mkdir(parents=True, exist_ok=True)
    slugs = {t.slug for t in types}
    written = 0
    for t in types:
        if t.page.exists() and not force:
            continue
        t.page.write_text(render_page(t, slugs), encoding="utf-8")
        written += 1
    print(f"{written} page(s) written, {len(types) - written} kept, {len(types)} types"
          + (f"; modules not built here: {', '.join(skipped)}" if skipped else ""))
    return 0


# --------------------------------------------------------------------------
# check


_FRONTMATTER = re.compile(r"\A---\n(.*?)\n---\n", re.DOTALL)
_ROW = re.compile(r"^\| `([^`]+)`")


def _section_rows(text: str, heading: str) -> set[str]:
    m = re.search(rf"^## {re.escape(heading)}\s*$(.*?)(?=^## |\Z)", text, re.MULTILINE | re.DOTALL)
    if not m:
        return set()
    names = set()
    for line in m.group(1).splitlines():
        if row := _ROW.match(line):
            names.add(row.group(1).split("(")[0].split(" ")[-1].split(".")[-1])
    return names


def _cache_option(build_dir: Path, name: str) -> bool | None:
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return None
    for line in cache.read_text(encoding="utf-8", errors="ignore").splitlines():
        if line.startswith(name + ":"):
            return line.split("=", 1)[1].strip().upper() in ("ON", "TRUE", "1", "YES")
    return None


def check(build_dir: Path, strict: bool) -> int:
    types, skipped = load_types(build_dir)
    disabled_types = {slug.lower() for slug, option in OPTIONAL_TYPES.items()
                      if _cache_option(build_dir, option) is False}
    errors: list[str] = []
    todo_total = 0
    by_slug = {t.slug: t for t in types}
    for t in types:
        if not t.page.is_file():
            errors.append(f"{t.module}/{t.name}: no page docs/qml-types/{t.slug}.md (run `generate`)")
            continue
        text = t.page.read_text(encoding="utf-8")
        fm = _FRONTMATTER.match(text)
        if not fm:
            errors.append(f"{t.slug}.md: missing frontmatter")
            continue
        front = dict(line.split(":", 1) for line in fm.group(1).splitlines() if ":" in line)
        front = {k.strip(): v.strip() for k, v in front.items()}
        for key, expected in (("title", t.name), ("kind", "qml"), ("import", t.module)):
            if front.get(key) != expected:
                errors.append(f"{t.slug}.md: frontmatter {key} is {front.get(key)!r}, build says {expected!r}")
        if front.get("category", "other") == "other":
            errors.append(f"{t.slug}.md: category is 'other'; add {t.name} to CATEGORIES in {Path(__file__).name}")
        for heading, members in (("Properties", t.properties), ("Signals", t.signals),
                                 ("Methods", t.methods), ("Attached Properties", t.attached)):
            documented = _section_rows(text, heading)
            built = {m.name for m in members}
            for missing in sorted(built - documented):
                errors.append(f"{t.slug}.md: {heading[:-1].lower()} `{missing}` exists in the build but not on the page")
            for extra in sorted(documented - built):
                option = OPTIONAL_MEMBERS.get((t.name, extra))
                if option and _cache_option(build_dir, option) is False:
                    continue
                errors.append(f"{t.slug}.md: {heading[:-1].lower()} `{extra}` is on the page but not in the build")
        todo_total += text.count("_TODO_")
    for page in sorted(PAGES_DIR.glob("*.md")) if PAGES_DIR.is_dir() else []:
        if page.stem in by_slug or page.stem == "index":
            continue
        # A page for a module this build did not produce (TerminalHost is
        # optional) cannot be checked here; its own frontmatter says which
        # module it belongs to, and that is enough to leave it alone.
        fm = _FRONTMATTER.match(page.read_text(encoding="utf-8"))
        module = ""
        if fm:
            for line in fm.group(1).splitlines():
                if line.startswith("import:"):
                    module = line.split(":", 1)[1].strip()
        if module in skipped or page.stem in disabled_types:
            continue
        errors.append(f"{page.name}: no exported QML type has this name"
                      + (f" (modules not built here: {', '.join(skipped)})" if skipped else ""))
    for line in errors:
        print(f"ERROR: {line}")
    covered = sum(1 for t in types if t.page.is_file())
    print(f"{covered}/{len(types)} exported types have a page; {todo_total} _TODO_ placeholder(s) left"
          + (f"; modules not built here: {', '.join(skipped)}" if skipped else "")
          + (f"; types disabled in this build: {', '.join(sorted(disabled_types))}" if disabled_types else ""))
    if strict and todo_total:
        print("ERROR: --strict: placeholders remain")
        return 1
    return 1 if errors else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("generate", "check"):
        p = sub.add_parser(name)
        p.add_argument("--build-dir", type=Path, default=REPO_ROOT / "build",
                       help="configured build tree with generated qml/ metadata")
        if name == "generate":
            p.add_argument("--force", action="store_true", help="overwrite existing pages")
        else:
            p.add_argument("--strict", action="store_true", help="also fail on _TODO_ placeholders")
    args = parser.parse_args()
    if not (args.build_dir / "qml").is_dir():
        print(f"ERROR: {args.build_dir} has no qml/ directory; configure and build first")
        return 2
    if args.command == "generate":
        return generate(args.build_dir, args.force)
    return check(args.build_dir, args.strict)


if __name__ == "__main__":
    sys.exit(main())
