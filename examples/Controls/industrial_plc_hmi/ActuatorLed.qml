import QtQml 2.15
import Terminal 1.0
import IndustrialPlcHmi 1.0

Rectangle {
    id: root
    width: parent ? Math.floor((parent.width - 1) / 2) : 10
    height: 1
    property string name: "VALVE"
    property bool active: false
    property string activeColor: "#008700"
    property string inactiveColor: "#303030"

    color: root.active ? root.activeColor : root.inactiveColor

    Text {
        anchors.centerIn: parent
        text: root.active ? "[" + root.name + ": ON]" : "[" + root.name + ": OFF]"
        color: root.active ? HmiTheme.textPrimary : HmiTheme.textDim
        bold: root.active
    }
}
