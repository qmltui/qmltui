import QtQml
import QmlTui

// A push button: a bordered Rectangle background and a centred label.
// Implicit size is the label plus 4 cells by 3 rows when bordered, or 1 row
// when a custom background has no border (Border.None). Three states, three
// distinct looks: the border is double while focused (single otherwise, none
// below 3 rows); the label is bold while focused, hovered (hoverEnabled) or
// highlighted; and the label is drawn in reverse video while `down`, i.e.
// while a mouse button is held, so a click flashes and the persistent double
// border reads as focus, not as a stuck press. Replace `background` or
// `contentItem` to restyle; set `textColor` for the label.
AbstractButton {
    id: control

    leftPadding: 2
    rightPadding: 2
    topPadding: background === defaultBackground ? 1 : (background ? background.topPadding : 0)
    bottomPadding: background === defaultBackground ? 1 : (background ? background.bottomPadding : 0)

    background: Rectangle {
        id: defaultBackground
        color: QmlTui.defaultColor
        border.style: control.height >= 3 ? (control.activeFocus ? Border.Double : Border.Single) : Border.None
    }

    contentItem: Text {
        text: control.text
        color: control.textColor
        bold: control.activeFocus || control.hovered || control.highlighted
        reversed: control.down
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
    }
}
