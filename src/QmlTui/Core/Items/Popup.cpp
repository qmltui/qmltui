#include <QmlTui/Core/Items/Popup.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QPointer>

namespace qmltui {

struct PopupPrivate {
    bool modal = false;
    bool dimBackground = true;
    Popup::ClosePolicies closePolicy =
        Popup::ClosePolicies(Popup::CloseOnEscape) | Popup::CloseOnOutsideClick;
    QPointer<Item> initialFocusItem;
    QPointer<Item> previousFocusItem;
    bool tookFocus = false;
    QPointer<Item> owner;
    quint64 openSerial = 0;
};

Popup::Popup(QObject* parent) : Item(parent), m_d(std::make_unique<PopupPrivate>()) {
    setItemFlag(ItemFlag::IsPopup);
    setItemFlag(ItemFlag::BoundedSelfPaint);
    // Popups are not focusable by default, but modal popups will trap input.
    setVisible(false);
}

Popup::~Popup() {
    Terminal* term = terminal();
    if (term) {
        term->unregisterPopup(this);
    }
}

bool Popup::modal() const {
    return m_d->modal;
}

void Popup::setModal(bool modal) {
    if (m_d->modal == modal) return;
    m_d->modal = modal;
    emit modalChanged();
    markDirty();
}

bool Popup::dimBackground() const {
    return m_d->dimBackground;
}

void Popup::setDimBackground(bool dim) {
    if (m_d->dimBackground == dim) return;
    m_d->dimBackground = dim;
    emit dimBackgroundChanged();
    markDirty();
}

Popup::ClosePolicies Popup::closePolicy() const {
    return m_d->closePolicy;
}

void Popup::setClosePolicy(ClosePolicies policy) {
    if (m_d->closePolicy == policy) return;
    m_d->closePolicy = policy;
    emit closePolicyChanged();
}

Item* Popup::initialFocusItem() const {
    return m_d->initialFocusItem.data();
}

void Popup::setInitialFocusItem(Item* item) {
    if (m_d->initialFocusItem == item) return;
    m_d->initialFocusItem = item;
    emit initialFocusItemChanged();
}

Item* Popup::owner() const {
    return m_d->owner.data();
}

void Popup::setOwner(Item* owner) {
    if (m_d->owner == owner) return;
    m_d->owner = owner;
    emit ownerChanged();
}

quint64 Popup::openSerial() const {
    return m_d->openSerial;
}

void Popup::setOpenSerial(quint64 serial) {
    m_d->openSerial = serial;
}

Popup* Popup::parentPopup() const {
    for (Item* current = m_d->owner ? m_d->owner.data() : parentItem(); current;
         current = current->parentItem()) {
        if (current->isPopup()) return static_cast<Popup*>(current);
    }
    return nullptr;
}

bool Popup::isDescendantOfPopup(const Popup* popup) const {
    for (const Popup* current = parentPopup(); current; current = current->parentPopup())
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
    QPointer<Popup> self(this);
    QPointer<Item> previousFocus;
    if (Terminal* term = terminal()) previousFocus = term->focusedItem();
    if (visible()) {
        if (Terminal* term = terminal()) term->registerPopup(this);
    } else {
        setVisible(true);
        if (!self) return;
    }
    if (Terminal* term = terminal()) {
        Item* target = m_d->initialFocusItem;
        if (!target || !target->visible() || !target->isEffectivelyEnabled() ||
            (target->focusPolicy() & Qt::TabFocus) == 0) {
            QList<Item*> focusable;
            term->findFocusableItems(this, focusable);
            target = focusable.isEmpty() ? nullptr : focusable.first();
        }
        // A popup with nothing to focus (a tooltip) must leave focus alone,
        // both now and when it closes.
        if (target || m_d->modal) {
            m_d->previousFocusItem = previousFocus;
            m_d->tookFocus = true;
            term->setFocusedItem(target);
        }
    }
}

void Popup::close() {
    // Closing a popup that is not open must not touch focus: there is nothing
    // to restore, and clearing focus would take it from whoever has it now.
    if (!visible()) {
        m_d->previousFocusItem.clear();
        return;
    }
    QPointer<Popup> self(this);
    const bool tookFocus = m_d->tookFocus;
    QPointer<Item> previousFocus = m_d->previousFocusItem;
    m_d->tookFocus = false;
    m_d->previousFocusItem.clear();
    setVisible(false);
    if (!self) return;
    // Restore focus only when this popup took it and still holds it; when the
    // popup never took focus, or focus has since moved elsewhere, closing must
    // not yank it back.
    if (Terminal* term = terminal(); term && tookFocus) {
        Item* focused = term->focusedItem();
        if (!focused || focused->isDescendantOf(this)) {
            Item* target = previousFocus.data();
            if (target && target->visible() && target->isEffectivelyEnabled() &&
                target->focusPolicy() != Qt::NoFocus)
                term->setFocusedItem(target);
            else
                term->setFocusedItem(nullptr);
        }
    }
}

void Popup::accept() {
    QPointer<Popup> self(this);
    emit accepted();
    if (!self) return;
    emit finished(true);
    if (!self) return;
    close();
}

void Popup::reject() {
    QPointer<Popup> self(this);
    emit rejected();
    if (!self) return;
    emit finished(false);
    if (!self) return;
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

void Popup::setParentItem(Item* parentItem) {
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

} // namespace qmltui
