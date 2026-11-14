# Contributing to QmlTui

Thank you for considering a contribution. Two things before the first pull
request.

## Licensing and the contributor agreement

QmlTui is offered under the LGPLv3, with separate commercial terms available
upon inquiry (see [LICENSE.md](LICENSE.md)). To preserve this model and
guarantee the open-source promise, every contribution to `src/` or `tests/`
needs the contributor agreement in [CLA.md](CLA.md). A check on the pull
request asks you to accept it once; later pull requests are covered.

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
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DQMLTUI_BUILD_TESTS=ON \
  -DQMLTUI_BUILD_EXAMPLES=ON \
  -DQMLTUI_ENABLE_IMAGE=ON \
  -DQMLTUI_ENABLE_PTYVIEW=ON
cmake --build build --parallel
ctest --test-dir build -LE Demo --output-on-failure
```

Before opening a pull request, verify formatting, documentation snippets, and skill metadata:

```bash
./scripts/format.py --check
python3 ./scripts/check_docs.py --build-dir build
python3 ./.agents/skills/qmltui-qml/check-skills.py --build-dir build
git diff --check
```

Pull requests touching private-Qt features (`Behavior` or `PropertyChanges`) or feature gating must keep both `QMLTUI_ENABLE_PRIVATE_QT=ON` and `QMLTUI_ENABLE_PRIVATE_QT=OFF` build trees green, which CI verifies across Linux, macOS, and Windows.
