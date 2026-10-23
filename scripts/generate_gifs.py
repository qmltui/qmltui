#!/usr/bin/env python3
"""Convert committed .cast files into GIFs (and gallery.svg) using scripts/render_cast_gif.py or agg."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
CASTS_DIR = REPO_ROOT / "docs" / "assets" / "casts"
RENDERER = REPO_ROOT / "scripts" / "render_cast_gif.py"

USAGE_TEXT = """\
Usage: scripts/generate_gifs.py [OPTIONS]

Convert committed .cast recordings into animated GIFs (and gallery.svg).

Options:
  --gallery       Convert only gallery-tour.cast -> gallery-tour.gif + gallery.svg
  --hello-world   Convert only hello-world.cast -> hello-world.gif
  --components    Convert only components/*.cast -> components/*.gif
  --all           Convert all casts (default when no scope flag is given)
  -j, --jobs N    Worker threads for render_cast_gif.py (default: 4)
  -h, --help      Show this help message

Environment:
  QMLTUI_USE_AGG=1  Use 'agg' instead of scripts/render_cast_gif.py for GIF output"""


def print_usage(exit_code: int = 0) -> int:
    stream = sys.stderr if exit_code != 0 else sys.stdout
    print(USAGE_TEXT, file=stream)
    return exit_code


def run_cmd(cmd: list[str]) -> int:
    res = subprocess.run(cmd, check=False)
    return res.returncode


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)

    do_gallery = False
    do_hello_world = False
    do_components = False
    scope_selected = False
    jobs_str = "4"

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
        else:
            print(f"error: unknown option or argument '{arg}'", file=sys.stderr)
            return print_usage(1)

    if not scope_selected:
        do_gallery = True
        do_hello_world = True
        do_components = True

    if not jobs_str.isdigit() or int(jobs_str) <= 0:
        print(
            f"error: invalid job count '{jobs_str}' (expected a positive integer)",
            file=sys.stderr,
        )
        return 1

    agg_bin: str | None = None
    if os.environ.get("QMLTUI_USE_AGG") == "1":
        agg_bin = shutil.which("agg")
        if not agg_bin:
            print(
                "warning: QMLTUI_USE_AGG=1 set, but 'agg' was not found in PATH; falling back to render_cast_gif.py",
                file=sys.stderr,
            )

    if do_gallery:
        gallery_cast = CASTS_DIR / "gallery-tour.cast"
        gallery_gif = CASTS_DIR / "gallery-tour.gif"
        gallery_svg = CASTS_DIR / "gallery.svg"
        if not gallery_cast.is_file():
            print(
                f"error: missing {gallery_cast} (run scripts/update_casts.py --gallery first)",
                file=sys.stderr,
            )
            return 1
        if agg_bin:
            print("==> gallery-tour.gif (agg) + gallery.svg")
            rc = run_cmd([agg_bin, str(gallery_cast), str(gallery_gif)])
            if rc != 0:
                return rc
            rc = run_cmd(
                [
                    sys.executable,
                    str(RENDERER),
                    str(gallery_cast),
                    "--svg-only",
                    "--svg",
                    str(gallery_svg),
                    "--svg-frame",
                    "0",
                ]
            )
            if rc != 0:
                return rc
        else:
            print("==> gallery-tour.gif + gallery.svg")
            rc = run_cmd(
                [
                    sys.executable,
                    str(RENDERER),
                    str(gallery_cast),
                    "-o",
                    str(gallery_gif),
                    "--svg",
                    str(gallery_svg),
                    "--svg-frame",
                    "0",
                ]
            )
            if rc != 0:
                return rc

    if do_hello_world:
        hello_cast = CASTS_DIR / "hello-world.cast"
        hello_gif = CASTS_DIR / "hello-world.gif"
        if not hello_cast.is_file():
            print(
                f"error: missing {hello_cast} (run scripts/update_casts.py --hello-world first)",
                file=sys.stderr,
            )
            return 1
        if agg_bin:
            print("==> hello-world.gif (agg)")
            rc = run_cmd([agg_bin, str(hello_cast), str(hello_gif)])
            if rc != 0:
                return rc
        else:
            print("==> hello-world.gif")
            rc = run_cmd([sys.executable, str(RENDERER), str(hello_cast), "-o", str(hello_gif)])
            if rc != 0:
                return rc

    if do_components:
        comp_casts = sorted((CASTS_DIR / "components").glob("*.cast"))
        if not comp_casts:
            print(
                f"error: no .cast files found in {CASTS_DIR / 'components'}/ (run scripts/update_casts.py --components first)",
                file=sys.stderr,
            )
            return 1
        if agg_bin:
            print("==> component GIFs (agg)")
            for cast_path in comp_casts:
                gif_path = cast_path.with_suffix(".gif")
                print(f"    {cast_path.name} -> {gif_path.name}")
                rc = run_cmd([agg_bin, str(cast_path), str(gif_path)])
                if rc != 0:
                    return rc
        else:
            print("==> component GIFs")
            rc = run_cmd(
                [
                    sys.executable,
                    str(RENDERER),
                    "-j",
                    jobs_str,
                    *(str(c) for c in comp_casts),
                ]
            )
            if rc != 0:
                return rc

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
