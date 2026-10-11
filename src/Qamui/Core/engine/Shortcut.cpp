#include <Qamui/Core/engine/Shortcut.h>

namespace qamui {

static QList<Shortcut*> s_shortcuts;

const QList<Shortcut*>& activeShortcuts() {
    return s_shortcuts;
}

void registerShortcut(Shortcut* shortcut) {
    if (shortcut && !s_shortcuts.contains(shortcut)) {
        s_shortcuts.append(shortcut);
    }
}

void unregisterShortcut(Shortcut* shortcut) {
    if (shortcut) {
        s_shortcuts.removeOne(shortcut);
    }
}

Shortcut::Shortcut(QObject *parent) : QObject(parent) {
    registerShortcut(this);
}

Shortcut::~Shortcut() {
    unregisterShortcut(this);
}

void Shortcut::setSequence(int sequence) {
    if (m_sequence != sequence) {
        m_sequence = sequence;
        emit sequenceChanged();
    }
}

void Shortcut::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged();
    }
}

} // namespace qamui
