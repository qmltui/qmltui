#!/usr/bin/env python3
"""Collects Clang source-based coverage across CTest runs and writes reports."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


MODULES = [
    ("Core", "src/QmlTui/Core/"),
    ("Controls", "src/QmlTui/Controls/"),
    ("Image", "src/QmlTui/Image/"),
    ("Runner", "src/QmlTui/Runner/"),
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


def badge_color(pct: float) -> str:
    if pct >= 90.0:
        return "#4c1"
    if pct >= 80.0:
        return "#97ca00"
    if pct >= 70.0:
        return "#a4a61d"
    if pct >= 60.0:
        return "#dfb317"
    if pct >= 50.0:
        return "#fe7d37"
    return "#e05d44"


def build_coverage_badge_svg(export_data: dict) -> str:
    data_entries = export_data.get("data", [])
    totals = data_entries[0].get("totals", {}) if data_entries else {}
    lines = totals.get("lines", {})
    covered = int(lines.get("covered", 0))
    count = int(lines.get("count", 0))

    if count > 0:
        pct = (covered / count) * 100.0
        value_text = f"{pct:.1f}%"
        color = badge_color(pct)
    else:
        value_text = "unknown"
        color = "#9f9f9f"

    label_text = "coverage"
    left_width = 61
    right_width = max(47, len(value_text) * 7 + 12)
    total_width = left_width + right_width
    left_center = (left_width / 2.0) * 10.0
    right_center = (left_width + right_width / 2.0) * 10.0
    left_text_len = (left_width - 10) * 10
    right_text_len = (right_width - 10) * 10
    aria_label = f"{label_text}: {value_text}"

    return f"""<svg xmlns="http://www.w3.org/2000/svg" width="{total_width}" height="20" role="img" aria-label="{aria_label}">
  <title>{aria_label}</title>
  <linearGradient id="s" x2="0" y2="100%">
    <stop offset="0" stop-color="#bbb" stop-opacity=".1"/>
    <stop offset="1" stop-opacity=".1"/>
  </linearGradient>
  <clipPath id="r">
    <rect width="{total_width}" height="20" rx="3" fill="#fff"/>
  </clipPath>
  <g clip-path="url(#r)">
    <rect width="{left_width}" height="20" fill="#555"/>
    <rect x="{left_width}" width="{right_width}" height="20" fill="{color}"/>
    <rect width="{total_width}" height="20" fill="url(#s)"/>
  </g>
  <g fill="#fff" text-anchor="middle" font-family="Verdana,Geneva,DejaVu Sans,sans-serif" text-rendering="geometricPrecision" font-size="110">
    <text aria-hidden="true" x="{left_center:.0f}" y="150" fill="#010101" fill-opacity=".3" transform="scale(.1)" textLength="{left_text_len}">{label_text}</text>
    <text x="{left_center:.0f}" y="140" transform="scale(.1)" fill="#fff" textLength="{left_text_len}">{label_text}</text>
    <text aria-hidden="true" x="{right_center:.0f}" y="150" fill="#010101" fill-opacity=".3" transform="scale(.1)" textLength="{right_text_len}">{value_text}</text>
    <text x="{right_center:.0f}" y="140" transform="scale(.1)" fill="#fff" textLength="{right_text_len}">{value_text}</text>
  </g>
</svg>
"""


def build_markdown_summary(export_data: dict, source_dir: Path) -> str:
    data_entries = export_data.get("data", [])
    if not data_entries:
        return "### Code Coverage\n\nNo coverage data exported.\n"

    entry = data_entries[0]
    files = entry.get("files", [])
    totals = entry.get("totals", {})

    src_prefix = source_dir.resolve().as_posix().rstrip("/") + "/"

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
        filename = f.get("filename", "").replace("\\", "/")
        rel = filename[len(src_prefix) :] if filename.startswith(src_prefix) else filename
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

    if not build_dir.is_dir():
        print(f"error: build directory does not exist: {build_dir}", file=sys.stderr)
        return 1
    if not source_dir.is_dir():
        print(f"error: source directory does not exist: {source_dir}", file=sys.stderr)
        return 1

    existing_objects: list[str] = []
    for obj in args.objects:
        if Path(obj).is_file():
            existing_objects.append(obj)
        else:
            print(f"warning: instrumented object not found, skipping: {obj}", file=sys.stderr)
    if not existing_objects:
        print("error: none of the specified --objects exist on disk; build the project first", file=sys.stderr)
        return 1

    try:
        if not args.skip_ctest:
            if coverage_dir.exists():
                shutil.rmtree(coverage_dir)
            profraw_dir.mkdir(parents=True, exist_ok=True)

            env = os.environ.copy()
            env["LLVM_PROFILE_FILE"] = str(profraw_dir / "%p-%m.profraw")
            subprocess.run(
                ["ctest", "--test-dir", str(build_dir), "-LE", "Demo", "--output-on-failure"],
                env=env,
                check=True,
            )
        else:
            coverage_dir.mkdir(parents=True, exist_ok=True)

        all_profraw = sorted(profraw_dir.glob("*.profraw"))
        profraw_files = [p for p in all_profraw if p.is_file() and p.stat().st_size > 0]
        if not profraw_files:
            print(f"error: no non-empty .profraw files found in {profraw_dir}", file=sys.stderr)
            return 1

        profraw_list_file = coverage_dir / "profraw_files.txt"
        profraw_list_file.write_text(
            "\n".join(str(p) for p in profraw_files) + "\n",
            encoding="utf-8",
        )

        profdata_file = coverage_dir / "qmltui.profdata"
        subprocess.run(
            [
                args.llvm_profdata,
                "merge",
                "-sparse",
                f"-input-files={profraw_list_file}",
                "-o",
                str(profdata_file),
            ],
            check=True,
        )

        source_paths = [
            str(p)
            for p in (
                source_dir / "src" / "QmlTui",
                source_dir / "tools" / "qmltui-runner",
            )
            if p.exists()
        ]
        object_args = [existing_objects[0], *[f"-object={obj}" for obj in existing_objects[1:]]]
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

        badge_svg = build_coverage_badge_svg(export_data)
        (coverage_dir / "coverage.svg").write_text(badge_svg, encoding="utf-8")

        step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
        if step_summary:
            with open(step_summary, "a", encoding="utf-8") as f:
                f.write(md_summary + "\n")
    except FileNotFoundError as exc:
        print(f"error: required tool not found: {exc}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as exc:
        if exc.stderr:
            print(exc.stderr, file=sys.stderr, end="")
        print(f"error: command failed with exit code {exc.returncode}: {exc.cmd}", file=sys.stderr)
        return exc.returncode or 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
