# QmlTui Contributor License Agreement

QmlTui's library, test, and tooling code (`src/`, `tests/`, and `tools/`) is
distributed under the GNU Lesser General Public License v3.0 (`LGPL-3.0-only`).
Examples, CMake and helper scripts, and documentation (`examples/`, `cmake/`,
`scripts/`, and `docs/`) are distributed under permissive `BSD-3-Clause` or
`CC-BY-4.0` terms as mapped in [`REUSE.toml`](REUSE.toml) and
[`LICENSE.md`](LICENSE.md).

To preserve long-term project stewardship and future licensing flexibility while
permanently guaranteeing an open-source release — and while letting every
contributor keep ownership of their own copyright — contributions to the
`LGPL-3.0-only` paths are covered by the license grant below.

## Agreement

By submitting a contribution to QmlTui (a pull request, patch, or other
material you intend to be included in the project), you, the contributor, agree
to the following with Yosri Ayed (the "Project Owner"):

1. **Copyright ownership and license grant (`LGPL-3.0-only` paths).** You retain
   ownership of the copyright in your contribution. For any contribution to the
   `LGPL-3.0-only` paths defined in [`REUSE.toml`](REUSE.toml) (`src/`,
   `tests/`, and `tools/`, excluding `CC-BY-4.0` documentation files), you
   grant the Project Owner (and any successor in project stewardship) a
   non-exclusive, perpetual, irrevocable, worldwide, royalty-free, transferable
   license to use, reproduce, modify, prepare derivative works of, publicly
   display, distribute, sublicense, and relicense that contribution and
   derivative works under `LGPL-3.0-only` and any other open-source or
   alternative license terms.
2. **Patent grant (`LGPL-3.0-only` paths).** For any contribution to those same
   paths, you grant the Project Owner and every recipient of software
   distributed by the Project Owner a non-exclusive, perpetual, irrevocable,
   worldwide, royalty-free patent license to make, have made, use, offer to
   sell, sell, import, and otherwise transfer your contribution, under only
   those patent claims licensable by you that are necessarily infringed by your
   contribution alone or by combination of your contribution with QmlTui at the
   time of submission. If any entity initiates patent litigation alleging that
   QmlTui or a contribution incorporated into QmlTui constitutes patent
   infringement, any patent licenses granted to that entity under this
   agreement terminate as of the date that litigation is filed.
3. **Open-source promise.** Any public distribution of QmlTui by the Project
   Owner that includes your contribution will remain available under
   `LGPL-3.0-only` (or a more permissive FSF-free and OSI-approved open-source
   license), in addition to any other terms.
4. **Your representations.** You represent that you are the original author of
   the contribution (or otherwise have the legal right to grant the licenses
   above, including any required authorization or waiver from your employer)
   and that, to your knowledge, the contribution does not infringe any third
   party's rights.
5. **No warranty, no obligation.** Your contribution is provided "as is",
   without warranties or conditions of any kind. The Project Owner is not
   obliged to include it.
6. **Scope.** Clauses 1 through 3 apply only to contributions to `src/`,
   `tests/`, and `tools/` (excluding `CC-BY-4.0` documentation files).
   Contributions to `examples/`, `cmake/`, `scripts/`, `docs/`, and other
   `BSD-3-Clause` or `CC-BY-4.0` paths mapped in [`REUSE.toml`](REUSE.toml) are
   submitted directly under their respective `BSD-3-Clause` or `CC-BY-4.0`
   terms without requiring this agreement. This agreement covers contributions
   you submit from the date you first accept it until you withdraw in writing;
   withdrawal is prospective and does not affect contributions already
   submitted.

## How to accept

On your first pull request that modifies `src/`, `tests/`, or `tools/`, add the
comment:

```text
I have read the CLA Document and I hereby sign the CLA
```

Later pull requests are covered automatically. If the CLA check needs to be
re-run on an existing pull request, comment `recheck`. Pull requests that only
touch `docs/`, `examples/`, `cmake/`, `scripts/`, or other `BSD-3-Clause` /
`CC-BY-4.0` paths do not require CLA sign-off.
