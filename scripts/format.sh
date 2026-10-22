#!/usr/bin/env bash
# QmlTui C++ and QML formatting helper using clang-format and qmlformat
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# Locate clang-format binary
find_clang_format() {
    if [[ -n "${CLANG_FORMAT:-}" ]] && command -v "$CLANG_FORMAT" >/dev/null 2>&1; then
        echo "$CLANG_FORMAT"
        return 0
    fi
    for cmd in clang-format clang-format-20 clang-format-19 clang-format-18 clang-format-17 /usr/bin/clang-format-18 /usr/bin/clang-format; do
        if command -v "$cmd" >/dev/null 2>&1; then
            echo "$cmd"
            return 0
        fi
    done
    return 1
}

# Locate git-clang-format binary
find_git_clang_format() {
    if [[ -n "${GIT_CLANG_FORMAT:-}" ]] && command -v "$GIT_CLANG_FORMAT" >/dev/null 2>&1; then
        echo "$GIT_CLANG_FORMAT"
        return 0
    fi
    for cmd in git-clang-format git-clang-format-20 git-clang-format-19 git-clang-format-18 git-clang-format-17 /usr/bin/git-clang-format-18 /usr/bin/git-clang-format; do
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
    if [[ -n "${QMLTUI_QT_LINUX:-}" && -x "$QMLTUI_QT_LINUX/bin/qmlformat" ]]; then
        echo "$QMLTUI_QT_LINUX/bin/qmlformat"
        return 0
    fi
    for cmd in qmlformat /usr/lib/qt6/bin/qmlformat /usr/bin/qmlformat; do
        if command -v "$cmd" >/dev/null 2>&1; then
            echo "$cmd"
            return 0
        fi
    done
    if command -v qtpaths6 >/dev/null 2>&1; then
        local qt_bins
        qt_bins="$(qtpaths6 --install-bins 2>/dev/null || true)"
        if [[ -n "$qt_bins" && -x "$qt_bins/qmlformat" ]]; then
            echo "$qt_bins/qmlformat"
            return 0
        fi
    fi
    for candidate in /usr/lib/*/qt6/bin/qmlformat; do
        if [[ -x "$candidate" ]]; then
            echo "$candidate"
            return 0
        fi
    done
    return 1
}

usage() {
    local exit_code="${1:-0}"
    cat << 'EOF'
Usage: ./scripts/format.sh [OPTIONS] [PATHS...]

Options:
  --staged        Target staged C++ and QML changes (default when no scope is given)
  --check         Check target files without modifying them; exit 1 if unformatted
  --all           Target all C++ and QML files in src/, tests/, examples/, and tools/
  --cpp-only      Limit formatting or check to C++ files (.cpp, .h, .hpp)
  --qml-only      Limit formatting or check to QML files (.qml)
  -h, --help      Show this help message

If specific files or directories are passed as arguments, they are formatted
(or checked when --check is given).
EOF
    exit "$exit_code"
}

SCOPE="staged"
CHECK_ONLY=0
FILTER="both"
PATHS=()

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
            usage 0
            ;;
        --)
            shift
            while [[ $# -gt 0 ]]; do
                SCOPE="files"
                PATHS+=("$1")
                shift
            done
            ;;
        -*)
            echo "Error: unknown option '$1'" >&2
            usage 1 >&2
            ;;
        *)
            SCOPE="files"
            PATHS+=("$1")
            shift
            ;;
    esac
done

CLANG_FORMAT_BIN="$(find_clang_format || true)"
GIT_CLANG_FORMAT_BIN="$(find_git_clang_format || true)"
QML_FORMAT_BIN="$(find_qmlformat || true)"

require_clang_format() {
    if [[ -z "$CLANG_FORMAT_BIN" ]]; then
        echo "Error: clang-format not found. Please install clang-format (e.g. clang-format-18)." >&2
        exit 1
    fi
}

require_qmlformat() {
    if [[ -z "$QML_FORMAT_BIN" ]]; then
        echo "Error: qmlformat not found. Please ensure Qt 6 tooling is installed." >&2
        exit 1
    fi
}

format_cpp_file() {
    local file="$1"
    require_clang_format
    "$CLANG_FORMAT_BIN" -i "$file"
}

format_qml_file() {
    local file="$1"
    require_qmlformat
    "$QML_FORMAT_BIN" -i "$file"
}

check_cpp_file() {
    local file="$1"
    require_clang_format
    local formatted
    if ! formatted="$("$CLANG_FORMAT_BIN" "$file")"; then
        echo "Error: clang-format failed on $file" >&2
        return 1
    fi
    if ! printf '%s\n' "$formatted" | cmp -s "$file" -; then
        diff -u "$file" <(printf '%s\n' "$formatted") || true
        return 1
    fi
    return 0
}

check_qml_file() {
    local file="$1"
    require_qmlformat
    local formatted
    if ! formatted="$("$QML_FORMAT_BIN" "$file")"; then
        echo "Error: qmlformat failed on $file" >&2
        return 1
    fi
    if ! printf '%s\n' "$formatted" | cmp -s "$file" -; then
        diff -u "$file" <(printf '%s\n' "$formatted") || true
        return 1
    fi
    return 0
}

ALL_DIRS=()
for d in src tests examples tools; do
    if [[ -d "$d" ]]; then
        ALL_DIRS+=("$d")
    fi
done

case "$SCOPE" in
    staged)
        if [[ "$CHECK_ONLY" -eq 0 ]]; then
            # Format C++
            if [[ "$FILTER" != "qml" ]]; then
                require_clang_format
                if [[ -z "$GIT_CLANG_FORMAT_BIN" ]]; then
                    while IFS= read -r -d '' file; do
                        case "$file" in
                            *.cpp|*.h|*.hpp)
                                if [[ -f "$file" ]]; then
                                    format_cpp_file "$file"
                                    git add "$file"
                                    echo "Formatted C++: $file"
                                fi
                                ;;
                        esac
                    done < <(git diff --cached --name-only -z --diff-filter=ACMR)
                else
                    "$GIT_CLANG_FORMAT_BIN" --binary "$CLANG_FORMAT_BIN" --extensions h,hpp,cpp
                fi
            fi

            # Format QML
            if [[ "$FILTER" != "cpp" ]]; then
                require_qmlformat
                while IFS= read -r -d '' file; do
                    case "$file" in
                        *.qml)
                            if [[ -f "$file" ]]; then
                                format_qml_file "$file"
                                git add "$file"
                                echo "Formatted QML: $file"
                            fi
                            ;;
                    esac
                done < <(git diff --cached --name-only -z --diff-filter=ACMR)
            fi
        else
            FAILED=0

            # Check C++
            if [[ "$FILTER" != "qml" ]]; then
                require_clang_format
                if [[ -n "$GIT_CLANG_FORMAT_BIN" ]]; then
                    DIFF="$("$GIT_CLANG_FORMAT_BIN" --binary "$CLANG_FORMAT_BIN" --extensions h,hpp,cpp --diff)"
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
                    while IFS= read -r -d '' file; do
                        case "$file" in
                            *.cpp|*.h|*.hpp)
                                if [[ -f "$file" ]] && ! check_cpp_file "$file" >/dev/null; then
                                    echo "Unformatted C++ file: $file" >&2
                                    FAILED=1
                                fi
                                ;;
                        esac
                    done < <(git diff --cached --name-only -z --diff-filter=ACMR)
                fi
            fi

            # Check QML
            if [[ "$FILTER" != "cpp" ]]; then
                require_qmlformat
                QML_VIOLATIONS=()
                while IFS= read -r -d '' file; do
                    case "$file" in
                        *.qml)
                            if [[ -f "$file" ]] && ! check_qml_file "$file" >/dev/null; then
                                QML_VIOLATIONS+=("$file")
                            fi
                            ;;
                    esac
                done < <(git diff --cached --name-only -z --diff-filter=ACMR)

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
                require_clang_format
                echo "Formatting all C++ files in ${ALL_DIRS[*]}..."
                while IFS= read -r -d '' file; do
                    format_cpp_file "$file"
                done < <(find "${ALL_DIRS[@]}" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0)
                echo "All C++ files formatted."
            fi

            if [[ "$FILTER" != "cpp" ]]; then
                require_qmlformat
                echo "Formatting all QML files in ${ALL_DIRS[*]}..."
                while IFS= read -r -d '' file; do
                    format_qml_file "$file"
                done < <(find "${ALL_DIRS[@]}" -type f -name "*.qml" -print0)
                echo "All QML files formatted."
            fi
        else
            FAILED=0
            if [[ "$FILTER" != "qml" ]]; then
                require_clang_format
                while IFS= read -r -d '' file; do
                    if ! check_cpp_file "$file" >/dev/null; then
                        echo "Unformatted C++ file: $file" >&2
                        FAILED=1
                    fi
                done < <(find "${ALL_DIRS[@]}" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0)
            fi

            if [[ "$FILTER" != "cpp" ]]; then
                require_qmlformat
                while IFS= read -r -d '' file; do
                    if ! check_qml_file "$file" >/dev/null; then
                        echo "Unformatted QML file: $file" >&2
                        FAILED=1
                    fi
                done < <(find "${ALL_DIRS[@]}" -type f -name "*.qml" -print0)
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
        FILES=()
        for target in "${PATHS[@]}"; do
            if [[ -f "$target" ]]; then
                FILES+=("$target")
            elif [[ -d "$target" ]]; then
                while IFS= read -r -d '' found; do
                    FILES+=("$found")
                done < <(find "$target" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.qml" \) -print0)
            else
                echo "Error: '$target' not found." >&2
                FAILED=1
            fi
        done

        for file in "${FILES[@]}"; do
            case "$file" in
                *.cpp|*.h|*.hpp)
                    if [[ "$FILTER" != "qml" ]]; then
                        if [[ "$CHECK_ONLY" -eq 0 ]]; then
                            format_cpp_file "$file"
                            echo "Formatted: $file"
                        else
                            diff_out=""
                            if ! diff_out="$(check_cpp_file "$file")"; then
                                echo "Unformatted C++ file: $file" >&2
                                printf '%s\n' "$diff_out" | head -n 20 >&2
                                FAILED=1
                            fi
                        fi
                    fi
                    ;;
                *.qml)
                    if [[ "$FILTER" != "cpp" ]]; then
                        if [[ "$CHECK_ONLY" -eq 0 ]]; then
                            format_qml_file "$file"
                            echo "Formatted: $file"
                        else
                            diff_out=""
                            if ! diff_out="$(check_qml_file "$file")"; then
                                echo "Unformatted QML file: $file" >&2
                                printf '%s\n' "$diff_out" | head -n 20 >&2
                                FAILED=1
                            fi
                        fi
                    fi
                    ;;
                *)
                    echo "Skipping unsupported file type: $file" >&2
                    ;;
            esac
        done

        if [[ "$FAILED" -ne 0 ]]; then
            exit 1
        fi
        ;;
esac
