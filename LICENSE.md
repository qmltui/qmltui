# Licensing

Qtui is available under the **GNU Lesser General Public License v3**, the
**GNU General Public License v3**, or a **commercial license**, at your
choice. Examples, the starter template, and the build glue are
**BSD-3-Clause** so they can be copied into your own project without
conditions. Documentation is **CC-BY-4.0**.

The license of a file is decided by its path. There are no per-file headers;
the machine-readable map is [`REUSE.toml`](REUSE.toml) and the full license
texts are in [`LICENSES/`](LICENSES/).

| Path | License |
| :--- | :--- |
| `src/` (Core, Controls, Extras, Canvas, Image, TerminalHost, TestRunner) and `tests/` | `LicenseRef-Qtui-Commercial OR LGPL-3.0-only OR GPL-3.0-only` |
| `examples/` | `BSD-3-Clause` |
| `cmake/`, `scripts/`, `.github/`, root `CMakeLists.txt` | `BSD-3-Clause` |
| `docs/`, every `*.md`, the `doc/` directories under `src/` | `CC-BY-4.0` |
| Third-party code fetched at build time (utf8proc, libvterm) | MIT, see [`THIRD-PARTY.md`](THIRD-PARTY.md) |

## Which license applies to you

- **LGPLv3**: link Qtui as shared libraries, keep Qtui's source and your
  changes to it available, and let your users relink against a modified
  Qtui. Your own application code stays under your own terms.
- **GPLv3**: your whole application is GPLv3. Required if you link Qtui
  statically and do not want the LGPL relinking obligation, or when you build
  for WebAssembly, where Qt itself is available only under GPLv3 or a
  commercial license.
- **Commercial**: none of the above obligations. Contact the copyright
  holder through <https://github.com/yosriayed/Qtui>.

Qtui's WebAssembly builds are offered under GPLv3 or the commercial license
only, mirroring Qt for WebAssembly.

## Contributing

Contributions require the contributor agreement in [`CLA.md`](CLA.md), so that
Qtui can keep being offered under all three licenses. See
[`CONTRIBUTING.md`](CONTRIBUTING.md).

Qtui is an independent project of its copyright holder. It is not a product of
The Qt Company. Qt is licensed separately; see <https://www.qt.io/licensing>.
