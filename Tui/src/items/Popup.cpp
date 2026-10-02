#include "Popup.h"
#include "Terminal.h"

namespace qtui {

Popup::Popup(QObject *parent) : Item(parent) {
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

void Popup::open() {
    setVisible(true);
}

void Popup::close() {
    setVisible(false);
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
