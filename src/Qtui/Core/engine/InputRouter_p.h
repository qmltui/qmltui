#pragma once

#include <QPointer>
#include <QString>
#include <QPoint>

namespace qtui {

class Terminal;
class Item;
class KeyEvent;
class MouseEvent;

class InputRouter {
public:
    explicit InputRouter(Terminal* terminal = nullptr);
    ~InputRouter() = default;

    void setTerminal(Terminal* terminal);
    Terminal* terminal() const { return m_terminal; }

    bool routeKey(int key, const QString& text, int modifiers, bool paste = false);
    void routeMouse(int button, int x, int y, bool release, bool motion = false,
                    int modifiers = 0, int wheelDeltaX = 0, int wheelDeltaY = 0);

    Item* hitTestItem(Item* item, int absX, int absY, int parentAbsX, int parentAbsY) const;
    void cycleFocus(bool forward, Item* scope = nullptr);

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

} // namespace qtui
