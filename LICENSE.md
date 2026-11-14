# Licensing

QmlTui is available under the **GNU Lesser General Public License v3 (LGPLv3)**.
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
| `cmake/`, `scripts/`, `.github/`, root `CMakeLists.txt` | `BSD-3-Clause` |
| `docs/`, every `*.md` | `CC-BY-4.0` |
| Third-party code fetched at build time (utf8proc, libvterm) | MIT, see [`THIRD-PARTY.md`](THIRD-PARTY.md) |

## Open-Source and Commercial Terms

Under **LGPLv3**, you may link QmlTui as shared libraries (or provide relinking
mechanisms for static binaries) while keeping your application code under your
own terms, provided modifications to QmlTui itself remain available under
LGPLv3. Because LGPLv3 is a set of additional permissions on top of GPLv3, you
may also combine QmlTui with GPLv3 works under the terms of GPLv3.

Separate commercial terms are available upon inquiry for organizations unable to
meet those obligations. Contact the copyright holder through the project
repository at <https://github.com/qmltui/qmltui>.

## Dependencies and WebAssembly

Qt and third-party dependencies remain subject to their own licenses (see
[`THIRD-PARTY.md`](THIRD-PARTY.md) and <https://www.qt.io/licensing>).

When building for WebAssembly, open-source Qt is available under GPLv3 or
commercial terms only. While QmlTui's source remains under LGPLv3, linking
against open-source Qt for WebAssembly produces a combined binary subject to
the GPLv3.

## Contributing

Contributions require the contributor agreement in [`CLA.md`](CLA.md), so that
the copyright holder retains the right to offer separate commercial terms upon
inquiry while guaranteeing the open-source promise. See
[`CONTRIBUTING.md`](CONTRIBUTING.md).

QmlTui is an independent project of its copyright holder. It is not a product of
The Qt Company. Qt is licensed separately; see <https://www.qt.io/licensing>.
