import QtQml 2.15
import Qtui

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    id: rootWindow
   
    color: "green"
    border.style: "single"
    border.color: "yellow"
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 1
        anchors.leftMargin: 2
        anchors.bottomMargin: 1
        anchors.rightMargin: 2
        color: "#111111"
        border.style: "double"
        border.color: "gray"
    }
}
