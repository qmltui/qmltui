# Stream Mode Decision (design notes, 2026-09)

Design notes written before the numbered plans, kept because they record why
Qamui has two renderers. Stream mode has since shipped as `StreamRenderer` behind
`Terminal.mode`; the current roadmap is [README.md](README.md).

## Target Use Case: Coding Agent TUI

One intended future use case is a terminal-based coding agent interface, similar
in interaction style to Codex or Claude Code.

That style of interface should not be forced entirely through the current
full-screen retained renderer. Coding-agent TUIs often use the terminal
emulator's normal scrollback as the conversation transcript: messages are
appended and streamed to stdout, while only the active prompt/status area is
redrawn interactively.

## Current Rendering Model

The current library is a full-screen retained-mode terminal UI:

- `TerminalEngine` owns raw terminal input.
- `Terminal` is the root of a QML item tree.
- Items render into a fixed-size `TextGridBuffer`.
- `TextGridBuffer::diff()` emits ANSI updates for changed cells.
- `Terminal.mode` selects full-screen or inline presentation. The current
  public contract is documented in [the QML API](../qml_api.md#other-core-elements);
  the stream roadmap below records the earlier design motivation.

This model is useful for dashboards, forms, modal screens, focused widgets,
approval prompts, panels, and other controlled terminal UI regions.

## Stream Rendering Mode

Add a second rendering mode for coding-agent style applications.

Stream mode:

- stay on the normal terminal screen by default;
- preserve terminal emulator scrollback;
- avoid clearing the terminal on startup and shutdown;
- append streamed assistant/tool/user output naturally;
- reserve a live prompt/status region at the bottom;
- redraw only the prompt/status region when editing input or receiving updates;
- support robust key decoding, bracketed paste, and multiline editing;
- remain compatible with modal overlays or approval prompts when needed.

The likely architecture is:

- `TerminalDevice`: low-level terminal mode, writes, cursor control, resize.
- `InputDecoder`: byte stream to key/mouse/paste events.
- `FrameRenderer`: retained `TextGridBuffer` frame renderer.
- `StreamRenderer`: append-only output renderer plus live bottom region.
- `PromptComposer`: multiline input editor used by agent-style apps.

## Immediate Direction

Do not rewrite the architecture before stabilizing the existing core.

The current full-screen renderer is a valid foundation and should be made
reliable first. The next work should focus on correctness and testability:

1. Extract and test terminal input decoding.
2. Add automated tests for rendering, layout, focus, text input, and scrolling.
3. Fix known behavioral gaps such as scroll-aware mouse hit testing.
4. Make existing primitives predictable before adding many new controls.
5. Keep stream mode in mind by avoiding deeper coupling between
   terminal device management, input decoding, and full-screen rendering.

Stream mode is implemented as a second renderer rather than replacing the
existing frame renderer.
