import QtQml
import Qtui

// A push button: a bordered Rectangle background and a centred label.
// Implicit size is the label plus 4 cells by 3 rows. The border is single,
// double while focused, and dropped on a button shorter than 3 rows; the
// label is bold while focused, hovered (hoverEnabled) or highlighted. Replace
// `background` or `contentItem` to restyle; set `textColor` for the label.
AbstractButton {
    id: control

    leftPadding: 2
    rightPadding: 2
    topPadding: 1
    bottomPadding: 1

    background: Rectangle {
        color: "default"
        border.color: "default"
        border.style: control.height >= 3 ? (control.activeFocus ? "double" : "single") : "none"
    }

    contentItem: Text {
        text: control.text
        color: control.textColor
        bold: control.activeFocus || control.hovered || control.highlighted
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
    }
}
