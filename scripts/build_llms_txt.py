#!/usr/bin/env python3
"""Generates AI/LLM documentation bundles (llms.txt and llms-full.txt) for QmlTui.

Follows the llmstxt.org specification:
- llms.txt: Concise markdown index with project summary, core QML/architecture rules,
  and categorized links + descriptions for Getting Started, Tutorials, Topics,
  QML Types, and C++ Classes.
- llms-full.txt: Complete single-file reference combining the QmlTui QML authoring
  skill (.agents/skills/qmltui-qml/SKILL.md), getting-started guide, tutorials,
  topic guides, QML type reference pages, and C++ class reference pages.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


def parse_frontmatter(text: str) -> tuple[dict[str, str], str]:
    """Extract simple YAML frontmatter key-value pairs and return (meta, body)."""
    meta: dict[str, str] = {}
    body = text
    if text.startswith("---\n"):
        parts = text.split("---", 2)
        if len(parts) >= 3:
            for line in parts[1].strip().splitlines():
                if ":" in line:
                    k, v = line.split(":", 1)
                    meta[k.strip()] = v.strip().strip('"').strip("'")
            body = parts[2].lstrip("\n")
    return meta, body


def extract_title_and_summary(md_file: Path) -> tuple[str, str, dict[str, str]]:
    """Extract (title, one-line summary, frontmatter) from a markdown document."""
    text = md_file.read_text(encoding="utf-8")
    meta, body = parse_frontmatter(text)
    title = meta.get("title", "")
    summary = meta.get("summary", "")

    lines = body.splitlines()
    for line in lines:
        stripped = line.strip()
        if not title and stripped.startswith("# "):
            title = stripped[2:].strip()
            break

    if not title:
        title = md_file.stem.replace("-", " ").title()

    if not summary:
        # Find the first prose paragraph line (skip headings, tables, code blocks, lists, comments)
        in_code = False
        para_lines: list[str] = []
        for line in lines:
            stripped = line.strip()
            if stripped.startswith("```"):
                in_code = not in_code
                continue
            if in_code:
                continue
            if not stripped:
                if para_lines:
                    break
                continue
            if (
                stripped.startswith("#")
                or stripped.startswith("|")
                or stripped.startswith("<!--")
                or stripped.startswith("- ")
                or stripped.startswith("* ")
                or re.match(r"^\d+\.\s", stripped)
            ):
                if para_lines:
                    break
                continue
            para_lines.append(stripped)

        if para_lines:
            raw_para = " ".join(para_lines)
            # Strip "[More...](#detailed-description)"
            raw_para = re.sub(r"\s*\[More\.\.\.\]\([^)]+\)", "", raw_para)
            # Strip markdown links [label](url) -> label
            raw_para = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", raw_para)
            # Take first sentence
            m = re.match(r"^(.+?\.)(?:\s|$)", raw_para)
            summary = m.group(1) if m else raw_para

    return title, summary, meta


def verify_index_links(docs_dir: Path) -> list[str]:
    """Verify that all local .md links inside index pages resolve to existing files."""
    errors: list[str] = []
    index_files = [
        docs_dir / "README.md",
        docs_dir / "getting-started.md",
        docs_dir / "examples.md",
        docs_dir / "tutorials" / "index.md",
        docs_dir / "topics" / "index.md",
        docs_dir / "qml-types" / "index.md",
        docs_dir / "cpp-classes" / "index.md",
    ]
    link_re = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")
    for idx in index_files:
        if not idx.is_file():
            errors.append(f"Missing required documentation file: {idx}")
            continue
        content = idx.read_text(encoding="utf-8")
        for match in link_re.finditer(content):
            target = match.group(2).split("#", 1)[0].strip()
            if not target or target.startswith(("http://", "https://", "mailto:")):
                continue
            resolved = (idx.parent / target).resolve()
            if not resolved.exists():
                errors.append(f"Broken link in {idx}: '{target}' -> {resolved} does not exist")
    return errors


def build_llms_index(source_dir: Path, docs_dir: Path) -> str:
    """Generate the concise llms.txt index following llmstxt.org conventions."""
    lines: list[str] = [
        "# QmlTui",
        "",
        "> QmlTui is a C++20 and Qt 6.8+ retained-mode terminal UI framework built on `QtQml` and a custom cell-grid renderer. It enables declarative QML interfaces, property bindings, C++ `QAbstractItemModel` data views, keyboard/mouse input handling, and headless snapshot testing for Linux, macOS, and Windows Terminal without `QtQuick` or a display server.",
        "",
        "## Core QML & Architecture Rules",
        "",
        "- **Allowed Imports**: Use `import QmlTui`, `import QmlTui.Controls`, `import QmlTui.Image`, `import QmlTui.TerminalHost`, `import QtQml`, and `import QtQml.Models`. **Never** import `QtQuick` or `QtQuick.*`.",
        "- **Pragmas & Typing**: Start QML files with `pragma ComponentBehavior: Bound`. Use concrete property types (`int`, `string`, `bool`, `rgba`) and declare `required property` on view delegates.",
        "- **Integer Cell Geometry**: All coordinates and dimensions (`x`, `y`, `width`, `height`, `spacing`, `padding`, `margins`) are integer character cells (columns and rows).",
        "- **Terminal Modes**: `Terminal.AltScreen` (default full-screen alternate buffer) and `Terminal.Inline` (normal terminal buffer with bounded `Scrollback` output).",
        "- **Borders**: Grouped `border { style: \"single\" | \"double\" | \"rounded\" | \"ascii\"; color: ... }` belongs to `Rectangle` and `Panel`. `TableView` uses `showOuterBorder`, `showColumnGrid`, `showRowGrid`, and `gridStyle`.",
        "- **Keyboard Quit Path**: `Terminal` does not map `Q` or `Escape` to exit automatically; applications must provide an explicit keyboard quit path (`Qt.quit()`) that respects active modals and focused text inputs.",
        "",
        "## Getting Started",
        "",
        "- [Getting Started](docs/getting-started.md): Requirements, CMake build configuration, minimal three-file quickstart (`CMakeLists.txt`, `main.cpp`, `Main.qml`), and consumer integration.",
        "- [Examples Catalog](docs/examples.md): Overview of runnable gallery, component recipes, topic demos, and showcase applications.",
        "- [QmlTui QML Authoring Guide](.agents/skills/qmltui-qml/SKILL.md): Authoritative QML coding conventions, allowed imports, typing rules, layout/border patterns, and verification checklist.",
        "",
        "## Tutorials",
        "",
    ]

    tutorials_dir = docs_dir / "tutorials"
    for md_file in sorted(tutorials_dir.glob("*.md")):
        if md_file.name == "index.md":
            continue
        title, summary, _ = extract_title_and_summary(md_file)
        rel = md_file.relative_to(source_dir).as_posix()
        desc = f": {summary}" if summary else ""
        lines.append(f"- [{title}]({rel}){desc}")

    lines.extend(["", "## Topics & Guides", ""])
    topics_dir = docs_dir / "topics"
    for md_file in sorted(topics_dir.glob("*.md")):
        if md_file.name == "index.md":
            continue
        title, summary, _ = extract_title_and_summary(md_file)
        rel = md_file.relative_to(source_dir).as_posix()
        desc = f": {summary}" if summary else ""
        lines.append(f"- [{title}]({rel}){desc}")

    lines.extend(["", "## QML Types", ""])
    qml_dir = docs_dir / "qml-types"
    for md_file in sorted(qml_dir.glob("*.md")):
        if md_file.name == "index.md":
            continue
        title, summary, meta = extract_title_and_summary(md_file)
        rel = md_file.relative_to(source_dir).as_posix()
        import_stmt = meta.get("import", "QmlTui")
        desc = f" (`{import_stmt}`): {summary}" if summary else f" (`{import_stmt}`)"
        lines.append(f"- [{title}]({rel}){desc}")

    lines.extend(["", "## C++ Classes", ""])
    cpp_dir = docs_dir / "cpp-classes"
    for md_file in sorted(cpp_dir.glob("*.md")):
        if md_file.name == "index.md":
            continue
        title, summary, _ = extract_title_and_summary(md_file)
        rel = md_file.relative_to(source_dir).as_posix()
        desc = f": {summary}" if summary else ""
        lines.append(f"- [{title}]({rel}){desc}")

    lines.append("")
    return "\n".join(lines)


def build_llms_full(source_dir: Path, docs_dir: Path) -> tuple[str, int]:
    """Generate the complete single-file llms-full.txt bundle and return (text, file_count)."""
    sections: list[str] = [
        "# QmlTui — Complete AI / LLM Reference Bundle",
        "",
        "> Single-file documentation bundle combining QmlTui's QML authoring rules, getting-started guide, tutorials, architectural topic guides, all QML type reference pages, and all C++ class reference pages.",
    ]

    ordered_files: list[Path] = []

    skill_file = source_dir / ".agents" / "skills" / "qmltui-qml" / "SKILL.md"
    if skill_file.is_file():
        ordered_files.append(skill_file)

    for top_doc in ("getting-started.md", "examples.md"):
        p = docs_dir / top_doc
        if p.is_file():
            ordered_files.append(p)

    for subfolder in ("tutorials", "topics", "qml-types", "cpp-classes"):
        folder = docs_dir / subfolder
        if not folder.is_dir():
            continue
        index_md = folder / "index.md"
        if index_md.is_file():
            ordered_files.append(index_md)
        for md_file in sorted(folder.glob("*.md")):
            if md_file.name != "index.md":
                ordered_files.append(md_file)

    for path in ordered_files:
        rel = path.relative_to(source_dir).as_posix()
        raw = path.read_text(encoding="utf-8")
        _, body = parse_frontmatter(raw)
        sections.append(f"\n---\n\n<!-- FILE: {rel} -->\n\n{body.strip()}\n")

    return "\n".join(sections), len(ordered_files)


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(
        description="Generate llms.txt and llms-full.txt AI documentation bundles for QmlTui."
    )
    parser.add_argument(
        "--source-dir",
        "--source",
        dest="source_dir",
        type=Path,
        default=repo_root,
        help="Root directory of the QmlTui repository (default: repo root)",
    )
    parser.add_argument(
        "--docs-dir",
        type=Path,
        default=None,
        help="Documentation directory (default: <source-dir>/docs)",
    )
    parser.add_argument(
        "--output-dir",
        "--out",
        dest="output_dir",
        type=Path,
        default=None,
        help="Output directory for llms.txt and llms-full.txt (default: build/docs)",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Verify all referenced markdown files exist and bundles generate cleanly.",
    )
    args = parser.parse_args()

    source_dir = args.source_dir.resolve()
    docs_dir = args.docs_dir.resolve() if args.docs_dir else (source_dir / "docs")

    required_paths = [
        source_dir / ".agents" / "skills" / "qmltui-qml" / "SKILL.md",
        docs_dir / "getting-started.md",
        docs_dir / "tutorials",
        docs_dir / "topics",
        docs_dir / "qml-types",
        docs_dir / "cpp-classes",
    ]
    missing = [str(p) for p in required_paths if not p.exists()]
    if missing:
        for m in missing:
            print(f"error: required path missing: {m}", file=sys.stderr)
        return 1

    link_errors = verify_index_links(docs_dir)
    if link_errors:
        for err in link_errors:
            print(f"error: {err}", file=sys.stderr)
        return 1

    llms_txt = build_llms_index(source_dir, docs_dir)
    llms_full_txt, file_count = build_llms_full(source_dir, docs_dir)

    if not llms_txt.strip() or not llms_full_txt.strip() or file_count < 80:
        print(
            f"error: bundle generation incomplete (included {file_count} files)",
            file=sys.stderr,
        )
        return 1

    if args.output_dir or not args.check:
        out_dir = (args.output_dir or (source_dir / "build" / "docs")).resolve()
        out_dir.mkdir(parents=True, exist_ok=True)
        (out_dir / "llms.txt").write_text(llms_txt, encoding="utf-8")
        (out_dir / "llms-full.txt").write_text(llms_full_txt, encoding="utf-8")
        print(
            f"Generated llms.txt ({len(llms_txt.splitlines())} lines) and "
            f"llms-full.txt ({len(llms_full_txt.splitlines())} lines from {file_count} files) in {out_dir}"
        )
    else:
        print(
            f"Verified llms.txt ({len(llms_txt.splitlines())} lines) and "
            f"llms-full.txt ({len(llms_full_txt.splitlines())} lines from {file_count} files)"
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
