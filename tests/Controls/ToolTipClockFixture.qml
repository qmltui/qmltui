import QtQml
import QmlTui
import QmlTui.Controls

// The tooltip delay and timeout run on the QmlTui clock, so a virtual-clock
// scenario can step to one millisecond before the tooltip appears and then
// across its timeout.
Terminal {
    id: root

    Button {
        x: 1
        y: 3
        text: "Target"
        focus: true
        ToolTip.text: "TIPTEXT"
        ToolTip.delay: 500
        ToolTip.timeout: 400
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q)
            Qt.quit();
    }
}
