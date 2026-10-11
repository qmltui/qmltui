#include <Qamui/Core/engine/FocusManager.h>
#include <Qamui/Core/items/Terminal.h>
#include <Qamui/Core/items/Item.h>

namespace qamui {

static bool isDescendant(Item* child, Item* ancestor) {
    QObject* p = child;
    while (p) {
        if (p == ancestor) return true;
        p = p->parent();
    }
    return false;
}

void FocusManager::cycleFocus(Terminal* terminal, bool forward, Item* scope) {
    if (!terminal) return;

    QList<Item*> focusableList;
    terminal->findFocusableItems(scope ? scope : terminal, focusableList);
    if (focusableList.isEmpty()) return;

    Item* currentFocus = terminal->focusedItem();
    int idx = focusableList.indexOf(currentFocus);

    Item* nextFocus = nullptr;
    if (idx == -1) {
        nextFocus = forward ? focusableList.first() : focusableList.last();
    } else {
        if (forward) {
            nextFocus = focusableList[(idx + 1) % focusableList.size()];
        } else {
            nextFocus = focusableList[(idx - 1 + focusableList.size()) % focusableList.size()];
        }
    }

    if (nextFocus) {
        nextFocus->setFocus(true);
    }
}

Item* FocusManager::resolveModalFocusOrigin(Terminal* terminal, Item* activeModalScope) {
    if (!terminal) return activeModalScope;
    Item* focused = terminal->focusedItem();
    if (focused && activeModalScope && isDescendant(focused, activeModalScope)) {
        return focused;
    }
    return activeModalScope;
}

} // namespace qamui
