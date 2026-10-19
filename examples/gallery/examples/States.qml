pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// States: Declarative State and PropertyChanges animated by Transition.
Column {
    id: statesDemo

    property bool active: false

    spacing: 1

    Text {
        text: "PropertyChanges describe each state; Transition animates between them."
        color: "#8b949e"
    }

    Button {
        text: statesDemo.active ? "Switch off" : "Switch on"
        focus: true
        onClicked: statesDemo.active = !statesDemo.active
    }

    Rectangle {
        id: card
        width: 40
        height: 3
        color: "#331111"
        border {
            style: Border.Single
            color: "#7f1d1d"
        }

        Text {
            id: cardLabel
            anchors.centerIn: parent
            text: "● OFF"
            bold: true
            color: "#fca5a5"
        }

        states: [
            State {
                name: "on"
                when: statesDemo.active
                PropertyChanges {
                    target: card
                    properties: {
                        "color": "#113311",
                        "border.color": "#166534"
                    }
                }
                PropertyChanges {
                    target: cardLabel
                    properties: {
                        "text": "● ON",
                        "color": "#86efac"
                    }
                }
            }
        ]

        transitions: [
            Transition {
                ColorAnimation {
                    duration: 400
                }
            }
        ]
    }

    Text {
        text: "state: " + (card.state === "" ? "(base)" : card.state)
        color: "#34d399"
    }
}
