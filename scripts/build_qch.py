#!/usr/bin/env python3
"""Builds Qt Help Project (.qhp) and HTML documentation from markdown sources.

This script parses docs/qml-types, docs/cpp-classes, docs/topics, and docs/tutorials,
generates self-contained HTML files with a Qt-style CSS stylesheet, extracts
symbol keywords (QML types, properties, signals, C++ classes and methods),
and writes qmltui.qhp ready for qhelpgenerator.
"""

from __future__ import annotations

import argparse
import html
import os
import re
import sys
from pathlib import Path

CSS_STYLESHEET = """
body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen, Ubuntu, Cantarell, monospace, sans-serif;
    color: #c9d1d9;
    background-color: #0d1117;
    line-height: 1.6;
    padding: 20px 40px;
    margin: 0 auto;
    max-width: 960px;
}
h1, h2, h3, h4 {
    color: #58a6ff;
    border-bottom: 1px solid #21262d;
    padding-bottom: 0.3em;
}
h1 { color: #2cde85; }
a { color: #58a6ff; text-decoration: none; }
a:hover { text-decoration: underline; }
pre, code {
    font-family: "JetBrains Mono", "Fira Code", monospace;
    background-color: #161b22;
    color: #f0f6fc;
}
pre {
    padding: 16px;
    border-radius: 6px;
    overflow: auto;
    border: 1px solid #30363d;
}
code {
    padding: 0.2em 0.4em;
    border-radius: 3px;
    font-size: 85%;
}
pre code {
    padding: 0;
    font-size: 100%;
    background-color: transparent;
}
table {
    border-collapse: collapse;
    width: 100%;
    margin: 16px 0;
}
th, td {
    border: 1px solid #30363d;
    padding: 8px 12px;
    text-align: left;
}
th {
    background-color: #161b22;
    color: #58a6ff;
}
tr:nth-child(even) { background-color: #0d1117; }
tr:nth-child(odd) { background-color: #161b22; }
blockquote {
    border-left: 4px solid #2cde85;
    padding-left: 16px;
    color: #8b949e;
    margin: 16px 0;
}
pre code.language-text {
    line-height: 1.25;
}
details {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 8px 14px;
    margin: 12px 0;
}
summary {
    cursor: pointer;
    font-weight: 600;
    color: #58a6ff;
}
"""

def parse_frontmatter(text: str) -> tuple[dict[str, str], str]:
    meta = {}
    body = text
    if text.startswith("---"):
        parts = text.split("---", 2)
        if len(parts) >= 3:
            for line in parts[1].strip().splitlines():
                if ":" in line:
                    k, v = line.split(":", 1)
                    meta[k.strip()] = v.strip().strip('"').strip("'")
            body = parts[2]
    return meta, body

def simple_markdown_to_html(md: str) -> str:
    """Fallback markdown parser if python-markdown is not installed."""
    lines = md.splitlines()
    html_lines = []
    in_code_block = False
    in_table = False
    table_header_done = False
    in_list = False

    for line in lines:
        stripped = line.strip()

        # Code blocks
        if stripped.startswith("```"):
            if in_code_block:
                html_lines.append("</code></pre>")
                in_code_block = False
            else:
                lang = stripped[3:].strip()
                html_lines.append(f'<pre><code class="language-{html.escape(lang)}">')
                in_code_block = True
            continue

        if in_code_block:
            html_lines.append(html.escape(line))
            continue

        # Tables
        if stripped.startswith("|") and stripped.endswith("|"):
            cells = [c.strip() for c in stripped[1:-1].split("|")]
            # Check for delimiter line
            if all(re.match(r"^:?-+:?$", c) for c in cells):
                table_header_done = True
                continue
            if not in_table:
                html_lines.append("<table>")
                in_table = True
                table_header_done = False

            tag = "th" if not table_header_done else "td"
            row_html = "".join(f"<{tag}>{format_inline(c)}</{tag}>" for c in cells)
            html_lines.append(f"<tr>{row_html}</tr>")
            continue
        elif in_table:
            html_lines.append("</table>")
            in_table = False

        # Lists
        if stripped.startswith("- ") or stripped.startswith("* "):
            if not in_list:
                html_lines.append("<ul>")
                in_list = True
            item_text = stripped[2:].strip()
            html_lines.append(f"<li>{format_inline(item_text)}</li>")
            continue
        elif in_list:
            html_lines.append("</ul>")
            in_list = False

        if not stripped:
            continue

        # Pass through <details> and <summary> tags cleanly without escaping
        summary_match = re.match(r"^<summary(?:\s+[^>]*)?>(.*)</summary>$", stripped, re.IGNORECASE)
        if summary_match:
            html_lines.append(f"<summary>{format_inline(summary_match.group(1))}</summary>")
            continue
        if re.match(r"^</?(?:details|summary)(?:\s+[^>]*)?>$", stripped, re.IGNORECASE):
            html_lines.append(stripped)
            continue

        # Headings (# through ######, including #### -> <h4>)
        heading_match = re.match(r"^(#{1,6})\s+(.*)$", stripped)
        if heading_match:
            level = len(heading_match.group(1))
            heading_text = heading_match.group(2).strip()
            slug = re.sub(r"[^a-zA-Z0-9_-]", "", heading_text.lower().replace(" ", "-"))
            html_lines.append(f'<h{level} id="{slug}">{format_inline(heading_text)}</h{level}>')
            continue

        # Blockquotes
        if stripped.startswith(">"):
            quote_text = stripped.lstrip(">").strip()
            html_lines.append(f"<blockquote>{format_inline(quote_text)}</blockquote>")
            continue

        # Regular paragraph
        html_lines.append(f"<p>{format_inline(stripped)}</p>")

    if in_code_block:
        html_lines.append("</code></pre>")
    if in_table:
        html_lines.append("</table>")
    if in_list:
        html_lines.append("</ul>")

    return "\n".join(html_lines)

def format_inline(text: str) -> str:
    # Protect inline code spans first so inner characters are not altered by bold/link regexes
    code_spans: list[str] = []
    def save_code(m: re.Match) -> str:
        code_spans.append(f"<code>{html.escape(m.group(1))}</code>")
        return f"\x00CODE{len(code_spans) - 1}\x00"

    text = re.sub(r"`([^`]+)`", save_code, text)

    # Protect <details> and <summary> tags so they pass through cleanly without escaping
    preserved_tags: list[str] = []
    def save_tag(m: re.Match) -> str:
        preserved_tags.append(m.group(0))
        return f"\x00TAG{len(preserved_tags) - 1}\x00"

    text = re.sub(r"</?(?:details|summary)(?:\s+[^>]*)?>", save_tag, text, flags=re.IGNORECASE)

    # Inline markdown links [text](target.md) and [text](target.md#anchor)
    def link_repl(m: re.Match) -> str:
        label, url = m.group(1), m.group(2).strip()
        if not url.startswith(("http://", "https://", "mailto:")):
            url = re.sub(r"\.md(#|$)", r".html\1", url)
        # Format bold/italic inside link label if present
        label = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", label)
        label = re.sub(r"\*([^*]+)\*", r"<em>\1</em>", label)
        return f'<a href="{html.escape(url)}">{label}</a>'

    text = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", link_repl, text)

    # Bold **text**
    text = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", text)
    # Italic *text*
    text = re.sub(r"\*([^*]+)\*", r"<em>\1</em>", text)

    # Restore preserved HTML tags and inline code spans
    for idx, tag in enumerate(preserved_tags):
        text = text.replace(f"\x00TAG{idx}\x00", tag)
    for idx, span in enumerate(code_spans):
        text = text.replace(f"\x00CODE{idx}\x00", span)
    return text

def convert_markdown(text: str) -> str:
    text = re.sub(r"<!--.*?-->", "", text, flags=re.DOTALL)
    try:
        import markdown
        rendered = markdown.markdown(text, extensions=["tables", "fenced_code", "toc"])
        # Ensure local .md links are converted to .html even when python-markdown is used
        rendered = re.sub(
            r'href="(?!https?://|mailto:)([^"]+?)\.md(#[^"]*)?"',
            lambda m: f'href="{m.group(1)}.html{m.group(2) or ""}"',
            rendered,
        )
        return rendered
    except ImportError:
        return simple_markdown_to_html(text)

def read_project_version(source_dir: Path) -> str:
    cmake_lists = source_dir / "CMakeLists.txt"
    if cmake_lists.is_file():
        m = re.search(
            r"project\s*\(\s*\w+\s+VERSION\s+([0-9]+(?:\.[0-9]+)*)",
            cmake_lists.read_text(encoding="utf-8"),
            re.IGNORECASE,
        )
        if m:
            return m.group(1)
    return "0.5.0"

def main():
    repo_root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description="Build Qt Help Project (.qhp) and HTML docs")
    parser.add_argument("--source", "--source-dir", dest="source", default=None, help="Root of repository")
    parser.add_argument("--docs-dir", dest="docs_dir", default=None, help="Documentation directory (default: <source>/docs)")
    parser.add_argument("--out", "--output-dir", dest="out", required=True, help="Output directory for generated files")
    args = parser.parse_args()

    if args.docs_dir:
        docs_dir = Path(args.docs_dir).resolve()
        source_dir = Path(args.source).resolve() if args.source else docs_dir.parent
    else:
        source_dir = Path(args.source).resolve() if args.source else repo_root
        docs_dir = source_dir / "docs"

    out_dir = Path(args.out).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    version = read_project_version(source_dir)

    # Write stylesheet
    (out_dir / "qmltui-doc.css").write_text(CSS_STYLESHEET, encoding="utf-8")

    files_list: list[str] = ["qmltui-doc.css"]
    keywords: list[tuple[str, str, str]] = []  # (name, id, ref)
    toc_entries: dict[str, list[tuple[str, str]]] = {
        "Getting Started": [],
        "Tutorials": [],
        "Topics": [],
        "QML Types": [],
        "C++ Classes": [],
    }

    # Helper to process a directory of markdown files
    def process_folder(subfolder: str, category: str):
        in_folder = docs_dir / subfolder
        if not in_folder.exists():
            return
        target_folder = out_dir / subfolder
        target_folder.mkdir(parents=True, exist_ok=True)

        for md_file in sorted(in_folder.glob("*.md")):
            rel_name = f"{subfolder}/{md_file.stem}.html"
            content = md_file.read_text(encoding="utf-8")
            meta, body = parse_frontmatter(content)
            title = meta.get("title") or md_file.stem.capitalize()

            # Keywords
            if subfolder == "qml-types":
                type_name = meta.get("title") or md_file.stem.capitalize()
                keywords.append((type_name, f"QML.{type_name}", rel_name))
                # Check for properties in tables
                for match in re.finditer(r"\|\s*`([a-zA-Z0-9_]+)`\s*\|", body):
                    prop_name = match.group(1)
                    if prop_name not in ("Property", "Type", "Default", "Signal", "Method"):
                        keywords.append((f"{type_name}.{prop_name}", f"QML.{type_name}.{prop_name}", f"{rel_name}#{prop_name}"))
            elif subfolder == "cpp-classes":
                cls_name = meta.get("title") or f"qmltui::{md_file.stem.capitalize()}"
                keywords.append((cls_name, cls_name, rel_name))

            body_html = convert_markdown(body)
            full_html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>{html.escape(title)} - QmlTui Documentation</title>
    <link rel="stylesheet" href="../qmltui-doc.css">
</head>
<body>
    {body_html}
</body>
</html>
"""
            out_file = target_folder / f"{md_file.stem}.html"
            out_file.write_text(full_html, encoding="utf-8")
            files_list.append(rel_name)
            toc_entries[category].append((title, rel_name))

    # Process all categories
    process_folder("qml-types", "QML Types")
    process_folder("cpp-classes", "C++ Classes")
    process_folder("topics", "Topics")
    process_folder("tutorials", "Tutorials")

    # Getting started
    gs_file = docs_dir / "getting-started.md"
    if gs_file.exists():
        content = gs_file.read_text(encoding="utf-8")
        meta, body = parse_frontmatter(content)
        body_html = convert_markdown(body)
        full_html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Getting Started - QmlTui Documentation</title>
    <link rel="stylesheet" href="qmltui-doc.css">
</head>
<body>
    {body_html}
</body>
</html>
"""
        (out_dir / "getting-started.html").write_text(full_html, encoding="utf-8")
        files_list.append("getting-started.html")
        toc_entries["Getting Started"].append(("Getting Started", "getting-started.html"))
        keywords.append(("Getting Started", "getting-started", "getting-started.html"))

    # Generate qmltui.qhp
    toc_xml_parts = []
    for section_title, entries in toc_entries.items():
        if not entries:
            continue
        toc_xml_parts.append(f'        <section title="{html.escape(section_title)}">')
        for title, ref in entries:
            toc_xml_parts.append(f'            <section title="{html.escape(title)}" ref="{html.escape(ref)}"/>')
        toc_xml_parts.append('        </section>')

    keywords_xml_parts = []
    for name, kw_id, ref in keywords:
        keywords_xml_parts.append(f'        <keyword name="{html.escape(name)}" id="{html.escape(kw_id)}" ref="{html.escape(ref)}"/>')

    files_xml_parts = [f'        <file>{html.escape(f)}</file>' for f in files_list]

    qhp_content = f"""<?xml version="1.0" encoding="UTF-8"?>
<QtHelpProject version="1.0">
    <namespace>org.qmltui.qmltui.{html.escape(version)}</namespace>
    <virtualFolder>qmltui</virtualFolder>
    <filterSection>
        <toc>
            <section title="QmlTui Reference Documentation" ref="getting-started.html">
{os.linesep.join(toc_xml_parts)}
            </section>
        </toc>
        <keywords>
{os.linesep.join(keywords_xml_parts)}
        </keywords>
        <files>
{os.linesep.join(files_xml_parts)}
        </files>
    </filterSection>
</QtHelpProject>
"""
    (out_dir / "qmltui.qhp").write_text(qhp_content, encoding="utf-8")
    print(f"Generated {len(files_list)} HTML doc files, {len(keywords)} keywords, and qmltui.qhp in {out_dir}")

if __name__ == "__main__":
    main()
