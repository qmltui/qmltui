#include <QmlTui/Core/Items/FocusScope.h>
#include <QmlTui/Core/Items/Terminal.h>

namespace qmltui {

FocusScope::FocusScope(QObject* parent) : Item(parent) {}

void FocusScope::setSubFocusItem(Item* item) {
    if (m_subFocusItem.data() == item) return;

    Item* oldSub = m_subFocusItem.data();
    m_subFocusItem = item;

    if (oldSub && oldSub != item) {
        oldSub->m_focus = false;
        emit oldSub->focusChanged();
    }

    if (activeFocus() && m_subFocusItem && !m_subFocusItem->isFocusScope()) {
        if (Terminal* term = terminal()) {
            if (term->focusedItem() != m_subFocusItem.data()) {
                term->setFocusedItem(m_subFocusItem.data());
            }
        }
    }
}

void FocusScope::setFocus(bool focus) {
    if (focus && (!isEffectivelyVisible() || !isEffectivelyEnabled())) return;

    Item::setFocus(focus);

    if (focus) {
        Item* target = m_subFocusItem.data();
        if (!target || !target->visible() || !target->isEffectivelyEnabled()) {
            QList<Item*> candidates;
            if (Terminal* term = terminal()) {
                for (Item* child : childItems()) {
                    term->findFocusableItems(child, candidates);
                }
                if (!candidates.isEmpty()) {
                    target = candidates.first();
                }
            }
        }
        if (target && target != this) {
            while (target && target->enclosingFocusScope() != this) {
                target = target->enclosingFocusScope();
            }
            if (target) {
                target->setFocus(true);
            }
        } else if (focusPolicy() != Qt::NoFocus) {
            if (Terminal* term = terminal()) {
                term->setFocusedItem(this);
            }
        }
    } else {
        if (Terminal* term = terminal()) {
            Item* curr = term->focusedItem();
            if (curr && curr->isDescendantOf(this)) {
                term->setFocusedItem(nullptr);
            }
        }
    }
}

} // namespace qmltui
