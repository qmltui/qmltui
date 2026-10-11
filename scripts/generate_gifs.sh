#!/usr/bin/env sh
# Convert committed .cast files into GIFs using agg.
# Usage: scripts/generate_gifs.sh [--gallery] [--components]
#   Defaults to all. Requires: agg (cargo install agg or from qamui-dev/agg-builds).
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

if ! command -v agg >/dev/null 2>&1; then
    echo "agg not found. Install: cargo install --git https://github.com/asciinema/agg" >&2
    exit 1
fi

if [ "$do_gallery" = "1" ] && [ -f "$casts_dir/gallery-tour.cast" ]; then
    echo "==> gallery-tour.gif"
    agg "$casts_dir/gallery-tour.cast" "$casts_dir/gallery-tour.gif"
fi

if [ "$do_components" = "1" ]; then
    echo "==> component GIFs"
    for cast in "$casts_dir/components/"*.cast; do
        [ -f "$cast" ] || continue
        gif="${cast%.cast}.gif"
        name=$(basename "$cast")
        echo "    $name"
        agg "$cast" "$gif"
    done
fi
