pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.isActive = !root.isActive
            event.accepted = true
        }
    }

    Rectangle {
        id: root
        width: 64
        height: 16
        color: "black"

        property bool isActive: false


        Rectangle {
            anchors.fill: parent
            border.style: Border.Single
            border.color: "gray"

            Column {
                anchors.centerIn: parent
                spacing: 1

                Text {
                    text: "⚡ QML STATES & TRANSITIONS DEMO"
                    color: "cyan"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }

                Text {
                    text: "Press [SPACE] or [ENTER] to toggle state"
                    color: "gray"
                    horizontalAlignment: Qt.AlignHCenter
                }

                Text { text: ""; height: 1 }

                Rectangle {
                    id: statusCard
                    width: 44
                    height: 5
                    border.style: Border.Double
                    border.color: root.isActive ? "green" : "red"
                    focusable: true
                    focus: true

                    Text {
                        id: statusLabel
                        anchors.fill: parent
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        color: root.isActive ? "green" : "red"
                        bold: true
                    }

                    states: [
                        State {
                            name: "OFF"
                            when: !root.isActive
                            PropertyChanges {
                                target: statusCard
                                properties: {
                                    "color": "#331111"
                                }
                            }
                            PropertyChanges {
                                target: statusLabel
                                properties: {
                                    "text": "● SYSTEM INACTIVE (OFF)"
                                }
                            }
                        },
                        State {
                            name: "ON"
                            when: root.isActive
                            PropertyChanges {
                                target: statusCard
                                properties: {
                                    "color": "#113311"
                                }
                            }
                            PropertyChanges {
                                target: statusLabel
                                properties: {
                                    "text": "● SYSTEM ACTIVE (ON)"
                                }
                            }
                        }
                    ]

                    transitions: [
                        Transition {
                            ColorAnimation { duration: 300 }
                        }
                    ]
                }

                Text { text: ""; height: 1 }

                Text {
                    text: "Press [Q] or [ESC] to quit"
                    color: "gray"
                    horizontalAlignment: Qt.AlignHCenter
                }
            }
        }
    }
}
