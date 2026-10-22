#!/usr/bin/env sh
# Convert committed .cast files into GIFs (and gallery.svg) using scripts/render_cast_gif.py or agg.
# Usage: scripts/generate_gifs.sh [OPTIONS]
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
casts_dir="$repo_dir/docs/assets/casts"
renderer="$repo_dir/scripts/render_cast_gif.py"

do_gallery=0
do_hello_world=0
do_components=0
scope_selected=0
jobs=4

usage() {
    cat << 'EOF'
Usage: scripts/generate_gifs.sh [OPTIONS]

Convert committed .cast recordings into animated GIFs (and gallery.svg).

Options:
  --gallery       Convert only gallery-tour.cast -> gallery-tour.gif + gallery.svg
  --hello-world   Convert only hello-world.cast -> hello-world.gif
  --components    Convert only components/*.cast -> components/*.gif
  --all           Convert all casts (default when no scope flag is given)
  -j, --jobs N    Worker threads for render_cast_gif.py (default: 4)
  -h, --help      Show this help message

Environment:
  QMLTUI_USE_AGG=1  Use 'agg' instead of scripts/render_cast_gif.py for GIF output
EOF
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
        *)
            echo "error: unknown option or argument '$1'" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [ "$scope_selected" = "0" ]; then
    do_gallery=1
    do_hello_world=1
    do_components=1
fi

case "$jobs" in
    ''|*[!0-9]*|0)
        echo "error: invalid job count '$jobs' (expected a positive integer)" >&2
        exit 1
        ;;
esac

use_agg=0
if [ "${QMLTUI_USE_AGG:-0}" = "1" ]; then
    if command -v agg >/dev/null 2>&1; then
        use_agg=1
    else
        echo "warning: QMLTUI_USE_AGG=1 set, but 'agg' was not found in PATH; falling back to render_cast_gif.py" >&2
    fi
fi

if ! command -v python3 >/dev/null 2>&1; then
    if [ "$use_agg" = "0" ] || [ "$do_gallery" = "1" ]; then
        echo "error: python3 not found in PATH" >&2
        exit 1
    fi
fi

if [ "$do_gallery" = "1" ]; then
    if [ ! -f "$casts_dir/gallery-tour.cast" ]; then
        echo "error: missing $casts_dir/gallery-tour.cast (run scripts/update_casts.sh --gallery first)" >&2
        exit 1
    fi
    if [ "$use_agg" = "1" ]; then
        echo "==> gallery-tour.gif (agg) + gallery.svg"
        agg "$casts_dir/gallery-tour.cast" "$casts_dir/gallery-tour.gif"
        python3 "$renderer" \
            "$casts_dir/gallery-tour.cast" \
            --svg-only \
            --svg "$casts_dir/gallery.svg" \
            --svg-frame 0
    else
        echo "==> gallery-tour.gif + gallery.svg"
        python3 "$renderer" \
            "$casts_dir/gallery-tour.cast" \
            -o "$casts_dir/gallery-tour.gif" \
            --svg "$casts_dir/gallery.svg" \
            --svg-frame 0
    fi
fi

if [ "$do_hello_world" = "1" ]; then
    if [ ! -f "$casts_dir/hello-world.cast" ]; then
        echo "error: missing $casts_dir/hello-world.cast (run scripts/update_casts.sh --hello-world first)" >&2
        exit 1
    fi
    if [ "$use_agg" = "1" ]; then
        echo "==> hello-world.gif (agg)"
        agg "$casts_dir/hello-world.cast" "$casts_dir/hello-world.gif"
    else
        echo "==> hello-world.gif"
        python3 "$renderer" \
            "$casts_dir/hello-world.cast" \
            -o "$casts_dir/hello-world.gif"
    fi
fi

if [ "$do_components" = "1" ]; then
    set -- "$casts_dir/components/"*.cast
    if [ ! -f "${1:-}" ]; then
        echo "error: no .cast files found in $casts_dir/components/ (run scripts/update_casts.sh --components first)" >&2
        exit 1
    fi
    if [ "$use_agg" = "1" ]; then
        echo "==> component GIFs (agg)"
        for cast in "$@"; do
            gif="${cast%.cast}.gif"
            name=$(basename "$cast")
            echo "    $name -> ${name%.cast}.gif"
            agg "$cast" "$gif"
        done
    else
        echo "==> component GIFs"
        python3 "$renderer" -j "$jobs" "$@"
    fi
fi
