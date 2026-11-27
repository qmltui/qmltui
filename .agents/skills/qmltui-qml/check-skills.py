#!/usr/bin/env python3
"""Validate QmlTui skills against source, Runner declarations, and generated QML metadata."""

from __future__ import annotations

import argparse
import ast
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote


SKILLS_DIR = Path(__file__).resolve().parents[1]
REPO_ROOT = SKILLS_DIR.parents[1]
IS_QMLTUI_REPO = (REPO_ROOT / "src/QmlTui/CMakeLists.txt").is_file()

QML_SKILL_PATH = SKILLS_DIR / "qmltui-qml/SKILL.md"
DEV_LOOP_SKILL_PATH = SKILLS_DIR / "qmltui-dev-loop/SKILL.md"
RUNNER_FACADE_REF = SKILLS_DIR / "qmltui-dev-loop/references/runner-facade.md"
RUNNER_DTS_PATH = REPO_ROOT / "src/QmlTui/Runner/qmltui-runner.d.ts"

EXAMPLE_TARGETS = (
    (
        "examples/recipes/status_badge/main.cpp",
        "qmltui_status_badge_demo",
        "examples/recipes/status_badge/CMakeLists.txt",
    ),
    (
        "examples/recipes/cpp_only/main.cpp",
        "qmltui_cpp_only_demo",
        "examples/recipes/cpp_only/CMakeLists.txt",
    ),
    (
        "tests/Runner/main.cpp",
        "qmltui_runner_fixture",
        "tests/Runner/CMakeLists.txt",
    ),
)

QMLTYPE_FILES = {
    "QmlTui": "qml/QmlTui/QmlTuiCore.qmltypes",
    "QmlTui.Controls": "qml/QmlTui/Controls/QmlTuiControls.qmltypes",
    "QmlTui.Image": "qml/QmlTui/Image/QmlTuiImage.qmltypes",
}

FENCE_OPEN_RE = re.compile(r"^(\s*)(`{3,}|~{3,})([^\n]*)$")
INLINE_CODE_RE = re.compile(r"`+[^`\n]*`+")
LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")


def strip_code_for_links(text: str) -> str:
    out_lines: list[str] = []
    in_fence = False
    fence_marker = ""
    for line in text.splitlines():
        if not in_fence:
            match = FENCE_OPEN_RE.match(line)
            if match:
                in_fence = True
                fence_marker = match.group(2)
                out_lines.append("")
                continue
            out_lines.append(INLINE_CODE_RE.sub("", line))
        else:
            stripped = line.strip()
            if stripped.startswith(fence_marker[0] * len(fence_marker)) and set(stripped) == {
                fence_marker[0]
            }:
                in_fence = False
                fence_marker = ""
            out_lines.append("")
    return "\n".join(out_lines)


def check_markdown_links(errors: list[str]) -> int:
    checked = 0
    md_files = sorted(SKILLS_DIR.rglob("*.md"))
    for md_path in md_files:
        skill_root = SKILLS_DIR / md_path.relative_to(SKILLS_DIR).parts[0]
        scrubbed = strip_code_for_links(md_path.read_text(encoding="utf-8"))
        for raw_target in LINK_RE.findall(scrubbed):
            target = raw_target.strip().strip("<>").split("#", 1)[0]
            if not target or re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target):
                continue
            checked += 1
            resolved = (md_path.parent / unquote(target)).resolve()
            rel_label = md_path.relative_to(SKILLS_DIR)
            if not resolved.exists():
                errors.append(f"broken link in {rel_label}: {raw_target}")
                continue
            try:
                resolved.relative_to(skill_root.resolve())
            except ValueError:
                errors.append(
                    f"non-portable link in {rel_label} escapes {skill_root.name}/: {raw_target}"
                )
    return checked


def check_skill_scripts(errors: list[str]) -> int:
    scripts = sorted(SKILLS_DIR.glob("*/scripts/*.py"))
    for script_path in scripts:
        rel_label = script_path.relative_to(SKILLS_DIR)
        try:
            ast.parse(script_path.read_text(encoding="utf-8"), filename=str(script_path))
        except SyntaxError as exc:
            errors.append(f"syntax error in {rel_label}: {exc}")
            continue
        result = subprocess.run(
            [sys.executable, str(script_path), "--help"],
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode != 0:
            errors.append(f"--help failed for {rel_label}: {result.stderr.strip()}")
    return len(scripts)


def parse_dts_interface_methods(dts_text: str, interface_name: str) -> set[str]:
    pattern = re.compile(
        rf"^interface\s+{re.escape(interface_name)}(?:\s+extends\s+[^{{]+)?\s*\{{([\s\S]*?)^\}}",
        re.MULTILINE,
    )
    match = pattern.search(dts_text)
    if not match:
        return set()
    body = match.group(1)
    return set(re.findall(r"^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\(", body, re.MULTILINE))


def check_runner_facade_coverage(errors: list[str]) -> int:
    if not RUNNER_DTS_PATH.is_file():
        return 0
    if not RUNNER_FACADE_REF.is_file():
        errors.append(
            f"missing Runner facade reference: {RUNNER_FACADE_REF.relative_to(SKILLS_DIR)}"
        )
        return 0

    dts_text = RUNNER_DTS_PATH.read_text(encoding="utf-8")
    facade_text = RUNNER_FACADE_REF.read_text(encoding="utf-8")
    checked = 0
    for iface in ("Runner", "Region"):
        methods = parse_dts_interface_methods(dts_text, iface)
        if not methods:
            errors.append(f"could not parse interface {iface} from {RUNNER_DTS_PATH.name}")
            continue
        for method in sorted(methods):
            checked += 1
            if not re.search(rf"`{re.escape(method)}\s*\(", facade_text):
                errors.append(
                    f"{iface}.{method}() from {RUNNER_DTS_PATH.name} is missing in "
                    f"{RUNNER_FACADE_REF.relative_to(SKILLS_DIR)}"
                )
    return checked


def check_example_targets(errors: list[str]) -> int:
    if not IS_QMLTUI_REPO:
        return 0
    for source_name, target, cmake_name in EXAMPLE_TARGETS:
        source_path = REPO_ROOT / source_name
        cmake_path = REPO_ROOT / cmake_name
        if not source_path.is_file():
            errors.append(f"missing named example file: {source_name}")
        if not cmake_path.is_file():
            errors.append(f"missing target definition file: {cmake_name}")
            continue
        cmake_text = cmake_path.read_text(encoding="utf-8")
        declaration = re.compile(
            rf"(?:qt_add_executable|add_executable)\s*\(\s*{re.escape(target)}(?:\s|\))"
        )
        if not declaration.search(cmake_text):
            errors.append(f"target {target} is not declared in {cmake_name}")
    return len(EXAMPLE_TARGETS)


def common_type_map() -> dict[str, set[str]]:
    if not QML_SKILL_PATH.is_file():
        return {}
    skill_text = QML_SKILL_PATH.read_text(encoding="utf-8")
    section_match = re.search(
        r"^## 6\. Common Type Map\s*$([\s\S]*?)^## 7\.",
        skill_text,
        re.MULTILINE,
    )
    if not section_match:
        return {}

    section = section_match.group(1)
    headings = list(re.finditer(r"^\*\*`import ([^`]+)`\*\*", section, re.MULTILINE))
    result: dict[str, set[str]] = {}
    for index, heading in enumerate(headings):
        module = heading.group(1)
        end = headings[index + 1].start() if index + 1 < len(headings) else len(section)
        block = section[heading.end() : end]
        upper_types = set(re.findall(r"`([A-Z][A-Za-z0-9]*)`", block))
        value_types = set(re.findall(r"`([a-z][A-Za-z0-9]*)`\s*\(value type\)", block))
        result[module] = upper_types | value_types
    return result


def cache_bool(build_dir: Path, name: str) -> bool | None:
    cache_path = build_dir / "CMakeCache.txt"
    if not cache_path.is_file():
        return None
    match = re.search(
        rf"^{re.escape(name)}:BOOL=(ON|OFF)$",
        cache_path.read_text(encoding="utf-8"),
        re.MULTILINE,
    )
    return None if not match else match.group(1) == "ON"


def cache_private_qt_enabled(build_dir: Path) -> bool | None:
    enabled = cache_bool(build_dir, "QMLTUI_ENABLE_PRIVATE_QT")
    if not enabled:
        return enabled
    cache_path = build_dir / "CMakeCache.txt"
    text = cache_path.read_text(encoding="utf-8")
    match = re.search(r"^Qt6QmlPrivate_DIR:PATH=(.+)$", text, re.MULTILINE)
    if not match or match.group(1).endswith("-NOTFOUND"):
        return False
    return True


def cache_image_enabled(build_dir: Path) -> bool | None:
    enabled = cache_bool(build_dir, "QMLTUI_ENABLE_IMAGE")
    if not enabled:
        return enabled
    cache_path = build_dir / "CMakeCache.txt"
    text = cache_path.read_text(encoding="utf-8")
    for pkg in ("Qt6Gui_DIR", "Qt6Network_DIR"):
        match = re.search(rf"^{pkg}:PATH=(.+)$", text, re.MULTILINE)
        if not match or match.group(1).endswith("-NOTFOUND"):
            return False
    return True


def check_qml_types(build_dir: Path, errors: list[str]) -> int:
    if not IS_QMLTUI_REPO:
        return 0
    modules = common_type_map()
    if set(modules) != set(QMLTYPE_FILES):
        missing = sorted(set(QMLTYPE_FILES) - set(modules))
        extra = sorted(set(modules) - set(QMLTYPE_FILES))
        errors.append(f"unexpected Common Type Map modules; missing={missing}, extra={extra}")

    private_qt_enabled = cache_private_qt_enabled(build_dir)
    image_enabled = cache_image_enabled(build_dir)
    checked = 0
    for module, types in modules.items():
        relative_qmltypes = QMLTYPE_FILES.get(module)
        if relative_qmltypes is None:
            continue
        qmltypes_path = build_dir / relative_qmltypes
        if module == "QmlTui.Image" and image_enabled is False:
            continue
        if not qmltypes_path.is_file():
            errors.append(f"missing generated metadata for {module}: {qmltypes_path}")
            continue
        metadata = qmltypes_path.read_text(encoding="utf-8")
        qmldir_path = qmltypes_path.parent / "qmldir"
        qmldir = qmldir_path.read_text(encoding="utf-8") if qmldir_path.is_file() else ""
        for qml_type in sorted(types):
            if module == "QmlTui" and qml_type == "Behavior" and private_qt_enabled is False:
                continue
            checked += 1
            qmltypes_export = f'"{module}/{qml_type} ' in metadata
            qmldir_export = re.search(
                rf"^{re.escape(qml_type)}\s+\d+(?:\.\d+)?\s+",
                qmldir,
                re.MULTILINE,
            )
            if not qmltypes_export and not qmldir_export:
                errors.append(
                    f"{module}.{qml_type} is absent from {relative_qmltypes} and qmldir"
                )
    return checked


def check_playground_types(errors: list[str]) -> None:
    if not IS_QMLTUI_REPO:
        return
    candidates = (
        REPO_ROOT / "examples/wasm/www/types.json",
        REPO_ROOT.parent / "qmltui-website/wasm/www/types.json",
        REPO_ROOT.parent / "website/wasm/www/types.json",
    )
    types_json_path = next((p for p in candidates if p.is_file()), None)
    if types_json_path is None:
        return
    data = json.loads(types_json_path.read_text(encoding="utf-8"))
    modules = common_type_map()
    for module in ("QmlTui", "QmlTui.Controls", "QmlTui.Image"):
        expected = modules.get(module, set())
        actual_mod = set(data.get("modules", {}).get(module, []))
        actual_types = {
            name
            for name, entry in data.get("types", {}).items()
            if entry.get("module") == module
        }
        if actual_mod != expected:
            missing = sorted(expected - actual_mod)
            extra = sorted(actual_mod - expected)
            errors.append(
                f"types.json modules[{module!r}] mismatch: missing={missing}, extra={extra}"
            )
        if actual_types != expected:
            missing = sorted(expected - actual_types)
            extra = sorted(actual_types - expected)
            errors.append(
                f"types.json types for {module} mismatch: missing={missing}, extra={extra}"
            )


def check_diff_whitespace(errors: list[str]) -> None:
    if not (REPO_ROOT / ".git").exists() or shutil.which("git") is None:
        return
    for extra_args, label in (([], "unstaged"), (["--cached"], "staged")):
        result = subprocess.run(
            ["git", "diff", *extra_args, "--check"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode:
            details = result.stdout.strip() or result.stderr.strip()
            errors.append(f"{label} git diff --check failed:\n{details}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=REPO_ROOT / "build",
        help="configured build tree containing generated qmltypes (default: build)",
    )
    args = parser.parse_args()
    build_dir = args.build_dir
    if not build_dir.is_absolute():
        build_dir = REPO_ROOT / build_dir

    for required in (QML_SKILL_PATH, DEV_LOOP_SKILL_PATH):
        if not required.is_file():
            print(f"ERROR: missing required skill file: {required}", file=sys.stderr)
            return 1

    errors: list[str] = []
    links = check_markdown_links(errors)
    scripts = check_skill_scripts(errors)
    facade_methods = check_runner_facade_coverage(errors)
    targets = check_example_targets(errors)
    qml_types = check_qml_types(build_dir, errors)
    check_playground_types(errors)
    check_diff_whitespace(errors)

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    print(
        f"Skill checks passed: {links} portable links, {scripts} helper scripts, "
        f"{facade_methods} Runner/Region methods, {targets} example targets, "
        f"{qml_types} generated QML types, and clean diff whitespace."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
