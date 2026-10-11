#!/usr/bin/env sh
# Regenerate committed casts using a configured, built tree.
# Usage: scripts/update_casts.sh [build-dir]
#   --gallery   regenerate only the gallery tour (default: all)
#   --components regenerate only component demos
set -eu
repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=${1:-"$repo_dir/build"}

do_gallery=1
do_components=1
for arg in "$@"; do
    case "$arg" in
        --gallery)    do_components=0 ;;
        --components) do_gallery=0 ;;
    esac
done

mkdir -p "$repo_dir/docs/assets/casts/components"

if [ "$do_gallery" = "1" ]; then
    echo "==> gallery tour"
    ctest --test-dir "$build_dir" -R '^gallery_tour$' --output-on-failure
    cp "$build_dir/examples/gallery/RunnerArtifacts/gallery_tour/gallery-tour.cast" \
       "$repo_dir/docs/assets/casts/gallery-tour.cast"
    echo "    docs/assets/casts/gallery-tour.cast"
fi

if [ "$do_components" = "1" ]; then
    echo "==> component demos"
    ctest --test-dir "$build_dir" -L Cast -R '_demo$' --output-on-failure
    # Copy each component cast from its artifact directory
    for cast_file in "$build_dir"/examples/components/*/RunnerArtifacts/*_demo/components/*.cast; do
        [ -f "$cast_file" ] || continue
        name=$(basename "$cast_file")
        cp "$cast_file" "$repo_dir/docs/assets/casts/components/$name"
        echo "    docs/assets/casts/components/$name"
    done
fi
