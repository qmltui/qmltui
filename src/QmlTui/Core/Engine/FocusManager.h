#pragma once

namespace qmltui {

class Terminal;
class Item;

class FocusManager {
public:
    static void cycleFocus(Terminal* terminal, bool forward, Item* scope = nullptr);
    static Item* resolveModalFocusOrigin(Terminal* terminal, Item* activeModalScope);
};

} // namespace qmltui
