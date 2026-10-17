#!/usr/bin/env python3
"""Collects Clang source-based coverage across CTest runs and writes reports."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path


MODULES = [
    ("Core", "src/QmlTui/Core/"),
    ("Controls", "src/QmlTui/Controls/"),
    ("Image", "src/QmlTui/Image/"),
    ("Runner", "src/QmlTui/Runner/"),
    ("TerminalHost", "src/QmlTui/TerminalHost/"),
    ("qmltui-runner CLI", "tools/qmltui-runner/"),
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--source-dir", required=True, type=Path)
    parser.add_argument("--coverage-dir", required=True, type=Path)
    parser.add_argument("--llvm-profdata", default="llvm-profdata")
    parser.add_argument("--llvm-cov", default="llvm-cov")
    parser.add_argument("--ignore-regex", required=True)
    parser.add_argument("--skip-ctest", action="store_true")
    parser.add_argument("--objects", nargs="+", required=True)
    return parser.parse_args()


def fmt_pct(covered: int, count: int) -> str:
    if count == 0:
        return "—"
    return f"{(covered / count) * 100.0:.1f}% ({covered}/{count})"


def build_markdown_summary(export_data: dict, source_dir: Path) -> str:
    data_entries = export_data.get("data", [])
    if not data_entries:
        return "### Code Coverage\n\nNo coverage data exported.\n"

    entry = data_entries[0]
    files = entry.get("files", [])
    totals = entry.get("totals", {})

    src_prefix = str(source_dir.resolve()) + "/"

    module_stats: dict[str, dict[str, int]] = {
        name: {
            "lines_covered": 0,
            "lines_count": 0,
            "functions_covered": 0,
            "functions_count": 0,
            "regions_covered": 0,
            "regions_count": 0,
            "branches_covered": 0,
            "branches_count": 0,
            "file_count": 0,
        }
        for name, _ in MODULES
    }

    for f in files:
        filename = f.get("filename", "")
        rel = filename[len(src_prefix) :] if filename.startswith(src_prefix) else filename
        rel = rel.replace("\\", "/")
        summary = f.get("summary", {})
        for mod_name, mod_prefix in MODULES:
            if rel.startswith(mod_prefix):
                st = module_stats[mod_name]
                st["file_count"] += 1
                for metric in ("lines", "functions", "regions", "branches"):
                    m = summary.get(metric, {})
                    st[f"{metric}_covered"] += int(m.get("covered", 0))
                    st[f"{metric}_count"] += int(m.get("count", 0))
                break

    lines = [
        "## Code Coverage Summary",
        "",
        "| Module | Files | Lines | Functions | Regions | Branches |",
        "| :--- | ---: | ---: | ---: | ---: | ---: |",
    ]

    for mod_name, _ in MODULES:
        st = module_stats[mod_name]
        if st["file_count"] == 0:
            continue
        lines.append(
            f"| **{mod_name}** | {st['file_count']} "
            f"| {fmt_pct(st['lines_covered'], st['lines_count'])} "
            f"| {fmt_pct(st['functions_covered'], st['functions_count'])} "
            f"| {fmt_pct(st['regions_covered'], st['regions_count'])} "
            f"| {fmt_pct(st['branches_covered'], st['branches_count'])} |"
        )

    tot_lines = totals.get("lines", {})
    tot_funcs = totals.get("functions", {})
    tot_regions = totals.get("regions", {})
    tot_branches = totals.get("branches", {})
    total_files = sum(st["file_count"] for st in module_stats.values())

    lines.append(
        f"| **Total** | **{total_files}** "
        f"| **{fmt_pct(int(tot_lines.get('covered', 0)), int(tot_lines.get('count', 0)))}** "
        f"| **{fmt_pct(int(tot_funcs.get('covered', 0)), int(tot_funcs.get('count', 0)))}** "
        f"| **{fmt_pct(int(tot_regions.get('covered', 0)), int(tot_regions.get('count', 0)))}** "
        f"| **{fmt_pct(int(tot_branches.get('covered', 0)), int(tot_branches.get('count', 0)))}** |"
    )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    build_dir = args.build_dir.resolve()
    source_dir = args.source_dir.resolve()
    coverage_dir = args.coverage_dir.resolve()
    profraw_dir = coverage_dir / "profraw"

    if not args.skip_ctest:
        if coverage_dir.exists():
            shutil.rmtree(coverage_dir)
        profraw_dir.mkdir(parents=True, exist_ok=True)

        env = os.environ.copy()
        env["LLVM_PROFILE_FILE"] = str(profraw_dir / "%p-%m.profraw")
        subprocess.run(
            ["ctest", "--test-dir", str(build_dir), "--output-on-failure"],
            env=env,
            check=True,
        )

    profraw_files = sorted(profraw_dir.glob("*.profraw"))
    if not profraw_files:
        print(f"error: no .profraw files found in {profraw_dir}", file=sys.stderr)
        return 1

    profdata_file = coverage_dir / "qmltui.profdata"
    subprocess.run(
        [
            args.llvm_profdata,
            "merge",
            "-sparse",
            *[str(p) for p in profraw_files],
            "-o",
            str(profdata_file),
        ],
        check=True,
    )

    source_paths = [
        str(source_dir / "src" / "QmlTui"),
        str(source_dir / "tools" / "qmltui-runner"),
    ]
    object_args = [args.objects[0], *[f"-object={obj}" for obj in args.objects[1:]]]
    common_cov_args = [
        f"-instr-profile={profdata_file}",
        f"-ignore-filename-regex={args.ignore_regex}",
        *object_args,
        "--",
        *source_paths,
    ]

    report_out = subprocess.run(
        [args.llvm_cov, "report", *common_cov_args],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    (coverage_dir / "summary.txt").write_text(report_out, encoding="utf-8")
    print(report_out)

    json_out = subprocess.run(
        [args.llvm_cov, "export", "-summary-only", *common_cov_args],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    (coverage_dir / "summary.json").write_text(json_out, encoding="utf-8")

    lcov_out = subprocess.run(
        [args.llvm_cov, "export", "-format=lcov", *common_cov_args],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    (coverage_dir / "lcov.info").write_text(lcov_out, encoding="utf-8")

    html_dir = coverage_dir / "html"
    subprocess.run(
        [
            args.llvm_cov,
            "show",
            "-format=html",
            f"-output-dir={html_dir}",
            "-show-line-counts-or-regions",
            "-show-instantiations=false",
            *common_cov_args,
        ],
        check=True,
    )

    export_data = json.loads(json_out)
    md_summary = build_markdown_summary(export_data, source_dir)
    summary_md_path = coverage_dir / "summary.md"
    summary_md_path.write_text(md_summary, encoding="utf-8")
    print(md_summary)

    step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if step_summary:
        with open(step_summary, "a", encoding="utf-8") as f:
            f.write(md_summary + "\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
