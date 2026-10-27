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
    property int currentIndex: count > 0 ? 0 : -1
    property int maxVisibleItems: 6
    property color textColor
    property color popupColor: color
    property color popupBorderColor: border.color
    property color selectionColor
    property color selectedTextColor
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
        dropdownPopup.open();
    }

    function close() {
        dropdownPopup.close();
    }

    function selectIndex(index) {
        if (index < 0 || index >= popupList.count)
            return;
        currentIndex = index;
        activated(index);
    }

    focusPolicy: enabled ? Qt.StrongFocus : Qt.NoFocus
    implicitWidth: Math.max(background ? background.implicitWidth : 0, 16, (contentItem ? contentItem.implicitWidth : 0) + (indicator ? indicator.implicitWidth : 0) + 5)
    implicitHeight: border.style !== Border.None ? 3 : Math.max(background ? background.implicitHeight : 0, 1)

    onCountChanged: {
        if (popupList.count <= 0) {
            currentIndex = -1;
        } else if (currentIndex < 0 || currentIndex >= popupList.count) {
            currentIndex = 0;
        }
    }
    onWidthChanged: d.layout()
    onHeightChanged: d.layout()
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
            if (control.currentIndex > 0)
                control.selectIndex(control.currentIndex - 1);
            event.accepted = true;
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_Right) {
            if (control.currentIndex + 1 < control.count)
                control.selectIndex(control.currentIndex + 1);
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
        property Item prevBackground: null
        property Item prevContentItem: null
        property Item prevIndicator: null
        readonly property bool showBrackets: control.border.style === Border.None && control.background === null && control.contentItem === defaultContentItem && control.indicator === defaultIndicator

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
        visible: d.showBrackets
        x: 0
        y: Math.max(0, Math.floor((control.height - 1) / 2))
        text: "["
        color: control.textColor
        bold: control.activeFocus || control.opened
    }

    Text {
        id: defaultContentItem
        visible: control.contentItem === defaultContentItem
        text: control.displayText
        clip: true
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
        visible: d.showBrackets
        x: Math.max(0, control.width - 1)
        y: Math.max(0, Math.floor((control.height - 1) / 2))
        text: "]"
        color: control.textColor
        bold: control.activeFocus || control.opened
    }

    MouseArea {
        anchors.fill: parent
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
                    control.selectIndex(control.currentIndex + 1);
            } else if (event.wheelDeltaY > 0 || event.wheelDeltaX < 0) {
                if (control.currentIndex > 0)
                    control.selectIndex(control.currentIndex - 1);
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
            readonly property bool hasExplicitSelection: control.selectionColor !== QmlTui.defaultColor && String(control.selectionColor) !== "#00000000"

            width: popupList.width
            height: 1
            color: isHighlighted && hasExplicitSelection ? control.selectionColor : control.popupColor

            Text {
                width: parent.width
                height: 1
                text: (optionRow.index === control.currentIndex ? "▸ " : "  ") + control.itemText(optionRow.modelData)
                clip: true
                color: optionRow.isHighlighted && control.selectedTextColor !== QmlTui.defaultColor && String(control.selectedTextColor) !== "#00000000" ? control.selectedTextColor : control.textColor
                bold: optionRow.isHighlighted || optionRow.index === control.currentIndex
                reversed: optionRow.isHighlighted && !optionRow.hasExplicitSelection
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
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
