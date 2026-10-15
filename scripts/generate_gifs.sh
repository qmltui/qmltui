#!/usr/bin/env sh
# Convert committed .cast files into GIFs (and gallery.svg) using scripts/render_cast_gif.py or agg.
# Usage: scripts/generate_gifs.sh [--gallery] [--components]
set -eu
repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
casts_dir="$repo_dir/docs/assets/casts"

do_gallery=1
do_components=1
for arg in "$@"; do
    case "$arg" in
        --gallery)    do_components=0 ;;
        --components) do_gallery=0 ;;
    esac
done

if [ "${QMLTUI_USE_AGG:-0}" = "1" ] && command -v agg >/dev/null 2>&1; then
    if [ "$do_gallery" = "1" ] && [ -f "$casts_dir/gallery-tour.cast" ]; then
        echo "==> gallery-tour.gif (agg)"
        agg "$casts_dir/gallery-tour.cast" "$casts_dir/gallery-tour.gif"
    fi
    if [ "$do_components" = "1" ]; then
        echo "==> component GIFs (agg)"
        for cast in "$casts_dir/components/"*.cast; do
            [ -f "$cast" ] || continue
            gif="${cast%.cast}.gif"
            name=$(basename "$cast")
            echo "    $name"
            agg "$cast" "$gif"
        done
    fi
    exit 0
fi

if [ "$do_gallery" = "1" ] && [ -f "$casts_dir/gallery-tour.cast" ]; then
    echo "==> gallery-tour.gif + gallery.svg"
    python3 "$repo_dir/scripts/render_cast_gif.py" \
        "$casts_dir/gallery-tour.cast" \
        -o "$casts_dir/gallery-tour.gif" \
        --svg "$casts_dir/gallery.svg" \
        --svg-frame 0
fi

if [ "$do_components" = "1" ]; then
    echo "==> component GIFs"
    python3 "$repo_dir/scripts/render_cast_gif.py" -j 4 "$casts_dir/components/"*.cast
fi
