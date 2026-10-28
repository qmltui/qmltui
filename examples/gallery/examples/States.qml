pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// States: Declarative State and PropertyChanges animated by Transition.
Column {
    id: statesDemo

    property string mode: ""

    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "PropertyChanges and Transition animate geometry and colors."
        color: "#8b949e"
    }

    Row {
        spacing: 1

        Button {
            text: statesDemo.mode === "on" ? "Standby (off)" : "Switch on"
            focus: true
            onClicked: statesDemo.mode = (statesDemo.mode === "on" ? "" : "on")
        }

        Button {
            text: "Overload"
            onClicked: statesDemo.mode = "overload"
        }

        Button {
            text: "Reset"
            onClicked: statesDemo.mode = ""
        }
    }

    Rectangle {
        id: card
        width: 48
        height: 8
        color: "#161b22"

        Text {
            id: cardLabel
            x: 2
            y: 1
            text: "● STANDBY (OFF)"
            bold: true
            color: "#c9d1d9"
        }

        Text {
            id: cardDetail
            x: 2
            y: 2
            text: "workers: 0 / 8  ·  throughput: idle"
            color: "#8b949e"
        }

        Text {
            id: stageText
            x: 2
            y: 4
            text: "IDLE 12%"
            color: "#8b949e"
            bold: true
        }

        // Animated indicator bar whose x, width, and colours transition between states
        Rectangle {
            id: stageBadge
            x: 2
            y: 5
            width: 12
            height: 2
            color: "#21262d"
        }

        states: [
            State {
                name: "on"
                when: statesDemo.mode === "on"
                PropertyChanges {
                    target: card
                    properties: {
                        "color": "#122d24"
                    }
                }
                PropertyChanges {
                    target: cardLabel
                    properties: {
                        "text": "● ONLINE (ON)",
                        "color": "#00ff66"
                    }
                }
                PropertyChanges {
                    target: cardDetail
                    properties: {
                        "text": "workers: 8 / 8  ·  throughput: 1,420 req/s",
                        "color": "#c9d1d9"
                    }
                }
                PropertyChanges {
                    target: stageBadge
                    properties: {
                        "x": 16,
                        "width": 16,
                        "color": "#0d1117"
                    }
                }
                PropertyChanges {
                    target: stageText
                    properties: {
                        "text": "ACTIVE 72%",
                        "color": "#00ff66"
                    }
                }
            },
            State {
                name: "overload"
                when: statesDemo.mode === "overload"
                PropertyChanges {
                    target: card
                    properties: {
                        "color": "#3b1219"
                    }
                }
                PropertyChanges {
                    target: cardLabel
                    properties: {
                        "text": "▲ OVERLOAD (ALERT)",
                        "color": "#fca5a5"
                    }
                }
                PropertyChanges {
                    target: cardDetail
                    properties: {
                        "text": "workers: 8 / 8  ·  queue backpressure!",
                        "color": "#fecaca"
                    }
                }
                PropertyChanges {
                    target: stageBadge
                    properties: {
                        "x": 30,
                        "width": 16,
                        "color": "#450a0a"
                    }
                }
                PropertyChanges {
                    target: stageText
                    properties: {
                        "text": "PEAK 100%",
                        "color": "#fca5a5"
                    }
                }
            }
        ]

        transitions: [
            Transition {
                NumberAnimation {
                    property: "x"
                    duration: 350
                    easing: Easing.OutCubic
                }
                NumberAnimation {
                    property: "width"
                    duration: 350
                    easing: Easing.OutCubic
                }
                ColorAnimation {
                    duration: 350
                }
            }
        ]
    }

    Text {
        text: "state: " + (card.state === "" ? "(base)" : card.state)
        color: "#34d399"
    }
}
