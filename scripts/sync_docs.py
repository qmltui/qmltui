#!/usr/bin/env python3
"""Synchronizes documentation and examples into website/src/content/

Ensures the website never duplicates content and uses docs/ and examples/
as the single source of truth. Rewrites relative markdown links for web URLs
and packages quickstart code snippets.
"""

from __future__ import annotations

import json
import os
import re
import shutil
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DOCS_DIR = REPO_ROOT / "docs"
WEBSITE_DIR = REPO_ROOT / "website"
CONTENT_DIR = WEBSITE_DIR / "src" / "content"
QUICKSTART_DIR = REPO_ROOT / "examples" / "quickstart"

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

def rewrite_links(text: str, current_section: str) -> str:
    """Rewrites relative markdown links to clean web URLs."""
    def link_repl(match):
        label, target = match.group(1), match.group(2)
        # e.g., ../qml-types/button.md -> /components/button
        # e.g., ./getting-started.md -> /docs/getting-started
        target = re.sub(r"\.md(#|$)", r"\1", target)
        if target.startswith("../qml-types/"):
            target = "/components/" + target.replace("../qml-types/", "")
        elif target.startswith("../cpp-classes/"):
            target = "/cpp/" + target.replace("../cpp-classes/", "")
        elif target.startswith("../topics/"):
            target = "/guide/" + target.replace("../topics/", "")
        elif target.startswith("../tutorials/"):
            target = "/tutorials/" + target.replace("../tutorials/", "")
        elif target.startswith("getting-started"):
            target = "/guide/getting-started"
        return f"[{label}]({target})"

    return re.sub(r"\[([^\]]+)\]\(([^)]+)\)", link_repl, text)

def sync_all():
    print(f"Syncing docs from {DOCS_DIR} into {CONTENT_DIR}...")
    CONTENT_DIR.mkdir(parents=True, exist_ok=True)

    # 1. Sync QML Types (Components)
    components_dir = CONTENT_DIR / "components"
    components_dir.mkdir(parents=True, exist_ok=True)
    component_index = []

    qml_types_src = DOCS_DIR / "qml-types"
    if qml_types_src.exists():
        for md_file in sorted(qml_types_src.glob("*.md")):
            if md_file.name == "index.md":
                continue
            content = md_file.read_text(encoding="utf-8")
            meta, body = parse_frontmatter(content)
            slug = md_file.stem
            meta["slug"] = slug
            meta["title"] = meta.get("title", slug.capitalize())
            meta["summary"] = meta.get("summary", "")
            meta["category"] = meta.get("category", "other")
            rewritten_body = rewrite_links(body, "components")

            # Write individual markdown
            (components_dir / f"{slug}.md").write_text(
                f"---\n{json.dumps(meta, indent=2)}\n---\n{rewritten_body}",
                encoding="utf-8"
            )
            component_index.append(meta)

    (CONTENT_DIR / "components.json").write_text(json.dumps(component_index, indent=2), encoding="utf-8")
    print(f"Synced {len(component_index)} QML component types.")

    # 2. Sync Guides and Topics
    guides_dir = CONTENT_DIR / "guides"
    guides_dir.mkdir(parents=True, exist_ok=True)
    topics_src = DOCS_DIR / "topics"
    guide_index = []
    if topics_src.exists():
        for md_file in sorted(topics_src.glob("*.md")):
            content = md_file.read_text(encoding="utf-8")
            meta, body = parse_frontmatter(content)
            slug = md_file.stem
            meta["slug"] = slug
            meta["title"] = meta.get("title", slug.replace("-", " ").title())
            rewritten_body = rewrite_links(body, "guide")
            (guides_dir / f"{slug}.md").write_text(
                f"---\n{json.dumps(meta, indent=2)}\n---\n{rewritten_body}",
                encoding="utf-8"
            )
            guide_index.append(meta)

    # Getting started
    gs_file = DOCS_DIR / "getting-started.md"
    if gs_file.exists():
        content = gs_file.read_text(encoding="utf-8")
        meta, body = parse_frontmatter(content)
        meta["slug"] = "getting-started"
        meta["title"] = "Getting Started"
        rewritten_body = rewrite_links(body, "guide")
        (guides_dir / "getting-started.md").write_text(
            f"---\n{json.dumps(meta, indent=2)}\n---\n{rewritten_body}",
            encoding="utf-8"
        )
        guide_index.insert(0, meta)

    (CONTENT_DIR / "guides.json").write_text(json.dumps(guide_index, indent=2), encoding="utf-8")
    print(f"Synced {len(guide_index)} guides.")

    # 3. Sync Quickstart Files (verbatim)
    quickstart_data = {}
    for filename in ["Main.qml", "main.cpp", "CMakeLists.txt"]:
        qs_file = QUICKSTART_DIR / filename
        if qs_file.exists():
            quickstart_data[filename] = qs_file.read_text(encoding="utf-8")

    (CONTENT_DIR / "quickstart.json").write_text(json.dumps(quickstart_data, indent=2), encoding="utf-8")
    print("Synced verbatim quickstart snippets.")

if __name__ == "__main__":
    sync_all()
