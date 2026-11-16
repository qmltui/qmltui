pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// A stack of pages where only the top one is visible. push() and replace()
// accept a QML file URL, a Component, or an existing Item; pages the view
// created are destroyed when popped or replaced, while externally supplied
// Items are only hidden.
Item {
    id: stackViewRoot

    property var initialItem: null
    readonly property alias currentItem: containerItem.currentItem
    readonly property alias depth: d.depth

    function push(itemOrUrl, properties) {
        const entry = d.instantiate(itemOrUrl, properties);
        if (!entry.item)
            return null;

        const newItem = entry.item;
        d.sizePage(newItem);

        let hadFocus = false;
        let savedFocus = null;
        if (containerItem.currentItem) {
            savedFocus = d.findFocusedWithin(containerItem.currentItem);
            hadFocus = savedFocus !== null;
            containerItem.currentItem.visible = false;
        }

        d.savedFocuses.push(savedFocus);
        d.pages.push(newItem);
        d.owned.push(entry.created);
        d.depth = d.pages.length;
        containerItem.currentItem = newItem;
        newItem.visible = true;

        if (hadFocus || stackViewRoot.activeFocus)
            d.focusPage(newItem, null);

        return newItem;
    }

    function replace(itemOrUrl, properties) {
        const entry = d.instantiate(itemOrUrl, properties);
        if (!entry.item)
            return null;

        const newItem = entry.item;
        d.sizePage(newItem);

        let hadFocus = stackViewRoot.activeFocus;
        let preservedSavedFocus = null;
        if (d.pages.length > 0) {
            const oldTop = d.pages.pop();
            const oldCreated = d.owned.pop();
            preservedSavedFocus = d.savedFocuses.pop();
            if (d.findFocusedWithin(oldTop) !== null)
                hadFocus = true;
            oldTop.visible = false;
            if (oldCreated)
                oldTop.destroy();
        }

        d.savedFocuses.push(preservedSavedFocus);
        d.pages.push(newItem);
        d.owned.push(entry.created);
        d.depth = d.pages.length;
        containerItem.currentItem = newItem;
        newItem.visible = true;

        if (hadFocus)
            d.focusPage(newItem, null);

        return newItem;
    }

    function pop() {
        if (d.pages.length <= 1)
            return null;

        const popped = d.pages.pop();
        const created = d.owned.pop();
        const savedFocus = d.savedFocuses.pop();
        const hadFocus = d.findFocusedWithin(popped) !== null || stackViewRoot.activeFocus;

        d.depth = d.pages.length;
        popped.visible = false;
        if (created)
            popped.destroy();

        const prevItem = d.pages[d.pages.length - 1];
        containerItem.currentItem = prevItem;
        if (prevItem) {
            d.sizePage(prevItem);
            prevItem.visible = true;
            if (hadFocus)
                d.focusPage(prevItem, savedFocus);
        }

        return prevItem;
    }

    function clear() {
        while (d.pages.length > 0) {
            const item = d.pages.pop();
            const created = d.owned.pop();
            d.savedFocuses.pop();
            item.visible = false;
            if (created)
                item.destroy();
        }
        d.depth = 0;
        containerItem.currentItem = null;
    }

    onWidthChanged: {
        if (containerItem.currentItem)
            containerItem.currentItem.width = stackViewRoot.width;
    }

    onHeightChanged: {
        if (containerItem.currentItem)
            containerItem.currentItem.height = stackViewRoot.height;
    }

    Component.onCompleted: {
        if (stackViewRoot.initialItem !== null && stackViewRoot.initialItem !== undefined && d.pages.length === 0)
            stackViewRoot.push(stackViewRoot.initialItem);
    }

    QtObject {
        id: d

        property int depth: 0
        property var pages: []
        property var owned: []
        property var savedFocuses: []

        function instantiate(itemOrUrl, properties) {
            if (itemOrUrl === null || itemOrUrl === undefined)
                return {
                    item: null,
                    created: false
                };

            if (itemOrUrl instanceof Component) {
                const obj = itemOrUrl.createObject(containerItem, properties || {});
                return {
                    item: obj as Item,
                    created: obj !== null
                };
            }

            if (itemOrUrl instanceof Item) {
                itemOrUrl.parent = containerItem;
                return {
                    item: itemOrUrl,
                    created: false
                };
            }

            if (typeof itemOrUrl === "string" || !(itemOrUrl instanceof QtObject)) {
                const urlStr = String(itemOrUrl);
                if (urlStr.length === 0)
                    return {
                        item: null,
                        created: false
                    };
                const component = Qt.createComponent(itemOrUrl);
                if (component.status === Component.Ready) {
                    const obj = component.createObject(containerItem, properties || {});
                    component.destroy();
                    return {
                        item: obj as Item,
                        created: obj !== null
                    };
                }
                console.error("StackView failed to load QML:", itemOrUrl, component.errorString());
                component.destroy();
            }
            return {
                item: null,
                created: false
            };
        }

        function sizePage(page: Item) {
            page.x = 0;
            page.y = 0;
            page.width = stackViewRoot.width;
            page.height = stackViewRoot.height;
        }

        function findFocusedWithin(item: Item): Item {
            if (!item)
                return null;
            if (item.activeFocus)
                return item;
            const kids = item.children;
            for (let i = 0; i < kids.length; ++i) {
                const found = d.findFocusedWithin(kids[i]);
                if (found)
                    return found;
            }
            return null;
        }

        function findFirstFocusable(item: Item): Item {
            if (!item || !item.visible || !item.enabled)
                return null;
            if (item.focusPolicy !== Qt.NoFocus)
                return item;
            const kids = item.children;
            for (let i = 0; i < kids.length; ++i) {
                const found = d.findFirstFocusable(kids[i]);
                if (found)
                    return found;
            }
            return null;
        }

        function focusPage(page: Item, preferred: Item) {
            if (preferred && preferred.visible && preferred.enabled) {
                preferred.forceActiveFocus();
                return;
            }
            const target = d.findFirstFocusable(page);
            if (target)
                target.forceActiveFocus();
            else if (page)
                page.forceActiveFocus();
        }
    }

    Item {
        id: containerItem
        property Item currentItem: null
        anchors.fill: parent
    }
}
