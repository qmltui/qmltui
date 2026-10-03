import QtQml 2.15
import Qtui

Item {
    id: stackViewRoot

    readonly property alias currentItem: containerItem.currentItem
    readonly property int depth: stackItems.length

    property var stackItems: []

    Item {
        id: containerItem
        anchors.fill: parent
        property Item currentItem: null
    }

    function push(itemOrUrl, properties) {
        let newItem = null;
        if (typeof itemOrUrl === "string") {
            let component = Qt.createComponent(itemOrUrl);
            if (component.status === Component.Ready) {
                newItem = component.createObject(containerItem, properties || {});
            } else {
                console.error("StackView.push failed to load QML:", itemOrUrl, component.errorString());
                return null;
            }
        } else if (itemOrUrl instanceof Component) {
            newItem = itemOrUrl.createObject(containerItem, properties || {});
        } else if (itemOrUrl instanceof Item) {
            newItem = itemOrUrl;
            newItem.parent = containerItem;
        }

        if (!newItem) return null;

        newItem.x = 0;
        newItem.y = 0;
        newItem.width = stackViewRoot.width;
        newItem.height = stackViewRoot.height;

        // Hide previous top item
        if (containerItem.currentItem) {
            containerItem.currentItem.visible = false;
        }

        stackItems.push(newItem);
        containerItem.currentItem = newItem;
        newItem.visible = true;

        return newItem;
    }

    function pop() {
        if (stackItems.length <= 1) return null;

        let popped = stackItems.pop();
        popped.visible = false;
        popped.destroy();

        let prevItem = stackItems[stackItems.length - 1];
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
        while (stackItems.length > 0) {
            let item = stackItems.pop();
            item.visible = false;
            item.destroy();
        }
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
