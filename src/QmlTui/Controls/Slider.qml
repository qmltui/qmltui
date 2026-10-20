import QtQml
import QmlTui

// Interactive single-row slider over `[from, to]` in increments of `stepSize`,
// controlled with Left/Right/Home/End keys, mouse click/drag, or mouse wheel.
Rectangle {
    id: control
    property real value: 0.0
    property real from: 0.0
    property real to: 100.0
    property real stepSize: 1.0
    property rgba textColor: "default"
    property bool showValue: true
    readonly property int trackWidth: Math.max(0, control.width - (control.showValue ? valueText.implicitWidth + 1 : 0))

    focusPolicy: enabled ? Qt.StrongFocus : Qt.NoFocus
    implicitWidth: 25
    implicitHeight: 1
    Text {
        anchors.verticalCenter: parent.verticalCenter
        x: 0
        text: {
            const tw = control.trackWidth;
            if (tw <= 0)
                return "";
            const range = Math.max(0.001, control.to - control.from);
            const pct = Math.min(1.0, Math.max(0.0, (control.value - control.from) / range));
            const handlePos = Math.round(pct * (tw - 1));

            // Focus is visible in the track itself: a heavy line while the
            // slider has active focus, a light one otherwise. Bold alone is
            // invisible on box-drawing glyphs in most terminals.
            const track = control.activeFocus ? "━" : "─";
            let str = "";
            for (let i = 0; i < tw; ++i) {
                if (i === handlePos) {
                    str += "●";
                } else {
                    str += track;
                }
            }
            return str;
        }
        color: control.textColor
        bold: control.activeFocus
    }

    Text {
        id: valueText
        visible: control.showValue
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
        }
        text: "[" + Math.round(control.value) + "]"
        color: control.textColor
        // The readout inverts while focused, so a one-row slider shows focus
        // even when its track is too short to read.
        bold: control.activeFocus
        reversed: control.activeFocus
    }

    Keys.onPressed: event => {
        if (!control.enabled)
            return;
        if (event.key === Qt.Key_Left) {
            control.value = Math.max(control.from, control.value - control.stepSize);
            event.accepted = true;
        } else if (event.key === Qt.Key_Right) {
            control.value = Math.min(control.to, control.value + control.stepSize);
            event.accepted = true;
        } else if (event.key === Qt.Key_Home) {
            control.value = control.from;
            event.accepted = true;
        } else if (event.key === Qt.Key_End) {
            control.value = control.to;
            event.accepted = true;
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        function updateFromX(posX) {
            if (control.trackWidth <= 0)
                return;
            const range = Math.max(0.001, control.to - control.from);
            const pct = Math.min(1.0, Math.max(0.0, posX / control.trackWidth));
            const step = control.stepSize > 0 ? control.stepSize : 1;
            const raw = control.from + pct * range;
            control.value = Math.min(control.to, Math.max(control.from, control.from + Math.round((raw - control.from) / step) * step));
        }
        onPressed: event => {
            if (!control.enabled)
                return;
            mouseArea.updateFromX(event.x);
        }
        onClicked: event => {
            if (!control.enabled)
                return;
            mouseArea.updateFromX(event.x);
        }
        onMouseXChanged: {
            if (!control.enabled || !mouseArea.pressed)
                return;
            mouseArea.updateFromX(mouseArea.mouseX);
        }
        onWheel: event => {
            if (!control.enabled)
                return;
            const step = control.stepSize > 0 ? control.stepSize : 1;
            if (event.wheelDeltaY > 0 || event.wheelDeltaX > 0) {
                control.value = Math.min(control.to, control.value + step);
            } else if (event.wheelDeltaY < 0 || event.wheelDeltaX < 0) {
                control.value = Math.max(control.from, control.value - step);
            }
        }
    }
}
