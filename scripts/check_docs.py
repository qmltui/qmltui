#!/usr/bin/env python3
"""Validate QmlTui Markdown documentation links, example coverage, and QML snippets."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from urllib.parse import unquote


REPO_ROOT = Path(__file__).resolve().parents[1]

ALLOWED_SNIPPET_IMPORTS = {
    "QmlTui",
    "QmlTui.Controls",
    "QmlTui.Image",
    "QmlTui.PtyView",
    "QtQml",
    "QtQml.Models",
    "Qt.labs.qmlmodels",
    "QtCore",
}

FENCE_OPEN_RE = re.compile(r"^(\s*)(`{3,}|~{3,})([^\n]*)$")
IMPORT_RE = re.compile(r"^\s*import\s+([A-Za-z0-9_.]+)", re.MULTILINE)
INLINE_CODE_RE = re.compile(r"`+[^`\n]*`+")
MARKDOWN_LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
HEADING_RE = re.compile(r"^\s*#{1,6}\s+(.*?)\s*#*\s*$")
HTML_ID_RE = re.compile(r"""\b(?:id|name)\s*=\s*["']([^"']+)["']""")
CPP_TYPE_DECL_RE = re.compile(
    r"^\s*(?:(?:class|struct)\s+(?:[A-Z0-9_]+_EXPORT\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*(?:final\s*)?(?::|\{)|using\s+([A-Za-z_][A-Za-z0-9_]*)\s*=)",
    re.MULTILINE,
)
FRONTMATTER_RE = re.compile(r"\A---\n(.*?)\n---\n", re.DOTALL)


def cache_value(build_dir: Path, name: str) -> str | None:
    cache_path = build_dir / "CMakeCache.txt"
    if not cache_path.is_file():
        return None
    match = re.search(
        rf"^{re.escape(name)}:[A-Z_]+=(.*)$",
        cache_path.read_text(encoding="utf-8", errors="replace"),
        re.MULTILINE,
    )
    return None if not match else match.group(1).strip()


def cache_bool(build_dir: Path, name: str) -> bool | None:
    val = cache_value(build_dir, name)
    if val in ("ON", "TRUE", "1"):
        return True
    if val in ("OFF", "FALSE", "0"):
        return False
    return None


def cache_private_qt_enabled(build_dir: Path) -> bool:
    enabled = cache_bool(build_dir, "QMLTUI_ENABLE_PRIVATE_QT")
    if enabled is False:
        return False
    private_dir = cache_value(build_dir, "Qt6QmlPrivate_DIR")
    if private_dir and private_dir.endswith("-NOTFOUND"):
        return False
    qmltypes_path = build_dir / "qml/QmlTui/QmlTuiCore.qmltypes"
    if qmltypes_path.is_file():
        return '"QmlTui/Behavior ' in qmltypes_path.read_text(encoding="utf-8")
    return True


def discover_qmllint(build_dir: Path, explicit: Path | None) -> Path | None:
    if explicit is not None:
        return explicit if explicit.is_file() else None

    env_override = os.environ.get("QMLTUI_QMLLINT")
    if env_override:
        candidate = Path(env_override)
        if candidate.is_file():
            return candidate

    exe_names = ("qmllint.exe", "qmllint") if os.name == "nt" else ("qmllint", "qmllint6")

    for cache_key in ("Qt6QmlTools_DIR", "Qt6Qml_DIR", "Qt6_DIR"):
        raw_dir = cache_value(build_dir, cache_key)
        if not raw_dir or raw_dir.endswith("-NOTFOUND"):
            continue
        cmake_dir = Path(raw_dir)
        for ancestor in (cmake_dir, *list(cmake_dir.parents)[:4]):
            for subdir in ("bin", "libexec"):
                for exe in exe_names:
                    candidate = ancestor / subdir / exe
                    if candidate.is_file():
                        return candidate

    for fallback_dir in (Path("/usr/lib/qt6/bin"), Path("/usr/lib64/qt6/bin")):
        for exe in exe_names:
            candidate = fallback_dir / exe
            if candidate.is_file():
                return candidate

    for exe in exe_names:
        found = shutil.which(exe)
        if found:
            return Path(found)

    return None


def iter_doc_files(include_historical: bool = False, include_extra_md: bool = False) -> list[Path]:
    files = [
        REPO_ROOT / "README.md",
        REPO_ROOT / "CONTRIBUTING.md",
        REPO_ROOT / "AGENTS.md",
    ]
    if include_extra_md:
        for extra_name in ("LICENSE.md", "THIRD-PARTY.md", "SECURITY.md", "CLA.md", "CODE_OF_CONDUCT.md"):
            files.append(REPO_ROOT / extra_name)
    docs_dir = REPO_ROOT / "docs"
    for md_path in sorted(docs_dir.rglob("*.md")):
        rel = md_path.relative_to(REPO_ROOT).as_posix()
        if "/node_modules/" in rel or "/.vitepress/" in rel:
            continue
        if not include_historical:
            if rel.startswith("docs/contributing/"):
                continue
            if re.match(r"^docs/changes/0\.\d+\.md$", rel):
                continue
        files.append(md_path)
    if include_extra_md:
        examples_dir = REPO_ROOT / "examples"
        if examples_dir.is_dir():
            for md_path in sorted(examples_dir.rglob("README.md")):
                rel = md_path.relative_to(REPO_ROOT).as_posix()
                if "/.venv/" in rel or "/node_modules/" in rel:
                    continue
                files.append(md_path)
        github_dir = REPO_ROOT / ".github"
        if github_dir.is_dir():
            for md_path in sorted(github_dir.glob("*.md")):
                files.append(md_path)
    return [p for p in files if p.is_file()]


def strip_fenced_blocks(text: str) -> str:
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
            out_lines.append(line)
        else:
            stripped = line.strip()
            if stripped.startswith(fence_marker[0] * len(fence_marker)) and set(stripped) == {
                fence_marker[0]
            }:
                in_fence = False
                fence_marker = ""
            out_lines.append("")
    return "\n".join(out_lines)


def _clean_heading_text(heading_text: str) -> str:
    text = re.sub(r"!\[([^\]]*)\]\([^)]+\)", r"\1", heading_text)
    text = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", text)
    text = re.sub(r"<[^>]+>", "", text)
    return text.replace("`", "").replace("*", "").replace("~", "").strip().lower()


def github_slug(heading_text: str) -> str:
    text = _clean_heading_text(heading_text)
    out: list[str] = []
    for ch in text:
        if ch.isalnum() or ch in (" ", "-", "_"):
            out.append("-" if ch == " " else ch)
    return "".join(out)


def vitepress_slug(heading_text: str) -> str:
    # Mirrors @mdit-vue/shared slugify() used by VitePress.
    text = _clean_heading_text(heading_text)
    slug = re.sub(r"[^0-9a-zA-Z\u00c0-\uffff]+", "-", text).strip("-")
    if slug and slug[0].isdigit():
        slug = f"_{slug}"
    return slug


def collect_markdown_anchors(
    md_path: Path, cache: dict[Path, tuple[set[str], set[str]]]
) -> tuple[set[str], set[str]]:
    resolved = md_path.resolve()
    if resolved in cache:
        return cache[resolved]
    gh_anchors: set[str] = set()
    vp_anchors: set[str] = set()
    if not resolved.is_file():
        cache[resolved] = (gh_anchors, vp_anchors)
        return gh_anchors, vp_anchors
    scrubbed = strip_fenced_blocks(resolved.read_text(encoding="utf-8", errors="replace"))
    gh_counts: dict[str, int] = {}
    vp_counts: dict[str, int] = {}
    for line in scrubbed.splitlines():
        for html_id in HTML_ID_RE.findall(line):
            clean_id = html_id.strip().lower()
            gh_anchors.add(clean_id)
            vp_anchors.add(clean_id)
        match = HEADING_RE.match(line)
        if not match:
            continue
        raw_heading = match.group(1)
        custom_id = re.search(r"\s*\{#([A-Za-z0-9_.-]+)\}\s*$", raw_heading)
        if custom_id:
            cid = custom_id.group(1).lower()
            gh_anchors.add(cid)
            vp_anchors.add(cid)
            raw_heading = raw_heading[: custom_id.start()]
        gh = github_slug(raw_heading)
        if gh:
            count = gh_counts.get(gh, 0)
            gh_counts[gh] = count + 1
            gh_anchors.add(gh if count == 0 else f"{gh}-{count}")
        vp = vitepress_slug(raw_heading)
        if vp:
            count = vp_counts.get(vp, 0)
            vp_counts[vp] = count + 1
            vp_anchors.add(vp if count == 0 else f"{vp}-{count}")
    cache[resolved] = (gh_anchors, vp_anchors)
    return gh_anchors, vp_anchors


def check_markdown_links(errors: list[str]) -> int:
    checked = 0
    docs_root = (REPO_ROOT / "docs").resolve()
    anchor_cache: dict[Path, tuple[set[str], set[str]]] = {}
    vitepress_include_re = re.compile(r"^\s*<<<\s+([^\s{#]+)")
    for md_path in iter_doc_files(include_historical=True, include_extra_md=True):
        raw_text = md_path.read_text(encoding="utf-8")
        scrubbed = strip_fenced_blocks(raw_text)
        for line_no, line in enumerate(scrubbed.splitlines(), start=1):
            inc_match = vitepress_include_re.match(line)
            if inc_match:
                inc_target = unquote(inc_match.group(1).strip())
                if inc_target.startswith("@/"):
                    inc_path = (docs_root / inc_target[2:]).resolve()
                else:
                    inc_path = (md_path.parent / inc_target).resolve()
                checked += 1
                if not inc_path.is_file():
                    errors.append(
                        f"broken VitePress snippet include in {md_path.relative_to(REPO_ROOT)}:{line_no}: {inc_target}"
                    )
                continue
            clean_line = INLINE_CODE_RE.sub("", line)
            for raw_target in MARKDOWN_LINK_RE.findall(clean_line):
                target = raw_target.strip().strip("<>")
                if " " in target and (target.endswith('"') or target.endswith("'")):
                    target = target.split()[0]
                if not target or re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target):
                    continue
                path_part, _, fragment = target.partition("#")
                path_part = path_part.split("?", 1)[0]
                fragment = unquote(fragment).strip().lower()
                if not path_part and not fragment:
                    continue
                checked += 1
                if not path_part:
                    resolved_target = md_path
                else:
                    decoded = unquote(path_part)
                    if decoded.startswith("/"):
                        rel_doc = decoded.lstrip("/")
                        candidates = [
                            docs_root / rel_doc,
                            docs_root / f"{rel_doc}.md",
                            docs_root / rel_doc / "index.md",
                            docs_root / rel_doc / "README.md",
                        ]
                    else:
                        base = (md_path.parent / decoded).resolve()
                        candidates = [
                            base,
                            base.parent / f"{base.name}.md",
                            base / "index.md",
                            base / "README.md",
                        ]
                    resolved_target = next((c for c in candidates if c.exists()), None)
                    if resolved_target is None:
                        errors.append(
                            f"broken link in {md_path.relative_to(REPO_ROOT)}:{line_no}: {raw_target}"
                        )
                        continue
                    if resolved_target.is_dir():
                        index_md = resolved_target / "index.md"
                        readme_md = resolved_target / "README.md"
                        if index_md.is_file():
                            resolved_target = index_md
                        elif readme_md.is_file():
                            resolved_target = readme_md
                        elif fragment:
                            errors.append(
                                f"broken anchor in {md_path.relative_to(REPO_ROOT)}:{line_no}: {raw_target}"
                            )
                            continue
                if fragment and resolved_target.is_file() and resolved_target.suffix == ".md":
                    gh_anchors, vp_anchors = collect_markdown_anchors(resolved_target, anchor_cache)
                    in_docs = False
                    try:
                        resolved_target.resolve().relative_to(docs_root)
                        in_docs = True
                    except ValueError:
                        in_docs = False
                    if fragment not in gh_anchors:
                        errors.append(
                            f"broken anchor in {md_path.relative_to(REPO_ROOT)}:{line_no}: {raw_target}"
                        )
                    elif in_docs and fragment not in vp_anchors:
                        errors.append(
                            f"VitePress-incompatible anchor in {md_path.relative_to(REPO_ROOT)}:{line_no}: {raw_target} (matches GitHub slug only)"
                        )
    return checked


def _strip_cpp_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", text)


def check_frontmatter_cpp_types(errors: list[str]) -> int:
    qualified_declared: set[str] = set()
    unqualified_declared: set[str] = {"QObject"}
    src_root = REPO_ROOT / "src/QmlTui"
    for header in src_root.rglob("*.h"):
        if header.name.endswith("_p.h"):
            continue
        text = _strip_cpp_comments(header.read_text(encoding="utf-8", errors="replace"))
        has_qmltui_ns = bool(re.search(r"\bnamespace\s+qmltui\b", text))
        for match in CPP_TYPE_DECL_RE.finditer(text):
            name = match.group(1) or match.group(2)
            if not name:
                continue
            if has_qmltui_ns:
                qualified_declared.add(f"qmltui::{name}")
            else:
                unqualified_declared.add(name)

    checked = 0
    for md_path in sorted((REPO_ROOT / "docs/qml-types").glob("*.md")):
        if md_path.name == "index.md":
            continue
        raw = md_path.read_text(encoding="utf-8")
        fm = FRONTMATTER_RE.match(raw)
        if not fm:
            continue
        cpp_match = re.search(r"^cpp:\s*([^\n#]+)", fm.group(1), re.MULTILINE)
        if not cpp_match:
            continue
        cpp_val = cpp_match.group(1).strip().strip('"').strip("'")
        if not cpp_val or cpp_val.lower() in ("null", "none", "~"):
            continue
        checked += 1
        valid = (
            cpp_val in qualified_declared
            if "::" in cpp_val
            else cpp_val in unqualified_declared
        )
        if not valid:
            errors.append(
                f"invalid frontmatter cpp type in {md_path.relative_to(REPO_ROOT)}: {cpp_val!r} not declared as a public type under src/QmlTui/"
            )
    return checked


def check_examples_coverage(errors: list[str]) -> tuple[int, int]:
    examples_doc = (REPO_ROOT / "docs/examples.md").read_text(encoding="utf-8")

    gallery_files = sorted((REPO_ROOT / "examples/gallery/examples").glob("*.qml"))
    for qml_file in gallery_files:
        if qml_file.name not in examples_doc and qml_file.stem not in examples_doc:
            errors.append(
                f"gallery example {qml_file.relative_to(REPO_ROOT)} is not listed in docs/examples.md"
            )

    recipe_dirs = sorted(
        p
        for p in (REPO_ROOT / "examples/recipes").iterdir()
        if p.is_dir()
        and any((p / marker).exists() for marker in ("CMakeLists.txt", "README.md", "main.py"))
    )
    for recipe_dir in recipe_dirs:
        if recipe_dir.name not in examples_doc:
            errors.append(
                f"recipe {recipe_dir.relative_to(REPO_ROOT)} is not listed in docs/examples.md"
            )

    return len(gallery_files), len(recipe_dirs)


def extract_qml_snippets(md_path: Path, errors: list[str]) -> list[tuple[int, str]]:
    lines = md_path.read_text(encoding="utf-8").splitlines()
    snippets: list[tuple[int, str]] = []
    idx = 0
    while idx < len(lines):
        match = FENCE_OPEN_RE.match(lines[idx])
        if not match:
            idx += 1
            continue
        fence = match.group(2)
        info = match.group(3).strip()
        start_line = idx + 2
        prev_nonblank = idx - 1
        while prev_nonblank >= 0 and not lines[prev_nonblank].strip():
            prev_nonblank -= 1
        ignored_by_comment = (
            prev_nonblank >= 0 and "check-docs: ignore" in lines[prev_nonblank]
        ) or ("check-docs-ignore" in info)

        idx += 1
        body: list[str] = []
        while idx < len(lines):
            stripped = lines[idx].strip()
            if stripped.startswith(fence[0] * len(fence)) and set(stripped) == {fence[0]}:
                break
            body.append(lines[idx])
            idx += 1
        idx += 1

        lang = info.split()[0] if info else ""
        if lang != "qml" or ignored_by_comment:
            continue
        code = "\n".join(body).strip() + "\n"
        imports = IMPORT_RE.findall(code)
        for imp in imports:
            if imp == "QtQuick" or imp.startswith("QtQuick."):
                errors.append(
                    f"forbidden QtQuick import in {md_path.relative_to(REPO_ROOT)}:{start_line}: import {imp}"
                )
        if not any(imp == "QmlTui" or imp.startswith("QmlTui.") for imp in imports):
            continue
        if re.search(r"^\s*Terminal\s*\{", code, re.MULTILINE):
            if not re.search(r"\bQt\.quit\s*\(", code):
                errors.append(
                    f"complete Terminal QML snippet in {md_path.relative_to(REPO_ROOT)}:{start_line} is missing a keyboard quit path (Qt.quit())"
                )
        if any(imp not in ALLOWED_SNIPPET_IMPORTS for imp in imports):
            continue
        snippets.append((start_line, code))
    return snippets


def uses_private_qt_syntax(code: str) -> bool:
    if re.search(r"\bBehavior\s+on\b", code):
        return True
    for match in re.finditer(r"\bPropertyChanges\s*\{([^}]*)\}", code, re.DOTALL):
        body = match.group(1)
        if not re.search(r"\bproperties\s*:", body):
            return True
    return False


def check_qml_snippets(build_dir: Path, explicit_qmllint: Path | None, errors: list[str]) -> int:
    qml_import_dir = build_dir / "qml"
    core_qmldir = qml_import_dir / "QmlTui/qmldir"
    if not core_qmldir.is_file():
        errors.append(f"missing built QML module directory: {core_qmldir}")
        return 0

    qmllint = discover_qmllint(build_dir, explicit_qmllint)
    if qmllint is None:
        errors.append(
            "could not locate qmllint; pass --qmllint <path> or set QMLTUI_QMLLINT"
        )
        return 0

    has_image = (qml_import_dir / "QmlTui/Image/qmldir").is_file()
    has_ptyview = (qml_import_dir / "QmlTui/PtyView/qmldir").is_file()
    has_private_qt = cache_private_qt_enabled(build_dir)

    pending: list[tuple[Path, int, str]] = []
    for md_path in iter_doc_files(include_historical=False):
        for start_line, code in extract_qml_snippets(md_path, errors):
            imports = set(IMPORT_RE.findall(code))
            if "QmlTui.Image" in imports and not has_image:
                continue
            if "QmlTui.PtyView" in imports and not has_ptyview:
                continue
            if not has_private_qt and uses_private_qt_syntax(code):
                continue
            pending.append((md_path, start_line, code))

    if not pending:
        return 0

    with tempfile.TemporaryDirectory(prefix="qmltui-doc-snippets-") as tmp:
        tmp_dir = Path(tmp)
        file_map: dict[str, tuple[Path, int]] = {}
        qml_paths: list[str] = []
        for index, (md_path, start_line, code) in enumerate(pending):
            snippet_path = tmp_dir / f"Snippet{index:03d}.qml"
            snippet_path.write_text(code, encoding="utf-8")
            file_map[snippet_path.name] = (md_path, start_line)
            qml_paths.append(str(snippet_path))

        cmd = [
            str(qmllint),
            "-I",
            str(qml_import_dir),
            "--unqualified",
            "disable",
            *qml_paths,
        ]
        result = subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode != 0:
            combined = (result.stdout + "\n" + result.stderr).strip()
            mapped_lines: list[str] = []
            for out_line in combined.splitlines():
                replaced = out_line
                for temp_name, (orig_path, orig_start) in file_map.items():
                    m = re.search(rf"([^\s:]*{re.escape(temp_name)}):(\d+)", replaced)
                    if m:
                        snippet_line = int(m.group(2))
                        doc_line = orig_start + snippet_line - 1
                        rel = orig_path.relative_to(REPO_ROOT)
                        replaced = (
                            replaced[: m.start()]
                            + f"{rel}:{doc_line}"
                            + replaced[m.end() :]
                        )
                mapped_lines.append(replaced)
            errors.append("qmllint failed on documentation QML snippets:\n" + "\n".join(mapped_lines))

    return len(pending)


def check_nav_manifest(errors: list[str]) -> int:
    nav_path = REPO_ROOT / "docs/nav.json"
    if not nav_path.is_file():
        errors.append("missing docs/nav.json sidebar manifest")
        return 0
    data = json.loads(nav_path.read_text(encoding="utf-8"))
    docs_root = REPO_ROOT / "docs"
    seen_links: set[str] = set()
    checked = 0

    def walk(nodes: list[dict]) -> None:
        nonlocal checked
        for node in nodes:
            link = node.get("link")
            if link and not node.get("websiteOnly"):
                checked += 1
                clean = link.split("#", 1)[0].split("?", 1)[0]
                seen_links.add(clean.rstrip("/") or "/")
                if clean == "/":
                    target = docs_root / "README.md"
                elif clean.endswith("/"):
                    target = docs_root / clean.lstrip("/") / "index.md"
                else:
                    target = docs_root / f"{clean.lstrip('/')}.md"
                if not target.is_file():
                    errors.append(f"docs/nav.json link {link!r} does not match a file ({target})")
            if isinstance(node.get("items"), list):
                walk(node["items"])

    walk(data.get("sidebar", []))

    for subdir in ("guides", "qml-types", "cpp-classes"):
        for md_file in sorted((docs_root / subdir).glob("*.md")):
            expected = f"/{subdir}" if md_file.name == "index.md" else f"/{subdir}/{md_file.stem}"
            if expected not in seen_links:
                errors.append(f"{md_file.relative_to(REPO_ROOT)} is missing from docs/nav.json")

    return checked


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=REPO_ROOT / "build",
        help="configured build tree containing built QML modules (default: build)",
    )
    parser.add_argument(
        "--qmllint",
        type=Path,
        default=None,
        help="optional explicit path to the qmllint executable",
    )
    args = parser.parse_args()
    build_dir = args.build_dir
    if not build_dir.is_absolute():
        build_dir = REPO_ROOT / build_dir

    errors: list[str] = []
    links = check_markdown_links(errors)
    nav_links = check_nav_manifest(errors)
    gallery_count, recipe_count = check_examples_coverage(errors)
    cpp_frontmatter = check_frontmatter_cpp_types(errors)
    snippets = check_qml_snippets(build_dir, args.qmllint, errors)

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1

    print(
        f"Documentation checks passed: {links} links/anchors, {nav_links} nav entries, "
        f"{gallery_count} gallery examples, {recipe_count} recipes, "
        f"{cpp_frontmatter} frontmatter C++ symbols, and {snippets} QML snippets verified with qmllint."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
