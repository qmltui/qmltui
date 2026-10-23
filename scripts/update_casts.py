#!/usr/bin/env python3
"""Regenerate committed Asciinema v2 (.cast) recordings using a configured build tree."""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent

USAGE_TEXT = """\
Usage: scripts/update_casts.py [OPTIONS] [build-dir]

Regenerate committed Asciinema v2 (.cast) recordings using a configured build tree.

Options:
  --gallery       Regenerate only docs/assets/casts/gallery-tour.cast
  --hello-world   Regenerate only docs/assets/casts/hello-world.cast
  --components    Regenerate only docs/assets/casts/components/*.cast
  --all           Regenerate all casts (default when no scope flag is given)
  -j, --jobs N    Parallel CTest jobs for component scenarios (default: CPU count)
  -h, --help      Show this help message"""


def print_usage(exit_code: int = 0) -> int:
    stream = sys.stderr if exit_code != 0 else sys.stdout
    print(USAGE_TEXT, file=stream)
    return exit_code


def detect_jobs() -> int:
    env_jobs = os.environ.get("CTEST_PARALLEL_LEVEL", "").strip()
    if env_jobs.isdigit() and int(env_jobs) > 0:
        return int(env_jobs)
    cpu_count = os.cpu_count()
    return cpu_count if cpu_count and cpu_count > 0 else 4


def has_cache_line(cache_text: str, pattern: str) -> bool:
    return re.search(pattern, cache_text, re.MULTILINE) is not None


def find_component_casts(build_dir: Path) -> list[Path]:
    casts: list[Path] = []
    casts.extend(sorted(build_dir.glob("examples/gallery/RunnerArtifacts/*/components/*.cast")))
    casts.extend(sorted(build_dir.glob("examples/recipes/*/RunnerArtifacts/*/components/*.cast")))
    return casts


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)

    do_gallery = False
    do_hello_world = False
    do_components = False
    scope_selected = False
    build_dir_arg: str | None = None
    jobs_str: str | None = os.environ.get("CTEST_PARALLEL_LEVEL") or None

    idx = 0
    while idx < len(args):
        arg = args[idx]
        if arg == "--gallery":
            do_gallery = True
            scope_selected = True
            idx += 1
        elif arg in ("--hello-world", "--helloworld"):
            do_hello_world = True
            scope_selected = True
            idx += 1
        elif arg == "--components":
            do_components = True
            scope_selected = True
            idx += 1
        elif arg == "--all":
            do_gallery = True
            do_hello_world = True
            do_components = True
            scope_selected = True
            idx += 1
        elif arg in ("-j", "--jobs"):
            if idx + 1 >= len(args):
                print(f"error: {arg} requires a positive integer argument", file=sys.stderr)
                return print_usage(1)
            jobs_str = args[idx + 1]
            idx += 2
        elif arg.startswith("--jobs="):
            jobs_str = arg.split("=", 1)[1]
            idx += 1
        elif arg in ("-h", "--help"):
            return print_usage(0)
        elif arg.startswith("-"):
            print(f"error: unknown option '{arg}'", file=sys.stderr)
            return print_usage(1)
        else:
            if build_dir_arg is not None:
                print(
                    f"error: unexpected extra argument '{arg}' (build-dir already set to '{build_dir_arg}')",
                    file=sys.stderr,
                )
                return print_usage(1)
            build_dir_arg = arg
            idx += 1

    if not scope_selected:
        do_gallery = True
        do_hello_world = True
        do_components = True

    build_dir = Path(build_dir_arg) if build_dir_arg else REPO_ROOT / "build"

    if jobs_str is None:
        jobs = detect_jobs()
    else:
        if not jobs_str.isdigit() or int(jobs_str) <= 0:
            print(
                f"error: invalid job count '{jobs_str}' (expected a positive integer)",
                file=sys.stderr,
            )
            return 1
        jobs = int(jobs_str)

    ctest_bin = shutil.which("ctest")
    if not ctest_bin:
        print("error: ctest not found in PATH", file=sys.stderr)
        return 1

    if not build_dir.is_dir() or not (build_dir / "CTestTestfile.cmake").is_file():
        print(f"error: '{build_dir}' is not a configured CTest build directory.", file=sys.stderr)
        print("Configure and build first:", file=sys.stderr)
        print(
            f'  cmake -S . -B "{build_dir}" -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON',
            file=sys.stderr,
        )
        print(f'  cmake --build "{build_dir}"', file=sys.stderr)
        return 1

    cache_file = build_dir / "CMakeCache.txt"
    if cache_file.is_file():
        cache_text = cache_file.read_text(encoding="utf-8", errors="replace")
        if has_cache_line(cache_text, r"^QMLTUI_BUILD_EXAMPLES:BOOL=OFF$"):
            print(
                f"error: QMLTUI_BUILD_EXAMPLES is OFF in '{build_dir}'; reconfigure with -DQMLTUI_BUILD_EXAMPLES=ON",
                file=sys.stderr,
            )
            return 1
        if has_cache_line(cache_text, r"^QMLTUI_BUILD_TESTS:BOOL=OFF$") and not has_cache_line(
            cache_text, r"^QMLTUI_ENABLE_RUNNER:BOOL=ON$"
        ):
            print(
                f"error: Runner scenarios are disabled in '{build_dir}'; reconfigure with -DQMLTUI_BUILD_TESTS=ON",
                file=sys.stderr,
            )
            return 1
        for opt in ("QMLTUI_ENABLE_PRIVATE_QT", "QMLTUI_ENABLE_IMAGE", "QMLTUI_ENABLE_PTY_VIEW"):
            if has_cache_line(cache_text, rf"^{opt}:BOOL=OFF$"):
                print(
                    f"warning: {opt}=OFF in '{build_dir}'; casts for that module will not be regenerated",
                    file=sys.stderr,
                )
        if not has_cache_line(cache_text, r"^QMLTUI_ENABLE_PRIVATE_QT:BOOL=OFF$") and (
            not has_cache_line(cache_text, r"^Qt6QmlPrivate_DIR:PATH=")
            or has_cache_line(cache_text, r"^Qt6QmlPrivate_DIR:PATH=.*-NOTFOUND$")
        ):
            print(
                f"warning: Qt6::QmlPrivate not found in '{build_dir}'; casts requiring private Qt will not be regenerated",
                file=sys.stderr,
            )

    casts_dir = REPO_ROOT / "docs" / "assets" / "casts"
    components_dir = casts_dir / "components"
    components_dir.mkdir(parents=True, exist_ok=True)

    if do_gallery:
        print("==> gallery tour")
        gallery_cast = (
            build_dir / "examples" / "gallery" / "RunnerArtifacts" / "gallery_tour" / "gallery-tour.cast"
        )
        gallery_cast.unlink(missing_ok=True)
        res = subprocess.run(
            [ctest_bin, "--test-dir", str(build_dir), "-R", "^gallery_tour$", "--output-on-failure"],
            check=False,
        )
        if res.returncode != 0:
            return res.returncode
        if not gallery_cast.is_file() or gallery_cast.stat().st_size == 0:
            print(f"error: gallery_tour did not produce {gallery_cast}", file=sys.stderr)
            return 1
        shutil.copyfile(gallery_cast, casts_dir / "gallery-tour.cast")
        print("    docs/assets/casts/gallery-tour.cast")

    if do_hello_world:
        print("==> hello world")
        hello_cast = (
            build_dir / "examples" / "helloworld" / "RunnerArtifacts" / "hello_world" / "hello-world.cast"
        )
        hello_cast.unlink(missing_ok=True)
        res = subprocess.run(
            [ctest_bin, "--test-dir", str(build_dir), "-R", "^hello_world$", "--output-on-failure"],
            check=False,
        )
        if res.returncode != 0:
            return res.returncode
        if not hello_cast.is_file() or hello_cast.stat().st_size == 0:
            print(f"error: hello_world did not produce {hello_cast}", file=sys.stderr)
            return 1
        shutil.copyfile(hello_cast, casts_dir / "hello-world.cast")
        print("    docs/assets/casts/hello-world.cast")

    if do_components:
        print("==> component demos")
        for stale_cast in find_component_casts(build_dir):
            stale_cast.unlink(missing_ok=True)

        res = subprocess.run(
            [
                ctest_bin,
                "--test-dir",
                str(build_dir),
                "-j",
                str(jobs),
                "-L",
                "Cast",
                "-E",
                "^(gallery_tour|hello_world|test_runner_.*)$",
                "--output-on-failure",
            ],
            check=False,
        )
        if res.returncode != 0:
            return res.returncode

        copied = 0
        for cast_file in find_component_casts(build_dir):
            if not cast_file.is_file() or cast_file.stat().st_size == 0:
                continue
            name = cast_file.name
            shutil.copyfile(cast_file, components_dir / name)
            print(f"    docs/assets/casts/components/{name}")
            copied += 1

        if copied == 0:
            print(f"error: no component casts were generated in '{build_dir}'", file=sys.stderr)
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
