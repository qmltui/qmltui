# Licensing

QmlTui is available under the **GNU Lesser General Public License v3.0 (`LGPL-3.0-only`)**.
Examples, the starter template, and the build glue are **BSD-3-Clause** so they
can be copied into your own project without conditions. Documentation is
**CC-BY-4.0**.

The license of a file is decided by its path. There are no per-file headers;
the machine-readable map is [`REUSE.toml`](REUSE.toml) and the full license
texts are in [`LICENSES/`](LICENSES/).

| Path | License (SPDX) |
| :--- | :--- |
| `src/` (Core, Controls, Image, PtyView, Runner), `tests/`, and `tools/` | `LGPL-3.0-only` |
| `examples/` | `BSD-3-Clause` |
| `cmake/`, `scripts/`, `.github/`, `.agents/`, `.githooks/`, root `CMakeLists.txt` | `BSD-3-Clause` |
| `docs/`, every `*.md` | `CC-BY-4.0` |
| Third-party code fetched at build time (utf8proc, libvterm) | `MIT` (plus `Unicode-DFS-2015` data files in `utf8proc`), see [`THIRD-PARTY.md`](THIRD-PARTY.md) |

## LGPL-3.0-only Summary

Under **LGPL-3.0-only**, you may link QmlTui as shared libraries (or provide relinking
mechanisms for static binaries) while keeping your application code under your
own terms, provided modifications to QmlTui itself remain available under
LGPL-3.0-only. Because LGPLv3 is a set of additional permissions on top of GPLv3, you
may also combine QmlTui with GPLv3 works under the terms of GPLv3.

## Dependencies and WebAssembly

Qt and third-party dependencies remain subject to their own licenses (see
[`THIRD-PARTY.md`](THIRD-PARTY.md) and <https://www.qt.io/licensing>).

When building for WebAssembly, open-source Qt is available under GPLv3 or
commercial Qt terms. While QmlTui's source remains under `LGPL-3.0-only`, linking
against open-source Qt for WebAssembly produces a combined binary subject to
the GPLv3.

## Contributing

Contributors retain ownership of their copyright. Contributions to the
`LGPL-3.0-only` paths (`src/`, `tests/`, and `tools/`) use the non-exclusive
license grant in [`CLA.md`](CLA.md) so the project preserves long-term licensing
flexibility while guaranteeing an open-source release. Contributions to
`examples/`, `cmake/`, `scripts/`, and `docs/` are accepted directly under their
permissive `BSD-3-Clause` or `CC-BY-4.0` terms without a CLA. See
[`CONTRIBUTING.md`](CONTRIBUTING.md).

QmlTui is an independent open-source project and is not a product of The Qt
Company. Qt is licensed separately; see <https://www.qt.io/licensing>.
