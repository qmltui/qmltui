# Inline mode and scrollback

`Terminal.FullScreen` uses the alternate screen and fills the viewport.
`Terminal.Inline` uses the normal buffer and sizes itself to visible content,
up to the terminal row count minus one. Bind `mode` to switch presentation.
Give inline content finite height; avoid vertically filling a content-sized root.

`Scrollback` is nonvisual. `print()` appends text, `append()` snapshots an item,
and a model plus delegate can append structured rows. Already committed history
is not rewritten. In full-screen mode, accepted output waits until inline mode
resumes or the session stops. The shared queue is limited to 1 MiB; watch
`outputRejected(reason)` when producers can outrun output.

Use Runner's `expectScrollbackText()` for history and `expectText()` for the
live frame. See [Runner](../cpp-classes/runner.md), [Terminal](../qml-types/terminal.md),
[Scrollback](../qml-types/scrollback.md), the [inline REPL](../../examples/topics/inline_repl/),
and the [streaming agent example](../../examples/topics/agent_stream/).
