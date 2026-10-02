#include "Popup.h"
#include "Terminal.h"

namespace qtui {

Popup::Popup(QObject *parent) : Rectangle(parent) {
    // Popups are not focusable by default, but modal popups will trap input.
    setVisible(false);
}

Popup::~Popup() {
    Terminal* term = terminal();
    if (term) {
        term->unregisterPopup(this);
    }
}

void Popup::setModal(bool modal) {
    if (m_modal == modal) return;
    m_modal = modal;
    emit modalChanged();
    markDirty();
}

void Popup::setDimBackground(bool dim) {
    if (m_dimBackground == dim) return;
    m_dimBackground = dim;
    emit dimBackgroundChanged();
    markDirty();
}

void Popup::setClosePolicy(int policy) {
    if (m_closePolicy == policy) return;
    m_closePolicy = policy;
    emit closePolicyChanged();
}

void Popup::setInitialFocusItem(Item *item) {
    if (m_initialFocusItem == item) return;
    m_initialFocusItem = item;
    emit initialFocusItemChanged();
}

void Popup::setOwner(Item *owner) {
    if (m_owner == owner) return;
    m_owner = owner;
    emit ownerChanged();
}

Popup *Popup::parentPopup() const {
    for (Item *current = m_owner ? m_owner.data() : parentItem(); current;
         current = current->parentItem()) {
        if (auto *popup = qobject_cast<Popup*>(current)) return popup;
    }
    return nullptr;
}

bool Popup::isDescendantOfPopup(const Popup *popup) const {
    for (const Popup *current = parentPopup(); current; current = current->parentPopup())
        if (current == popup) return true;
    return false;
}

void Popup::open() {
    if (Terminal *term = terminal())
        m_previousFocusItem = term->focusedItem();
    if (visible()) {
        if (Terminal *term = terminal()) term->registerPopup(this);
    } else {
        setVisible(true);
    }
    if (Terminal *term = terminal()) {
        Item *target = m_initialFocusItem;
        if (!target || !target->visible() || !target->isEffectivelyEnabled() || !target->focusable()) {
            QList<Item*> focusable;
            term->findFocusableItems(this, focusable);
            target = focusable.isEmpty() ? nullptr : focusable.first();
        }
        if (target || m_modal)
            term->setFocusedItem(target);
    }
}

void Popup::close() {
    setVisible(false);
    if (Terminal *term = terminal()) {
        Item *target = m_previousFocusItem;
        if (target && target->visible() && target->isEffectivelyEnabled() && target->focusable())
            term->setFocusedItem(target);
        else
            term->setFocusedItem(nullptr);
    }
    m_previousFocusItem.clear();
}

void Popup::accept() {
    emit accepted();
    emit finished(true);
    close();
}

void Popup::reject() {
    emit rejected();
    emit finished(false);
    close();
}

void Popup::setVisible(bool visible) {
    if (this->visible() == visible) return;
    Item::setVisible(visible);

    Terminal* term = terminal();
    if (term) {
        if (visible) {
            term->registerPopup(this);
            emit opened();
        } else {
            term->unregisterPopup(this);
            emit closed();
        }
    }
}

void Popup::setParentItem(Item *parentItem) {
    Terminal* oldTerm = terminal();
    if (oldTerm && visible()) {
        oldTerm->unregisterPopup(this);
    }

    Item::setParentItem(parentItem);

    Terminal* newTerm = terminal();
    if (newTerm && visible()) {
        newTerm->registerPopup(this);
    }
}

} // namespace qtui
