#!/usr/bin/env bash
# QmlTui C++ formatting helper using clang-format / git-clang-format
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# Locate clang-format binary
find_clang_format() {
    for cmd in clang-format clang-format-19 clang-format-18 clang-format-17 /usr/bin/clang-format-18 /usr/bin/clang-format; do
        if command -v "$cmd" >/dev/null 2>&1; then
            echo "$cmd"
            return 0
        fi
    done
    return 1
}

# Locate git-clang-format binary
find_git_clang_format() {
    for cmd in git-clang-format git-clang-format-19 git-clang-format-18 git-clang-format-17 /usr/bin/git-clang-format-18 /usr/bin/git-clang-format; do
        if command -v "$cmd" >/dev/null 2>&1; then
            echo "$cmd"
            return 0
        fi
    done
    return 1
}

CLANG_FORMAT="$(find_clang_format || true)"
GIT_CLANG_FORMAT="$(find_git_clang_format || true)"

usage() {
    cat << 'EOF'
Usage: ./scripts/format.sh [OPTIONS] [FILES...]

Options:
  --staged        Format staged C++ changes (default when no arguments given)
  --check         Check staged C++ changes without modifying files; exit 1 if unformatted
  --all           Format all C++ files in src/, tests/, and examples/
  -h, --help      Show this help message

If specific files are passed as arguments, clang-format will format them in-place.
EOF
    exit 0
}

if [[ -z "$CLANG_FORMAT" ]]; then
    echo "Error: clang-format not found. Please install clang-format (e.g. clang-format-18)." >&2
    exit 1
fi

MODE="staged"
FILES=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --staged)
            MODE="staged"
            shift
            ;;
        --check|--diff)
            MODE="check"
            shift
            ;;
        --all)
            MODE="all"
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            MODE="files"
            FILES+=("$1")
            shift
            ;;
    esac
done

case "$MODE" in
    staged)
        if [[ -z "$GIT_CLANG_FORMAT" ]]; then
            echo "git-clang-format not found; formatting staged files with clang-format directly..." >&2
            git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(cpp|h)$' | while read -r file; do
                if [[ -f "$file" ]]; then
                    "$CLANG_FORMAT" -i "$file"
                    git add "$file"
                    echo "Formatted: $file"
                fi
            done
        else
            "$GIT_CLANG_FORMAT" --binary "$CLANG_FORMAT" --extensions h,cpp
        fi
        ;;
    check)
        if [[ -z "$GIT_CLANG_FORMAT" ]]; then
            echo "git-clang-format not found; cannot check staged diff." >&2
            exit 1
        fi
        # Check staged changes
        DIFF="$("$GIT_CLANG_FORMAT" --binary "$CLANG_FORMAT" --extensions h,cpp --diff)"
        if [[ "$DIFF" != *"no modified files to format"* && "$DIFF" != *"clang-format did not modify any files"* && -n "$DIFF" ]]; then
            echo "=================================================================" >&2
            echo "Code style violations detected in staged C++ files:" >&2
            echo "=================================================================" >&2
            echo "$DIFF" >&2
            echo "=================================================================" >&2
            echo "To format staged files automatically, run:" >&2
            echo "  ./scripts/format.sh" >&2
            echo "=================================================================" >&2
            exit 1
        else
            echo "Clang-format check passed: staged files match code style."
        fi
        ;;
    all)
        echo "Formatting all C++ files in src/, tests/, examples/..."
        find src tests examples -type f \( -name "*.cpp" -o -name "*.h" \) | while read -r file; do
            "$CLANG_FORMAT" -i "$file"
        done
        echo "All C++ files formatted."
        ;;
    files)
        for file in "${FILES[@]}"; do
            if [[ -f "$file" ]]; then
                "$CLANG_FORMAT" -i "$file"
                echo "Formatted: $file"
            else
                echo "Warning: $file not found." >&2
            fi
        done
        ;;
esac
