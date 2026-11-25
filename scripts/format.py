#!/usr/bin/env python3
"""QmlTui C++ and QML formatting helper using clang-format and qmlformat."""

from __future__ import annotations

import difflib
import glob
import os
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
CPP_EXTENSIONS = {".cpp", ".h", ".hpp"}
QML_EXTENSIONS = {".qml"}
ALL_SCAN_DIRS = ("src", "tests", "examples", "tools")
PRUNED_DIR_NAMES = {".git", ".venv", "venv", "node_modules", "__pycache__", ".qt", ".rcc"}


def is_executable(path: str | Path) -> bool:
    p = Path(path)
    return p.is_file() and os.access(p, os.X_OK)


def resolve_command(cmd: str) -> str | None:
    found = shutil.which(cmd)
    if found:
        return found
    if is_executable(cmd):
        return str(Path(cmd).resolve())
    return None


def find_clang_format() -> str | None:
    env_bin = os.environ.get("CLANG_FORMAT", "").strip()
    if env_bin:
        resolved = resolve_command(env_bin)
        if resolved:
            return resolved

    candidates = [
        "clang-format",
        "clang-format-23",
        "clang-format-22",
        "clang-format-21",
        "clang-format-20",
        "clang-format-19",
        "clang-format-18",
        "clang-format-17",
        "/usr/bin/clang-format",
        "/usr/bin/clang-format-18",
        "/opt/homebrew/bin/clang-format",
        "/usr/local/bin/clang-format",
        "C:/Program Files/LLVM/bin/clang-format.exe",
    ]
    for cmd in candidates:
        resolved = resolve_command(cmd)
        if resolved:
            return resolved
    return None


def find_git_clang_format() -> str | None:
    env_bin = os.environ.get("GIT_CLANG_FORMAT", "").strip()
    if env_bin:
        resolved = resolve_command(env_bin)
        if resolved:
            return resolved

    candidates = [
        "git-clang-format",
        "git-clang-format-23",
        "git-clang-format-22",
        "git-clang-format-21",
        "git-clang-format-20",
        "git-clang-format-19",
        "git-clang-format-18",
        "git-clang-format-17",
        "/usr/bin/git-clang-format",
        "/usr/bin/git-clang-format-18",
        "/opt/homebrew/bin/git-clang-format",
        "/usr/local/bin/git-clang-format",
    ]
    for cmd in candidates:
        resolved = resolve_command(cmd)
        if resolved:
            return resolved
    return None


def find_qmlformat() -> str | None:
    env_bin = os.environ.get("QMLFORMAT", "").strip()
    if env_bin:
        resolved = resolve_command(env_bin)
        if resolved:
            return resolved

    exe_names = ("qmlformat.exe", "qmlformat") if os.name == "nt" else ("qmlformat",)
    for qt_env in ("QMLTUI_QT_LINUX", "QMLTUI_QT_WINDOWS", "QMLTUI_QT_MACOS", "QTDIR"):
        qt_root = os.environ.get(qt_env, "").strip()
        if qt_root:
            for exe_name in exe_names:
                candidate = Path(qt_root) / "bin" / exe_name
                if is_executable(candidate):
                    return str(candidate)

    for cmd in (
        "qmlformat",
        "/usr/lib/qt6/bin/qmlformat",
        "/usr/bin/qmlformat",
        "/opt/homebrew/opt/qt/bin/qmlformat",
        "/usr/local/opt/qt/bin/qmlformat",
    ):
        resolved = resolve_command(cmd)
        if resolved:
            return resolved

    for qtpaths_cmd in ("qtpaths6", "qtpaths"):
        qtpaths_bin = shutil.which(qtpaths_cmd)
        if not qtpaths_bin:
            continue
        res = subprocess.run(
            [qtpaths_bin, "--install-bins"],
            capture_output=True,
            text=True,
            check=False,
        )
        if res.returncode == 0:
            qt_bins = res.stdout.strip()
            if qt_bins:
                for exe_name in exe_names:
                    candidate = Path(qt_bins) / exe_name
                    if is_executable(candidate):
                        return str(candidate)

    for candidate_str in sorted(glob.glob("/usr/lib/*/qt6/bin/qmlformat")):
        if is_executable(candidate_str):
            return candidate_str

    return None


USAGE_TEXT = """\
Usage: ./scripts/format.py [OPTIONS] [PATHS...]

Options:
  --staged        Target staged C++ and QML changes (default when no scope is given)
  --check         Check target files without modifying them; exit 1 if unformatted
  --all           Target all C++ and QML files in src/, tests/, examples/, and tools/
  --cpp-only      Limit formatting or check to C++ files (.cpp, .h, .hpp)
  --qml-only      Limit formatting or check to QML files (.qml)
  -h, --help      Show this help message

If specific files or directories are passed as arguments, they are formatted
(or checked when --check is given)."""


def print_usage(exit_code: int = 0) -> int:
    stream = sys.stderr if exit_code != 0 else sys.stdout
    print(USAGE_TEXT, file=stream)
    return exit_code


def require_clang_format(clang_format_bin: str | None) -> str:
    if not clang_format_bin:
        print(
            "Error: clang-format not found. Install clang-format==23.1.3 (matching CI, e.g. `pipx install clang-format==23.1.3`).",
            file=sys.stderr,
        )
        raise SystemExit(1)
    return clang_format_bin


def require_qmlformat(qml_format_bin: str | None) -> str:
    if not qml_format_bin:
        print(
            "Error: qmlformat not found. Please ensure Qt 6 tooling is installed.",
            file=sys.stderr,
        )
        raise SystemExit(1)
    return qml_format_bin


def format_file_in_place(tool_bin: str, file_path: Path) -> None:
    res = subprocess.run([tool_bin, "-i", str(file_path)], cwd=REPO_ROOT, check=False)
    if res.returncode != 0:
        raise SystemExit(res.returncode)


def check_file_formatted(tool_bin: str, tool_name: str, file_path: Path) -> tuple[bool, str]:
    res = subprocess.run(
        [tool_bin, str(file_path)],
        cwd=REPO_ROOT,
        capture_output=True,
        check=False,
    )
    if res.returncode != 0:
        err = res.stderr.decode("utf-8", errors="replace").strip()
        if err:
            print(err, file=sys.stderr)
        print(f"Error: {tool_name} failed on {file_path}", file=sys.stderr)
        return False, ""

    try:
        original_bytes = (REPO_ROOT / file_path if not file_path.is_absolute() else file_path).read_bytes()
    except OSError as exc:
        print(f"Error reading {file_path}: {exc}", file=sys.stderr)
        return False, ""

    original_text = original_bytes.decode("utf-8", errors="replace").replace("\r\n", "\n")
    formatted_text = res.stdout.decode("utf-8", errors="replace").replace("\r\n", "\n")
    if formatted_text and not formatted_text.endswith("\n"):
        formatted_text += "\n"

    if original_text == formatted_text:
        return True, ""

    diff_lines = difflib.unified_diff(
        original_text.splitlines(keepends=True),
        formatted_text.splitlines(keepends=True),
        fromfile=str(file_path),
        tofile=f"{file_path} (formatted)",
    )
    return False, "".join(diff_lines)


def staged_files() -> list[Path]:
    res = subprocess.run(
        ["git", "diff", "--cached", "--name-only", "-z", "--diff-filter=ACMR"],
        cwd=REPO_ROOT,
        capture_output=True,
        check=True,
    )
    raw = res.stdout.decode("utf-8", errors="replace")
    paths: list[Path] = []
    for entry in raw.split("\0"):
        if entry:
            p = Path(entry)
            if (REPO_ROOT / p).is_file():
                paths.append(p)
    return paths


def run_git_clang_format(git_clang_format_bin: str, clang_format_bin: str, extra_args: list[str]) -> subprocess.CompletedProcess[str] | None:
    cmd = [
        git_clang_format_bin,
        "--binary",
        clang_format_bin,
        "--extensions",
        "h,hpp,cpp",
        *extra_args,
    ]
    try:
        return subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError:
        try:
            return subprocess.run(
                [sys.executable, *cmd],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                check=False,
            )
        except OSError:
            return None


def collect_files_in_dirs(dirs: list[Path], extensions: set[str]) -> list[Path]:
    collected: list[Path] = []
    for d in dirs:
        abs_dir = REPO_ROOT / d if not d.is_absolute() else d
        if not abs_dir.is_dir():
            continue
        for root, dirnames, filenames in os.walk(abs_dir):
            dirnames[:] = [
                dn for dn in dirnames
                if dn not in PRUNED_DIR_NAMES and not dn.startswith("build")
            ]
            for name in filenames:
                p = Path(root) / name
                if p.suffix in extensions:
                    try:
                        collected.append(p.relative_to(REPO_ROOT))
                    except ValueError:
                        collected.append(p)
    collected.sort()
    return collected


def first_n_lines(text: str, n: int = 20) -> str:
    lines = text.splitlines()
    return "\n".join(lines[:n])


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)

    scope = "staged"
    check_only = False
    filter_mode = "both"
    paths: list[str] = []

    idx = 0
    while idx < len(args):
        arg = args[idx]
        if arg == "--staged":
            scope = "staged"
            idx += 1
        elif arg in ("--check", "--diff"):
            check_only = True
            idx += 1
        elif arg == "--all":
            scope = "all"
            idx += 1
        elif arg == "--cpp-only":
            filter_mode = "cpp"
            idx += 1
        elif arg == "--qml-only":
            filter_mode = "qml"
            idx += 1
        elif arg in ("-h", "--help"):
            return print_usage(0)
        elif arg == "--":
            idx += 1
            while idx < len(args):
                scope = "files"
                paths.append(args[idx])
                idx += 1
        elif arg.startswith("-"):
            print(f"Error: unknown option '{arg}'", file=sys.stderr)
            return print_usage(1)
        else:
            scope = "files"
            paths.append(arg)
            idx += 1

    orig_cwd = Path.cwd()
    os.chdir(REPO_ROOT)

    clang_format_bin = find_clang_format()
    git_clang_format_bin = find_git_clang_format()
    qml_format_bin = find_qmlformat()

    all_dirs = [Path(d) for d in ALL_SCAN_DIRS if (REPO_ROOT / d).is_dir()]
    all_dirs_str = " ".join(str(d) for d in all_dirs)

    if scope == "staged":
        staged = staged_files()
        staged_cpp = [p for p in staged if p.suffix in CPP_EXTENSIONS]
        staged_qml = [p for p in staged if p.suffix in QML_EXTENSIONS]
        unstaged = set(
            subprocess.check_output(
                ["git", "diff", "--name-only"], cwd=REPO_ROOT, text=True
            ).splitlines()
        )

        if not check_only:
            needs_manual_restage = False
            if filter_mode != "qml" and staged_cpp:
                cf_bin = require_clang_format(clang_format_bin)
                if git_clang_format_bin:
                    res = run_git_clang_format(git_clang_format_bin, cf_bin, ["--staged"])
                    if res is None:
                        print("Error: failed to invoke git-clang-format.", file=sys.stderr)
                        return 1
                    if res.returncode not in (0, 1):
                        detail = (res.stderr or res.stdout or "").strip()
                        if detail:
                            print(detail, file=sys.stderr)
                        print(
                            "Error: git-clang-format --staged refused to modify partially "
                            "staged or conflicted files; stage or stash unstaged hunks first.",
                            file=sys.stderr,
                        )
                        return res.returncode or 1
                    if res.stdout.strip():
                        print(res.stdout.strip())
                    if res.returncode == 1:
                        for file_path in staged_cpp:
                            if file_path.as_posix() not in unstaged:
                                subprocess.run(
                                    ["git", "add", str(file_path)], cwd=REPO_ROOT, check=True
                                )
                            else:
                                needs_manual_restage = True
                else:
                    for file_path in staged_cpp:
                        format_file_in_place(cf_bin, file_path)
                        if file_path.as_posix() not in unstaged:
                            subprocess.run(
                                ["git", "add", str(file_path)], cwd=REPO_ROOT, check=True
                            )
                        else:
                            needs_manual_restage = True
                        print(f"Formatted C++: {file_path}")

            if filter_mode != "cpp" and staged_qml:
                qf_bin = require_qmlformat(qml_format_bin)
                for file_path in staged_qml:
                    format_file_in_place(qf_bin, file_path)
                    if file_path.as_posix() not in unstaged:
                        subprocess.run(["git", "add", str(file_path)], cwd=REPO_ROOT, check=True)
                    else:
                        needs_manual_restage = True
                    print(f"Formatted QML: {file_path}")
            if needs_manual_restage:
                print(
                    "Note: partially staged files were formatted in the working tree without "
                    "re-staging unstaged hunks; review with 'git diff' and re-stage as needed."
                )
            return 0

        failed = False
        if filter_mode != "qml" and staged_cpp:
            cf_bin = require_clang_format(clang_format_bin)
            if git_clang_format_bin:
                res = run_git_clang_format(git_clang_format_bin, cf_bin, ["--staged", "--diff"])
                if res is not None and res.returncode in (0, 1):
                    diff_out = res.stdout.strip()
                    if (
                        diff_out
                        and "no modified files to format" not in diff_out
                        and "clang-format did not modify any files" not in diff_out
                    ):
                        print(
                            "=================================================================",
                            file=sys.stderr,
                        )
                        print(
                            "Code style violations detected in staged C++ files:",
                            file=sys.stderr,
                        )
                        print(
                            "=================================================================",
                            file=sys.stderr,
                        )
                        print(diff_out, file=sys.stderr)
                        print(
                            "=================================================================",
                            file=sys.stderr,
                        )
                        failed = True
                else:
                    detail = ((res.stderr or res.stdout) if res is not None else "").strip()
                    if detail:
                        print(detail, file=sys.stderr)
                    print(
                        "Error: git-clang-format --staged --diff failed (e.g. partially staged file).",
                        file=sys.stderr,
                    )
                    failed = True
            else:
                print(
                    "Warning: git-clang-format not found; checking C++ files one by one...",
                    file=sys.stderr,
                )
                for file_path in staged_cpp:
                    ok, _ = check_file_formatted(cf_bin, "clang-format", file_path)
                    if not ok:
                        print(f"Unformatted C++ file: {file_path}", file=sys.stderr)
                        failed = True

        if filter_mode != "cpp" and staged_qml:
            qf_bin = require_qmlformat(qml_format_bin)
            qml_violations: list[tuple[Path, str]] = []
            for file_path in staged_qml:
                ok, diff_out = check_file_formatted(qf_bin, "qmlformat", file_path)
                if not ok:
                    qml_violations.append((file_path, diff_out))

            if qml_violations:
                print(
                    "=================================================================",
                    file=sys.stderr,
                )
                print(
                    "Code style violations detected in staged QML files:",
                    file=sys.stderr,
                )
                print(
                    "=================================================================",
                    file=sys.stderr,
                )
                for file_path, diff_out in qml_violations:
                    print(f"  {file_path}", file=sys.stderr)
                    if diff_out:
                        print(first_n_lines(diff_out, 20), file=sys.stderr)
                print(
                    "=================================================================",
                    file=sys.stderr,
                )
                failed = True

        if failed:
            print("To format staged files automatically, run:", file=sys.stderr)
            print("  ./scripts/format.py", file=sys.stderr)
            print(
                "=================================================================",
                file=sys.stderr,
            )
            return 1

        print("Format check passed: staged files match code style.")
        return 0

    if scope == "all":
        if not check_only:
            if filter_mode != "qml":
                cf_bin = require_clang_format(clang_format_bin)
                print(f"Formatting all C++ files in {all_dirs_str}...")
                for file_path in collect_files_in_dirs(all_dirs, CPP_EXTENSIONS):
                    format_file_in_place(cf_bin, file_path)
                print("All C++ files formatted.")

            if filter_mode != "cpp":
                qf_bin = require_qmlformat(qml_format_bin)
                print(f"Formatting all QML files in {all_dirs_str}...")
                for file_path in collect_files_in_dirs(all_dirs, QML_EXTENSIONS):
                    format_file_in_place(qf_bin, file_path)
                print("All QML files formatted.")
            return 0

        failed = False
        if filter_mode != "qml":
            cf_bin = require_clang_format(clang_format_bin)
            for file_path in collect_files_in_dirs(all_dirs, CPP_EXTENSIONS):
                ok, _ = check_file_formatted(cf_bin, "clang-format", file_path)
                if not ok:
                    print(f"Unformatted C++ file: {file_path}", file=sys.stderr)
                    failed = True

        if filter_mode != "cpp":
            qf_bin = require_qmlformat(qml_format_bin)
            for file_path in collect_files_in_dirs(all_dirs, QML_EXTENSIONS):
                ok, _ = check_file_formatted(qf_bin, "qmlformat", file_path)
                if not ok:
                    print(f"Unformatted QML file: {file_path}", file=sys.stderr)
                    failed = True

        if failed:
            print("To format all files automatically, run:", file=sys.stderr)
            print("  ./scripts/format.py --all", file=sys.stderr)
            return 1

        print(f"Format check passed: all files in {all_dirs_str} match code style.")
        return 0

    # scope == "files"
    failed = False
    target_files: list[Path] = []
    supported_exts = CPP_EXTENSIONS | QML_EXTENSIONS
    for raw_target in paths:
        raw_path = Path(raw_target)
        candidate = raw_path if raw_path.is_absolute() else (orig_cwd / raw_path)
        if not candidate.exists():
            candidate = REPO_ROOT / raw_path
        if candidate.exists():
            resolved = candidate.resolve()
            try:
                target = resolved.relative_to(REPO_ROOT)
            except ValueError:
                target = resolved
            if target.is_file():
                target_files.append(target)
            elif target.is_dir():
                target_files.extend(collect_files_in_dirs([target], supported_exts))
            else:
                print(f"Error: '{raw_target}' not found.", file=sys.stderr)
                failed = True
        else:
            print(f"Error: '{raw_target}' not found.", file=sys.stderr)
            failed = True

    for file_path in target_files:
        if file_path.suffix in CPP_EXTENSIONS:
            if filter_mode != "qml":
                cf_bin = require_clang_format(clang_format_bin)
                if not check_only:
                    format_file_in_place(cf_bin, file_path)
                    print(f"Formatted: {file_path}")
                else:
                    ok, diff_out = check_file_formatted(cf_bin, "clang-format", file_path)
                    if not ok:
                        print(f"Unformatted C++ file: {file_path}", file=sys.stderr)
                        if diff_out:
                            print(first_n_lines(diff_out, 20), file=sys.stderr)
                        failed = True
        elif file_path.suffix in QML_EXTENSIONS:
            if filter_mode != "cpp":
                qf_bin = require_qmlformat(qml_format_bin)
                if not check_only:
                    format_file_in_place(qf_bin, file_path)
                    print(f"Formatted: {file_path}")
                else:
                    ok, diff_out = check_file_formatted(qf_bin, "qmlformat", file_path)
                    if not ok:
                        print(f"Unformatted QML file: {file_path}", file=sys.stderr)
                        if diff_out:
                            print(first_n_lines(diff_out, 20), file=sys.stderr)
                        failed = True
        else:
            print(f"Skipping unsupported file type: {file_path}", file=sys.stderr)

    if check_only and not failed:
        print(f"Format check passed: {len(target_files)} file(s) match code style.")

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
