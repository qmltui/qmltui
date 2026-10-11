import QtQml
import Qamui

// The interaction half of every button-like control, in the shape of Qt Quick
// Controls' AbstractButton: text, pressed/down, checkable/checked with
// toggle(), autoExclusive, highlighted, opt-in hover, and the two items a
// control is made of. `background` fills the control at z -1; `contentItem`
// gets the area inside the paddings. Button, CheckBox, RadioButton and Switch
// only supply defaults for those two items. Assign your own to restyle:
//
//     Button {
//         background: Rectangle { color: "#65e6a0"; border.style: "none" }
//         contentItem: Text { text: parent.text; color: "#0b0f14" }
//     }
//
// Enter or Space while focused, or a mouse click, activates the control:
// toggle() when checkable, then clicked(). The engine withholds keys and mouse
// events from disabled items; the guards here keep direct calls consistent.
// `pressed` follows the mouse only: terminals report no key releases.
Item {
    id: control

    property string text: ""
    property bool pressed: false
    property bool down: pressed
    property bool checkable: false
    property bool checked: false
    property bool autoExclusive: false
    property bool highlighted: false
    property bool hoverEnabled: false
    readonly property alias hovered: mouseArea.containsMouse
    property rgba textColor: "default"

    property int padding: 0
    property int topPadding: padding
    property int bottomPadding: padding
    property int leftPadding: padding
    property int rightPadding: padding
    readonly property int availableWidth: Math.max(0, width - leftPadding - rightPadding)
    readonly property int availableHeight: Math.max(0, height - topPadding - bottomPadding)

    property Item background: null
    property Item contentItem: null

    signal clicked()
    signal toggled(bool checked)

    focusable: enabled
    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            leftPadding + (contentItem ? contentItem.implicitWidth : 0) + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             topPadding + (contentItem ? contentItem.implicitHeight : 0) + bottomPadding)

    // Flips `checked` and emits toggled(); a checked autoExclusive control
    // stays checked, so a group of RadioButtons never ends up with none.
    function toggle() {
        if (!checkable || (checked && autoExclusive))
            return
        checked = !checked
        toggled(checked)
    }

    QtObject {
        id: d
        function activate() {
            if (!control.enabled)
                return
            if (control.checkable)
                control.toggle()
            control.clicked()
        }
        // background fills the control; contentItem gets the padded area, or
        // is centred in the whole control when the paddings leave no room
        // (a one-row Button keeps its label visible).
        function layout() {
            const bg = control.background
            if (bg) {
                bg.parent = control   // adopts an item QML created for the property
                bg.x = 0
                bg.y = 0
                bg.width = control.width
                bg.height = control.height
                bg.z = -1
            }
            const ci = control.contentItem
            if (!ci)
                return
            ci.parent = control
            const cw = ci.implicitWidth
            const ch = ci.implicitHeight
            if (control.availableWidth > 0) {
                ci.x = control.leftPadding
                ci.width = control.availableWidth
            } else {
                ci.width = Math.min(cw, control.width)
                ci.x = Math.max(0, Math.floor((control.width - ci.width) / 2))
            }
            if (control.availableHeight > 0) {
                ci.y = control.topPadding
                ci.height = control.availableHeight
            } else {
                ci.height = Math.min(ch, control.height)
                ci.y = Math.max(0, Math.floor((control.height - ci.height) / 2))
            }
        }
    }

    onBackgroundChanged: d.layout()
    onContentItemChanged: d.layout()
    onWidthChanged: d.layout()
    onHeightChanged: d.layout()
    onTopPaddingChanged: d.layout()
    onBottomPaddingChanged: d.layout()
    onLeftPaddingChanged: d.layout()
    onRightPaddingChanged: d.layout()
    Component.onCompleted: d.layout()

    Connections {
        target: control.contentItem
        function onImplicitWidthChanged() { d.layout() }
        function onImplicitHeightChanged() { d.layout() }
    }

    Keys.onPressed: (event) => {
        if (control.enabled && (event.key === Qt.Key_Return || event.key === Qt.Key_Space)) {
            d.activate()
            event.accepted = true
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        z: 1
        hoverEnabled: control.hoverEnabled

        onPressed: (event) => {
            if (!control.enabled)
                return
            control.focus = true
            control.pressed = true
        }
        onReleased: (event) => {
            control.pressed = false
        }
        onClicked: (event) => {
            d.activate()
        }
    }
}
