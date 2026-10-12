import QtQml
import QmlTui

Text {
    id: control

    property bool running: true
    property int interval: 100
    property list<string> frames: ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]

    onFramesChanged: timer.index = 0

    text: control.frames.length > 0 ? control.frames[timer.index % control.frames.length] : ""
    bold: true
    color: "default"

    Timer {
        id: timer
        property int index: 0

        interval: Math.max(1, control.interval)
        running: control.running && control.visible && control.frames.length > 0
        repeat: true
        onTriggered: {
            if (control.frames.length > 0) {
                timer.index = (timer.index + 1) % control.frames.length
            }
        }
    }
}
