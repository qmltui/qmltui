#include <QmlTui/Core/Items/Layout.h>
#include <QmlTui/Core/Items/Item.h>

namespace qmltui {

LayoutAttached *Layout::qmlAttachedProperties(QObject *object) {
    auto *attached = new LayoutAttached(object);
    if (Item *item = qobject_cast<Item*>(object)) {
        if (Item *parent = item->parentItem()) {
            QObject::connect(attached, &LayoutAttached::changed, parent,
                             [parent] { parent->markDirty(); });
        }
    }
    return attached;
}

} // namespace qmltui
