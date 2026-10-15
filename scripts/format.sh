#!/usr/bin/env bash
# QmlTui C++ and QML formatting helper using clang-format and qmlformat
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

# Locate qmlformat binary
find_qmlformat() {
    if [[ -n "${QMLFORMAT:-}" ]] && command -v "$QMLFORMAT" >/dev/null 2>&1; then
        echo "$QMLFORMAT"
        return 0
    fi
    for cmd in qmlformat /usr/lib/qt6/bin/qmlformat /usr/bin/qmlformat; do
        if command -v "$cmd" >/dev/null 2>&1; then
            echo "$cmd"
            return 0
        fi
    done
    if [[ -n "${QMLTUI_QT_LINUX:-}" && -x "$QMLTUI_QT_LINUX/bin/qmlformat" ]]; then
        echo "$QMLTUI_QT_LINUX/bin/qmlformat"
        return 0
    fi
    return 1
}

CLANG_FORMAT="$(find_clang_format || true)"
GIT_CLANG_FORMAT="$(find_git_clang_format || true)"
QML_FORMAT="$(find_qmlformat || true)"

usage() {
    cat << 'EOF'
Usage: ./scripts/format.sh [OPTIONS] [FILES...]

Options:
  --staged        Target staged C++ and QML changes (default when no scope is given)
  --check         Check target files without modifying them; exit 1 if unformatted
  --all           Target all C++ and QML files in src/, tests/, examples/, and tools/
  --cpp-only      Limit formatting or check to C++ files (.cpp, .h)
  --qml-only      Limit formatting or check to QML files (.qml)
  -h, --help      Show this help message

If specific files are passed as arguments, they are formatted (or checked with --check).
EOF
    exit 0
}

if [[ -z "$CLANG_FORMAT" ]]; then
    echo "Error: clang-format not found. Please install clang-format (e.g. clang-format-18)." >&2
    exit 1
fi

if [[ -z "$QML_FORMAT" ]]; then
    echo "Error: qmlformat not found. Please ensure Qt 6 tooling is installed." >&2
    exit 1
fi

SCOPE="staged"
CHECK_ONLY=0
FILTER="both"
FILES=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --staged)
            SCOPE="staged"
            shift
            ;;
        --check|--diff)
            CHECK_ONLY=1
            shift
            ;;
        --all)
            SCOPE="all"
            shift
            ;;
        --cpp-only)
            FILTER="cpp"
            shift
            ;;
        --qml-only)
            FILTER="qml"
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            SCOPE="files"
            FILES+=("$1")
            shift
            ;;
    esac
done

format_cpp_file() {
    local file="$1"
    "$CLANG_FORMAT" -i "$file"
}

format_qml_file() {
    local file="$1"
    "$QML_FORMAT" -i "$file"
}

check_cpp_file() {
    local file="$1"
    if ! "$CLANG_FORMAT" "$file" | cmp -s "$file" -; then
        diff -u "$file" <("$CLANG_FORMAT" "$file") || true
        return 1
    fi
    return 0
}

check_qml_file() {
    local file="$1"
    local formatted
    formatted="$("$QML_FORMAT" "$file")"
    if ! cmp -s "$file" <(echo "$formatted"); then
        diff -u "$file" <(echo "$formatted") || true
        return 1
    fi
    return 0
}

ALL_DIRS=(src tests examples)
if [[ -d tools ]]; then
    ALL_DIRS+=(tools)
fi

case "$SCOPE" in
    staged)
        if [[ "$CHECK_ONLY" -eq 0 ]]; then
            # Format C++
            if [[ "$FILTER" != "qml" ]]; then
                if [[ -z "$GIT_CLANG_FORMAT" ]]; then
                    git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(cpp|h)$' | while read -r file; do
                        if [[ -f "$file" ]]; then
                            format_cpp_file "$file"
                            git add "$file"
                            echo "Formatted C++: $file"
                        fi
                    done
                else
                    "$GIT_CLANG_FORMAT" --binary "$CLANG_FORMAT" --extensions h,cpp
                fi
            fi

            # Format QML
            if [[ "$FILTER" != "cpp" ]]; then
                git diff --cached --name-only --diff-filter=ACMR | grep -E '\.qml$' | while read -r file; do
                    if [[ -f "$file" ]]; then
                        format_qml_file "$file"
                        git add "$file"
                        echo "Formatted QML: $file"
                    fi
                done
            fi
        else
            FAILED=0

            # Check C++
            if [[ "$FILTER" != "qml" ]]; then
                if [[ -n "$GIT_CLANG_FORMAT" ]]; then
                    DIFF="$("$GIT_CLANG_FORMAT" --binary "$CLANG_FORMAT" --extensions h,cpp --diff)"
                    if [[ "$DIFF" != *"no modified files to format"* && "$DIFF" != *"clang-format did not modify any files"* && -n "$DIFF" ]]; then
                        echo "=================================================================" >&2
                        echo "Code style violations detected in staged C++ files:" >&2
                        echo "=================================================================" >&2
                        echo "$DIFF" >&2
                        echo "=================================================================" >&2
                        FAILED=1
                    fi
                else
                    echo "Warning: git-clang-format not found; checking C++ files one by one..." >&2
                    while IFS= read -r file; do
                        if [[ -f "$file" ]]; then
                            if ! check_cpp_file "$file" >/dev/null; then
                                echo "Unformatted C++ file: $file" >&2
                                FAILED=1
                            fi
                        fi
                    done < <(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(cpp|h)$' || true)
                fi
            fi

            # Check QML
            if [[ "$FILTER" != "cpp" ]]; then
                QML_VIOLATIONS=()
                while IFS= read -r file; do
                    if [[ -f "$file" ]]; then
                        if ! check_qml_file "$file" >/dev/null; then
                            QML_VIOLATIONS+=("$file")
                        fi
                    fi
                done < <(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.qml$' || true)

                if [[ ${#QML_VIOLATIONS[@]} -gt 0 ]]; then
                    echo "=================================================================" >&2
                    echo "Code style violations detected in staged QML files:" >&2
                    echo "=================================================================" >&2
                    for f in "${QML_VIOLATIONS[@]}"; do
                        echo "  $f" >&2
                        check_qml_file "$f" | head -n 20 >&2 || true
                    done
                    echo "=================================================================" >&2
                    FAILED=1
                fi
            fi

            if [[ "$FAILED" -ne 0 ]]; then
                echo "To format staged files automatically, run:" >&2
                echo "  ./scripts/format.sh" >&2
                echo "=================================================================" >&2
                exit 1
            else
                echo "Format check passed: staged files match code style."
            fi
        fi
        ;;

    all)
        if [[ "$CHECK_ONLY" -eq 0 ]]; then
            if [[ "$FILTER" != "qml" ]]; then
                echo "Formatting all C++ files in ${ALL_DIRS[*]}..."
                while IFS= read -r file; do
                    format_cpp_file "$file"
                done < <(find "${ALL_DIRS[@]}" -type f \( -name "*.cpp" -o -name "*.h" \))
                echo "All C++ files formatted."
            fi

            if [[ "$FILTER" != "cpp" ]]; then
                echo "Formatting all QML files in ${ALL_DIRS[*]}..."
                while IFS= read -r file; do
                    format_qml_file "$file"
                done < <(find "${ALL_DIRS[@]}" -type f -name "*.qml")
                echo "All QML files formatted."
            fi
        else
            FAILED=0
            if [[ "$FILTER" != "qml" ]]; then
                while IFS= read -r file; do
                    if ! check_cpp_file "$file" >/dev/null; then
                        echo "Unformatted C++ file: $file" >&2
                        FAILED=1
                    fi
                done < <(find "${ALL_DIRS[@]}" -type f \( -name "*.cpp" -o -name "*.h" \))
            fi

            if [[ "$FILTER" != "cpp" ]]; then
                while IFS= read -r file; do
                    if ! check_qml_file "$file" >/dev/null; then
                        echo "Unformatted QML file: $file" >&2
                        FAILED=1
                    fi
                done < <(find "${ALL_DIRS[@]}" -type f -name "*.qml")
            fi

            if [[ "$FAILED" -ne 0 ]]; then
                echo "To format all files automatically, run:" >&2
                echo "  ./scripts/format.sh --all" >&2
                exit 1
            else
                echo "Format check passed: all files in ${ALL_DIRS[*]} match code style."
            fi
        fi
        ;;

    files)
        FAILED=0
        for file in "${FILES[@]}"; do
            if [[ -f "$file" ]]; then
                case "$file" in
                    *.cpp|*.h)
                        if [[ "$FILTER" != "qml" ]]; then
                            if [[ "$CHECK_ONLY" -eq 0 ]]; then
                                format_cpp_file "$file"
                                echo "Formatted: $file"
                            elif ! check_cpp_file "$file" >/dev/null; then
                                echo "Unformatted C++ file: $file" >&2
                                check_cpp_file "$file" | head -n 20 >&2 || true
                                FAILED=1
                            fi
                        fi
                        ;;
                    *.qml)
                        if [[ "$FILTER" != "cpp" ]]; then
                            if [[ "$CHECK_ONLY" -eq 0 ]]; then
                                format_qml_file "$file"
                                echo "Formatted: $file"
                            elif ! check_qml_file "$file" >/dev/null; then
                                echo "Unformatted QML file: $file" >&2
                                check_qml_file "$file" | head -n 20 >&2 || true
                                FAILED=1
                            fi
                        fi
                        ;;
                    *)
                        echo "Skipping unknown file type: $file" >&2
                        ;;
                esac
            else
                echo "Warning: $file not found." >&2
            fi
        done
        if [[ "$CHECK_ONLY" -ne 0 && "$FAILED" -ne 0 ]]; then
            exit 1
        fi
        ;;
esac
