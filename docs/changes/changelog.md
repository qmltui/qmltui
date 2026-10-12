# Changes in 0.5.0

The framework and QML modules use the QmlTui name. Environment settings use
`QMLTUI_*`, including Runner's `QMLTUI_RUNNER_*` variables. Frame logging uses
`qmltui.frames`. Consult the porting guides for removed APIs and previous names.

The gallery now lives under `examples/gallery`; `examples/concepts` became
`examples/topics`. Example targets consistently use `qmltui_<directory>_demo`.
Full applications moved to the separate qmltui-apps project.

Documentation now has Getting Started, Tutorials, Topics, QML Types, C++ Classes,
Examples, and Changes entry points. Reference documentation no longer lives
beside implementation files.

Runner supports committed-scrollback assertions, deterministic virtual time,
and asciicast v2 recording with `RECORD`, `startRecording()`, `stopRecording()`,
and `marker()`. See the [Runner API](../cpp-classes/runner.md).

- [Porting from 0.1](0.1.md)
- [Porting from 0.3](0.3.md)
- [Porting from 0.4](0.4.md)
