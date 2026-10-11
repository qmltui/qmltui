#!/usr/bin/env python3
"""Validate repository-local Qamui skills against source and generated QML metadata."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote


REPO_ROOT = Path(__file__).resolve().parents[3]
SKILL_PATHS = (
    REPO_ROOT / ".agents/skills/qamui-qml/SKILL.md",
    REPO_ROOT / ".agents/skills/qamui-dev-loop/SKILL.md",
)
EXAMPLE_TARGETS = (
    (
        "examples/components/animated_sprite/main.cpp",
        "qamui_animated_sprite_demo_test",
        "examples/components/animated_sprite/CMakeLists.txt",
    ),
    (
        "tests/Runner/main.cpp",
        "qamui_runner_fixture",
        "tests/Runner/CMakeLists.txt",
    ),
)

QMLTYPE_FILES = {
    "Qamui": "qml/Qamui/QamuiCore.qmltypes",
    "Qamui.Controls": "qml/Qamui/Controls/QamuiControls.qmltypes",
    "Qamui.Image": "qml/Qamui/Image/QamuiImage.qmltypes",
    "Qamui.TerminalHost": "qml/Qamui/TerminalHost/QamuiTerminalHost.qmltypes",
}


def check_markdown_links(errors: list[str]) -> int:
    checked = 0
    link_pattern = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
    for skill_path in SKILL_PATHS:
        text = skill_path.read_text(encoding="utf-8")
        for raw_target in link_pattern.findall(text):
            target = raw_target.strip().strip("<>").split("#", 1)[0]
            if not target or re.match(r"^[a-z][a-z0-9+.-]*:", target):
                continue
            checked += 1
            resolved = (skill_path.parent / unquote(target)).resolve()
            if not resolved.exists():
                errors.append(f"broken link in {skill_path.relative_to(REPO_ROOT)}: {raw_target}")
    return checked


def check_example_targets(errors: list[str]) -> int:
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
    skill_text = SKILL_PATHS[0].read_text(encoding="utf-8")
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
        block = section[heading.end():end]
        result[module] = set(re.findall(r"`([A-Z][A-Za-z0-9]*)`", block))
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


def check_qml_types(build_dir: Path, errors: list[str]) -> int:
    modules = common_type_map()
    if set(modules) != set(QMLTYPE_FILES):
        missing = sorted(set(QMLTYPE_FILES) - set(modules))
        extra = sorted(set(modules) - set(QMLTYPE_FILES))
        errors.append(f"unexpected Common Type Map modules; missing={missing}, extra={extra}")

    behavior_enabled = cache_bool(build_dir, "QAMUI_ENABLE_BEHAVIOR")
    terminal_host_enabled = cache_bool(build_dir, "QAMUI_ENABLE_TERMINAL_HOST")
    checked = 0
    for module, types in modules.items():
        relative_qmltypes = QMLTYPE_FILES.get(module)
        if relative_qmltypes is None:
            continue
        qmltypes_path = build_dir / relative_qmltypes
        if module == "Qamui.TerminalHost" and terminal_host_enabled is False:
            continue
        if not qmltypes_path.is_file():
            errors.append(f"missing generated metadata for {module}: {qmltypes_path}")
            continue
        metadata = qmltypes_path.read_text(encoding="utf-8")
        qmldir_path = qmltypes_path.parent / "qmldir"
        qmldir = qmldir_path.read_text(encoding="utf-8") if qmldir_path.is_file() else ""
        for qml_type in sorted(types):
            if module == "Qamui" and qml_type == "Behavior" and behavior_enabled is False:
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


def check_diff_whitespace(errors: list[str]) -> None:
    if not (REPO_ROOT / ".git").exists():
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

    errors: list[str] = []
    links = check_markdown_links(errors)
    targets = check_example_targets(errors)
    qml_types = check_qml_types(build_dir, errors)
    check_diff_whitespace(errors)

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    print(
        f"Skill checks passed: {links} links, {targets} example targets, "
        f"{qml_types} generated QML types, and clean diff whitespace."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
