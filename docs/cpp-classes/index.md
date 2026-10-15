# C++ Classes

QmlTui exposes C++ classes for bootstrapping terminal sessions, writing custom visual items that paint directly into the character grid, supplying dynamic images, and automating headless tests.

## Application & Engine Lifecycle

- [TerminalEngine](terminalengine.md) — Central runtime coordinator managing terminal I/O, input event dispatch, resize notifications, and off-screen double-buffered frame rendering.
- [QtMessageRouting](qtmessagerouting.md) — Unified routing pipeline that separates immediate diagnostic logging from deferred stderr replay during alternate-screen terminal sessions.

## Writing Custom C++ Items & Painting

- [Item](item.md) — Base visual class providing cell-grid geometry, anchors, focus navigation, state machines, and virtual `paint()` and input hooks.
- [TextGridBuffer](textgridbuffer.md) — Off-screen 2D character canvas offering text, line, box, and rectangle drawing primitives along with differential ANSI frame serialization.
- [Cell & TextAttribute](cell.md) — Value types representing a single terminal grid cell, its inline UTF-8 grapheme cluster, display width, colors, and SGR text styling flags.
- [Color](color.md) — `QtGui`-free color value supporting `#RGB`/`#RRGGBB`/`#AARRGGBB` hex literals, CSS/SVG named colors, and theme-aware `ansi:` palette indices.
- [Rectangle](rectangle.md) — Visual item that renders a background color fill and optional single, double, rounded, or heavy box-drawing borders.
- [Text](text.md) — Visual item that renders aligned single-line or multi-line plain, HTML-formatted, or ANSI-escaped text.
- [MouseArea](mousearea.md) — Invisible interaction item that captures terminal mouse presses, releases, clicks, wheel events, and opt-in hover motion.

## Custom Image Providers

- [ImageProvider](imageprovider.md) — Engine-registered C++ interface that supplies `QImage` frames to QML `Image` items under custom `image://` URLs.

## Headless Testing & Automation

- [Runner](runner.md) — JavaScript-driven test harness and automation oracle that injects input, awaits committed frames, and writes diagnostic artifacts.
- [MemoryTerminalBackend](memoryterminalbackend.md) — In-memory `TerminalBackend` implementation for exercising raw byte output, input decoding, and resize handling in unit tests without a PTY.

## Platform & Protocol Internals

- [TerminalBackend](terminalbackend.md) — Abstract platform seam isolating POSIX `termios`, Windows Console VT, and WebAssembly terminal I/O behind a common interface.
- [TerminalDevice](terminaldevice.md) — Platform-neutral output queue and raw-mode state manager built on top of `TerminalBackend`.
- [InputDecoder](inputdecoder.md) — Stateful parser that translates raw VT byte sequences and bracketed paste payloads into Qt key and SGR mouse events.
- [ColorMapper (internal)](colormapper.md) — Internal helper bridging legacy string color parsing call sites to `qmltui::Color`.
