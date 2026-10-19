pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Behavior: Implicitly animates a property whenever its value changes.
// Requires QMLTUI_ENABLE_PRIVATE_QT=ON.
Column {
    id: behaviorDemo

    property bool warm: false

    spacing: 1

    Text {
        text: "Left/Right move the block; Space recolours it via Behavior."
        color: "#8b949e"
    }

    Item {
        id: stage
        width: 40
        height: 3
        focusable: true
        focus: true

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Left)
                block.x = Math.max(0, block.x - 12);
            else if (event.key === Qt.Key_Right)
                block.x = Math.min(stage.width - block.width, block.x + 12);
            else if (event.key === Qt.Key_Space)
                behaviorDemo.warm = !behaviorDemo.warm;
            else
                return;
            event.accepted = true;
        }

        Rectangle {
            id: block
            x: 0
            width: 12
            height: 3
            color: behaviorDemo.warm ? "#f87171" : "#2cde85"
            border {
                style: Border.Single
                color: "#0d1117"
            }

            // Animate horizontal position changes automatically
            Behavior on x {
                NumberAnimation {
                    duration: 250
                    easing: Easing.OutCubic
                }
            }

            // Animate color changes automatically
            Behavior on color {
                ColorAnimation {
                    duration: 400
                }
            }

            Text {
                anchors.centerIn: parent
                text: "x " + block.x
                color: "#0d1117"
                bold: true
            }
        }
    }

    Text {
        text: "Behavior on x: NumberAnimation 250 ms, Easing.OutCubic"
        color: "#7ee0ff"
    }

    Text {
        text: "Target x: " + block.x + "   colour: " + (behaviorDemo.warm ? "warm" : "cool")
        color: "#2cde85"
        bold: true
    }
}
