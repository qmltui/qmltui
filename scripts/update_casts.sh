#!/usr/bin/env sh
# Regenerate committed casts using a configured, built tree.
# Usage: scripts/update_casts.sh [OPTIONS] [build-dir]
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=""
build_dir_set=0

do_gallery=0
do_hello_world=0
do_components=0
scope_selected=0
jobs="${CTEST_PARALLEL_LEVEL:-}"

usage() {
    cat << 'EOF'
Usage: scripts/update_casts.sh [OPTIONS] [build-dir]

Regenerate committed Asciinema v2 (.cast) recordings using a configured build tree.

Options:
  --gallery       Regenerate only docs/assets/casts/gallery-tour.cast
  --hello-world   Regenerate only docs/assets/casts/hello-world.cast
  --components    Regenerate only docs/assets/casts/components/*.cast
  --all           Regenerate all casts (default when no scope flag is given)
  -j, --jobs N    Parallel CTest jobs for component scenarios (default: CPU count)
  -h, --help      Show this help message
EOF
}

detect_jobs() {
    if command -v nproc >/dev/null 2>&1; then
        nproc
    elif command -v getconf >/dev/null 2>&1 && getconf _NPROCESSORS_ONLN >/dev/null 2>&1; then
        getconf _NPROCESSORS_ONLN
    elif command -v sysctl >/dev/null 2>&1 && sysctl -n hw.ncpu >/dev/null 2>&1; then
        sysctl -n hw.ncpu
    else
        echo 4
    fi
}

while [ $# -gt 0 ]; do
    case "$1" in
        --gallery)
            do_gallery=1
            scope_selected=1
            shift
            ;;
        --hello-world|--helloworld)
            do_hello_world=1
            scope_selected=1
            shift
            ;;
        --components)
            do_components=1
            scope_selected=1
            shift
            ;;
        --all)
            do_gallery=1
            do_hello_world=1
            do_components=1
            scope_selected=1
            shift
            ;;
        -j|--jobs)
            if [ $# -lt 2 ]; then
                echo "error: $1 requires a positive integer argument" >&2
                usage >&2
                exit 1
            fi
            jobs="$2"
            shift 2
            ;;
        --jobs=*)
            jobs="${1#--jobs=}"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            echo "error: unknown option '$1'" >&2
            usage >&2
            exit 1
            ;;
        *)
            if [ "$build_dir_set" = "1" ]; then
                echo "error: unexpected extra argument '$1' (build-dir already set to '$build_dir')" >&2
                usage >&2
                exit 1
            fi
            build_dir="$1"
            build_dir_set=1
            shift
            ;;
    esac
done

if [ "$scope_selected" = "0" ]; then
    do_gallery=1
    do_hello_world=1
    do_components=1
fi

if [ -z "$build_dir" ]; then
    build_dir="$repo_dir/build"
fi

if [ -z "$jobs" ]; then
    jobs=$(detect_jobs)
fi
case "$jobs" in
    ''|*[!0-9]*|0)
        echo "error: invalid job count '$jobs' (expected a positive integer)" >&2
        exit 1
        ;;
esac

if ! command -v ctest >/dev/null 2>&1; then
    echo "error: ctest not found in PATH" >&2
    exit 1
fi

if [ ! -d "$build_dir" ] || [ ! -f "$build_dir/CTestTestfile.cmake" ]; then
    echo "error: '$build_dir' is not a configured CTest build directory." >&2
    echo "Configure and build first:" >&2
    echo "  cmake -S . -B \"$build_dir\" -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON" >&2
    echo "  cmake --build \"$build_dir\"" >&2
    exit 1
fi

if [ -f "$build_dir/CMakeCache.txt" ]; then
    if grep -q '^QMLTUI_BUILD_EXAMPLES:BOOL=OFF$' "$build_dir/CMakeCache.txt"; then
        echo "error: QMLTUI_BUILD_EXAMPLES is OFF in '$build_dir'; reconfigure with -DQMLTUI_BUILD_EXAMPLES=ON" >&2
        exit 1
    fi
    if grep -q '^QMLTUI_BUILD_TESTS:BOOL=OFF$' "$build_dir/CMakeCache.txt" && \
       ! grep -q '^QMLTUI_ENABLE_RUNNER:BOOL=ON$' "$build_dir/CMakeCache.txt"; then
        echo "error: Runner scenarios are disabled in '$build_dir'; reconfigure with -DQMLTUI_BUILD_TESTS=ON" >&2
        exit 1
    fi
    for opt in QMLTUI_ENABLE_PRIVATE_QT QMLTUI_ENABLE_IMAGE QMLTUI_ENABLE_PTY_VIEW; do
        if grep -q "^${opt}:BOOL=OFF$" "$build_dir/CMakeCache.txt"; then
            echo "warning: ${opt}=OFF in '$build_dir'; casts for that module will not be regenerated" >&2
        fi
    done
fi

mkdir -p "$repo_dir/docs/assets/casts/components"

if [ "$do_gallery" = "1" ]; then
    echo "==> gallery tour"
    gallery_cast="$build_dir/examples/gallery/RunnerArtifacts/gallery_tour/gallery-tour.cast"
    rm -f "$gallery_cast"
    ctest --test-dir "$build_dir" -R '^gallery_tour$' --output-on-failure
    if [ ! -s "$gallery_cast" ]; then
        echo "error: gallery_tour did not produce $gallery_cast" >&2
        exit 1
    fi
    cp "$gallery_cast" "$repo_dir/docs/assets/casts/gallery-tour.cast"
    echo "    docs/assets/casts/gallery-tour.cast"
fi

if [ "$do_hello_world" = "1" ]; then
    echo "==> hello world"
    hello_cast="$build_dir/examples/helloworld/RunnerArtifacts/hello_world/hello-world.cast"
    rm -f "$hello_cast"
    ctest --test-dir "$build_dir" -R '^hello_world$' --output-on-failure
    if [ ! -s "$hello_cast" ]; then
        echo "error: hello_world did not produce $hello_cast" >&2
        exit 1
    fi
    cp "$hello_cast" "$repo_dir/docs/assets/casts/hello-world.cast"
    echo "    docs/assets/casts/hello-world.cast"
fi

if [ "$do_components" = "1" ]; then
    echo "==> component demos"
    # Remove pre-existing component cast artifacts in the build tree so stale
    # files from renamed or removed scenarios are never copied into docs/.
    for stale_cast in \
        "$build_dir"/examples/gallery/RunnerArtifacts/*/components/*.cast \
        "$build_dir"/examples/recipes/*/RunnerArtifacts/*/components/*.cast; do
        [ -f "$stale_cast" ] && rm -f "$stale_cast"
    done

    ctest --test-dir "$build_dir" -j "$jobs" -L Cast -E '^(gallery_tour|hello_world|test_runner_.*)$' --output-on-failure

    copied=0
    for cast_file in \
        "$build_dir"/examples/gallery/RunnerArtifacts/*/components/*.cast \
        "$build_dir"/examples/recipes/*/RunnerArtifacts/*/components/*.cast; do
        [ -s "$cast_file" ] || continue
        name=$(basename "$cast_file")
        cp "$cast_file" "$repo_dir/docs/assets/casts/components/$name"
        echo "    docs/assets/casts/components/$name"
        copied=$((copied + 1))
    done

    if [ "$copied" -eq 0 ]; then
        echo "error: no component casts were generated in '$build_dir'" >&2
        exit 1
    fi
fi
