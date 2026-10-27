# Recording demos

Runner captures real ANSI terminal bytes as [asciicast v2](https://docs.asciinema.org/manual/asciicast/v2/),
including colors and attributes. No recorder process or desktop capture is needed.
Register a demo scenario with `CLOCK REAL RECORD <relative-path.cast>`; advance time
with `sleep()` to create visible holds. `marker(label)` names
steps. `stopRecording()` closes the current cast; `startRecording(relativePath)`
starts another with a full repaint. See the [Runner API](../cpp-classes/runner.md).

In `examples/CMakeLists.txt`, `QMLTUI_RECORD_DIR` points to `docs/assets/casts/` and
`QMLTUI_RECORD_POST_COMMAND` runs `agg --renderer resvg --font-size 22 --line-height 1.08 "<CAST>" "<CAST_STEM>.gif"` automatically
whenever `agg` is found in `PATH`. All recording scenarios carry the `Demo` CTest
label so normal `ctest -L Runner` runs stay fast.

## Prerequisites

Install `agg` once for local demo regeneration:

```sh
cargo install --git https://github.com/asciinema/agg
```

## Regenerating casts and GIFs

Configure and build with examples and tests enabled (use a Behavior-enabled, Image-enabled, PtyView-enabled tree so all gallery tiles are present):

```sh
cmake -S . -B build -DQMLTUI_BUILD_TESTS=ON -DQMLTUI_BUILD_EXAMPLES=ON -DQMLTUI_ENABLE_IMAGE=ON -DQMLTUI_ENABLE_PTYVIEW=ON
cmake --build build -j$(nproc)
```

Run all `Demo` scenarios in parallel (records `.cast` files into `docs/assets/casts/` and converts each to `.gif` via `agg`):

```sh
ctest --test-dir build -L Demo -j$(nproc) --output-on-failure
```

Or regenerate a specific demo:

```sh
# Gallery tour only
ctest --test-dir build -R "^gallery_tour$" --output-on-failure

# Hello World only
ctest --test-dir build -R "^hello_world_demo$" --output-on-failure

# Single component demo
ctest --test-dir build -R "^gallery_button_demo$" --output-on-failure
```

Inspect the GIFs for Unicode glyphs, colors, and animation pacing.
Additional application casts are maintained in qmltui-apps.
