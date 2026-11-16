#pragma once

// Internal: per-thread active Shortcut registry queried by InputRouter.

#include <QmlTui/Core/Engine/Shortcut.h>

#include <QList>

namespace qmltui {

const QList<Shortcut*>& activeShortcuts();
void registerShortcut(Shortcut* shortcut);
void unregisterShortcut(Shortcut* shortcut);

} // namespace qmltui
