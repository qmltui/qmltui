#!/usr/bin/env python3
"""Compile and render every QML example in docs/qml-types.

Each ```qml fenced block that is a complete file (it has an import line)
becomes a Main.qml in its own directory and is run
through qmltui_doc_example_runner with QMLTUI_RUNNER_CAPTURE_AFTER_MS set, so the block
has to parse, resolve every type it imports, build its item tree with a
Terminal root, and commit a frame. The runner's result.json decides.

    check_doc_examples.py --runner build/tests/Docs/qmltui_doc_example_runner \
        --qml-import-path build/qml --artifact-dir build/tests/Docs/DocExampleArtifacts

Registered as test_component_doc_examples (Plan 04 section 2, rule 1).
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT / "scripts"))
from component_docs import OPTIONAL_TYPES, _cache_option

PAGES_DIR = REPO_ROOT / "docs/qml-types"
_BLOCK = re.compile(r"^```qml\s*\n(.*?)^```", re.MULTILINE | re.DOTALL)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--qml-import-path", type=Path, required=True)
    parser.add_argument("--artifact-dir", type=Path, required=True)
    parser.add_argument("--only", help="run only pages whose slug contains this text")
    parser.add_argument("--capture-after-ms", type=int, default=100,
                        help="settle time before the frame is captured (timers, models)")
    parser.add_argument("--include-drafts", action="store_true",
                        help="also run examples on pages that still contain _TODO_ placeholders")
    args = parser.parse_args()

    if args.artifact_dir.exists():
        shutil.rmtree(args.artifact_dir)
    args.artifact_dir.mkdir(parents=True)

    env = dict(os.environ)
    env.update({
        "QMLTUI_RUNNER_CAPTURE_AFTER_MS": str(args.capture_after_ms),
        "QMLTUI_RUNNER_COLUMNS": "80",
        "QMLTUI_RUNNER_ROWS": "24",
        "QML_IMPORT_PATH": str(args.qml_import_path),
        "QML2_IMPORT_PATH": str(args.qml_import_path),
        "QT_FORCE_STDERR_LOGGING": "1",
    })
    for name in ("QMLTUI_RUNNER_SCRIPT", "QMLTUI_RUNNER_EVAL", "QMLTUI_RUNNER_LIVE", "QMLTUI_RUNNER_EMBEDDED_SCENARIO", "QMLTUI_RUNNER_RECORD"):
        env.pop(name, None)

    build_dir = args.qml_import_path.parent
    disabled_types = {name.lower() for name, opt in OPTIONAL_TYPES.items()
                      if _cache_option(build_dir, opt) is False}

    failures: list[str] = []
    total = 0
    skipped = 0
    fragments = 0
    for page in sorted([*PAGES_DIR.glob("*.md"), *(REPO_ROOT / "docs/topics").glob("*.md"), *(REPO_ROOT / "docs/tutorials").glob("*.md")]):
        if args.only and args.only not in page.stem:
            continue
        if page.stem.lower() in disabled_types:
            skipped += 1
            continue
        text = page.read_text(encoding="utf-8")
        if "_TODO_" in text and not args.include_drafts:
            # A page still being written has a placeholder example; the
            # component_docs.py --strict gate reports those pages instead.
            skipped += 1
            continue
        for index, match in enumerate(_BLOCK.finditer(text), start=1):
            body = match.group(1)
            if not re.search(r"^import\s", body, re.MULTILINE):
                fragments += 1     # an illustrative snippet, not a complete file
                continue
            # If the snippet imports a QmlTui module not built here, skip it.
            imports = re.findall(r"^import\s+([A-Za-z0-9_.]+)", body, re.MULTILINE)
            if any(imp.startswith("QmlTui") and not args.qml_import_path.joinpath(*imp.split(".")).joinpath("qmldir").is_file() for imp in imports):
                skipped += 1
                continue
            # If the snippet uses a type disabled in this build, skip it.
            if any(re.search(rf"\b{re.escape(name)}\b", body) for name, opt in OPTIONAL_TYPES.items() if _cache_option(build_dir, opt) is False):
                skipped += 1
                continue
            total += 1
            name = f"{page.stem}-{index}"
            work = args.artifact_dir / name
            work.mkdir()
            main_qml = work / "Main.qml"
            main_qml.write_text(match.group(1), encoding="utf-8")
            env["QMLTUI_RUNNER_ARTIFACT_DIR"] = str(work)
            try:
                run = subprocess.run([str(args.runner), str(main_qml)], env=env,
                                     capture_output=True, text=True, timeout=30)
            except subprocess.TimeoutExpired:
                failures.append(f"{name}: timed out")
                continue
            result_path = work / "result.json"
            result = json.loads(result_path.read_text()) if result_path.is_file() else {}
            if run.returncode != 0 or result.get("status") != "passed":
                detail = (run.stderr.strip().splitlines() or ["(no stderr)"])[-6:]
                failures.append(f"{name}: exit {run.returncode}, status {result.get('status')!r}\n    "
                                + "\n    ".join(detail))
                continue
            frame = (work / "capture.txt").read_text(encoding="utf-8") if (work / "capture.txt").is_file() else ""
            if not frame.strip():
                failures.append(f"{name}: rendered an empty frame")
    for failure in failures:
        print(f"ERROR: {failure}")
    print(f"{total - len(failures)}/{total} documentation examples load and render"
          + (f"; {skipped} draft page(s) with _TODO_ skipped" if skipped else "")
          + (f"; {fragments} snippet(s) without an import line not run" if fragments else "")
          + f" (artifacts under {args.artifact_dir})")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
