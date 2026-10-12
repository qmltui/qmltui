pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root
    focus: true
    property bool alternate: false

    component AppButton: Button {
        background: Rectangle {
            color: "#202830"
            border.color: "cyan"
            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? Border.Double : Border.Single) : Border.None
        }
        textColor: root.alternate ? "#abcdef" : "#123456"
    }

    Button { id: plain; width: 14; height: 1; text: "Default" }
    AppButton {
        id: custom
        y: 2
        width: 14
        height: 1
        text: "Custom"
        ToolTip.text: "Tip"
        ToolTip.timeout: 0
        ToolTip.textColor: "#ffffff"
        ToolTip.color: "#654321"
    }
    AppButton { y: 4; width: 14; height: 1; text: "Reusable" }
    TabBar { y: 6; width: 30; tabs: ["One", "Two"] }
    TextInput { y: 8; width: 20; placeholderText: "Hint" }
    Canvas {
        y: 10
        width: 3
        height: 1
        renderMode: Canvas.HalfBlock
        Point { x: 0; y: 0 }
        Point { x: 1; y: 1 }
        Line { x1: 2; y1: 0; x2: 2; y2: 1 }
    }
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
        else if (event.key === Qt.Key_F)
            custom.focus = true
        else if (event.key === Qt.Key_D)
            custom.enabled = false
        else if (event.key === Qt.Key_C)
            root.alternate = !root.alternate
        else if (event.key === Qt.Key_T)
            custom.ToolTip.visible = true
        else
            return
        event.accepted = true
    }
}
