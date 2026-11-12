pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// Interactive integer stepper over `[from, to]` in increments of `stepSize`,
// controlled with Up/Down/Left/Right/Home/End keys, mouse clicks on the
// decrement/increment halves, or the mouse wheel. Supports custom
// `background`, `downIndicator`, `contentItem`, and `upIndicator` slots.
Rectangle {
    id: control

    property int value: 0
    property int from: 0
    property int to: 99
    property int stepSize: 1
    property bool wrap: false
    property string prefix: ""
    property string suffix: ""
    property color textColor
    readonly property string displayText: prefix + String(value) + suffix
    property Item background: null
    property Item downIndicator: defaultDownIndicator
    property Item contentItem: defaultContentItem
    property Item upIndicator: defaultUpIndicator

    signal valueModified

    function increase() {
        const step = Math.max(1, stepSize);
        if (value + step <= to) {
            value = value + step;
        } else if (wrap) {
            value = from;
        } else {
            value = to;
        }
    }

    function decrease() {
        const step = Math.max(1, stepSize);
        if (value - step >= from) {
            value = value - step;
        } else if (wrap) {
            value = to;
        } else {
            value = from;
        }
    }

    focusPolicy: enabled ? Qt.StrongFocus : Qt.NoFocus
    implicitWidth: Math.max(background ? background.implicitWidth : 0, 14, (downIndicator ? downIndicator.implicitWidth : 0) + (contentItem ? contentItem.implicitWidth : 0) + (upIndicator ? upIndicator.implicitWidth : 0) + 2)
    implicitHeight: border.style !== Border.None ? 3 : Math.max(background ? background.implicitHeight : 0, 1)

    onWidthChanged: d.layout()
    onHeightChanged: d.layout()
    onBackgroundChanged: d.layout()
    onDownIndicatorChanged: d.layout()
    onContentItemChanged: d.layout()
    onUpIndicatorChanged: d.layout()
    Component.onCompleted: d.layout()

    Keys.onPressed: event => {
        if (!control.enabled)
            return;
        const prev = control.value;
        if (event.key === Qt.Key_Up || event.key === Qt.Key_Right || event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
            control.increase();
            if (control.value !== prev)
                control.valueModified();
            event.accepted = true;
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_Left || event.key === Qt.Key_Minus) {
            control.decrease();
            if (control.value !== prev)
                control.valueModified();
            event.accepted = true;
        } else if (event.key === Qt.Key_PageUp) {
            const next = Math.min(control.to, control.value + Math.max(1, control.stepSize) * 10);
            if (next !== prev) {
                control.value = next;
                control.valueModified();
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_PageDown) {
            const next = Math.max(control.from, control.value - Math.max(1, control.stepSize) * 10);
            if (next !== prev) {
                control.value = next;
                control.valueModified();
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_Home) {
            if (control.from !== prev) {
                control.value = control.from;
                control.valueModified();
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_End) {
            if (control.to !== prev) {
                control.value = control.to;
                control.valueModified();
            }
            event.accepted = true;
        }
    }

    QtObject {
        id: d

        property Item prevBackground: null
        property Item prevDownIndicator: null
        property Item prevContentItem: null
        property Item prevUpIndicator: null

        function syncSlot(item: Item, prevItem: Item, builtIn: Item): Item {
            if (prevItem && prevItem !== item) {
                prevItem.visible = false;
                if (prevItem !== builtIn)
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

            const inset = control.border.style !== Border.None ? 2 : 0;

            d.prevDownIndicator = d.syncSlot(control.downIndicator, d.prevDownIndicator, defaultDownIndicator);
            if (control.downIndicator) {
                const downW = Math.max(1, control.downIndicator.implicitWidth > 0 ? control.downIndicator.implicitWidth : control.downIndicator.width);
                const downH = Math.max(1, control.downIndicator.implicitHeight > 0 ? control.downIndicator.implicitHeight : 1);
                control.downIndicator.x = inset;
                control.downIndicator.y = Math.max(0, Math.floor((control.height - downH) / 2));
                control.downIndicator.width = downW;
                control.downIndicator.height = downH;
            }

            d.prevUpIndicator = d.syncSlot(control.upIndicator, d.prevUpIndicator, defaultUpIndicator);
            if (control.upIndicator) {
                const upW = Math.max(1, control.upIndicator.implicitWidth > 0 ? control.upIndicator.implicitWidth : control.upIndicator.width);
                const upH = Math.max(1, control.upIndicator.implicitHeight > 0 ? control.upIndicator.implicitHeight : 1);
                control.upIndicator.x = Math.max(inset, control.width - inset - upW);
                control.upIndicator.y = Math.max(0, Math.floor((control.height - upH) / 2));
                control.upIndicator.width = upW;
                control.upIndicator.height = upH;
            }

            d.prevContentItem = d.syncSlot(control.contentItem, d.prevContentItem, defaultContentItem);
            if (control.contentItem) {
                const ciH = Math.max(1, control.contentItem.implicitHeight > 0 ? control.contentItem.implicitHeight : 1);
                const startX = control.downIndicator ? control.downIndicator.x + control.downIndicator.width + 1 : inset;
                const endX = control.upIndicator ? Math.max(startX, control.upIndicator.x - 1) : Math.max(startX, control.width - inset);
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
        target: control.downIndicator
        function onImplicitWidthChanged() {
            d.layout();
        }
        function onImplicitHeightChanged() {
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
        target: control.upIndicator
        function onImplicitWidthChanged() {
            d.layout();
        }
        function onImplicitHeightChanged() {
            d.layout();
        }
    }

    Text {
        id: defaultDownIndicator
        visible: control.downIndicator === defaultDownIndicator
        text: "[-]"
        color: control.textColor
        bold: control.activeFocus
        dim: !control.wrap && control.value <= control.from
    }

    Text {
        id: defaultContentItem
        visible: control.contentItem === defaultContentItem
        text: " " + control.displayText + " "
        horizontalAlignment: Qt.AlignHCenter
        color: control.textColor
        bold: control.activeFocus
        reversed: control.activeFocus
    }

    Text {
        id: defaultUpIndicator
        visible: control.upIndicator === defaultUpIndicator
        text: "[+]"
        color: control.textColor
        bold: control.activeFocus
        dim: !control.wrap && control.value >= control.to
    }

    MouseArea {
        x: 0
        y: 0
        width: control.width
        height: control.height
        z: 1
        onClicked: event => {
            if (!control.enabled)
                return;
            control.forceActiveFocus();
            const prev = control.value;
            if (event.x < Math.floor(control.width / 2)) {
                control.decrease();
            } else {
                control.increase();
            }
            if (control.value !== prev)
                control.valueModified();
        }
        onWheel: event => {
            if (!control.enabled)
                return;
            const prev = control.value;
            if (event.wheelDeltaY > 0 || event.wheelDeltaX > 0) {
                control.increase();
            } else if (event.wheelDeltaY < 0 || event.wheelDeltaX < 0) {
                control.decrease();
            }
            if (control.value !== prev)
                control.valueModified();
        }
    }
}
