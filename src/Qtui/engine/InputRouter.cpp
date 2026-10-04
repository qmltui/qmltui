#include "InputRouter_p.h"
#include "InputScope.h"
#include "FocusManager.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "Shortcut.h"
#include "Keys.h"
#include "../items/Item.h"
#include "../items/Terminal.h"

namespace qtui {

bool InputRouter::isDescendantOf(Item* child, Item* ancestor) {
    QObject* p = child;
    while (p) {
        if (p == ancestor) return true;
        p = p->parent();
    }
    return false;
}

InputRouter::InputRouter(Terminal* terminal)
    : m_terminal(terminal) {}

void InputRouter::setTerminal(Terminal* terminal) {
    m_terminal = terminal;
}

bool InputRouter::routeKey(int key, const QString& text, int modifiers, bool paste) {
    if (!m_terminal) return false;

    KeyEvent event(key, text, modifiers, paste);

    for (Shortcut* sc : activeShortcuts()) {
        if (sc && sc->enabled() && sc->sequence() == key) {
            emit sc->activated();
            return true;
        }
    }

    auto dispatchToItem = [](Item* item, KeyEvent& ev) {
        if (!item->isEffectivelyEnabled()) return;

        Keys* attached = qobject_cast<Keys*>(qmlAttachedPropertiesObject<Keys>(item, false));
        bool beforeItem = attached && (attached->priority() == Keys::BeforeItem);

        auto emitAttachedSignals = [&]() {
            if (!attached) return;
            if (ev.key() == Qt::Key_Return || ev.key() == Qt::Key_Enter) emit attached->returnPressed(&ev);
            else if (ev.key() == Qt::Key_Escape) emit attached->escapePressed(&ev);
            else if (ev.key() == Qt::Key_Space) emit attached->spacePressed(&ev);
            else if (ev.key() == Qt::Key_Up) emit attached->upPressed(&ev);
            else if (ev.key() == Qt::Key_Down) emit attached->downPressed(&ev);
            else if (ev.key() == Qt::Key_Left) emit attached->leftPressed(&ev);
            else if (ev.key() == Qt::Key_Right) emit attached->rightPressed(&ev);
            else if (ev.key() == Qt::Key_Tab) emit attached->tabPressed(&ev);
            if (!ev.isAccepted()) emit attached->pressed(&ev);
        };

        if (beforeItem) {
            emitAttachedSignals();
            if (ev.isAccepted()) return;
            item->keyPressEvent(&ev);
        } else {
            item->keyPressEvent(&ev);
            if (ev.isAccepted()) return;
            emitAttachedSignals();
        }

        if (!ev.isAccepted() && attached) {
            for (Item* forwardedItem : attached->forwardToItems()) {
                if (forwardedItem && forwardedItem->isEffectivelyEnabled()) {
                    forwardedItem->keyPressEvent(&ev);
                    if (ev.isAccepted()) return;
                    Keys* fwdAttached = qobject_cast<Keys*>(qmlAttachedPropertiesObject<Keys>(forwardedItem, false));
                    if (fwdAttached) {
                        emit fwdAttached->pressed(&ev);
                        if (ev.isAccepted()) return;
                    }
                }
            }
        }
    };

    const QList<InputScope*> scopes = m_terminal->orderedInputScopes();
    InputScope* activeModalScope = nullptr;
    for (int i = scopes.size() - 1; i >= 0; --i) {
        InputScope* s = scopes[i];
        if (s && s->isScopeVisible() && s->isScopeModal()) {
            activeModalScope = s;
            break;
        }
    }

    if (activeModalScope) {
        InputScope *topScope = activeModalScope;
        for (auto it = scopes.crbegin(); it != scopes.crend(); ++it)
            if (*it && (*it)->isScopeVisible()
                && (*it == activeModalScope || (*it)->isDescendantOfScope(activeModalScope))) {
                topScope = *it;
                break;
            }
        if (key == Qt::Key_Escape && topScope->closeOnEscape()) {
            topScope->rejectScope();
            return true;
        }
        
        Item* modalItem = activeModalScope->asItem();
        Item* current = FocusManager::resolveModalFocusOrigin(m_terminal.data(), modalItem);
        
        while (current) {
            dispatchToItem(current, event);
            if (event.isAccepted()) break;
            
            if (current == modalItem) break;
            current = qobject_cast<Item*>(current->parent());
        }
        if (!event.isAccepted()) {
            if (key == Qt::Key_Tab)
                FocusManager::cycleFocus(m_terminal.data(), true, modalItem);
            else if (key == Qt::Key_Backtab)
                FocusManager::cycleFocus(m_terminal.data(), false, modalItem);
        }
        return true;
    }

    if (key == Qt::Key_Escape) {
        for (auto it = scopes.crbegin(); it != scopes.crend(); ++it) {
            InputScope *scope = *it;
            if (scope && scope->isScopeVisible() && scope->closeOnEscape()) {
                scope->rejectScope();
                return true;
            }
        }
    }

    Item* focused = m_terminal->focusedItem();
    if (focused && (!focused->visible() || !focused->isEffectivelyEnabled()))
        m_terminal->setFocusedItem(nullptr);
    Item* current = m_terminal->focusedItem() ? m_terminal->focusedItem() : m_terminal.data();

    while (current) {
        dispatchToItem(current, event);
        if (event.isAccepted()) break;

        current = qobject_cast<Item*>(current->parent());
    }

    if (!event.isAccepted()) {
        if (key == Qt::Key_Tab) {
            FocusManager::cycleFocus(m_terminal.data(), true);
        } else if (key == Qt::Key_Backtab) {
            FocusManager::cycleFocus(m_terminal.data(), false);
        }
    }
    return event.isAccepted();
}

bool InputRouter::dispatchMouseEvent(Item* target, int x, int y, int button,
                                     int modifiers, bool motion, bool release,
                                     int wheelDeltaX, int wheelDeltaY,
                                     Item* stopAncestor) {
    if (!target) return false;

    // Check child mouse event filters on ancestors
    QList<Item*> filteringAncestors;
    for (Item* anc = target->parentItem(); anc; anc = anc->parentItem()) {
        if (anc->filtersChildMouseEvents()) {
            filteringAncestors.prepend(anc); // top-to-bottom
        }
        if (anc == stopAncestor) break;
    }

    for (Item* filterItem : filteringAncestors) {
        int filterAbsX = filterItem->absolutePosition().x();
        int filterAbsY = filterItem->absolutePosition().y();
        MouseEvent filterEvent(x - filterAbsX, y - filterAbsY, button,
                               modifiers, motion, wheelDeltaX, wheelDeltaY);
        if (filterItem->childMouseEventFilter(target, &filterEvent) || filterEvent.isAccepted()) {
            return true;
        }
    }

    bool accepted = false;
    Item* current = target;
    while (current) {
        int currentAbsX = current->absolutePosition().x();
        int currentAbsY = current->absolutePosition().y();
        MouseEvent currentEvent(x - currentAbsX, y - currentAbsY, button,
                                modifiers, motion, wheelDeltaX, wheelDeltaY);

        if (wheelDeltaX != 0 || wheelDeltaY != 0) {
            current->wheelEvent(&currentEvent);
        } else if (motion) {
            current->mouseMoveEvent(&currentEvent);
        } else if (release) {
            current->mouseReleaseEvent(&currentEvent);
        } else {
            current->mousePressEvent(&currentEvent);
        }
        if (currentEvent.isAccepted()) {
            accepted = true;
            break;
        }

        if (current == stopAncestor) break;
        current = qobject_cast<Item*>(current->parent());
    }
    return accepted;
}

void InputRouter::routeMouse(int button, int x, int y, bool release, bool motion,
                             int modifiers, int wheelDeltaX, int wheelDeltaY) {
    if (!m_terminal) return;
    const bool buttonMotion = motion && ((button & 3) != 3);
    const bool isWheel = (wheelDeltaX != 0 || wheelDeltaY != 0);
    const bool isButtonPress = (!release && !motion && !isWheel && (button >= 0 && button <= 2));

    const QList<InputScope*> scopes = m_terminal->orderedInputScopes();
    InputScope *activeModal = m_terminal->topModalScope();

    // 1. Modality check against stale grab
    if (m_implicitMouseGrabItem && activeModal) {
        if (!isDescendantOf(m_implicitMouseGrabItem.data(), activeModal->asItem())) {
            m_implicitMouseGrabItem = nullptr;
        }
    }

    // 2. Outside-press armed scope dismissal on release
    if (release && m_outsidePressArmedScope) {
        Item* armedItem = m_outsidePressArmedScope.data();
        m_outsidePressArmedScope = nullptr;
        if (armedItem) {
            QPoint absPos = armedItem->absolutePosition();
            bool inside = (x >= absPos.x() && x < absPos.x() + armedItem->width() &&
                           y >= absPos.y() && y < absPos.y() + armedItem->height());
            if (!inside) {
                for (InputScope* s : scopes) {
                    if (s && s->asItem() == armedItem && s->isScopeVisible()) {
                        m_implicitMouseGrabItem = nullptr;
                        s->rejectScope();
                        return;
                    }
                }
            }
        }
    }

    // 3. Implicit grab dispatch (for drag motion and release)
    if (m_implicitMouseGrabItem) {
        Item* grabItem = m_implicitMouseGrabItem.data();
        if (grabItem && grabItem->visible() && grabItem->isEffectivelyEnabled()) {
            if (motion) m_terminal->setHoveredItem(grabItem);
            if (motion && !buttonMotion) {
                bool requested = false;
                for (Item* current = grabItem; current; current = current->parentItem()) {
                    if (m_terminal->isMouseMotionRequester(current)) {
                        requested = true;
                        break;
                    }
                }
                if (!requested) {
                    if (release) m_implicitMouseGrabItem = nullptr;
                    return;
                }
            }

            QPointer<Item> grabGuard(grabItem);
            dispatchMouseEvent(grabItem, x, y, button, modifiers, motion, release,
                               wheelDeltaX, wheelDeltaY);
            if (release) {
                m_implicitMouseGrabItem = nullptr;
            }
            return;
        }
        if (release) {
            m_implicitMouseGrabItem = nullptr;
        }
    }

    // 4. Outside-press arming for CloseOnOutsideClick scopes
    if (isButtonPress) {
        m_outsidePressArmedScope = nullptr;
        for (int i = scopes.count() - 1; i >= 0; --i) {
            InputScope* scope = scopes[i];
            if (!scope || !scope->isScopeVisible() || !scope->closeOnOutsideClick()) continue;
            if (activeModal && scope != activeModal && !scope->isDescendantOfScope(activeModal)) continue;
            Item* scopeItem = scope->asItem();
            if (!scopeItem) continue;
            QPoint absPos = scopeItem->absolutePosition();
            bool inside = (x >= absPos.x() && x < absPos.x() + scopeItem->width() &&
                           y >= absPos.y() && y < absPos.y() + scopeItem->height());
            if (!inside) {
                m_outsidePressArmedScope = scopeItem;
                break;
            } else {
                break;
            }
        }
    }

    // 5. Scopes dispatch
    for (int i = scopes.count() - 1; i >= 0; --i) {
        InputScope* scope = scopes[i];
        if (!scope || !scope->isScopeVisible()) continue;
        if (activeModal && scope != activeModal && !scope->isDescendantOfScope(activeModal)) continue;
        
        Item* scopeItem = scope->asItem();
        if (!scopeItem) continue;

        QPoint absPos = scopeItem->absolutePosition();
        bool inside = (x >= absPos.x() && x < absPos.x() + scopeItem->width() &&
                       y >= absPos.y() && y < absPos.y() + scopeItem->height());
                       
        if (inside) {
            m_outsidePressArmedScope = nullptr;
            int parentAbsX = absPos.x() - scopeItem->x();
            int parentAbsY = absPos.y() - scopeItem->y();
            Item* target = hitTestItem(scopeItem, x, y, parentAbsX, parentAbsY);
            if (target) {
                if (motion) m_terminal->setHoveredItem(target);
                if (motion && !buttonMotion) {
                    bool requested = false;
                    for (Item* current = target; current; current = current->parentItem()) {
                        if (m_terminal->isMouseMotionRequester(current)) {
                            requested = true;
                            break;
                        }
                    }
                    if (!requested)
                        return;
                }

                QPointer<Item> targetGuard(target);
                bool accepted = dispatchMouseEvent(target, x, y, button, modifiers,
                                                   motion, release, wheelDeltaX, wheelDeltaY, scopeItem);
                if (accepted && isButtonPress && targetGuard) {
                    m_implicitMouseGrabItem = targetGuard.data();
                }
                return;
            }
        } else {
            if (scope->isScopeModal()) {
                if (motion) m_terminal->setHoveredItem(nullptr);
                return;
            }
        }
    }

    if (activeModal) {
        if (motion) m_terminal->setHoveredItem(nullptr);
        return;
    }

    // 6. Terminal root hit-test & dispatch
    Item* target = hitTestItem(m_terminal, x, y, 0, 0);
    if (motion) m_terminal->setHoveredItem(target);
    if (target) {
        if (motion && !buttonMotion) {
            bool requested = false;
            for (Item* current = target; current; current = current->parentItem()) {
                if (m_terminal->isMouseMotionRequester(current)) {
                    requested = true;
                    break;
                }
            }
            if (!requested)
                return;
        }

        QPointer<Item> targetGuard(target);
        bool accepted = dispatchMouseEvent(target, x, y, button, modifiers,
                                           motion, release, wheelDeltaX, wheelDeltaY);
        if (accepted && isButtonPress && targetGuard) {
            m_implicitMouseGrabItem = targetGuard.data();
        }
    }
}

void InputRouter::cycleFocus(bool forward, Item* scope) {
    FocusManager::cycleFocus(m_terminal.data(), forward, scope);
}

Item* InputRouter::hitTestItem(Item* item, int absX, int absY, int parentAbsX, int parentAbsY) const {
    if (!item || !item->visible() || !item->isEffectivelyEnabled()) return nullptr;

    int itemAbsX = parentAbsX + item->x();
    int itemAbsY = parentAbsY + item->y();

    bool inside = (absX >= itemAbsX && absX < itemAbsX + item->width() &&
                   absY >= itemAbsY && absY < itemAbsY + item->height());

    if (item->clip() && !inside) {
        return nullptr;
    }

    int childParentAbsX = itemAbsX;
    int childParentAbsY = itemAbsY;
    
    QPoint offset = item->contentOffset();
    childParentAbsX -= offset.x();
    childParentAbsY -= offset.y();

    const auto& children = item->stackingChildren();
    for (int i = children.size() - 1; i >= 0; --i) {
        Item* child = children[i];
        Item* hit = hitTestItem(child, absX, absY, childParentAbsX, childParentAbsY);
        if (hit) return hit;
    }

    if (inside) {
        return item;
    }

    return nullptr;
}

} // namespace qtui
