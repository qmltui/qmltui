#pragma once

// Internal: dispatches decoded key, paste, and mouse events across the Item
// tree, modal InputScopes, Shortcuts, and focus chain.

#include <QPointer>
#include <QString>

namespace qmltui {

class Terminal;
class Item;
class KeyEvent;
class MouseEvent;

// Routes keyboard and mouse events for a root Terminal. Owns the implicit
// mouse-press grab and modal outside-click dismissal state.
class InputRouter {
public:
    explicit InputRouter(Terminal* terminal = nullptr);
    ~InputRouter() = default;

    void setTerminal(Terminal* terminal);
    Terminal* terminal() const { return m_terminal; }

    // Dispatches a key or bracketed-paste event; returns true if accepted.
    bool routeKey(int key, const QString& text, int modifiers, bool paste = false);
    // Updates hover state and dispatches press, release, drag, or wheel events.
    void routeMouse(int button, int x, int y, bool release, bool motion = false,
                    int modifiers = 0, int wheelDeltaX = 0, int wheelDeltaY = 0);

    // Topmost visible, enabled descendant of item containing (absX, absY) in
    // stacking order, respecting clip rects.
    Item* hitTestItem(Item* item, int absX, int absY, int parentAbsX, int parentAbsY) const;

    // Item currently holding the implicit grab from a mouse press until release.
    Item* implicitMouseGrabItem() const { return m_implicitMouseGrabItem.data(); }

private:
    static bool isDescendantOf(Item* child, Item* ancestor);
    bool dispatchMouseEvent(Item* target, int x, int y, int button,
                            int modifiers, bool motion, bool release,
                            int wheelDeltaX, int wheelDeltaY,
                            Item* stopAncestor = nullptr);

    QPointer<Terminal> m_terminal;
    QPointer<Item> m_implicitMouseGrabItem;
    QPointer<Item> m_outsidePressArmedScope;
};

} // namespace qmltui
