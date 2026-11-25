#include <QmlTui/Core/Engine/FocusManager.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/Item.h>

namespace qmltui {

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
        nextFocus->forceActiveFocus();
    }
}

Item* FocusManager::resolveModalFocusOrigin(Terminal* terminal, Item* activeModalScope) {
    if (!terminal) return activeModalScope;
    Item* focused = terminal->focusedItem();
    if (focused && focused->isDescendantOf(activeModalScope)) {
        return focused;
    }
    return activeModalScope;
}

} // namespace qmltui
