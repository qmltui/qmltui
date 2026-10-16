# Inline mode and scrollback

`Terminal.AltScreen` uses the alternate screen and fills the viewport.
`Terminal.Inline` uses the normal buffer and sizes itself to visible content,
up to the terminal row count minus one. Bind `mode` to switch presentation.
Give inline content finite height; avoid vertically filling a content-sized root.

Mouse events use the live region's screen position, not an assumed bottom
alignment. InlineRenderer requests a cursor-position report (`CSI 6 n`)
after drawing and whenever scrollback, prompt height, or viewport size changes.
The final cursor row is outside the live region. Terminals must support that
report for live inline mouse input; while the position is unknown, input does
not activate items, and releasing an existing grab cancels its click.

`Scrollback` is nonvisual. `print()` appends text, `append()` snapshots an item,
and a model plus delegate can append structured rows. Already committed history
is not rewritten. In alt-screen mode, accepted output waits until inline mode
resumes or the session stops. The shared queue is limited to 1 MiB; watch
`outputRejected(reason)` when producers can outrun output.

See [Terminal](../qml-types/terminal.md),
[Scrollback](../qml-types/scrollback.md), the [inline REPL](../../examples/topics/inline_repl/),
and the [streaming agent example](../../examples/topics/agent_stream/).
