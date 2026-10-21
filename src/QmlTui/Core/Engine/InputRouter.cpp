#include <QmlTui/Core/Engine/InputRouter_p.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Engine/InputScope.h>
#include <QmlTui/Core/Engine/FocusManager.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QmlTui/Core/Engine/Shortcut.h>
#include <QmlTui/Core/Engine/Keys.h>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Terminal.h>

namespace qmltui {

bool InputRouter::isDescendantOf(Item* child, Item* ancestor) {
    QObject* p = child;
    while (p) {
        if (p == ancestor) return true;
        p = p->parent();
    }
    return false;
}

InputRouter::InputRouter(Terminal* terminal) : m_terminal(terminal) {}

void InputRouter::setTerminal(Terminal* terminal) {
    m_terminal = terminal;
}

bool InputRouter::routeKey(int key, const QString& text, int modifiers, bool paste) {
    if (!m_terminal) return false;

    KeyEvent event(key, text, modifiers, paste);

    auto dispatchToItem = [](Item* item, KeyEvent& ev) {
        if (!item->isEffectivelyEnabled() || !item->isEffectivelyVisible()) return;

        QPointer<Item> itemGuard(item);
        QPointer<Keys> attached =
            qobject_cast<Keys*>(qmlAttachedPropertiesObject<Keys>(item, false));
        bool beforeItem = attached && (attached->priority() == Keys::BeforeItem);

        auto emitAttachedSignals = [&]() {
            if (!attached) return;
            if (ev.key() == Qt::Key_Return || ev.key() == Qt::Key_Enter)
                emit attached->returnPressed(&ev);
            else if (ev.key() == Qt::Key_Escape)
                emit attached->escapePressed(&ev);
            else if (ev.key() == Qt::Key_Space)
                emit attached->spacePressed(&ev);
            else if (ev.key() == Qt::Key_Up)
                emit attached->upPressed(&ev);
            else if (ev.key() == Qt::Key_Down)
                emit attached->downPressed(&ev);
            else if (ev.key() == Qt::Key_Left)
                emit attached->leftPressed(&ev);
            else if (ev.key() == Qt::Key_Right)
                emit attached->rightPressed(&ev);
            else if (ev.key() == Qt::Key_Tab)
                emit attached->tabPressed(&ev);
            if (attached && !ev.isAccepted()) emit attached->pressed(&ev);
        };

        if (beforeItem) {
            emitAttachedSignals();
            if (ev.isAccepted()) return;
            if (!itemGuard) return;
            itemGuard->keyPressEvent(&ev);
        } else {
            if (!itemGuard) return;
            itemGuard->keyPressEvent(&ev);
            if (ev.isAccepted()) return;
            emitAttachedSignals();
        }

        if (!ev.isAccepted() && attached) {
            const QList<QPointer<Item>> forwardedItems = attached->forwardToItems();
            for (const QPointer<Item>& forwardedGuard : forwardedItems) {
                if (forwardedGuard && forwardedGuard->isEffectivelyEnabled() &&
                    forwardedGuard->isEffectivelyVisible()) {
                    forwardedGuard->keyPressEvent(&ev);
                    if (ev.isAccepted()) return;
                    if (!forwardedGuard) return;
                    QPointer<Keys> fwdAttached = qobject_cast<Keys*>(
                        qmlAttachedPropertiesObject<Keys>(forwardedGuard.data(), false));
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

    Item* modalItem = activeModalScope ? activeModalScope->asItem() : nullptr;
    Item* focusTarget = modalItem
                            ? FocusManager::resolveModalFocusOrigin(m_terminal.data(), modalItem)
                            : m_terminal->focusedItem();
    const bool editableTextFocused =
        focusTarget && focusTarget->isEffectivelyVisible() && focusTarget->isEffectivelyEnabled() &&
        (focusTarget->inherits("qmltui::TextInput") || focusTarget->inherits("qmltui::TextArea")) &&
        !focusTarget->property("readOnly").toBool();
    const bool unmodifiedPrintable =
        (modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) == 0 &&
        !text.isEmpty() && text.at(0).isPrint() && key != Qt::Key_Escape && key != Qt::Key_Tab &&
        key != Qt::Key_Backtab && key != Qt::Key_Return && key != Qt::Key_Enter;

    if (!paste && !(unmodifiedPrintable && editableTextFocused)) {
        const QList<Shortcut*> currentShortcuts = activeShortcuts();
        QList<QPointer<Shortcut>> shortcutGuards;
        shortcutGuards.reserve(currentShortcuts.size());
        for (Shortcut* sc : currentShortcuts) {
            shortcutGuards.append(sc);
        }
        for (const QPointer<Shortcut>& scGuard : shortcutGuards) {
            Shortcut* sc = scGuard.data();
            if (!sc || !sc->enabled()) continue;
            const bool matches =
                ((key | modifiers) == sc->sequence()) || (modifiers == 0 && key == sc->sequence());
            if (!matches) continue;
            QObject* p = sc->parent();
            while (p && !qobject_cast<Item*>(p))
                p = p->parent();
            Item* scItem = qobject_cast<Item*>(p);
            if (scItem && (!scItem->isEffectivelyVisible() || !scItem->isEffectivelyEnabled()))
                continue;
            if (modalItem &&
                (!scItem || (scItem != modalItem && !isDescendantOf(scItem, modalItem))))
                continue;
            emit sc->activated();
            return true;
        }
    }

    if (activeModalScope) {
        InputScope* topScope = activeModalScope;
        for (auto it = scopes.crbegin(); it != scopes.crend(); ++it)
            if (*it && (*it)->isScopeVisible() &&
                (*it == activeModalScope || (*it)->isDescendantOfScope(activeModalScope))) {
                topScope = *it;
                break;
            }
        if (key == Qt::Key_Escape && topScope->closeOnEscape()) {
            topScope->rejectScope();
            return true;
        }

        Item* current = FocusManager::resolveModalFocusOrigin(m_terminal.data(), modalItem);

        while (current) {
            QPointer<Item> next = current->parentItem();
            dispatchToItem(current, event);
            if (event.isAccepted()) break;

            if (current == modalItem) break;
            current = next.data();
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
            InputScope* scope = *it;
            if (scope && scope->isScopeVisible() && scope->closeOnEscape()) {
                scope->rejectScope();
                return true;
            }
        }
    }

    Item* focused = m_terminal->focusedItem();
    if (focused && (!focused->isEffectivelyVisible() || !focused->isEffectivelyEnabled()))
        m_terminal->setFocusedItem(nullptr);
    Item* current = m_terminal->focusedItem() ? m_terminal->focusedItem() : m_terminal.data();

    while (current) {
        QPointer<Item> next = current->parentItem();
        dispatchToItem(current, event);
        if (event.isAccepted()) break;

        current = next.data();
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

bool InputRouter::dispatchMouseEvent(Item* target, int x, int y, int button, int modifiers,
                                     bool motion, bool release, int wheelDeltaX, int wheelDeltaY,
                                     Item* stopAncestor) {
    if (!target) return false;

    // Check child mouse event filters on ancestors
    const MouseEvent::Type eventType = wheelDeltaX != 0 || wheelDeltaY != 0
                                           ? MouseEvent::Type::Wheel
                                       : motion  ? MouseEvent::Type::Move
                                       : release ? MouseEvent::Type::Release
                                                 : MouseEvent::Type::Press;

    QList<QPointer<Item>> filteringAncestors;
    for (Item* anc = target->parentItem(); anc; anc = anc->parentItem()) {
        if (anc->filtersChildMouseEvents()) {
            filteringAncestors.prepend(anc); // top-to-bottom
        }
        if (anc == stopAncestor) break;
    }

    QPointer<Item> targetGuard(target);
    for (const QPointer<Item>& filterGuard : filteringAncestors) {
        if (!targetGuard) return true;
        if (!filterGuard) continue;
        Item* filterItem = filterGuard.data();
        int filterAbsX = filterItem->absolutePosition().x();
        int filterAbsY = filterItem->absolutePosition().y();
        MouseEvent filterEvent(eventType, x - filterAbsX, y - filterAbsY, button, modifiers,
                               wheelDeltaX, wheelDeltaY);
        if (filterItem->childMouseEventFilter(targetGuard.data(), &filterEvent) ||
            filterEvent.isAccepted()) {
            return true;
        }
    }

    if (!targetGuard) return true;

    if (eventType == MouseEvent::Type::Press) {
        for (Item* item = targetGuard.data(); item && item != stopAncestor;
             item = item->parentItem()) {
            if ((item->focusPolicy() & Qt::ClickFocus) && item->isEffectivelyVisible() &&
                item->isEffectivelyEnabled()) {
                item->forceActiveFocus();
                break;
            }
        }
    }

    bool accepted = false;
    Item* current = targetGuard.data();
    while (current) {
        int currentAbsX = current->absolutePosition().x();
        int currentAbsY = current->absolutePosition().y();
        QPointer<Item> next = current->parentItem();
        MouseEvent currentEvent(eventType, x - currentAbsX, y - currentAbsY, button, modifiers,
                                wheelDeltaX, wheelDeltaY);

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
        current = next.data();
    }
    return accepted;
}

void InputRouter::routeMouse(int button, int x, int y, bool release, bool motion, int modifiers,
                             int wheelDeltaX, int wheelDeltaY) {
    if (!m_terminal) return;
    const auto* renderer = m_terminal->engine()->renderer();
    if (!renderer) return;
    const auto offset = renderer->mouseRowOffset();
    if (!offset) {
        // Never guess a target while a cursor report is outstanding. Still
        // release an existing grab outside its bounds so it cannot stay stuck.
        m_terminal->setHoveredItem(nullptr);
        if (release) {
            QPointer<Item> grab = m_implicitMouseGrabItem;
            m_implicitMouseGrabItem = nullptr;
            m_outsidePressArmedScope = nullptr;
            if (grab) {
                const auto pos = grab->absolutePosition();
                dispatchMouseEvent(grab, pos.x() - 1, pos.y() - 1, button, modifiers, false, true,
                                   0, 0);
            }
        }
        return;
    }
    const int localY = y - *offset;
    const auto* frame = renderer->currentBuffer();
    const bool inFrame =
        m_terminal->mode() != Terminal::Mode::Inline ||
        (frame && x >= 0 && x < frame->width() && localY >= 0 && localY < frame->height());
    const bool buttonMotion = motion && ((button & 3) != 3);
    const bool isWheel = (wheelDeltaX != 0 || wheelDeltaY != 0);
    const bool isButtonPress = (!release && !motion && !isWheel && (button >= 0 && button <= 2));

    const QList<InputScope*> scopes = m_terminal->orderedInputScopes();
    InputScope* activeModal = m_terminal->topModalScope();

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
            bool inside = (inFrame && x >= absPos.x() && x < absPos.x() + armedItem->width() &&
                           localY >= absPos.y() && localY < absPos.y() + armedItem->height());
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
        if (grabItem && grabItem->isEffectivelyVisible() && grabItem->isEffectivelyEnabled()) {
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

            dispatchMouseEvent(grabItem, x, localY, button, modifiers, motion, release, wheelDeltaX,
                               wheelDeltaY);
            if (release) {
                m_implicitMouseGrabItem = nullptr;
            }
            return;
        }
        m_implicitMouseGrabItem = nullptr;
    }

    // 4. Outside-press arming for CloseOnOutsideClick scopes
    if (isButtonPress) {
        m_outsidePressArmedScope = nullptr;
        for (int i = scopes.size() - 1; i >= 0; --i) {
            InputScope* scope = scopes[i];
            if (!scope || !scope->isScopeVisible() || !scope->closeOnOutsideClick()) continue;
            if (activeModal && scope != activeModal && !scope->isDescendantOfScope(activeModal))
                continue;
            Item* scopeItem = scope->asItem();
            if (!scopeItem) continue;
            QPoint absPos = scopeItem->absolutePosition();
            bool inside = (inFrame && x >= absPos.x() && x < absPos.x() + scopeItem->width() &&
                           localY >= absPos.y() && localY < absPos.y() + scopeItem->height());
            if (!inside) {
                m_outsidePressArmedScope = scopeItem;
                break;
            } else {
                break;
            }
        }
    }

    // 5. Scopes dispatch
    for (int i = scopes.size() - 1; i >= 0; --i) {
        InputScope* scope = scopes[i];
        if (!scope || !scope->isScopeVisible()) continue;
        if (activeModal && scope != activeModal && !scope->isDescendantOfScope(activeModal))
            continue;

        Item* scopeItem = scope->asItem();
        if (!scopeItem) continue;

        QPoint absPos = scopeItem->absolutePosition();
        bool inside = (inFrame && x >= absPos.x() && x < absPos.x() + scopeItem->width() &&
                       localY >= absPos.y() && localY < absPos.y() + scopeItem->height());

        if (inside) {
            m_outsidePressArmedScope = nullptr;
            int parentAbsX = absPos.x() - scopeItem->x();
            int parentAbsY = absPos.y() - scopeItem->y();
            Item* target = hitTestItem(scopeItem, x, localY, parentAbsX, parentAbsY);
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
                    if (!requested) return;
                }

                QPointer<Item> targetGuard(target);
                bool accepted = dispatchMouseEvent(target, x, localY, button, modifiers, motion,
                                                   release, wheelDeltaX, wheelDeltaY, scopeItem);
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
    Item* target = inFrame ? hitTestItem(m_terminal, x, localY, 0, 0) : nullptr;
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
            if (!requested) return;
        }

        QPointer<Item> targetGuard(target);
        bool accepted = dispatchMouseEvent(target, x, localY, button, modifiers, motion, release,
                                           wheelDeltaX, wheelDeltaY);
        if (accepted && isButtonPress && targetGuard) {
            m_implicitMouseGrabItem = targetGuard.data();
        }
    }
}

Item* InputRouter::hitTestItem(Item* item, int absX, int absY, int parentAbsX,
                               int parentAbsY) const {
    if (!item || !item->visible() || !item->isEffectivelyEnabled()) return nullptr;

    int itemAbsX = parentAbsX + item->x();
    int itemAbsY = parentAbsY + item->y();

    bool inside = (absX >= itemAbsX && absX < itemAbsX + item->width() && absY >= itemAbsY &&
                   absY < itemAbsY + item->height());

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

} // namespace qmltui
