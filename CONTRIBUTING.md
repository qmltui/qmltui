# Contributing to Qamui

Thank you for considering a contribution. Two things before the first pull
request.

## Licensing and the contributor agreement

Qamui is offered under LGPL-3.0-only or GPL-3.0-only, with separate commercial
terms available upon inquiry (see [LICENSE.md](LICENSE.md)). To preserve this
model and guarantee the open-source promise, every contribution to `src/` or
`tests/` needs the contributor agreement in [CLA.md](CLA.md). A check on the
pull request asks you to accept it once; later pull requests are covered.

Contributions to `examples/`, `cmake/`, `scripts/`, and `docs/` are
BSD-3-Clause or CC-BY-4.0 and do not need the agreement, but the same check
runs on every pull request for simplicity.

Do not add license headers to files. The license of a file follows from its
path via `REUSE.toml`. CI runs `reuse lint`; if you add a file type or
directory the map does not cover, extend `REUSE.toml` in the same pull
request.

## Development

Build and test instructions, the QML conventions, and the test workflow are in
[AGENTS.md](AGENTS.md) and [docs/](docs/). The short version:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Pull requests must keep both Behavior trees green (`QAMUI_ENABLE_BEHAVIOR` ON
and OFF), which CI checks on Linux, macOS, and Windows.
