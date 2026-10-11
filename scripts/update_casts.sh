#!/usr/bin/env sh
# Regenerate the committed gallery tour using a configured, built tree.
set -eu
repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=${1:-"$repo_dir/build"}
ctest --test-dir "$build_dir" -R '^gallery_tour$' --output-on-failure
mkdir -p "$repo_dir/docs/assets/casts"
cp "$build_dir/examples/gallery/RunnerArtifacts/gallery_tour/gallery-tour.cast" \
   "$repo_dir/docs/assets/casts/gallery-tour.cast"
