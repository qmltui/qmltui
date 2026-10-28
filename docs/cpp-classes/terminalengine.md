# TerminalEngine

`TerminalEngine` coordinates terminal I/O, input decoding, resize handling, protocol negotiation, and frame rendering for a root [`Terminal`](terminal.md).

- **Header**: `src/QmlTui/Core/Engine/TerminalEngine.h` *(in-tree engine header; external C++ applications interact through [`qmltui::Terminal`](terminal.md))*
- **Inherits**: `QObject`

## Signals

- `void frameRendered(quint64 sequence)` — emitted after a frame is painted and committed, passing the monotonically increasing frame sequence number.
- `void scrollbackCommitted(quint64 totalRows)` — emitted before `frameRendered()` when a frame commits scrollback rows in `Inline` mode.
- `void capabilitiesChanged()` — emitted when detected or overridden terminal capabilities change.
- `void outputSessionInvalidated(quint64 generation)` — emitted when the output session generation changes or a write fails, allowing image/resource caches to invalidate terminal-side handles.
- `void renderFailed(const QString &message)` — emitted when a synchronous frame write or asynchronous output drain fails.
- `void clipboardReadFinished(const QString &text, const QString &code)` — emitted when an OSC 52 clipboard read completes (`code` is empty on success).

## Public Methods

- `explicit TerminalEngine(Terminal *rootWindow, QObject *parent = nullptr)` — constructs the engine bound to `rootWindow`.
- `void requestRender()` (`Q_INVOKABLE`) — schedules an asynchronous frame render on the next event-loop tick.
- `void stop()` — stops the active renderer, flushes pending scrollback and control sequences, restores terminal state, and ends the alternate-screen message session.
- `quint64 frameSequence() const` — returns the monotonically increasing sequence number of the most recently committed frame.
- `FrameStats lastFrameStats() const` / `FrameStatsTotals frameStatsTotals() const` — returns timing and byte metrics for the most recently committed frame and cumulative session totals.
- `std::optional<TextGridBuffer> currentFrame() const` — returns the most recently committed character-cell grid, or `std::nullopt` before the first frame commits.
- `void setRenderer(Renderer *renderer)` / `Renderer *renderer() const` — installs or returns the active renderer (`AltScreenRenderer` or `InlineRenderer`).
- `void setMaxFrameRate(int framesPerSecond)` / `int maxFrameRate() const` — configures the frame rate cap (`0` disables pacing; defaults to `60` via `QMLTUI_MAX_FPS`, or `0` under `Runner`).
- `void setOutputQueueLimit(size_t bytes)` / `size_t outputQueueLimit() const` — configures the pending output byte threshold (`256 KiB` by default) above which frame commits are deferred for backpressure.
- `const TerminalCapabilities &capabilities() const` / `void setCapabilities(const TerminalCapabilities &capabilities)` — returns or overrides terminal capabilities (cancelling pending protocol probes).
- `bool enqueueControlSequence(QByteArray bytes)` / `QByteArray takeControlSequences()` — queues or drains terminal resource commands emitted before the next cell diff (bounded to `ControlSequenceQueueLimit` = `4 MiB`).
- `bool enqueuePostControlSequence(QByteArray bytes)` / `QByteArray takePostControlSequences()` — queues or drains terminal resource commands (such as Sixel graphics) emitted after the cell diff each frame.
- `int terminalWidth() const` / `int terminalHeight() const` — current terminal viewport dimensions in columns and rows.
- `bool submitScrollback(const QString &text)` / `bool submitScrollback(const TextGridBuffer &rows)` — queues text or painted rows for scrollback emission (bounded to `ScrollbackQueueLimit` = `1 MiB`).
- `size_t pendingScrollbackBytes() const` / `size_t pendingScrollbackRowCount() const` — returns the queued scrollback byte or cell-row count awaiting emission.
- `void setScrollbackHistoryEnabled(bool enabled)` / `bool scrollbackHistoryEnabled() const` — enables or queries cell-level scrollback history recording (enabled automatically in `Runner` sessions).
- `void setScrollbackHistoryLimit(int rows)` / `int scrollbackHistoryLimit() const` — configures the maximum retained scrollback rows (`DefaultScrollbackHistoryLimit` = `5000`, overridden by `QMLTUI_SCROLLBACK_HISTORY_ROWS`).
- `quint64 committedScrollbackTotal() const` / `int committedScrollbackRowCount() const` / `TextGridBuffer committedScrollback(quint64 fromTotalRow = 0) const` — inspects monotonic and retained scrollback rows committed in `Inline` mode.
- `QString requestClipboardText(int timeoutMs)` / `void cancelClipboardRead()` — starts or cancels an OSC 52 clipboard read (exposed in QML via [`Clipboard`](../qml-types/clipboard.md)).
- `void copyToClipboard(const QString &text)` — sends an OSC 52 clipboard write sequence when output is enabled.
- `void queryColors()` (`Q_INVOKABLE`) — queries the host terminal's default foreground (`OSC 10`), default background (`OSC 11`), and 16 ANSI palette colors (`OSC 4;0..15`).
- `void updateBackgroundColor()` / `void reapplyBackgroundColor()` — synchronizes the host terminal window background via OSC 11 (`\033]11;#rrggbb\a`) when the root `Terminal` has an opaque background in `AltScreen` mode, or resets it via OSC 111 (`\033]111\a`) on exit or in `Inline` mode.
- `static std::string osc52Sequence(const QString &text)` / `static std::string osc11Sequence(const Color &color)` / `static std::string osc111Sequence()` — helper encoders for OSC 52, OSC 11, and OSC 111 sequences.

## Protocol Negotiation and Resource Output

When an interactive output session starts (with both `stdin` and `stdout` attached to a TTY), the engine queries Kitty keyboard flags, cell pixel size (`CSI 16 t`), synchronized output (`?2026$p`), and primary device attributes (`DA1`). The Kitty graphics APC probe is sent only to terminals known to parse APC strings cleanly (`kitty`, `Ghostty`, `WezTerm`, `Konsole`). Query replies never leak as key events. `DA1` closes negotiation, bounded by a 100 ms timer (`QMLTUI_QUERY_TIMEOUT_MS`). Environment overrides (`QMLTUI_COLOR_DEPTH`, `QMLTUI_SYNC_OUTPUT`, `QMLTUI_KITTY_KEYBOARD`, `QMLTUI_KITTY_GRAPHICS`) are re-applied after replies so explicit overrides always win.

`Terminal::enqueueControlSequence(QByteArray)` queues a batch of terminal resource commands (such as image uploads and placements) up to a 4 MiB engine budget. Queued commands are emitted immediately before the same frame's cell diff and flushed before renderer switches or shutdown.
