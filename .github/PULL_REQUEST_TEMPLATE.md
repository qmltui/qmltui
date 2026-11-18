## Summary

<!-- Briefly describe what changed and why. Link any related issue (`Fixes #...`). -->

## Verification

<!-- Check all that apply (see CONTRIBUTING.md): -->

- [ ] Built and ran relevant CTest unit / `Runner` scenarios (`ctest --test-dir build -LE Demo --output-on-failure`)
- [ ] Verified rendered terminal frames (`qmltui-runner` / snapshots) for visible UI changes
- [ ] Ran `./scripts/format.py --check src tools examples tests` and `git diff --check`
- [ ] Ran `python3 ./scripts/check_docs.py --build-dir build` (if touching docs, examples, or public APIs)
- [ ] Tested `-DQMLTUI_ENABLE_PRIVATE_QT=OFF` in addition to `ON` (if touching `Behavior`, `PropertyChanges`, or feature gating)

## Licensing & CLA

- Contributions limited to `docs/`, `examples/`, `cmake/`, `scripts/`, or `.github/` are accepted directly under their `BSD-3-Clause` / `CC-BY-4.0` terms without CLA sign-off.
- First-time contributions touching `src/`, `tests/`, or `tools/` (`LGPL-3.0-only`) require commenting `I have read the CLA Document and I hereby sign the CLA` on the pull request (see [`CLA.md`](CLA.md)).
