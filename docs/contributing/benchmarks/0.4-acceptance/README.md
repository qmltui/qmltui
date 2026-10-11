# Qamui 0.4 acceptance checks

## Scope and method

Linux validation of the proposed rendering changes plus acceptance fixes.
The baseline is commit `f823653`, with only the current
`tests/Core/benchmark_render.cpp` and its CMake target added for measurement.
The candidate is the working tree; neither snapshot was committed by this check.

Both benchmark builds use GCC 13.3, Qt 6.8.6, CMake `Release`, Behavior OFF,
and tests, examples, benchmarks, and TerminalHost ON. The host reports an Intel
Core Ultra 9 185H under a Microsoft hypervisor. CPU frequency and host load are
not controlled, so timings are indicative rather than portable thresholds.

Each of three separate processes per version runs five warm-up frames and 50
measured frames for each scenario, size, and mode. Runs alternate baseline and
candidate, with no concurrent build or test process started by this check.
The benchmark source is byte-identical in both builds. Each JSON file preserves
the per-run medians, p95 values, output sizes, and sampling parameters.

Measurement source SHA-256:
`9ba8e3680ae7aa4580b4eccacff08a69b1adcc52e47145ed9db743e04d1168a0`.
Both benchmark compile commands use `-O3 -DNDEBUG -std=gnu++20 -fPIC`.

Representative invocation (set `QML_IMPORT_PATH` to that build's `qml` directory):

```sh
env QT_QPA_PLATFORM=offscreen QML_IMPORT_PATH=<build>/qml \
  <build>/tests/benchmark_render --strict --frames 50 --json before-1.json
```

`paintMedianUs` includes traversal/layout/paint but excludes the preceding
buffer clear for item scenarios. `updateMedianUs` includes the mutation callback
(including setters and helper work); `updatePaintMedianUs` is the median of each
frame's combined update and paint duration, not a sum of independent medians.
Diff/encoding is timed separately. Construction, terminal writes, and terminal
emulator rendering are not measured. Values are truncated to whole microseconds.

An idle microbenchmark deliberately paints repeatedly to measure paint cost.
It does not measure idle application scheduling. The application idle-budget
scenarios separately check frame counts over a settled 500 ms interval.

## Benchmark results

These values are the median of the three process-level medians, in microseconds.
The speedup is baseline divided by candidate.

| Idle scene | Size | Baseline | Candidate | Speedup |
|---|---:|---:|---:|---:|
| 50 Text rows | 80x24 | 116,399 us | 36 us | 3,233x |
| 50 Text rows | 240x70 | 237,774 us | 137 us | 1,736x |
| Bordered Rectangle | 80x24 | 81 us | 14 us | 5.8x |
| Bordered Rectangle | 240x70 | 776 us | 98 us | 7.9x |
| ListView, 1,000 rows | 80x24 | 359 us | 61 us | 5.9x |
| ListView, 1,000 rows | 240x70 | 991 us | 98 us | 10.1x |
| Composite view | 80x24 | 631 us | 280 us | 2.3x |
| Composite view | 240x70 | 1,491 us | 890 us | 1.7x |

The author's direction and order of magnitude reproduce. Exact values differ,
as expected on an uncontrolled host: their 80x24 text result was 102 ms to 28
us; this run is 116 ms to 36 us. At 240x70, this run is 238 ms to 137 us versus
their 248 ms to 66 us. Rectangle and ListView improvements also reproduce.

Moving parsing into a setter does not hide the changing-text cost. For the
mutating 50-row scene, median combined update+paint falls from 35,658 us to 33
us at 80x24, and from 120,678 us to 101 us at 240x70. The raw buffer-fill path
also improves 5.0x to 7.0x on idle paint, supporting the value-storage/encoder
changes independently of text caching. Composite gains are smaller at 1.7x to
2.3x, which is a useful reminder not to extrapolate the text result to every UI.

All idle samples had zero encoded diff bytes. Raw measurements are in
[`before-1.json`](before-1.json), [`before-2.json`](before-2.json),
[`before-3.json`](before-3.json), [`after-1.json`](after-1.json),
[`after-2.json`](after-2.json), and [`after-3.json`](after-3.json).

## Acceptance fixes

- Preserve pending bytes and the notifier when disabling asynchronous output
  times out. Make final shutdown bounded and idempotent, restore descriptor/raw
  input state, and report an undelivered tail instead of subsequently blocking.
- Invalidate TableView layout when its column-width provider changes.
- Complete Stream prompt layout before choosing its buffer height, including
  the direct `print()` path.
- Repair the view benchmark's delegate contract and one-column model setup,
  and reject missing delegates instead of timing an empty view.
- Keep the custom C++ item example runnable without optional Behavior.
- Document API/layout migration and the process-lifetime long-glyph intern table.

## Test results

| Configuration | Focused checks | Full configured CTest suite |
|---|---|---|
| Release, Behavior ON, TerminalHost ON | render loop and terminal device passed | 72/72 passed |
| Release, Behavior OFF, TerminalHost ON, fresh build | render loop, terminal device, custom item smoke passed | 71/71 passed |

The extra enabled test is Industrial PLC. Both configurations also passed the
installed external custom-item consumer check. The repaired `benchmark_views`
runs successfully without the former delegate warnings.

Focused regressions cover same-frame TableView delegate geometry, Stream prompt
growth/shrink and direct printing, queue recovery after timeout, partial writes
with a slow reader, descriptor restoration, raw input restoration, and bounded
destruction. Pipe and real PTY pairs are exercised; the destructor case uses the
default 2 s deadline against a terminal that is not reading.

The btop, file-manager, and service-monitor idle scenarios passed in both trees,
with zero recorded paint-time invalidations. File manager and service monitor
require zero additional frames in 500 ms; btop permits up to two because its
clock/model timers may fire. This is not a reproduction of the author's exact
baseline btop frame count.

Committed `final.txt` frames and corresponding `result.json` files were inspected
under `build/examples/{apps,concepts}/.../TestRunnerArtifacts/` for these three
idle scenarios and `custom_cpp_item_smoke`; the disabled custom-item frame was
also inspected in the fresh build. Application checks use quiet committed-frame
capture, not a human-observed terminal emulator session.

Commands use `QT_QPA_PLATFORM=offscreen` on this headless host:

```sh
env QT_QPA_PLATFORM=offscreen ctest --test-dir <build> \
  --output-on-failure -R '^(test_terminal_device|test_render_loop|custom_cpp_item_smoke)$'
env QT_QPA_PLATFORM=offscreen ctest --test-dir <build> --output-on-failure -j4
```

## Remaining limits

macOS and experimental Windows were not exercised. PTY tests cover kernel
backpressure, ordering, bounded shutdown, and terminal mode restoration, not
visual compatibility with every terminal emulator. Unique long glyph clusters
remain interned until process exit; no bounded-memory redesign is included.

Use separate build directories for Behavior ON and OFF. Reusing a partially
generated ON directory for OFF produced stale QML registration/link errors;
the disabled validation uses a fresh directory instead.
