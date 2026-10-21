pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: root

    property string connectionState: "disconnected"
    property bool isConnecting: connectionState === "connecting"

    Timer {
        id: connectTimer
        interval: 200
        repeat: false
        onTriggered: {
            root.connectionState = "connected";
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q) {
            Qt.quit();
            event.accepted = true;
        } else if (event.key === Qt.Key_Escape) {
            if (root.isConnecting) {
                connectTimer.stop();
                root.connectionState = "disconnected";
            } else {
                Qt.quit();
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_C) {
            if (root.connectionState === "disconnected") {
                root.connectionState = "connecting";
                connectTimer.restart();
            } else {
                connectTimer.stop();
                root.connectionState = "disconnected";
            }
            event.accepted = true;
        } else if (event.key === Qt.Key_R) {
            connectTimer.stop();
            root.connectionState = "disconnected";
            event.accepted = true;
        } else if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
            if (card1.activeFocus) {
                card2.focus = true;
            } else {
                card1.focus = true;
            }
            event.accepted = true;
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: "black"

        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 1

            // Title banner
            Rectangle {
                width: parent.width
                height: 3
                color: "#1a1a24"
                border.style: Border.Single
                border.color: "#3b4252"

                Text {
                    anchors.centerIn: parent
                    text: "INDEPENDENT STATES & TRANSITIONS DEMO"
                    color: "cyan"
                    bold: true
                }
            }

            // Connection status bar (driven by StateGroup)
            Rectangle {
                id: statusBar
                width: parent.width
                height: 3
                color: "#201515"
                border.style: Border.Single
                border.color: "#e06c75"

                Row {
                    anchors.centerIn: parent
                    spacing: 2

                    Rectangle {
                        id: statusBadge
                        width: 3
                        height: 1
                        color: "#e06c75"
                    }

                    Text {
                        id: statusText
                        text: "STATUS: Offline (Disconnected)"
                        color: "white"
                        bold: true
                    }
                }
            }

            // Interactive focus cards (driven by Item.states)
            Row {
                width: parent.width
                spacing: 2

                Rectangle {
                    id: card1
                    width: 36
                    height: 7
                    focusPolicy: Qt.StrongFocus
                    focus: true
                    color: "#1e1e1e"
                    border.style: Border.Single
                    border.color: "#555555"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 1

                        Text {
                            id: card1Header
                            text: "[Card 1: Primary Node]"
                            color: "gray"
                        }
                        Text {
                            id: card1Desc
                            text: "Press Tab to switch focus"
                            color: "silver"
                        }
                        Text {
                            id: card1Status
                            text: "Mode: Standby"
                            color: "yellow"
                        }
                    }

                    states: [
                        State {
                            name: "focused"
                            when: card1.activeFocus
                            PropertyChanges {
                                target: card1
                                border.color: "#61afef"
                                color: "#282c34"
                            }
                            PropertyChanges {
                                card1Header.color: "#61afef"
                                card1Header.bold: true
                                card1Status.text: "Mode: Active (Focused)"
                            }
                        }
                    ]

                    transitions: [
                        Transition {
                            ColorAnimation {
                                duration: 100
                            }
                        }
                    ]
                }

                Rectangle {
                    id: card2
                    width: 36
                    height: 7
                    focusPolicy: Qt.StrongFocus
                    focus: false
                    color: "#1e1e1e"
                    border.style: Border.Single
                    border.color: "#555555"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 1
                        spacing: 1

                        Text {
                            id: card2Header
                            text: "[Card 2: Secondary Node]"
                            color: "gray"
                        }
                        Text {
                            id: card2Desc
                            text: "Independent focus states"
                            color: "silver"
                        }
                        Text {
                            id: card2Status
                            text: "Mode: Standby"
                            color: "yellow"
                        }
                    }

                    states: [
                        State {
                            name: "focused"
                            when: card2.activeFocus
                            PropertyChanges {
                                target: card2
                                border.color: "#61afef"
                                color: "#282c34"
                            }
                            PropertyChanges {
                                card2Header.color: "#61afef"
                                card2Header.bold: true
                                card2Status.text: "Mode: Active (Focused)"
                            }
                        }
                    ]

                    transitions: [
                        Transition {
                            ColorAnimation {
                                duration: 100
                            }
                        }
                    ]
                }
            }

            // Keyboard navigation legend
            Rectangle {
                width: parent.width
                height: 4
                color: "#181818"
                border.style: Border.Single
                border.color: "#333333"

                Column {
                    anchors.fill: parent
                    anchors.margins: 1

                    Text {
                        text: "Controls: [Tab] Focus  [C] Connect/Disconnect  [R] Reset  [Esc/Q] Quit"
                        color: "gray"
                    }
                    Text {
                        text: "Escape contextually cancels in-flight connection before quitting."
                        color: "darkgray"
                    }
                }
            }
        }
    }

    StateGroup {
        id: connectionGroup
        state: root.connectionState

        states: [
            State {
                name: "connecting"
                PropertyChanges {
                    statusBadge.color: "#e5c07b"
                    statusText.text: "STATUS: Connecting..."
                    statusBar.border.color: "#e5c07b"
                    statusBar.color: "#282414"
                }
            },
            State {
                name: "connected"
                PropertyChanges {
                    statusBadge.color: "#98c379"
                    statusText.text: "STATUS: Connected (Cluster Healthy)"
                    statusBar.border.color: "#98c379"
                    statusBar.color: "#162818"
                }
            }
        ]

        transitions: [
            Transition {
                to: "connecting"
                ColorAnimation {
                    duration: 200
                }
            },
            Transition {
                to: "connected"
                ColorAnimation {
                    duration: 200
                }
            },
            Transition {
                to: "disconnected"
                ColorAnimation {
                    duration: 200
                }
            }
        ]
    }
}
