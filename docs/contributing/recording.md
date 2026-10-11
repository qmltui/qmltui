# Recording demos

Runner captures real ANSI terminal bytes as [asciicast v2](https://docs.asciinema.org/manual/asciicast/v2/),
including colors and attributes. No recorder process or desktop capture is needed.
Register a Runner scenario with `CLOCK VIRTUAL RECORD example.cast`; advance time
with `sleep()` or `advanceTime()` to create visible holds. `marker(label)` names
steps. `stopRecording()` closes the current cast; `startRecording(relativePath)`
starts another with a full repaint. See the [Runner API](../cpp-classes/runner.md).

Build the gallery test executable, then run:

```sh
scripts/update_casts.sh build-qamui
ctest --test-dir build-qamui -L Cast --output-on-failure
```

The update script copies the gallery cast from its scenario artifact directory
into `docs/assets/casts`. Run it twice and compare the files: virtual-clock
recordings omit wall-clock metadata and should be identical for the same build.
The optional Image and Behavior modules change which gallery tiles are present;
use the Behavior-enabled, Image-enabled tree for the committed tour.

GIF and SVG conversion are local publishing steps, not CI dependencies:

```sh
cargo install --git https://github.com/asciinema/agg
agg docs/assets/casts/gallery-tour.cast docs/assets/casts/gallery-tour.gif
npm install --global svg-term-cli
svg-term --in docs/assets/casts/gallery-tour.cast --out docs/assets/casts/gallery.svg --at 500
```

Inspect the resulting GIF and SVG, especially Unicode glyphs, colors, and resizing.
The gallery cast ends before its inline-mode demo; the regular gallery scenario
still tests inline behavior. Additional application casts are maintained in qamui-apps.
