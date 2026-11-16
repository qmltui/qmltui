pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// Dropdown selection control with an inline summary row and a Popup option list.
// Accepts any model supported by ListView (`QAbstractItemModel`, `ListModel`,
// string/object arrays, or integer counts) and supports custom `background`,
// `contentItem`, `indicator`, and `delegate` slots.
Rectangle {
    id: control

    property alias model: popupList.model
    property string textRole: ""
    property int currentIndex: d.defaultIndex
    property int maxVisibleItems: 6
    property color textColor
    property color popupColor: QmlTui.defaultColor
    property color popupBorderColor: border.color
    property color selectionColor: QmlTui.invalidColor
    property color selectedTextColor: QmlTui.invalidColor
    readonly property alias count: popupList.count
    readonly property string currentText: (d.modelRevision, textAt(currentIndex))
    property string displayText: currentText
    readonly property alias popup: dropdownPopup
    readonly property bool opened: dropdownPopup.visible
    property Item background: null
    property Item contentItem: defaultContentItem
    property Item indicator: defaultIndicator
    property Component delegate: defaultDelegate

    signal activated(int index)

    function itemText(entry) {
        if (entry === undefined || entry === null)
            return "";
        if (typeof entry === "object") {
            if (textRole !== "" && entry[textRole] !== undefined)
                return String(entry[textRole]);
            if (entry.text !== undefined)
                return String(entry.text);
            if (entry.display !== undefined)
                return String(entry.display);
            if (entry.label !== undefined)
                return String(entry.label);
            if (entry.name !== undefined)
                return String(entry.name);
            if (entry.title !== undefined)
                return String(entry.title);
            const keys = Object.keys(entry);
            for (let i = 0; i < keys.length; ++i) {
                const k = keys[i];
                if (k === "enabled" || k === "checkable" || k === "modelIndex" || k === "foreground" || k === "background" || k === "textAlignment" || k === "checkState" || k === "decoration" || k === "edit")
                    continue;
                if (entry[k] !== undefined && entry[k] !== null)
                    return String(entry[k]);
            }
            if (entry.edit !== undefined)
                return String(entry.edit);
            return "";
        }
        return String(entry);
    }

    function textAt(index) {
        if (index < 0 || index >= popupList.count)
            return "";
        if (textRole !== "") {
            const roleValue = popupList.itemData(index, textRole);
            if (roleValue !== undefined && roleValue !== null)
                return String(roleValue);
        }
        return itemText(popupList.itemData(index));
    }

    function find(text) {
        const total = popupList.count;
        for (let i = 0; i < total; ++i) {
            if (textAt(i) === text)
                return i;
        }
        return -1;
    }

    function open() {
        if (!enabled || popupList.count <= 0)
            return;
        popupList.currentIndex = Math.max(0, Math.min(popupList.count - 1, currentIndex));
        d.positionPopup();
        dropdownPopup.open();
    }

    function close() {
        dropdownPopup.close();
    }

    function selectIndex(index) {
        if (index < 0 || index >= popupList.count)
            return;
        d.selectedIndex = index;
        if (currentIndex !== index)
            currentIndex = index;
        activated(index);
    }

    focusPolicy: enabled ? Qt.StrongFocus : Qt.NoFocus
    implicitWidth: Math.max(background ? background.implicitWidth : 0, 16, (contentItem ? contentItem.implicitWidth : 0) + (indicator ? indicator.implicitWidth : 0) + 5)
    implicitHeight: border.style !== Border.None ? 3 : Math.max(background ? background.implicitHeight : 0, 1)

    onEnabledChanged: {
        if (!enabled && opened)
            close();
    }
    onMaxVisibleItemsChanged: {
        if (dropdownPopup.visible)
            d.positionPopup();
    }
    onCountChanged: {
        if (popupList.count <= 0 || d.selectedIndex < 0 || d.selectedIndex >= popupList.count)
            d.selectedIndex = 0;
        if (dropdownPopup.visible)
            d.positionPopup();
    }
    onWidthChanged: {
        d.layout();
        if (dropdownPopup.visible)
            d.positionPopup();
    }
    onHeightChanged: {
        d.layout();
        if (dropdownPopup.visible)
            d.positionPopup();
    }
    onBackgroundChanged: d.layout()
    onContentItemChanged: d.layout()
    onIndicatorChanged: d.layout()
    Component.onCompleted: d.layout()

    Keys.onPressed: event => {
        if (!control.enabled)
            return;
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space || ((event.modifiers & Qt.AltModifier) && event.key === Qt.Key_Down)) {
            control.open();
            event.accepted = true;
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_Left) {
            if (control.currentIndex > 0 && control.count > 0)
                control.selectIndex(Math.min(control.currentIndex - 1, control.count - 1));
            event.accepted = true;
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_Right) {
            if (control.count > 0 && control.currentIndex + 1 < control.count)
                control.selectIndex(Math.max(0, control.currentIndex + 1));
            event.accepted = true;
        } else if (event.key === Qt.Key_Home && control.count > 0) {
            control.selectIndex(0);
            event.accepted = true;
        } else if (event.key === Qt.Key_End && control.count > 0) {
            control.selectIndex(control.count - 1);
            event.accepted = true;
        }
    }

    QtObject {
        id: d

        property int modelRevision: 0
        property int selectedIndex: 0
        property Item prevBackground: null
        property Item prevContentItem: null
        property Item prevIndicator: null
        readonly property int defaultIndex: control.count <= 0 ? -1 : (d.selectedIndex >= 0 && d.selectedIndex < control.count ? d.selectedIndex : 0)
        readonly property bool showBrackets: control.border.style === Border.None && control.background === null && control.contentItem === defaultContentItem && control.indicator === defaultIndicator

        function positionPopup() {
            const desiredW = Math.max(control.width, 16);
            const desiredH = Math.min(Math.max(1, control.maxVisibleItems), Math.max(1, control.count)) + 2;
            let rootItem = control as Item;
            while (rootItem.parent !== null)
                rootItem = rootItem.parent;
            if (rootItem === control || rootItem.width <= 0 || rootItem.height <= 0) {
                dropdownPopup.x = 0;
                dropdownPopup.y = control.height;
                dropdownPopup.width = desiredW;
                dropdownPopup.height = desiredH;
                return;
            }
            const absPos = control.absolutePosition();
            const termW = rootItem.width;
            const termH = rootItem.height;
            const popupW = Math.min(desiredW, Math.max(3, termW));
            const spaceBelow = termH - (absPos.y + control.height);
            const spaceAbove = absPos.y;
            let popupH = desiredH;
            let relY = control.height;
            if (spaceBelow < desiredH && spaceAbove > spaceBelow) {
                popupH = Math.min(desiredH, Math.max(3, spaceAbove));
                relY = -popupH;
            } else if (spaceBelow < desiredH) {
                popupH = Math.min(desiredH, Math.max(3, termH));
                relY = Math.max(0, termH - popupH) - absPos.y;
            }
            let relX = 0;
            if (absPos.x + popupW > termW)
                relX = Math.max(-absPos.x, termW - popupW - absPos.x);
            dropdownPopup.x = relX;
            dropdownPopup.y = relY;
            dropdownPopup.width = popupW;
            dropdownPopup.height = popupH;
        }

        function syncSlot(item: Item, prevItem: Item, BuiltIn: Item): Item {
            if (prevItem && prevItem !== item) {
                prevItem.visible = false;
                if (prevItem !== BuiltIn)
                    prevItem.parent = null;
            }
            if (item) {
                item.parent = control;
                item.visible = true;
            }
            return item;
        }

        function layout() {
            d.prevBackground = d.syncSlot(control.background, d.prevBackground, null);
            if (control.background) {
                control.background.x = 0;
                control.background.y = 0;
                control.background.width = control.width;
                control.background.height = control.height;
                control.background.z = -1;
            }

            const hasBorder = control.border.style !== Border.None;
            const inset = hasBorder ? 1 : 0;
            const leftBracketWidth = d.showBrackets ? 1 : 0;
            const rightBracketWidth = d.showBrackets ? 1 : 0;

            d.prevIndicator = d.syncSlot(control.indicator, d.prevIndicator, defaultIndicator);
            if (control.indicator) {
                const indW = Math.max(1, control.indicator.implicitWidth > 0 ? control.indicator.implicitWidth : control.indicator.width);
                const indH = Math.max(1, control.indicator.implicitHeight > 0 ? control.indicator.implicitHeight : 1);
                const rightPad = hasBorder ? 1 : 0;
                control.indicator.width = indW;
                control.indicator.height = indH;
                control.indicator.x = Math.max(inset, control.width - inset - rightBracketWidth - rightPad - indW);
                control.indicator.y = Math.max(0, Math.floor((control.height - indH) / 2));
            }

            d.prevContentItem = d.syncSlot(control.contentItem, d.prevContentItem, defaultContentItem);
            if (control.contentItem) {
                const ciH = Math.max(1, control.contentItem.implicitHeight > 0 ? control.contentItem.implicitHeight : 1);
                const startX = inset + leftBracketWidth + (d.showBrackets || hasBorder ? 1 : 0);
                const endX = control.indicator ? Math.max(startX, control.indicator.x - 1) : Math.max(startX, control.width - inset - rightBracketWidth);
                control.contentItem.x = startX;
                control.contentItem.y = Math.max(0, Math.floor((control.height - ciH) / 2));
                control.contentItem.width = Math.max(0, endX - startX);
                control.contentItem.height = ciH;
            }
        }
    }

    Connections {
        target: control.border
        function onStyleChanged() {
            d.layout();
        }
    }

    Connections {
        target: control.contentItem
        function onImplicitWidthChanged() {
            d.layout();
        }
        function onImplicitHeightChanged() {
            d.layout();
        }
    }

    Connections {
        target: control.indicator
        function onImplicitWidthChanged() {
            d.layout();
        }
        function onImplicitHeightChanged() {
            d.layout();
        }
    }

    Text {
        id: leftBracket
        x: 0
        y: Math.max(0, Math.floor((control.height - 1) / 2))
        visible: d.showBrackets
        text: "["
        color: control.textColor
        bold: control.activeFocus || control.opened
    }

    Text {
        id: defaultContentItem
        clip: true
        visible: control.contentItem === defaultContentItem
        text: control.displayText
        color: control.textColor
        bold: control.activeFocus || control.opened
    }

    Text {
        id: defaultIndicator
        visible: control.indicator === defaultIndicator
        text: control.opened ? "▴" : "▾"
        color: control.textColor
        bold: control.activeFocus || control.opened
        reversed: control.activeFocus || control.opened
    }

    Text {
        id: rightBracket
        x: Math.max(0, control.width - 1)
        y: Math.max(0, Math.floor((control.height - 1) / 2))
        visible: d.showBrackets
        text: "]"
        color: control.textColor
        bold: control.activeFocus || control.opened
    }

    MouseArea {
        x: 0
        y: 0
        width: control.width
        height: control.height
        z: 1
        onClicked: {
            if (!control.enabled)
                return;
            control.forceActiveFocus();
            if (control.opened)
                control.close();
            else
                control.open();
        }
        onWheel: event => {
            if (!control.enabled || control.count <= 0)
                return;
            if (event.wheelDeltaY < 0 || event.wheelDeltaX > 0) {
                if (control.currentIndex + 1 < control.count)
                    control.selectIndex(Math.max(0, control.currentIndex + 1));
            } else if (event.wheelDeltaY > 0 || event.wheelDeltaX < 0) {
                if (control.currentIndex > 0)
                    control.selectIndex(Math.min(control.currentIndex - 1, control.count - 1));
            }
        }
    }

    Component {
        id: defaultDelegate

        Rectangle {
            id: optionRow
            required property int index
            required property var modelData
            readonly property bool isHighlighted: index === popupList.currentIndex
            readonly property bool hasExplicitSelection: control.selectionColor.valid || control.selectedTextColor.valid

            width: popupList.width
            height: 1
            color: isHighlighted && control.selectionColor.valid ? control.selectionColor : control.popupColor

            Text {
                width: parent.width
                height: 1
                clip: true
                text: (optionRow.index === control.currentIndex ? "▸ " : "  ") + control.itemText(optionRow.modelData)
                color: optionRow.isHighlighted && control.selectedTextColor.valid ? control.selectedTextColor : control.textColor
                bold: optionRow.isHighlighted || optionRow.index === control.currentIndex
                reversed: optionRow.isHighlighted && !optionRow.hasExplicitSelection
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!control.enabled)
                        return;
                    popupList.currentIndex = optionRow.index;
                    control.selectIndex(optionRow.index);
                    dropdownPopup.accept();
                }
            }
        }
    }

    Popup {
        id: dropdownPopup
        x: 0
        y: control.height
        width: Math.max(control.width, 16)
        height: Math.min(Math.max(1, control.maxVisibleItems), Math.max(1, control.count)) + 2
        modal: true
        dimBackground: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnOutsideClick
        initialFocusItem: popupList

        Rectangle {
            anchors.fill: parent
            color: control.popupColor
            border {
                style: Border.Single
                color: control.popupBorderColor
            }
        }

        ListView {
            id: popupList
            x: 1
            y: 1
            width: Math.max(0, dropdownPopup.width - 2)
            height: Math.max(0, dropdownPopup.height - 2)
            clip: true
            delegate: control.delegate

            onModelDataChanged: d.modelRevision++

            Keys.onPressed: event => {
                if (!control.enabled)
                    return;
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                    if (popupList.currentIndex >= 0 && popupList.currentIndex < control.count) {
                        control.selectIndex(popupList.currentIndex);
                    }
                    dropdownPopup.accept();
                    event.accepted = true;
                }
            }
        }
    }
}
