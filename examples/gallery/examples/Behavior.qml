pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Behavior: animate a property whenever it changes. One tile of the QmlTui
// gallery (examples/gallery) and a complete app on its own. Needs a build
// with QMLTUI_ENABLE_BEHAVIOR=ON; the gallery hides this tile otherwise.
Terminal {
    id: root
    color: "#0d1117"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit();
    }
    property bool warm: false

    Column {
        x: 2
        y: 1
        spacing: 1
        Text {
            text: "Left/Right move the block, Space recolours it. Every change is animated by a Behavior."
            color: "#8b949e"
        }
        // The keys live on a focusable item so they also work when this file
        // is hosted inside the gallery, which owns the root's key handling.
        Item {
            id: stage
            width: 60
            height: 3
            focusable: true
            focus: true
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Left)
                    block.x = Math.max(0, block.x - 12);
                else if (event.key === Qt.Key_Right)
                    block.x = Math.min(stage.width - block.width, block.x + 12);
                else if (event.key === Qt.Key_Space)
                    root.warm = !root.warm;
                else
                    return;
                event.accepted = true;
            }
            Rectangle {
                id: block
                x: 0
                width: 12
                height: 3
                color: root.warm ? "#f87171" : "#2cde85"
                border.style: Border.Single
                border.color: "#0d1117"
                // The property still jumps in your code; the Behavior turns the
                // jump into an animation from the old value to the new one.
                Behavior on x {
                    NumberAnimation {
                        duration: 250
                        easing: Easing.OutCubic
                    }
                }
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
            text: "Behavior on color: ColorAnimation 400 ms"
            color: "#7ee0ff"
        }
        Text {
            text: "Target x: " + block.x + "   colour: " + (root.warm ? "warm" : "cool")
            color: "#2cde85"
            bold: true
        }
    }
}
