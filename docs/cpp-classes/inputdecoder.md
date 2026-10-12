# InputDecoder

## 1. Class Overview

`InputDecoder` parses raw byte streams read from standard terminal input (`stdin`) and translates ANSI escape sequences into structured Qt key events (`Qt::Key_*`) and mouse event coordinates (SGR mouse mode tracking).

`InputDecoder` is used internally by `TerminalEngine` to process keyboard presses and mouse actions in real time.

## 2. Project Structure and Dependencies

- **Header**: `src/Qamui/Core/Primitives/InputDecoder.h`
- **Implementation**: `src/Qamui/Core/Primitives/InputDecoder.cpp`
- **Dependencies**: `<functional>`, `<string>`, `<QString>`, Qt Core (`Qt::Key`).

## 3. Class Hierarchy and Role

- **Base Class**: None.
- **Role**: Input parser turning raw byte streams into Qt key codes and mouse coordinates via std::function callbacks.

## 9. Public Methods

#### void setKeyCallback(KeyCallback cb)
Registers a callback function invoked whenever a valid key sequence is decoded from the input buffer. The callback receives `int qtKey`, `const QString& text`, and `int modifiers`.

#### void setPasteCallback(PasteCallback cb)
Registers a callback for a complete bracketed-paste payload, preserving the
distinction between typed text and pasted text.

#### void setMouseCallback(MouseCallback cb)
Registers a callback function invoked whenever an SGR mouse tracking sequence is decoded. The callback receives `int button`, `int x`, `int y`, `bool release`, and `bool motion`.

#### void setDetailedMouseCallback(DetailedMouseCallback cb)
Registers a callback receiving normalized button, modifier, motion, and signed
wheel-delta fields while retaining `setMouseCallback` for compatibility.

#### void decode(const char* buffer, size_t size)
Appends raw incoming terminal bytes to the internal decoding buffer and triggers sequence parsing.

#### bool hasPendingEscape() const / void flushPendingEscape()
A lone ESC is held back because it may begin a sequence whose remaining bytes
are still in flight. `TerminalEngine` runs a short timer after each read
(`QAMUI_ESCAPE_TIMEOUT_MS`, default 50 ms) and calls `flushPendingEscape()`
when it expires with the ESC still alone, delivering `Qt::Key_Escape`. Nothing
is pending inside a bracketed paste or a partial sequence. Terminals with the
kitty disambiguate flag pushed report Escape as `CSI 27 u` and never need this.

## 10. Kitty Keyboard Protocol

`CSI unicode-key[:shifted-key[:base-layout-key]] ; modifiers[:event-type] [; text-codepoints] u`
is decoded as a key. The first parameter is the code point (letters map through
`keyForCharacter()`; 27, 13, 9 and 127 keep their legacy meaning as Escape,
Return, Tab and Backspace; Shift+Tab becomes `Qt::Key_Backtab`). Code points
in `57344..63743` map through a functional-key table: F13 to F35, the keypad
(with `Qt::KeypadModifier`), CapsLock and the other lock and menu keys. Media
keys and the modifier keys themselves are consumed without an event.

The modifier field is one plus a bitfield: 1 Shift, 2 Alt, 4 Ctrl, 8 Super,
16 Hyper, 32 Meta, 64 CapsLock, 128 NumLock. Super and Meta map to
`Qt::MetaModifier`; Hyper and the lock bits are dropped. The same field is
accepted on the legacy arrow, edit and function key forms. The event-type
sub-parameter defaults to press; a repeat dispatches another press, a release
or an unknown type is consumed without a key and without leaking bytes as text.

Text follows the legacy path: a printable code point (the shifted one when
Shift is held and the terminal reports it) becomes the key's text unless Ctrl
or Alt is held, so `Ctrl+A` never inserts an `a`. Explicitly reported text
(flag 16) wins when present. `CSI ? flags u` is a query reply, not a key, and
reaches the reply callback.

## 11. Ownership and Lifecycle

`InputDecoder` is owned by `TerminalEngine`.

## 12. Thread Safety

`InputDecoder` must be called on the main application thread where terminal I/O notifications are received.

## 15. External Communication

- **Input Source**: Reads raw VT100 / ANSI terminal escape sequences from standard input (`stdin`).
- **Data Protocol**: Parses ANSI CSI sequences (e.g. `\033[A` for Up Arrow, `\033[<0;10;5M` for SGR mouse press).

## 16. Usage Example

```cpp
#include <Qamui/Core/Primitives/InputDecoder.h>
#include <iostream>

using namespace qamui;

int main() {
    InputDecoder decoder;

    decoder.setKeyCallback([](int key, const QString& text, int modifiers) {
        std::cout << "Key pressed: " << key << " Text: " << text.toStdString() << std::endl;
    });

    const char rawInput[] = "\033[A"; // Up arrow escape sequence
    decoder.decode(rawInput, sizeof(rawInput) - 1);
    return 0;
}
```

## Clipboard replies

OSC 52 replies have a separate framing bound allowing 1 MiB of decoded text
plus Base64 expansion and framing. Other control strings retain their 4 KiB
limit. `setClipboardErrorCallback()` reports malformed or oversized OSC 52
frames; discarded bytes do not become keyboard input. The engine validates
selection, Base64 and UTF-8. Fragmented string frames resume scanning without
rescanning their whole buffered payload. Bracketed paste bypasses reply parsing.
