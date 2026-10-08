# Plan 08: Terminal Protocols (Control-Sequence Parser, Kitty Keyboard, Kitty Graphics)

## 1. Goal

Treat the terminal as a device Qtui negotiates with rather than guesses about.
Three deliverables, each its own PR, in this order:

1. A real control-sequence parser in `InputDecoder`, a query-and-reply path
   for capabilities, and an engine hook for out-of-band byte sequences.
2. The kitty keyboard protocol, so Escape is unambiguous, Ctrl and Alt
   combinations carry their real modifiers, and Ctrl+Shift, Ctrl+Enter and
   Shift+Enter become expressible.
3. The kitty graphics protocol in `Qtui.Image`, using Unicode placeholders so
   an image is ordinary cells to the frame renderer.

Everything degrades to today's behaviour when the terminal does not answer a
query. Nothing regresses on xterm, Windows Terminal, or an asciinema recording.

This plan implements items 1 to 3 of Plan 06 section 5. Signal safety (Plan 06
section 5 item 4) must land before WP2 and WP3: the keyboard protocol has to be popped on
`SIGTSTP` and pushed again on `SIGCONT`, and that requires the handlers.

## Implementation status

WP1 has an initial implementation of structured input framing, bounded discard
recovery, capability negotiation, QML capability exposure, and queued resource
commands in both renderers. The queue budget is 4 MiB and each batch is accepted
atomically. Programmatic capability overrides suppress negotiation. Focused
coverage lives in `test_input_decoder` and `test_terminal_queries`, including a
committed QML frame and immediate/asynchronous output failures.

Review of the initial implementation (2026-09-12) found and fixed: ESC followed
by `]`, `_` or `P` was framed as a string control and swallowed Alt-prefixed
typing (now framed only when the first payload byte matches a reply shape);
SS3 inherited the CSI scan-to-final grammar (now one final byte, else Alt+Shift+O);
probes were sent to pipes and the APC probe to terminals that print it (now
gated on both channels being interactive, and the graphics probe on a known
terminal); a second negotiation re-seeded from environment defaults and could
downgrade proven capabilities on timeout (now seeded from the current set);
environment overrides were checked in two places with different spellings
(now one function applied after detection and after replies); a failed probe
write was reported twice and never retried; the output-error path did not
reset renderer state; `setTerminalOutputEnabled(false)` emitted the
invalidation before the flag flipped. `TestRunner.typeText()` now feeds bytes
through the decoder instead of mirroring its table.

Known gap carried into WP2: the decoder delivers a lone Escape only when a
following byte arrives, on a real terminal as much as under test. The kitty
disambiguate flag removes the ambiguity on capable terminals; legacy terminals
need an Escape timeout, which WP2 should add alongside the push.

Signal safety (the prerequisite) is implemented (2026-09-12):
`PosixTerminalBackend` installs handlers for the fatal signals, SIGTERM/SIGHUP/
SIGINT/SIGQUIT, and job control while in raw mode. Each restores the terminal
(reporting modes off, cursor shown, background reset, normal screen) and the
saved termios with async-signal-safe calls only, then re-raises so a crash
still dumps core and a stop still stops. SIGCONT re-enters raw mode in the
handler and emits `resumed()`, which `TerminalEngine::handleResume()` follows
by re-enabling every mode and repainting. Covered by `test_signal_safety`,
which drives a real process under a pseudo-terminal through Ctrl+Z, fg and
SIGTERM and checks both the bytes and the tty modes.

WP3 kitty graphics is implemented (2026-09-13). `Image` gains `Kitty` and
`Auto` render modes, `Auto` is the default, and `graphics::resolve()` chooses
kitty when the terminal supports it and true colour, else half-block. The kitty
path uploads the source as PNG once under a process-unique id, creates a Unicode
virtual placement sized to the item's cells, and paints placeholder cells that
carry the id in their foreground; it frees the upload on source change, item
destruction (per item) and engine stop (`a=d,d=A`, all images). Covered by
`test_kitty_graphics` through the memory backend. The graphics-protocol seam
lives in `src/Qtui/Image/TerminalGraphics.{h,cpp}`: a `Protocol` enum, a
`resolve()` preference order, and the kitty encoders, so sixel and iTerm2 add
a detector case, a `resolve()` case, and an encoder without touching `Image`.

Sixel is implemented too (2026-09-13). `graphics::sixelEncode()` quantises to a
256-entry palette and emits a DCS sixel string; `Image::paintSixel()` reserves
the image cells, scales the source to the placement pixels, and enqueues a
cursor-positioned sixel through a new post-diff control queue
(`enqueuePostControlSequence`) drained by both renderers after the cell diff.
`resolve()` prefers kitty, then sixel, then none, so `Auto` picks sixel on
Windows Terminal and kitty elsewhere. Re-emit is keyed on source, geometry and
output-session generation, so a resize, resume or renderer switch redraws it. Each re-emit first clears the item rectangle with blanks, because sixel with
P2=1 leaves unset pixels untouched and transparent frames would otherwise
accumulate under one another (found on Windows Terminal, 2026-09-13).
Covered by `test_kitty_graphics` (sixel cases). Real-terminal verification (2026-09-13): kitty confirmed on Ghostty, sixel confirmed on Windows Terminal. Documented limitation: sixel
paints at the cursor, so it is drawn after the cells and cannot sit under other
cell content the way kitty placeholders can; it is meant for a dedicated image
region. Inline (Stream) mode uses absolute positioning that assumes full-screen;
sixel is intended for FullScreen for now.

The iTerm2 inline-image protocol is the remaining candidate and would follow
the same seam (a detector, a `resolve()` case, an encoder).

Keyboard mode activation and general Kitty key decoding (WP2) remain subsequent
work. No real-terminal Kitty compatibility claim is established by the
memory-backend tests; a manual check on kitty or Ghostty is still owed.

## 2. Baseline (before WP1; verified on 2026-09-12)

- `InputDecoder::processBuffer()` matches control sequences by hand. From
  `InputDecoder.cpp:228` it finds the final byte, takes the bytes between as a
  string, and compares it to literals (`"3"`, `"3;"`, `"27;2;9"`). Function keys
  are fourteen `else if` branches. There is no parameter list, no
  sub-parameter (`:`) support, and no recognition of APC strings (`ESC _` to
  `ESC \`), used by kitty graphics replies (keyboard replies use CSI). The
  one kitty-shaped case, `9;2u` for Shift+Tab, is a literal match.
- `TerminalCapabilities` (`primitives/TerminalCapabilities.h`) holds colour
  depth, synchronized output, tty state and program name, derived once from
  environment variables. The terminal is never asked anything and no reply is
  ever parsed.
- `FrameRenderer` emits the diffed cells between `\033[?2026h` and
  `\033[?2026l` (`FrameRenderer.cpp:49`). No item can hand the engine a byte
  sequence to write; paint only produces cells.
- `Image` renders `HalfBlock`, `Braille`, and `ASCII` (`Image.cpp:183`). Plan 00
  mentions a sixel fallback; no sixel code exists. The module links `Qt6::Gui`, so it
  can encode PNG.
- `Cell` stores up to 12 UTF-8 bytes inline (`Cell::InlineGlyphCapacity`).
  Verify placeholder encoding against the protocol's actual diacritic table;
  do not assume every combining mark occupies two UTF-8 bytes.
- `AnsiEncoder_p.h` maps foreground colours to the configured colour depth.
  Placeholder image IDs must not pass through lossy colour conversion.
- `MemoryTerminalBackend` (`TestRunner/MemoryTerminalBackend.h`) captures
  everything the engine writes and feeds bytes in. Every test below that says
  "end to end" uses it.

## 3. WP1: Parser, Queries, and the Raw-Sequence Hook — 3–4 days

### Control-sequence parser

One function reads a framed sequence from the current buffer position and
returns a struct, or reports that more bytes are needed:

```cpp
struct ControlSequence {
    enum Kind { Csi, Ss3, Apc, Osc, Dcs } kind;
    char privateMarker = 0;                       // '?', '>', '<', '=' or 0
    QVarLengthArray<QVarLengthArray<int, 3>, 8> params;  // params[i][j]; ':' splits j
    QByteArrayView intermediates;                // bytes 0x20..0x2F, e.g. DECRQM $
    char finalByte = 0;                           // CSI and SS3
    QByteArrayView payload;                       // APC, OSC, DCS body
    int length = 0;                               // bytes consumed
};
```

Rules:

- Parameters are decimal, `;` separates, `:` splits sub-parameters. Preserve
  omitted values distinctly (for example, -1); each consumer applies its own
  defaults. Kitty modifiers and event types default to 1, not 0. Reject integer
  overflow and invalid Unicode scalar values before dispatch.
- Parse CSI parameter, intermediate, and final bytes separately. The final is
  in `0x40..0x7E`; retain intermediate bytes so `CSI ? 2026 ; status $ y`
  can be recognized without confusing it with a key.
- APC and DCS terminate at ST (`ESC \`); OSC also accepts BEL. Preserve a
  terminator split across reads. Views are valid only during synchronous
  dispatch, before the input buffer is modified.
- Hard limits: 64 bytes for CSI, 4 KB for incoming string sequences. On overflow,
  stop buffering but remain in a discard state until the sequence terminates
  or an explicit parser cancellation/resynchronization occurs. Never reinterpret
  the remaining payload as keys. Define CAN/SUB cancellation and ESC restart
  rules and test them, including an unterminated oversized string.
- Preserve standalone Escape timing, split UTF-8, Alt-prefixed input, mouse
  input, and bracketed paste. Paste payload must bypass control-reply routing.

Every existing branch is rewritten on top of the struct: arrow and edit keys
become a table on `finalByte`, function keys a table on `params[0][0]`,
mouse SGR reads `params[0..2]` directly. `xtermModifiers()` reads
`params[1][0]`. The existing `test_input_decoder` suite is the regression net;
it must pass unchanged before any new case is added.

### Query-and-reply path

At engine output-session startup, write one batch after renderer startup
(and alternate-screen entry in Frame mode), before the first frame. Do not
probe when terminal output is disabled:

| Query | Bytes | Reply fills |
| :--- | :--- | :--- |
| Kitty keyboard flags | `CSI ? u` | `capabilities.kittyKeyboard` |
| Kitty graphics probe | `APC G i=31,s=1,v=1,a=q,t=d,f=24;AAAA ST` | `capabilities.kittyGraphics` |
| Cell size in pixels | `CSI 16 t` | `capabilities.cellPixelSize` |
| Synchronized output | `CSI ? 2026 $ p` (DECRQM) | `capabilities.syncOutput`, replacing the guess |
| Primary DA | `CSI c` | completion marker for supported ordered replies |

Replies arrive through the decoder and are matched by complete sequence shape
(private marker, parameters, intermediates, final byte, or graphics query ID).
Route recognized replies away from keys even after the query window closes.
A valid `CSI ? 0 u` proves keyboard support: the flags describe the current
mode, not a support bitmask. For DECRQM, distinguish unsupported status 0 from
supported modes that are currently set or reset; keep explicit overrides.

Send DA1 last as the protocol-recommended completion marker, with a separate
100 ms negotiation deadline (`QTUI_QUERY_TIMEOUT_MS` override). Do not tie the
deadline to frame pacing or delay the first frame. Test missing, duplicate,
malformed, and late replies; late replies are consumed without reopening the
closed window. Document that the deadline can yield conservative fallback on
slow connections. Keep environment-derived defaults until negotiation closes.
The graphics query proves transport support only; WP3 separately determines
whether Unicode placeholders are usable.

`QTUI_COLOR_DEPTH` and `QTUI_SYNC_OUTPUT` remain overrides. Two new ones,
`QTUI_KITTY_KEYBOARD=0` and `QTUI_KITTY_GRAPHICS=0`, disable a protocol the
terminal claims but gets wrong.

`TerminalCapabilities` gains `kittyKeyboard`, `kittyGraphics`, `cellPixelSize`,
and `Terminal` exposes them read-only through a `capabilities` grouped
property (`Terminal.capabilities.kittyGraphics`), with one `capabilitiesChanged`
signal when the query window closes if values changed. Capability changes
must request a frame and invalidate affected image caches. Refresh cell pixel
size after resize; subsequent changes also notify and repaint.

### Raw-sequence hook

```cpp
// Terminal.h
bool enqueueControlSequence(QByteArray bytes);  // false if disabled, stopped, or over budget
```

Keep this C++ hook internal to framework output integration initially. It is
for resource commands that preserve cursor and text rendition state; arbitrary
cursor movement would invalidate the renderer's model.

Enqueueing requests a render, including when no cells are dirty. After paint,
`FrameRenderer` drains commands before that same frame's cell diff, inside
synchronized output when enabled. `StreamRenderer` drains before its live
region. A command-only frame must be written even when the cell diff is empty.
Preserve enqueue order and keep each chunked upload contiguous.

Bound queued bytes and expose acceptance to the caller. An upload and placement
must be accepted atomically before their placeholders can be committed. Disabled
output retains no commands and must not mark resources as uploaded. Once a frame
is accepted by the device's output queue, retire its commands; on write failure
or discarded pending output, invalidate the output session and its image
resources instead of replaying an uncertain partial transmission. WP3 owns
rebuilding those resources. Teardown must flush resource cleanup before renderer
stop, even if no further frame is scheduled.

Tests: preserve the existing `test_input_decoder` regression suite and add
split-at-every-byte, omitted-parameter, intermediate-byte, overflow/discard,
cancellation, unknown-sequence, paste-isolation, and APC cases. A new
`test_terminal_queries.cpp` drives `MemoryTerminalBackend`, asserts one query
batch per output session, and covers overrides, zero keyboard flags, all DECRQM
statuses, timeout, late replies, and capability-driven repaint. Queue tests cover
commands from paint preceding the same frame's cells, command-only frames,
ordering, budget rejection, disabled output, teardown, and write failure.

Exit criterion: `test_input_decoder` passes with the rewritten decoder; the
query batch and replies are covered end to end; no behaviour change on a
terminal that answers nothing.

## 4. WP2: Kitty Keyboard Protocol — 2–3 days

### Scope

Enable flag 1 only, "disambiguate escape codes", with the push and pop form:

- push: `CSI > 1 u` after the query window closes and `kittyKeyboard` is true
- pop: `CSI < u` before leaving the active screen on stop or suspension;
  push again after raw-mode and screen re-entry on resume

Track whether this output session actually pushed a mode; never pop an unowned
stack entry. Main and alternate screens have separate stacks. Integrate with
Plan 06's signal-safe backend lifecycle; do not call QObject or allocating queue
APIs from a POSIX signal handler. Apply the same ownership rules when output is
disabled/re-enabled or the renderer mode changes, and cancel pending negotiation
on stop.

With flag 1 alone, ordinary text still arrives as UTF-8. Escape and ambiguous
modified keys become distinguishable; unmodified Enter, Tab, and Backspace keep
their legacy bytes. Test Ctrl+Enter and Shift+Enter explicitly on real terminals.

Flags 2 (event types), 4 (alternate keys), 8 (all keys as escape codes), and
16 (associated text) are not enabled. Flag 16 requires flag 8. Future opt-in
support belongs in a later plan; the decoder can still safely consume these
fields when received.

### Decoding

Final byte `u`:

```
CSI unicode-key[:shifted-key[:base-layout-key]] ; modifiers[:event-type] [; text-codepoints] u
```

- `params[0][0]` is the code point. Letters and printable characters go
  through `keyForCharacter()`. The functional-key range (`57344..63743`) maps
  through a table: `57358` CapsLock, `57399..57408` keypad digits,
  `57376..57398` F13 to F35, media keys ignored.
- Legacy values in the first parameter keep their meaning: 27 Escape,
  13 Enter, 9 Tab, 127 Backspace.
- `params[1][0]` is one plus a bitfield: 1 Shift, 2 Alt, 4 Ctrl, 8 Super,
  16 Hyper, 32 Meta, 64 CapsLock, 128 NumLock. Convert to `Qt::KeyboardModifiers`;
  Super and Meta map to `Qt::MetaModifier`; Hyper, CapsLock, and NumLock
  are dropped because this API has no corresponding modifier bits.
- `params[1][1]`, the event type, defaults to 1 (press). Type 2 (repeat)
  dispatches another press; type 3 (release) is consumed without dispatch.
  Reject unsupported event types without leaking sequence bytes into text.
- `params[2]`, associated text, is decoded from validated Unicode scalars when
  present. Do not expect it under flag 1. Specify text synthesis for legacy-like
  modified printable keys and test it against `TextInput`; decoding the key's
  code point alone must not insert control shortcuts as text.
- Legacy arrows and edit keys in kitty form (`CSI 1 ; mods A`,
  `CSI 3 ; mods ~`) already parse through WP1's tables; the `9;2u` literal for
  Shift+Tab is subsumed by the general `u` case, preserving Qtui's
  `Qt::Key_Backtab` convention.

The reply `CSI ? flags u` is a WP1 query reply, not a key.

### Fallback

Without a valid support reply, send no push and keep legacy decoding. A terminal
that answers but misbehaves is disabled with `QTUI_KITTY_KEYBOARD=0`. Record
terminal versions used for acceptance, including Windows Terminal; a Windows
unit-test CI row alone does not establish emulator protocol support.

### Tests

- `test_input_decoder`: every `u` form above, modifier combinations, a
  delivered repeat, ignored release, omitted modifiers, the CapsLock bit dropped,
  a functional key, sub-parameter forms with shifted and base keys, and the query reply routed away from keys.
- `test_terminal_queries` (WP1) gains: push written only after a positive
  reply (including zero flags), balanced pop on stop, no push/pop when the
  reply is absent, and stop during pending negotiation.
- `test_signal_safety` (Plan 06 section 5 item 4) gains: pop precedes the
  terminal reset on `SIGTSTP`, push follows raw-mode re-entry on `SIGCONT`.
- A `Keys.onPressed` scenario through `MemoryTerminalBackend`: feed the kitty
  bytes for Ctrl+Shift+A and assert `event.key == Qt.Key_A` with both
  modifiers. Render the received key and modifiers in the scenario and assert
  that committed frame. Also exercise text entry, Shift+Tab, repeats, Ctrl+Enter,
  and Shift+Enter without accidental text insertion.

Exit criterion: those tests pass; `examples/concepts/focus_and_keys` documents
Ctrl+Enter and Shift+Enter as working in kitty-capable terminals and states
the fallback.

## 5. WP3: Kitty Graphics in `Qtui.Image` — 5–8 days

### Prototype gate

Before implementing the full Image integration, demonstrate a chunked PNG upload,
a virtual placement, clipped placeholder cells, and suspend/resume recovery in
real terminals. Record emulator versions and results. This gate resolves resource
lifetime and placement semantics before committing to the implementation below.
Memory-backend tests verify Qtui's output, not emulator behaviour.

### Transmission and placeholders

Unicode placeholders let retained cells represent image fragments. Cell clipping
and diffing can remain the basis of rendering, but resource lifetime, exact image
ID encoding, and terminal resets need explicit integration.

For a multi-chunk upload, emit:

```
APC G a=t,t=d,f=100,i=<id>,q=2,m=1;<first base64 chunk> ST
APC G m=1;<continuation base64 chunk> ST
APC G m=0;<last base64 chunk> ST
APC G a=p,U=1,i=<id>,c=<cols>,r=<rows>,q=2 ST
```

Each base64 payload is at most 4096 bytes, with non-final chunks a multiple of
four bytes. For a single chunk, put `m=0` on the first command. Transmission uses
`a=t`: `a=T` without `U=1` would create an unwanted cursor-positioned placement.
`f=100` is PNG encoded through `QImage::save()` and `QBuffer`; handle encoding
failure by falling back. `q=2` suppresses acknowledgements, so queue acceptance
is not proof that a terminal successfully allocated an image.

A placeholder is U+10EEEE plus explicit row and column diacritics from the
protocol table, with a non-zero image ID encoded in the foreground's low 24 bits.
Use engine-owned IDs below 2^24; handle exhaustion without reusing a live ID.
Include explicit row and column values in every cell so clipping and overlapping
images do not depend on neighbouring cells. Verify UTF-8 storage and display
width against the actual table rather than assuming fixed diacritic byte sizes.

For this PR, require effective TrueColor output for Kitty rendering. The existing
ANSI encoder must emit the ID exactly; indexed/monochrome settings fall back to
HalfBlock, including explicit Kitty requests. This avoids silently quantizing
IDs. Supporting protocol colours independently of application colour depth is a
possible later extension.

### Geometry and fill modes

Use measured `cellPixelSize` for physical aspect ratio. If unavailable, use a
documented 1:2 width-to-height cell estimate. Refresh sizing after pixel geometry
changes, not only changes to the item's cell rectangle.

Virtual placements preserve image aspect ratio; they do not implement arbitrary
Image fill modes. Prepare a raster with the placement's physical aspect ratio:

- `Stretch`: resample into the full target pixel rectangle.
- `PreserveAspectFit`: choose an aligned smaller cell rectangle, then fit the
  source within its pixel canvas; transparent padding absorbs cell rounding.
- `PreserveAspectCrop`: scale to cover and crop into the target pixel rectangle,
  preserving the existing Image alignment/crop contract.
- `Pad` and `Tile`: compose the source into the target pixel canvas using the
  documented Image semantics; cover their alignment in tests.

Bound the prepared raster to 2048 pixels on its long edge, retaining aspect
ratio. Bound encoded bytes and total queued upload bytes as well. If preparation
or queue acceptance fails, render HalfBlock for that frame and avoid a tight
retry loop. Cache preparation by source revision, fill mode, alignment, target
size, and pixel geometry; do not encode PNG on every paint.

Start with a documented implementation limit of 256 × 256 placement cells,
subject to verification against the protocol table. Larger terminals are valid:
fall back for oversized placements instead of silently truncating them. Tiling
multiple placements is outside this PR.

### Engine-owned resources and paint

The engine owns a resource registry and an output-session generation. Image holds
a resource handle, not an assumption that bytes enqueued during an earlier paint
still exist in the terminal.

1. Prepare or reuse the raster and request a resource/virtual placement for the
   current generation. Reuse an uploaded source where only placement changes;
   retransmit if the prepared pixels changed.
2. Atomically enqueue required upload and placement commands before committing
   placeholders. Register the resource only when its frame is accepted by the
   device output queue; headless paint must not mark it uploaded.
3. Write placeholders for the visible cell rectangle, retaining their original
   row/column coordinates under clipping. Avoid duplicate commands on repeated
   paint or unchanged frames.

Source replacement, item destruction, mode changes, and moves between Terminal
roots release the handle. The registry schedules `a=d,d=I,i=<id>` when the last
owner releases it. Cleanup requests schedule command-only frames. Stop deletes
owned IDs and flushes cleanup before leaving the active screen; avoid a blanket
terminal-wide delete that can remove another application's resources.

Invalidate the resource generation on screen reset/re-entry, suspend/resume,
output disable/re-enable, renderer replacement, or failed/discarded output.
After restoration, upload and place visible images before forcing a complete
placeholder redraw. Do not assume image data or virtual placements survive
alternate-screen entry or screen clearing. Teardown must remain safe when Image
outlives its Terminal or the engine has already stopped.

### Mode selection and compatibility

`Image.RenderMode` gains `Kitty` and `Auto`; preserve explicit `HalfBlock`,
`Braille`, and `ASCII`. `kittyGraphics` denotes a positive graphics probe;
add `kittyGraphicsPlaceholders` to distinguish verified placeholder support.
`Auto` selects Kitty only with both capabilities, effective TrueColor output,
and a supported placement size. An explicit Kitty request uses the same safe
fallback when prerequisites fail. `QTUI_KITTY_GRAPHICS=0` disables its use.

A graphics `OK` reply alone does not establish placeholder support. Initially
use a conservative terminal identity allowlist backed by recorded real-terminal
results; unknown terminals fall back. Test kitty and Ghostty as initial
candidates, then WezTerm, Konsole, and Windows Terminal before making any support
claims. Record versions and detection limitations in the example documentation.

Do not assume a tmux configuration switch makes unwrapped graphics commands pass
through. Since this plan does not implement multiplexer transport wrapping,
fall back under detected tmux sessions unless the prototype verifies the exact
transport and placeholder path. Document the tested configuration, not a blanket
`allow-passthrough` recommendation.

Keep HalfBlock as the default until the acceptance gate below passes. Change the
default to Auto in this PR only after that evidence exists. Cast scenarios must
explicitly choose HalfBlock so recordings remain deterministic.

### Tests and acceptance

Through `MemoryTerminalBackend`, with capabilities injected deterministically:

- First frame writes one complete upload, one virtual placement, then exact
  placeholder cells and ID colours. Include one- and multiple-chunk PNGs.
- An unchanged frame writes no graphics commands; placement-only changes reuse
  image data where possible.
- Source replacement and destruction release owned IDs; command-only cleanup
  and stop flush them without deleting unrelated IDs.
- Assert committed frames for every fill mode, alignment, partial clipping,
  overlap, movement, and pixel-size changes. Inspect prepared raster dimensions
  and content where a text frame cannot establish correct pixel composition.
- Suspend/resume, output toggling, renderer replacement, and injected output
  failure invalidate resources and rebuild them before the placeholder redraw.
- Queue-budget rejection, encoding failure, oversized placements, unsupported
  placeholders, and reduced colour depth all fall back without stale handles.
- Auto fallback matches today's HalfBlock frame against the committed
  `examples/components/image` snapshot. Cover explicit ASCII and Braille too.

Real-terminal acceptance must verify pixels, clipping/overlap, resize, cleanup,
and suspend/resume on each terminal admitted to the allowlist. Verify Frame and
Stream modes; if Stream cannot pass, explicitly restrict Kitty to Frame mode
and test Stream fallback. Smoke-test legacy input/output on xterm and Windows
Terminal, and inspect an asciinema recording with HalfBlock selected. Save
terminal versions, settings, frame artifacts, and visual captures in the handoff.

Exit criterion: automated tests and the real-terminal acceptance gate pass;
`examples/components/image` demonstrates Auto and explicit modes, and its doc
page records the tested support list, fallback rules, and multiplexer limits.
If the prototype fails, revise WP3 before landing it; WP1 and WP2 remain useful
independently.

## 6. Order, Estimates, and Exit

| Order | Package | Days | Depends on | Exit criterion |
| :---: | :--- | :---: | :--- | :--- |
| 0 | Signal safety (Plan 06 §5.4) | 0.5–1 | — | suspend/resume restores terminal state safely |
| 1 | WP1 parser, queries, hook | 3–4 | — | parser regressions and query/queue lifecycle tests pass |
| 2 | WP2 kitty keyboard | 2–3 | WP1, signal safety | key/modifier committed frames and balanced mode restoration |
| 3 | WP3 kitty graphics | 5–8 | WP1, signal safety, graphics prototype | resource recovery, fallback, and real-terminal acceptance |

Budget roughly 11–16 engineer-days including the prototype and compatibility
validation; availability of real terminal environments can add elapsed time.
WP1 can proceed before signal safety; WP2 and WP3 need it for lifecycle acceptance.
All deliverables land after the `v0.4.0` tag (Plan 00): decoder and output-path
changes must not disrupt launch recordings or a newcomer's first session.

For each implementation PR, build affected targets and run the narrowest relevant
CTest first, then the broader configured suite because these changes touch Core,
public APIs, and QML modules. Use the normal tests/examples-enabled tree. Exercise
Behavior-enabled and disabled trees if signal-safety integration touches feature
gating. Report configurations not exercised, focused and regression results,
committed-frame artifacts inspected, and real-terminal evidence. Run
`rtk git diff --check` before handoff.

## 7. Non-Goals

- Kitty keyboard flags 2, 4, 8, 16 (event types, alternate keys, all keys as
  escape codes, associated text). Opt-in properties in a later plan.
- Sixel and iTerm2 inline images. The hook from WP1 makes them possible; no
  target terminal in the launch list needs them.
- Cursor-positioned (non-virtual) graphics placements. They bypass the cell
  grid and would need special handling in the diff.
- Animation frames through the graphics protocol. A changing `source` already
  works by re-transmission.
- Focus reporting (`?1004`). Keep it in a separate change so parser and
  negotiation acceptance stay focused.

## 8. Protocol References

- [Xterm control sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html):
  CSI framing, intermediate bytes, OSC termination, DECRQM, and window queries.
- [Kitty keyboard protocol](https://sw.kovidgoyal.net/kitty/keyboard-protocol/):
  enhancement flags, default parameters, event types, detection, and screen stacks.
- [Kitty graphics protocol](https://sw.kovidgoyal.net/kitty/graphics-protocol/):
  transmission chunks, virtual placements, placeholder encoding, and reset rules.

These are the normative references for implementation; keep emulator support
claims separate from protocol requirements and back them with versioned tests.
