# InputDecoder

## 1. Class Overview

`InputDecoder` parses raw byte streams read from standard terminal input (`stdin`) and translates ANSI escape sequences into structured Qt key events (`Qt::Key_*`) and mouse event coordinates (SGR mouse mode tracking).

`InputDecoder` is used internally by `TerminalEngine` to process keyboard presses and mouse actions in real time.

## 2. Project Structure and Dependencies

- **Header**: `Terminal/src/core/InputDecoder.h`
- **Implementation**: `Terminal/src/core/InputDecoder.cpp`
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

## 11. Ownership and Lifecycle

`InputDecoder` is owned by `TerminalEngine`.

## 12. Thread Safety

`InputDecoder` must be called on the main application thread where terminal I/O notifications are received.

## 15. External Communication

- **Input Source**: Reads raw VT100 / ANSI terminal escape sequences from standard input (`stdin`).
- **Data Protocol**: Parses ANSI CSI sequences (e.g. `\033[A` for Up Arrow, `\033[<0;10;5M` for SGR mouse press).

## 16. Usage Example

```cpp
#include "InputDecoder.h"
#include <iostream>

using namespace qtui;

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
