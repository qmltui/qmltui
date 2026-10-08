import QtQml
import Qtui

// A stack of pages where only the top one is visible. push() accepts a QML
// file URL, a Component, or an existing Item; pages the view created are
// destroyed when popped, pages handed in as Items are only hidden.
Item {
    id: stackViewRoot

    readonly property alias currentItem: containerItem.currentItem
    readonly property alias depth: d.depth

    QtObject {
        id: d
        property int depth: 0
        property var pages: []     // Items, bottom first
        property var owned: []     // true where this view created the page
    }

    Item {
        id: containerItem
        anchors.fill: parent
        property Item currentItem: null
    }

    function push(itemOrUrl, properties) {
        let newItem = null;
        let created = false;
        if (typeof itemOrUrl === "string") {
            let component = Qt.createComponent(itemOrUrl);
            if (component.status === Component.Ready) {
                newItem = component.createObject(containerItem, properties || {});
                created = true;
            } else {
                console.error("StackView.push failed to load QML:", itemOrUrl, component.errorString());
                return null;
            }
        } else if (itemOrUrl instanceof Component) {
            newItem = itemOrUrl.createObject(containerItem, properties || {});
            created = true;
        } else if (itemOrUrl instanceof Item) {
            newItem = itemOrUrl;
            newItem.parent = containerItem;
        }

        if (!newItem) return null;

        newItem.x = 0;
        newItem.y = 0;
        newItem.width = stackViewRoot.width;
        newItem.height = stackViewRoot.height;

        if (containerItem.currentItem) {
            containerItem.currentItem.visible = false;
        }

        d.pages.push(newItem);
        d.owned.push(created);
        d.depth = d.pages.length;
        containerItem.currentItem = newItem;
        newItem.visible = true;

        return newItem;
    }

    function pop() {
        if (d.pages.length <= 1) return null;

        let popped = d.pages.pop();
        let created = d.owned.pop();
        d.depth = d.pages.length;
        popped.visible = false;
        if (created)
            popped.destroy();

        let prevItem = d.pages[d.pages.length - 1];
        containerItem.currentItem = prevItem;
        if (prevItem) {
            prevItem.x = 0;
            prevItem.y = 0;
            prevItem.width = stackViewRoot.width;
            prevItem.height = stackViewRoot.height;
            prevItem.visible = true;
        }

        return prevItem;
    }

    function clear() {
        while (d.pages.length > 0) {
            let item = d.pages.pop();
            let created = d.owned.pop();
            item.visible = false;
            if (created)
                item.destroy();
        }
        d.depth = 0;
        containerItem.currentItem = null;
    }

    onWidthChanged: {
        if (containerItem.currentItem) {
            containerItem.currentItem.width = stackViewRoot.width;
        }
    }

    onHeightChanged: {
        if (containerItem.currentItem) {
            containerItem.currentItem.height = stackViewRoot.height;
        }
    }
}
