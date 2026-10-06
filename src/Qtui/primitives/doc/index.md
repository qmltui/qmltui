# Qtui Core Module C++ Documentation

This directory contains developer reference documentation for the core C++ rendering and I/O infrastructure in **Qtui**.

## Class Index

- [TextGridBuffer](file:///home/yosri/qtui/Tui/src/core/doc/TextGridBuffer.md) — 2D grid buffer managing cell rendering, UTF-8 text drawing primitives, and ANSI escape sequence diff generation.
- [Cell](file:///home/yosri/qtui/Tui/src/core/doc/Cell.md) — Grid cell struct and `TextAttribute` enum defining character, color, and font formatting.
- [InputDecoder](file:///home/yosri/qtui/Tui/src/core/doc/InputDecoder.md) — ANSI terminal input stream parser translating stdin bytes into Qt key and mouse events.
- [TerminalDevice](TerminalDevice.md) — Platform-neutral output queue and raw-mode bookkeeping over a TerminalBackend.
- [TerminalBackend](TerminalBackend.md) — Platform seam with POSIX (termios, SIGWINCH) and Windows console (virtual-terminal input and output) implementations.
- [Color](Color.md) — Qtui's public QtCore-only color value for C++ and QML.
