#include <QmlTui/Core/Items/Layout.h>
#include <QmlTui/Core/Items/Item.h>
#include <QPointer>

namespace qmltui {

LayoutAttached* Layout::qmlAttachedProperties(QObject* object) {
    auto* attached = new LayoutAttached(object);
    if (Item* item = qobject_cast<Item*>(object)) {
        QPointer<Item> itemGuard(item);
        QObject::connect(attached, &LayoutAttached::changed, item, [itemGuard] {
            if (itemGuard && itemGuard->parentItem()) {
                itemGuard->parentItem()->markDirty(Item::DirtyFlag::LayoutDirty |
                                                   Item::DirtyFlag::SelfDirty);
            }
        });
    }
    return attached;
}

} // namespace qmltui
