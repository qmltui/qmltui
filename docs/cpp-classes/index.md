# C++ Classes

QmlTui exposes C++ classes for bootstrapping terminal sessions, writing custom visual items that paint directly into the character grid, supplying dynamic images, and automating headless tests.

## Application & Engine Lifecycle

- [Terminal](terminal.md): Root visual item and session coordinator managing `AltScreen`/`Inline` rendering modes, focus, hover, popups, clipboard, and terminal window state.
- [TerminalCapabilities](terminalcapabilities.md): Value type and `ColorDepth` enum describing detected host terminal capabilities and environment overrides.
- [QtMessageRouting](qtmessagerouting.md): Unified routing pipeline that separates immediate diagnostic logging from deferred stderr replay during alternate-screen terminal sessions.
- [TerminalEngine](terminalengine.md) *(in-tree engine internal)*: Central runtime coordinator managing terminal I/O, input event dispatch, resize notifications, and off-screen double-buffered frame rendering.

## Writing Custom C++ Items & Painting

- [Item](item.md): Base visual class providing cell-grid geometry, anchors, focus navigation, state machines, and virtual `paint()` and input hooks.
- [TextGridBuffer](textgridbuffer.md): Off-screen 2D character canvas offering text, line, box, and rectangle drawing primitives along with differential ANSI frame serialization.
- [Cell & TextAttribute](cell.md): Value types representing a single terminal grid cell, its inline UTF-8 grapheme cluster, display width, colors, and SGR text styling flags.
- [Color](color.md): `QtGui`-free color value supporting `#RGB`/`#RRGGBB`/`#AARRGGBB` hex literals, CSS/SVG named colors, and theme-aware `ansi:` palette indices.
- [Rectangle](rectangle.md): Visual item that renders a background color fill and optional single, double, rounded, or ASCII box-drawing borders.
- [Text](text.md): Visual item that renders aligned single-line or multi-line plain, HTML-formatted, or ANSI-escaped text, spans, and gradients.
- [TextStyle, Span & HighlightRange](text-styling.md): Sparse color and SGR text attribute overlays for `Text`, `TableView`, `TreeView`, and `TextArea`.
- [UnicodeText & GraphemeIterator](unicodetext.md): Grapheme cluster iteration, terminal column width measurement (`utf8proc`), elision, and zero-allocation UTF-8 encoding.
- [Subpixel Helpers](subpixel.md): Compile-time constants and UTF-8 glyph helpers for `2x4` Braille and `1x2` half-block sub-character rendering.
- [KeyEvent, MouseEvent, Shortcut & InputScope](input-events.md): Keyboard, mouse, global shortcut, and modal overlay input-routing types.
- [MouseArea](mousearea.md): Invisible interaction item that captures terminal mouse presses, releases, clicks, wheel events, and opt-in hover motion.
- [Image, AnimatedSprite & Core Item Headers](image-items.md): C++ classes for `qmltui::Image`, `qmltui::AnimatedSprite`, and the full index of installed `<QmlTui/Core/Items/*.h>` headers.

## Custom Image Providers

- [ImageProvider](imageprovider.md): Engine-registered C++ interface that supplies `QImage` frames to QML `Image` items under custom `image://` URLs.

## Headless Testing & Automation

- [Runner](runner.md): JavaScript-driven test harness and automation oracle that injects input, awaits committed frames, and writes diagnostic artifacts.
- [MemoryTerminalBackend](memoryterminalbackend.md): In-memory `TerminalBackend` implementation for exercising raw byte output, input decoding, and resize handling in unit tests without a PTY.

## Platform & Protocol Internals

- [TerminalBackend](terminalbackend.md): Public platform seam isolating POSIX `termios`, Windows Console VT, WebAssembly, and custom application terminal I/O behind a common interface.
- [TerminalDevice](terminaldevice.md) *(in-tree protocol internal)*: Platform-neutral output queue and raw-mode state manager built on top of `TerminalBackend`.
- [InputDecoder](inputdecoder.md) *(in-tree protocol internal)*: Stateful parser that translates raw VT byte sequences and bracketed paste payloads into Qt key and SGR mouse events.
