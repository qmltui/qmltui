# Contributing to QmlTui

Thank you for considering a contribution. Two things before the first pull
request.

## Licensing and the contributor agreement

QmlTui's library, test, and tooling code (`src/`, `tests/`, and `tools/`) is
distributed under the GNU Lesser General Public License v3.0 (`LGPL-3.0-only`;
see [LICENSE.md](LICENSE.md)). Contributors retain ownership of their copyright:
to preserve long-term licensing flexibility while guaranteeing an open-source
release, contributions to `src/`, `tests/`, or `tools/` require a one-time
comment accepting the non-exclusive license grant in [CLA.md](CLA.md) on your
first pull request touching those paths.

Contributions that only touch `examples/`, `cmake/`, `scripts/`, `docs/`, or
other `BSD-3-Clause` / `CC-BY-4.0` paths mapped in [`REUSE.toml`](REUSE.toml)
are accepted directly under those permissive terms and do not require CLA
sign-off (see clause 6 of [CLA.md](CLA.md)).

Do not add license headers to files. The license of a file follows from its
path via `REUSE.toml`. CI runs `reuse lint`; if you add a file type or
directory the map does not cover, extend `REUSE.toml` in the same pull
request.

## Development

Build and test instructions, QML conventions, and the test workflow are in
[AGENTS.md](AGENTS.md), [docs/contributing/adding-features.md](docs/contributing/adding-features.md), and [docs/](docs/). To configure, build, and run tests
locally (omit `-DQMLTUI_ENABLE_PTYVIEW=ON` on Windows, where `PtyView` is unsupported):

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DQMLTUI_BUILD_TESTS=ON \
  -DQMLTUI_BUILD_EXAMPLES=ON \
  -DQMLTUI_ENABLE_IMAGE=ON \
  -DQMLTUI_ENABLE_PTYVIEW=ON   # Linux and macOS only
cmake --build build --parallel
ctest --test-dir build -LE Demo --output-on-failure
```

Install the repository git hooks once to auto-check formatting before commits, and use `clang-format==23.1.3` (matching CI) to avoid formatter diffs:

```bash
git config core.hooksPath .githooks
```

Before opening a pull request, verify formatting, documentation snippets, skill metadata, and licensing (passing explicit directories to `format.py --check` checks the full tree; calling it with no paths checks staged files only):

```bash
./scripts/format.py --check src tools examples tests
python3 ./scripts/check_docs.py --build-dir build
python3 ./.agents/skills/qmltui-qml/check-skills.py --build-dir build
reuse lint
git diff --check
```

Pull requests touching private-Qt features (`Behavior` or `PropertyChanges`) or feature gating must keep both `QMLTUI_ENABLE_PRIVATE_QT=ON` and `QMLTUI_ENABLE_PRIVATE_QT=OFF` build trees green. Linux CI verifies both `Private Qt ON` and `OFF` matrices (plus ASan/UBSan and Clang coverage), while Windows and macOS CI verify `Private Qt ON` (see [docs/contributing/ci-runners.md](docs/contributing/ci-runners.md) and [docs/contributing/recording.md](docs/contributing/recording.md)).
