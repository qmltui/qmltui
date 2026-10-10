#include <Qtui/Core/items/Popup.h>
#include <Qtui/Core/items/Terminal.h>

namespace qtui {

Popup::Popup(QObject *parent) : Rectangle(parent) {
    setItemFlag(ItemFlag::IsPopup);
    // Popups are not focusable by default, but modal popups will trap input.
    setVisible(false);
    // A popup is an opaque surface: it must hide what it covers. "default"
    // paints the cells with the terminal's own background, so the popup
    // occludes while still following the user's theme, where "transparent"
    // would leave the content behind it showing through.
    setColor("default");
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
        if (current->isPopup()) return static_cast<Popup*>(current);
    }
    return nullptr;
}

bool Popup::isDescendantOfPopup(const Popup *popup) const {
    for (const Popup *current = parentPopup(); current; current = current->parentPopup())
        if (current == popup) return true;
    return false;
}

bool Popup::isDescendantOfScope(const InputScope* other) const {
    if (!other) return false;
    const Popup* otherPopup = dynamic_cast<const Popup*>(other);
    return otherPopup ? isDescendantOfPopup(otherPopup) : false;
}

void Popup::open() {
    // Snapshot focus before becoming visible: opening may already move it
    // (an application binding reacting to `visible` can clear the focused
    // item), and close() must restore what the user had before the popup.
    Item *previousFocus = nullptr;
    if (Terminal *term = terminal())
        previousFocus = term->focusedItem();
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
        // A popup with nothing to focus (a tooltip) must leave focus alone,
        // both now and when it closes.
        if (target || m_modal) {
            m_previousFocusItem = previousFocus;
            m_tookFocus = true;
            term->setFocusedItem(target);
        }
    }
}

void Popup::close() {
    // Closing a popup that is not open must not touch focus: there is nothing
    // to restore, and clearing focus would take it from whoever has it now.
    if (!visible()) {
        m_previousFocusItem.clear();
        return;
    }
    setVisible(false);
    // Restore focus only when this popup took it and still holds it; when the
    // popup never took focus, or focus has since moved elsewhere, closing must
    // not yank it back.
    if (Terminal *term = terminal(); term && m_tookFocus) {
        Item *focused = term->focusedItem();
        bool focusInsidePopup = focused == nullptr;
        for (Item *item = focused; item && !focusInsidePopup; item = item->parentItem())
            focusInsidePopup = item == this;
        if (focusInsidePopup) {
            Item *target = m_previousFocusItem;
            if (target && target->visible() && target->isEffectivelyEnabled() && target->focusable())
                term->setFocusedItem(target);
            else
                term->setFocusedItem(nullptr);
        }
    }
    m_tookFocus = false;
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
