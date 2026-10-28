# InputDecoder

`InputDecoder` parses raw terminal byte streams into Qt key events (`Qt::Key_*`), SGR mouse reports, bracketed paste payloads, and VT query replies. It is used internally by `TerminalEngine`.

- **Header**: `src/QmlTui/Core/Primitives/InputDecoder.h` *(in-tree protocol header; not installed as a public header)*

## Public Types and Constants

- `using KeyCallback = std::function<void(int qtKey, const QString &text, int modifiers)>`
- `using PasteCallback = std::function<void(const QString &text)>`
- `struct MouseInput { int button = 3; int x = 0; int y = 0; bool release = false; bool motion = false; int modifiers = 0; int wheelDeltaX = 0; int wheelDeltaY = 0; }`
- `using DetailedMouseCallback = std::function<void(const MouseInput &input)>`
- `using FocusCallback = std::function<void(bool focused)>`
- `struct ControlSequence` — framed terminal query reply (`Kind::Csi`, `Ss3`, `Apc`, `Osc`, or `Dcs`) with `privateMarker`, `params`, `intermediates`, `finalByte`, `payload`, and `int parameter(qsizetype index, qsizetype sub = 0, int fallback = 0) const`.
- `using ReplyCallback = std::function<void(const ControlSequence &)>`
- `static constexpr qsizetype MaxClipboardTextBytes = 1024 * 1024` — maximum decoded UTF-8 byte length (`1 MiB`) accepted from an OSC 52 clipboard reply.

## Public Methods

#### `void setKeyCallback(KeyCallback cb)`
Registers the callback invoked when a key sequence is decoded (`int qtKey`, `const QString &text`, `int modifiers`).

#### `void setPasteCallback(PasteCallback cb)`
Registers the callback invoked with the complete payload of a bracketed paste (`\033[200~...\033[201~`).

#### `void setDetailedMouseCallback(DetailedMouseCallback cb)`
Registers the callback receiving normalized button, modifier, motion, and signed wheel-delta fields (`MouseInput`).

#### `void setFocusCallback(FocusCallback cb)`
Registers the callback invoked when DEC mode 1004 focus-in (`CSI I`) or focus-out (`CSI O`) sequences arrive.

#### `void setReplyCallback(ReplyCallback cb)`
Registers the synchronous callback receiving framed `ControlSequence` replies (`CSI`, `SS3`, `APC`, `OSC`, `DCS`) that are not consumed as key, mouse, paste, or focus events.

#### `void setClipboardErrorCallback(std::function<void()> cb)`
Registers the callback invoked when a malformed or oversized OSC 52 frame is discarded.

#### `void decode(const char *buffer, size_t size)`
Appends incoming terminal bytes to the decoder buffer and dispatches completed sequences.

#### `bool hasPendingEscape() const` / `void flushPendingEscape()`
A bare `ESC` (`0x1b`) is held briefly because it may start a multi-byte escape sequence whose remaining bytes have not arrived yet. `TerminalEngine` runs a short timer (`QMLTUI_ESCAPE_TIMEOUT_MS`, default 50 ms) after each read and calls `flushPendingEscape()` if no further bytes arrive, delivering `Qt::Key_Escape`. Terminals with Kitty keyboard disambiguation enabled report Escape as `CSI 27 u` and do not need the timeout.

#### `static constexpr int keyForCharacter(char32_t codepoint)`
Returns the Qt key code produced by a character keystroke (`a`–`z` map to `Qt::Key_A`–`Qt::Key_Z`; other characters return `codepoint`).

## Kitty Keyboard Protocol

`CSI unicode-key[:shifted-key[:base-layout-key]] ; modifiers[:event-type] [; text-codepoints] u` is decoded as a key event. The first parameter is the Unicode code point (`27`, `13`, `9`, and `127` map to Escape, Return, Tab, and Backspace; `Shift+Tab` becomes `Qt::Key_Backtab`). Code points in `57344..63743` map through the functional-key table (`F13`–`F35`, keypad keys with `Qt::KeypadModifier`, and lock/menu keys). Standalone modifier and media key reports are consumed without emitting a key event.

The modifier field is `1 + bitmask`: `1` Shift, `2` Alt, `4` Ctrl, `8` Super, `16` Hyper, `32` Meta, `64` CapsLock, `128` NumLock. Super and Meta map to `Qt::MetaModifier`. The event-type sub-parameter defaults to press (`1`); repeat (`2`) dispatches another press, and release (`3`) is consumed without emitting a press event.

A printable code point (using the shifted key when Shift is active) becomes the event's `text` unless Ctrl or Alt is held, preventing `Ctrl+A` from inserting `"a"`. Explicitly reported text code points (flag 16) take precedence when present. `CSI ? flags u` is routed to the reply callback rather than key dispatch.

## OSC 52 Clipboard Replies

OSC 52 clipboard replies use a dedicated framing bound allowing up to 1 MiB of decoded UTF-8 text (`MaxClipboardTextBytes`) plus Base64 overhead. Other control strings keep a 4 KiB bound. `setClipboardErrorCallback()` reports malformed or oversized OSC 52 frames, and discarded bytes never leak as keyboard input.

## Example

```cpp
#include <QmlTui/Core/Primitives/InputDecoder.h> // in-tree build interface

qmltui::InputDecoder decoder;
decoder.setKeyCallback([](int key, const QString &text, int modifiers) {
    Q_UNUSED(text)
    Q_UNUSED(modifiers)
    // key == Qt::Key_Up
});

const char rawInput[] = "\033[A";
decoder.decode(rawInput, sizeof(rawInput) - 1);
```
