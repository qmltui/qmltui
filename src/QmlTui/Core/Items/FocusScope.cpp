#include <QmlTui/Core/Items/FocusScope.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QPointer>

namespace qmltui {

struct FocusScopePrivate {
    QPointer<Item> subFocusItem;
};

FocusScope::FocusScope(QObject* parent)
    : Item(parent), m_d(std::make_unique<FocusScopePrivate>()) {}

FocusScope::~FocusScope() = default;

Item* FocusScope::subFocusItem() const {
    return m_d->subFocusItem.data();
}

void FocusScope::setSubFocusItem(Item* item) {
    if (m_d->subFocusItem.data() == item) return;

    Item* oldSub = m_d->subFocusItem.data();
    m_d->subFocusItem = item;

    if (oldSub && oldSub != item) {
        oldSub->clearFocusFlagAndNotify();
    }

    if (activeFocus() && m_d->subFocusItem && !m_d->subFocusItem->isFocusScope()) {
        if (Terminal* term = terminal()) {
            if (term->focusedItem() != m_d->subFocusItem.data()) {
                term->setFocusedItem(m_d->subFocusItem.data());
            }
        }
    }
}

void FocusScope::setFocus(bool focus) {
    if (focus && (!isEffectivelyVisible() || !isEffectivelyEnabled())) return;

    Item::setFocus(focus);

    if (focus) {
        Item* target = m_d->subFocusItem.data();
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
