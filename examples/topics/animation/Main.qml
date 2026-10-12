import QtQml
import Qamui

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    id: rootWindow
    color: "black"

    property int stateCounter: 0

    property Timer animTimer: Timer {
        interval: 1500
        running: true
        repeat: true
        onTriggered: {
            rootWindow.stateCounter = (rootWindow.stateCounter + 1) % 4
        }
    }

    Rectangle {
        id: animationArea
        anchors.fill: parent
        color: "#111111"
        border.style: Border.Single
        border.color: "gray"

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            text: "Declarative TUI Animation & Transition Demo"
            color: "cyan"
            bold: true
        }

        // Animated visual box
        Rectangle {
            id: animatedBox
            width: 18
            height: 5
            anchors.left: animationArea.left
            anchors.top: animationArea.top
            border.style: Border.Double
            border.color: "yellow"
            color: "blue"

            states: [
                State {
                    name: "topLeft"
                    when: rootWindow.stateCounter === 0
                    PropertyChanges {
                        target: animatedBox.anchors
                        properties: {
                            "leftMargin": 2,
                            "topMargin": 2
                        }
                    }
                    PropertyChanges {
                        target: animatedBox
                        properties: { "color": "blue" }
                    }
                },
                State {
                    name: "topRight"
                    when: rootWindow.stateCounter === 1
                    PropertyChanges {
                        target: animatedBox.anchors
                        properties: {
                            "leftMargin": animationArea.width - animatedBox.width - 2,
                            "topMargin": 2
                        }
                    }
                    PropertyChanges {
                        target: animatedBox
                        properties: { "color": "green" }
                    }
                },
                State {
                    name: "bottomRight"
                    when: rootWindow.stateCounter === 2
                    PropertyChanges {
                        target: animatedBox.anchors
                        properties: {
                            "leftMargin": animationArea.width - animatedBox.width - 2,
                            "topMargin": animationArea.height - animatedBox.height - 2
                        }
                    }
                    PropertyChanges {
                        target: animatedBox
                        properties: { "color": "red" }
                    }
                },
                State {
                    name: "bottomLeft"
                    when: rootWindow.stateCounter === 3
                    PropertyChanges {
                        target: animatedBox.anchors
                        properties: {
                            "leftMargin": 2,
                            "topMargin": animationArea.height - animatedBox.height - 2
                        }
                    }
                    PropertyChanges {
                        target: animatedBox
                        properties: { "color": "magenta" }
                    }
                }
            ]

            transitions: [
                Transition {
                    from: "topLeft"
                    to: "topRight"
                    NumberAnimation {
                        target: animatedBox.anchors
                        duration: 800
                        easing: Easing.InOutQuad
                    }
                    ColorAnimation {
                        target: animatedBox
                        duration: 800
                    }
                },
                Transition {
                    from: "topRight"
                    to: "bottomRight"
                    NumberAnimation {
                        target: animatedBox.anchors
                        duration: 800
                        easing: Easing.OutBounce
                    }
                    ColorAnimation {
                        target: animatedBox
                        duration: 800
                    }
                },
                Transition {
                    from: "bottomRight"
                    to: "bottomLeft"
                    NumberAnimation {
                        target: animatedBox.anchors
                        duration: 800
                        easing: Easing.InOutQuad
                    }
                    ColorAnimation {
                        target: animatedBox
                        duration: 800
                    }
                },
                Transition {
                    from: "bottomLeft"
                    to: "topLeft"
                    NumberAnimation {
                        target: animatedBox.anchors
                        duration: 800
                        easing: Easing.OutBounce
                    }
                    ColorAnimation {
                        target: animatedBox
                        duration: 800
                    }
                }
            ]

            Text {
                anchors.centerIn: parent
                color: "white"
                bold: true
                text: "TUI ANIM"
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 1
            text: "Watching box move in 4 corner states with easing. Press 'q' to quit."
            color: "silver"
        }
    }
}
