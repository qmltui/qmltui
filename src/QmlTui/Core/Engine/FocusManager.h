#pragma once

// Internal: keyboard focus traversal helpers for InputRouter and TerminalEngine.

namespace qmltui {

class Terminal;
class Item;

// Keyboard focus traversal helpers used by InputRouter and TerminalEngine.
class FocusManager {
public:
    // Advances active focus to the next (or previous) focusable Item within
    // scope, or within the active modal scope / root Terminal when scope is null.
    static void cycleFocus(Terminal* terminal, bool forward, Item* scope = nullptr);
    // Returns the item from which focus traversal or key dispatch should start
    // when activeModalScope is open.
    static Item* resolveModalFocusOrigin(Terminal* terminal, Item* activeModalScope);
};

} // namespace qmltui
