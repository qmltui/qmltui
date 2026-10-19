# Recording demos

Runner captures real ANSI terminal bytes as [asciicast v2](https://docs.asciinema.org/manual/asciicast/v2/),
including colors and attributes. No recorder process or desktop capture is needed.
Register a Runner scenario with `CLOCK VIRTUAL RECORD example.cast`; advance time
with `sleep()` or `advanceTime()` to create visible holds. `marker(label)` names
steps. `stopRecording()` closes the current cast; `startRecording(relativePath)`
starts another with a full repaint. See the [Runner API](../cpp-classes/runner.md).

## Gallery tour

Build and run:

```sh
cmake --build build-qmltui --target qmltui_gallery
scripts/update_casts.sh build-qmltui --gallery
```

This runs `gallery_tour` and copies the cast to `docs/assets/casts/gallery-tour.cast`.
Run it twice and diff: virtual-clock casts are byte-identical for the same build.
Use the Behavior-enabled, Image-enabled tree; those modules change which tiles are present.

## Component demos

Every gallery component tile has a scenario in `examples/gallery/scenarios/*.js` with pacing and
representative interactions, registered against `qmltui_gallery` with `CLOCK VIRTUAL RECORD components/<name>.cast`.

Regenerate all 39 gallery component casts:

```sh
cmake --build build-qmltui -j$(nproc)
scripts/update_casts.sh build-qmltui --components
```

Casts are stored under `docs/assets/casts/components/`.

## GIF and SVG conversion

Local publishing steps; not run in CI (tools are absent on runners).

```sh
# Install once
cargo install --git https://github.com/asciinema/agg
npm install --global svg-term-cli

# All GIFs
scripts/generate_gifs.sh

# Gallery only / components only
scripts/generate_gifs.sh --gallery
scripts/generate_gifs.sh --components

# SVG still of the gallery grid (frame at 500 ms)
svg-term --in docs/assets/casts/gallery-tour.cast \
         --out docs/assets/casts/gallery.svg --at 500
```

Inspect the GIFs for Unicode glyphs, colors, and animation pacing.
The gallery cast ends before its inline-mode demo; the regular gallery scenario
still tests inline behavior. Additional application casts are maintained in qmltui-apps.
