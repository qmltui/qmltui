import QtQml
import Qtui

Terminal {
    id: root
    focus: true
    color: "#101820"

    property string fixtureStatus: "READY"

    Timer {
        interval: 75
        running: true
        repeat: false
        onTriggered: root.fixtureStatus = "ASYNC READY"
    }

    Text {
        x: 1
        y: 1
        text: "TESTRUNNER FIXTURE"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 3
        text: "STATUS: " + root.fixtureStatus
        color: "#7fdbca"
    }

    TextInput {
        id: input
        x: 1
        y: 5
        width: 50
        placeholderText: "type text here"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 7
        text: "INPUT: " + input.text
        color: "#f1fa8c"
    }

    Text {
        x: 1
        y: 9
        text: "界Z"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 11
        text: "DUPLICATE"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 12
        text: "DUPLICATE"
        color: "#ffffff"
    }

    Text {
        x: 1
        y: 23
        text: "."
        color: "#ffffff"
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_K) {
            root.fixtureStatus = "KEY K"
            event.accepted = true
        }
    }
}
