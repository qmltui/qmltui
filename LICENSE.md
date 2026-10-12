# Licensing

QmlTui is available under the **GNU Lesser General Public License v3** or the
**GNU General Public License v3**, at your choice. Examples, the starter
template, and the build glue are **BSD-3-Clause** so they can be copied into
your own project without conditions. Documentation is **CC-BY-4.0**.

The license of a file is decided by its path. There are no per-file headers;
the machine-readable map is [`REUSE.toml`](REUSE.toml) and the full license
texts are in [`LICENSES/`](LICENSES/).

| Path | License |
| :--- | :--- |
| `src/` (Core, Controls, Extras, Canvas, Image, TerminalHost, Runner) and `tests/` | `LGPL-3.0-only OR GPL-3.0-only` |
| `examples/` | `BSD-3-Clause` |
| `cmake/`, `scripts/`, `.github/`, root `CMakeLists.txt` | `BSD-3-Clause` |
| `docs/`, every `*.md`, the `doc/` directories under `src/` | `CC-BY-4.0` |
| Third-party code fetched at build time (utf8proc, libvterm) | MIT, see [`THIRD-PARTY.md`](THIRD-PARTY.md) |

## Open-Source Licensing Options

- **LGPL-3.0-only**: Link QmlTui as shared libraries (or provide relinking
  mechanisms for static binaries), keep modifications to QmlTui available under
  LGPLv3. Application code remains under your own terms.
- **GPL-3.0-only**: Application and combined works are distributed under GPLv3.

Separate commercial terms are available upon inquiry for organizations unable to
meet those obligations. Contact the copyright holder through the project
repository at <https://github.com/qmltui-dev/qmltui>.

## Dependencies and WebAssembly

Qt and third-party dependencies remain subject to their own licenses (see
[`THIRD-PARTY.md`](THIRD-PARTY.md) and <https://www.qt.io/licensing>).

When building for WebAssembly, open-source Qt is available under GPLv3 or
commercial terms only. While QmlTui's source remains `LGPL-3.0-only OR GPL-3.0-only`,
linking against open-source Qt for WebAssembly produces a combined binary
subject to the GPLv3.

## Contributing

Contributions require the contributor agreement in [`CLA.md`](CLA.md), so that
the copyright holder retains the right to offer separate commercial terms upon
inquiry while guaranteeing the open-source promise. See
[`CONTRIBUTING.md`](CONTRIBUTING.md).

QmlTui is an independent project of its copyright holder. It is not a product of
The Qt Company. Qt is licensed separately; see <https://www.qt.io/licensing>.
